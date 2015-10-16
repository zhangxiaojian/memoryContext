/**
 * =====================================================================================
 *
 *       Filename:  AllocSetContext.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年07月31日 11时42分40秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zj (), xiaojian_whu@163.com
 *        Company:  whu.sklse
 *
 * =====================================================================================
*/

#include <iostream>
#include <string>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include "AllocSetContext.h"

AllocSetContext::AllocSetContext(MemoryContextData* parent, string name,
		Size minContextSize, Size initBlockSize, Size maxBlockSize)
		:MemoryContextData(parent, name),
		blocks(NULL),
		minContextSize(minContextSize),
		initBlockSize(initBlockSize),
		maxBlockSize(maxBlockSize)
{
	/*
	*使参数对齐,强制使blocksize至少为1kb
	*/
	initBlockSize = MAXALIGN(initBlockSize);
	if(initBlockSize < 1024)
		initBlockSize = 1024;
	maxBlockSize = MAXALIGN(maxBlockSize);
	if(maxBlockSize < initBlockSize)
		maxBlockSize = initBlockSize;
	nextBlockSize = initBlockSize;

	/*
	* 计算allocChunkLimit，首先以根据FreeList大小的宏初始化，然后根据maxBlockSize，
	* 控制allocChunkLimit小于等于maxBlockSize的ALLOC_CHUNK_FRACTION分之一。
	*/
	allocChunkLimit = ALLOC_CHUNK_LIMIT;
	while((Size)(allocChunkLimit + ALLOC_CHUNKHDRSZ) > 
		(Size)((maxBlockSize - ALLOC_BLOCKHDRSZ) / ALLOC_CHUNK_FRACTION))
		allocChunkLimit >>= 1;

	/*
	* 内存上下文初始化的一个BLOCK，如果足够分配一个BLOCK需要的最小空间（至少需要一个Block头和一个Chunk头）
	* 那么就根据minContext的大小来分配首个Block块的大小。并使keeper指针指向它。
	*/
	if(minContextSize > ALLOC_BLOCKHDRSZ + ALLOC_CHUNKHDRSZ)
	{
		Size blksize = MAXALIGN(minContextSize);
		
		AllocBlock block;
		block = (AllocBlock)malloc(blksize);
		if(block == NULL)
		{
			TopMemoryContext->memoryContextStats();
			fprintf(stderr,"%s","failed to init block");
		}
		else
		{
			block->aset = this;
			block->freeptr = ((char*)block) + ALLOC_BLOCKHDRSZ;
			block->endptr = ((char*)block) + blksize;
			/*blocks 初始为NULL*/
			block->next = blocks;
			blocks = block;
			/* keeper指向的Block内存区域在整个内存上下文reset的时候不会被释放*/
			keeper = block;
		}
	}
}

/**
* 创建一个AllocSetContext，使用malloc或者jalloc分配空间，使用C++中提供的new placement在指定空间上
* 初始化对象。
*
*/
AllocSetContext* AllocSetContext::allocSetContextCreate(MemoryContextData* parent, string name,
	Size minContextSize, Size initBlockSize, Size maxBlockSize)
{
	Size needed = sizeof(AllocSetContext);
	void* nodeSpace;

	if(TopMemoryContext != NULL)
	{
		/*一般情况下，所有的内存上下文本身占有的内存都是在TopMemoryContext中分配的*/
		nodeSpace = TopMemoryContext->jalloc(needed);
	}
	else
	{
		/*如果TopMemoryContext为空，就使用malloc分配（此时应该是初始化TopMemoryContext）*/
		nodeSpace = malloc(needed);
		assert(nodeSpace != NULL);
	}
	/*初始化内存区域，避免AllocSetContext有成员初始化有问题*/
	memset(nodeSpace, 0, needed);

	return new(nodeSpace) AllocSetContext(parent, name, minContextSize, initBlockSize, maxBlockSize);
}

/**
* 在内存上下文中分配size大小的内存空间，返回空间的指针，并把内存加到该上下文中。
*
*/
void* AllocSetContext::jalloc(Size size)
{
	AllocBlock	block;
	AllocChunk	chunk;
	int			fidx;
	Size		chunk_size;
	Size		blksize;
	
	/*** 标记上下文已进了分配 ***/
	isReset = false;
	/*** 统计总的分配次数 ***/
#ifdef ALLOCSETCONTEXT_ANALY
	allocCount++;
#endif
	/*** 如果申请的空间大小超过allocChunkLimit 那么就为这次申请分配一个独立的内存块，并把这个内存块加到内存链表的第二个位置 ***/
	if (size > allocChunkLimit)
	{
		chunk_size = MAXALIGN(size);
		blksize = chunk_size + ALLOC_BLOCKHDRSZ + ALLOC_CHUNKHDRSZ;
		block = (AllocBlock) malloc(blksize);
		
		if (block == NULL)
		{
			TopMemoryContext->memoryContextStats();
			fprintf(stderr, "%s" , "out of memory");
			return NULL;
		}
		block->aset = this;
		block->freeptr = block->endptr = ((char *) block) + blksize;

		chunk = (AllocChunk) (((char *) block) + ALLOC_BLOCKHDRSZ);
		chunk->aset = this;
		chunk->size = chunk_size;

		/*
		 * 如果内存链表不为空就把新申请的内存块链接到第二个位置，保证活动块还是活动块
		 */
		if (blocks != NULL)
		{
			block->next = blocks->next;
			blocks->next = block;
		}
		else
		{
			block->next = NULL;
			blocks = block;
		}
		/** 统计分配一整个Block的次数 **/
#ifdef ALLOCSETCONTEXT_ANALY
		allocCountBlock++;
#endif
		return AllocChunkGetPointer(chunk);
	}

	/**
	* 此时申请的空间大小可以被看作一个Chunk，首先从FreeList中找出合适的空闲片，
	* 找出空闲片后，返回该片的数据域。
	*/

	fidx = AllocSetFreeIndex(size);
	chunk = freeList[fidx];
	if (chunk != NULL)
	{
		assert(chunk->size >= size);

		freeList[fidx] = (AllocChunk) chunk->aset;
		chunk->aset = (void *) this;

		return AllocChunkGetPointer(chunk);
	}
	
	/*** 分配一个Chunk ***/
	chunk_size = (1 << ALLOC_MINBITS) << fidx;
	assert(chunk_size >= size);

	/*** 如果当前活动块中有足够的空间，就从活动块中分配，否则就重新申请一个Block来分配***/
	if ((block = blocks) != NULL)
	{
		Size		availspace = block->endptr - block->freeptr;

		if (availspace < (chunk_size + ALLOC_CHUNKHDRSZ))
		{
			/*
			 * 活动块中的空间不足以完成此次分配，所以需要重新申请活动块，但是
			 * 当前活动块中可能存在没有使用的空间，把这些空间链接到空闲链表中，
			 * 可以在后续分配中使用。
			 *
			 * Because we can only get here when there's less than
			 * ALLOC_CHUNK_LIMIT left in the block, this loop cannot iterate
			 * more than ALLOCSET_NUM_FREELISTS-1 times.
			 */
			while (availspace >= ((1 << ALLOC_MINBITS) + ALLOC_CHUNKHDRSZ))
			{
				Size		availchunk = availspace - ALLOC_CHUNKHDRSZ;
				int			a_fidx = AllocSetFreeIndex(availchunk);

				/*
				 * In most cases, we'll get back the index of the next larger
				 * freelist than the one we need to put this chunk on.  The
				 * exception is when availchunk is exactly a power of 2.
				 */
				if (availchunk != ((Size) 1 << (a_fidx + ALLOC_MINBITS)))
				{
					a_fidx--;
					assert(a_fidx >= 0);
					availchunk = ((Size) 1 << (a_fidx + ALLOC_MINBITS));
				}

				chunk = (AllocChunk) (block->freeptr);

				block->freeptr += (availchunk + ALLOC_CHUNKHDRSZ);
				availspace -= (availchunk + ALLOC_CHUNKHDRSZ);

				chunk->size = availchunk;
				chunk->aset = (void *) freeList[a_fidx];
				freeList[a_fidx] = chunk;
			}

			/* Mark that we need to create a new block */
			block = NULL;
		}
	}

	/*
	 * 创建一个新的活动块,使用block判空标记是否需要创建block，1，可以将分配工作放在最后执行，不论当前活动块是否够用。
	 *                                                                                                         2，如果是分配的第一个block，需要在此处标记为keeper。
	 */
	if (block == NULL)
	{
		Size		required_size;

		/*
		 * The first such block has size initBlockSize, and we double the
		 * space in each succeeding block, but not more than maxBlockSize.
		 */
		blksize = nextBlockSize;
		nextBlockSize <<= 1;
		if (nextBlockSize > maxBlockSize)
			nextBlockSize = maxBlockSize;

		/*
		 * If initBlockSize is less than ALLOC_CHUNK_LIMIT, we could need more
		 * space... but try to keep it a power of 2.
		 */
		required_size = chunk_size + ALLOC_BLOCKHDRSZ + ALLOC_CHUNKHDRSZ;
		while (blksize < required_size)
			blksize <<= 1;

		/* Try to allocate it */
		block = (AllocBlock) malloc(blksize);

		/*
		 * We could be asking for pretty big blocks here, so cope if malloc
		 * fails.  But give up if there's less than a meg or so available...
		 */
		while (block == NULL && blksize > 1024 * 1024)
		{
			blksize >>= 1;
			if (blksize < required_size)
				break;
			block = (AllocBlock) malloc(blksize);
		}

		if (block == NULL)
		{
			TopMemoryContext->memoryContextStats();
			fprintf(stderr, "%s" , "out of memory");
			return NULL;
		}

		block->aset = this;
		block->freeptr = ((char *) block) + ALLOC_BLOCKHDRSZ;
		block->endptr = ((char *) block) + blksize;

		/*
		 * If this is the first block of the set, make it the "keeper" block.
		 * Formerly, a keeper block could only be created during context
		 * creation, but allowing it to happen here lets us have fast reset
		 * cycling even for contexts created with minContextSize = 0; that way
		 * we don't have to force space to be allocated in contexts that might
		 * never need any space.  Don't mark an oversize block as a keeper,
		 * however.
		 */
		if (keeper == NULL && blksize == initBlockSize)
			keeper = block;

		block->next = blocks;
		blocks = block;
	}

	/*
	 * 此时活动块中必然有足够的空间去分配一个chunk，分配之
	 */
	chunk = (AllocChunk) (block->freeptr);

	block->freeptr += (chunk_size + ALLOC_CHUNKHDRSZ);
	assert(block->freeptr <= block->endptr);

	chunk->aset = (void *) this;
	chunk->size = chunk_size;

#ifdef ALLOCSETCONTEXT_ANALY
	allocCountFreeList++;
#endif

	return AllocChunkGetPointer(chunk);
}
		
void* AllocSetContext::jrealloc(void* pointer, Size size)
{
		AllocChunk	chunk = AllocPointerGetChunk(pointer);
		Size		oldsize = chunk->size;

	/*
	 * 如果realloc的空间反而更小，那么就不用费事，直接返回即可。
	 */
	if (oldsize >= size)
	{
		return pointer;
	}

	if (oldsize > allocChunkLimit)
	{
		/*
		 * 如果原本的Chunk就是一个独立的Block，而又要扩充大小，那么就把这个块的大小重新realloc一下
		 */
		AllocBlock	block = blocks;
		AllocBlock	prevblock = NULL;
		Size		chksize;
		Size		blksize;

		while (block != NULL)
		{
			if (chunk == (AllocChunk) (((char *) block) + ALLOC_BLOCKHDRSZ))
				break;
			prevblock = block;
			block = block->next;
		}
		if (block == NULL)
		{
			TopMemoryContext->memoryContextStats();
			fprintf(stderr, "%s" , "out of memory");
			return NULL;
		}
		/* let's just make sure chunk is the only one in the block */
		assert(block->freeptr == (((char *) block) +(chunk->size + ALLOC_BLOCKHDRSZ + ALLOC_CHUNKHDRSZ)));

		/* Do the realloc */
		chksize = MAXALIGN(size);
		blksize = chksize + ALLOC_BLOCKHDRSZ + ALLOC_CHUNKHDRSZ;
		block = (AllocBlock) realloc(block, blksize);
		if (block == NULL)
		{
			TopMemoryContext->memoryContextStats();
			fprintf(stderr, "%s" , "out of memory");
			return NULL;
		}
		block->freeptr = block->endptr = ((char *) block) + blksize;

		/* Update pointers since block has likely been moved */
		chunk = (AllocChunk) (((char *) block) + ALLOC_BLOCKHDRSZ);
		if (prevblock == NULL)
			blocks = block;
		else
			prevblock->next = block;
		chunk->size = chksize;

		return AllocChunkGetPointer(chunk);
	}
	else
	{
		/*
		 * Small-chunk case.  We just do this by brute force, ie, allocate a
		 * new chunk and copy the data.  Since we know the existing data isn't
		 * huge, this won't involve any great memcpy expense, so it's not
		 * worth being smarter.  (At one time we tried to avoid memcpy when it
		 * was possible to enlarge the chunk in-place, but that turns out to
		 * misbehave unpleasantly for repeated cycles of
		 * palloc/repalloc/pfree: the eventually freed chunks go into the
		 * wrong freelist for the next initial palloc request, and so we leak
		 * memory indefinitely.  See pgsql-hackers archives for 2007-08-11.)
		 */
		AllocPointer newPointer;

		/* allocate new chunk */
		newPointer = jalloc(size);

		/* transfer existing data (certain to fit) */
		memcpy(newPointer, pointer, oldsize);

		/* free old chunk */
		jfree(pointer);

		return newPointer;
	}
}

/**
* 释放一片内存空间，如果内存是一个独立的Block，就还给OS，如果是一个简单的Chunk，就加到freeList中，以便下次使用。
*/
void AllocSetContext::jfree(void* pointer)
{
	AllocChunk	chunk = AllocPointerGetChunk(pointer);

#ifdef ALLOCSETCONTEXT_ANALY
	freeCount++;
#endif

	if (chunk->size > allocChunkLimit)
	{
		/*
		 * Big chunks are certain to have been allocated as single-chunk
		 * blocks.  Find the containing block and return it to malloc().
		 */
		AllocBlock	block = blocks;
		AllocBlock	prevblock = NULL;

		while (block != NULL)
		{
			if (chunk == (AllocChunk) (((char *) block) + ALLOC_BLOCKHDRSZ))
				break;
			prevblock = block;
			block = block->next;
		}
		if (block == NULL)
			fprintf(stderr, "could not find block containing chunk %p", chunk);
		/* let's just make sure chunk is the only one in the block */
		assert(block->freeptr == ((char *) block) +
			   (chunk->size + ALLOC_BLOCKHDRSZ + ALLOC_CHUNKHDRSZ));

		/* OK, remove block from aset's list and free it */
		if (prevblock == NULL)
			blocks = block->next;
		else
			prevblock->next = block->next;

		free(block);

#ifdef ALLOCSETCONTEXT_ANALY
		freeCountBlock++;
#endif
	}
	else
	{
		/* Normal case, put the chunk into appropriate freelist */
		int			fidx = AllocSetFreeIndex(chunk->size);
		chunk->aset = (void *) freeList[fidx];
		freeList[fidx] = chunk;

#ifdef ALLOCSETCONTEXT_ANALY
		freeCountFreeList++;
#endif
	}
}

/**
* 释放该内存上下文中所有的block。但本身对象所占用的内存空间要由TopMememoryContext来释放。
*/
void AllocSetContext::jdelete()
{
	AllocBlock	block = blocks;

	memset(freeList,0,sizeof(freeList));
	blocks = NULL;
	keeper = NULL;

	while (block != NULL)
	{
		AllocBlock	next = block->next;
		free(block);
		block = next;
	}
}

/**
* 重置内存上下文，AllocSetContext的重置语义指：释放除了keeper指向的的Block空间。
*/
void AllocSetContext::jreset()
{
	AllocBlock	block;
	block = blocks;

	memset(freeList,0,sizeof(freeList));
	/* New blocks list is either empty or just the keeper block */
	blocks = keeper;

	while (block != NULL)
	{
		AllocBlock	next = block->next;

		if (block == keeper)
		{
			/* Reset the block, but don't return it to malloc */
			char	   *datastart = ((char *) block) + ALLOC_BLOCKHDRSZ;
			block->freeptr = datastart;
			block->next = NULL;
		}
		else
		{
			/* Normal case, release the block */
			free(block);
		}
		block = next;
	}
	/* Reset block size allocation sequence, too */
	nextBlockSize = initBlockSize;
	/** 重置分析数据 **/
#ifdef ALLOCSETCONTEXT_ANALY
	resetAnaly();
#endif
	/* 标记上下文已被重置*/
	isReset = true;
}

/**
* 输出内存上下文的统计信息：包括总共占用的空间，已使用的空间，尚未使用的空间。
* 参数level表示输出时缩进的数量，使父子之间有层级结构。
*/
void AllocSetContext::jstats(int level)
{
	long		nblocks = 0;
	long		nchunks = 0;
	long		totalspace = 0;
	long		freespace = 0;
	AllocBlock	block;
	AllocChunk	chunk;
	int			fidx;
	int			i;
	string		pad;

	for (block = blocks; block != NULL; block = block->next)
	{
		nblocks++;
		totalspace += block->endptr - ((char *) block);
		freespace += block->endptr - block->freeptr;
	}
	for (fidx = 0; fidx < ALLOC_FREELIST_SIZE; fidx++)
	{
		for (chunk = freeList[fidx]; chunk != NULL;
			 chunk = (AllocChunk) chunk->aset)
		{
			nchunks++;
			freespace += chunk->size + ALLOC_CHUNKHDRSZ;
		}
	}

	for (i = 0; i < level; i++)
		pad = pad + "  ";

	cerr << pad <<getName() << " : \n" ;
#ifdef ALLOCSETCONTEXT_ANALY
	/* 基本参数信息 */
	cerr	<< pad << "\tMinContextSize: " << minContextSize << "\tMaxBlockSize: " << maxBlockSize << endl
		<< pad << "\tNextBlockSize: " << nextBlockSize << "\tFreeListSize: " << ALLOC_FREELIST_SIZE << endl
		<<pad << "\tAllocChunkLimit: " << allocChunkLimit << "\tMinChunkSize: " << (1 << ALLOC_MINBITS) <<endl;
	
	/* 申请释放信息 */
	cerr << pad <<"\tTotal " << allocCount << " times alloc; " << allocCountFreeList << " times is chunk; "
		<< allocCountBlock << " times is block; " << allocCount - allocCountFreeList - allocCountBlock << " flailed !" << endl
		<<pad << "\tTotal " << freeCount << " times free; " << freeCountFreeList << " times is chunk; " << freeCountBlock << " times is block; " << endl;
#endif

	/* 目前状态信息 */
	cerr << pad << "\t" << totalspace << " total in " << nblocks << " blocks; " << freespace << " free (" 
		<< nchunks << " chunks); " << totalspace - freespace << " used" << endl; 
}

/**
* 如果重置表示为空，指没有进行过任何内存分配。（并不是说上下文中没有什么内存可用）
*/
bool AllocSetContext::isEmpty()
{
	return isReset;
}

/**
* 初始化工作，在父类MemoryContext的构造函数中会调用init函数，而init是虚函数，所以可以在
* 创建内存上下文的最开始做一些初始化工作。AllocSetContext并不需要做什么工作。
*/
void AllocSetContext::init()
{

}


/* ----------
 * AllocSetFreeIndex -
 *
 *		根据size参数大小确定FreeList中的位置，确保调用前size < AllocChunkLimit
 * ----------
 */
int AllocSetContext::AllocSetFreeIndex(Size size)
{
	int			idx;
	unsigned int t,tsize;

	if (size > (1 << ALLOC_MINBITS))
	{
		tsize = (size - 1) >> ALLOC_MINBITS;

		/*
		 * At this point we need to obtain log2(tsize)+1, ie, the number of
		 * not-all-zero bits at the right.  We used to do this with a
		 * shift-and-count loop, but this function is enough of a hotspot to
		 * justify micro-optimization effort.  The best approach seems to be
		 * to use a lookup table.  Note that this code assumes that
		 * ALLOCSET_NUM_FREELISTS <= 17, since we only cope with two bytes of
		 * the tsize value.
		 */
		/**
		* 需要计算log2(tsize) + 1这个表达式，为了提高计算效率，使用查表得方式，
		* 表LogTable在AllocSetContext.h头文件中定义好。
		*/
		t = tsize >> 8;
		idx = t ? LogTable256[t] + 8 : LogTable256[tsize];

		assert (idx < ALLOC_FREELIST_SIZE);
	}
	else
		idx = 0;

	return idx;
}

#ifdef ALLOCSETCONTEXT_ANALY

void AllocSetContext::resetAnaly()
{
	if(isReset == false)
	{
		allocCount = allocCountFreeList = allocCountBlock = 0;
		freeCount = freeCountFreeList = freeCountBlock = 0;
	}
}

#endif
