/**
 * =====================================================================================
 *
 *       Filename:  AllocSetContext.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015��07��31�� 11ʱ42��40��
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
	*ʹ��������,ǿ��ʹblocksize����Ϊ1kb
	*/
	initBlockSize = MAXALIGN(initBlockSize);
	if(initBlockSize < 1024)
		initBlockSize = 1024;
	maxBlockSize = MAXALIGN(maxBlockSize);
	if(maxBlockSize < initBlockSize)
		maxBlockSize = initBlockSize;
	nextBlockSize = initBlockSize;

	/*
	* ����allocChunkLimit�������Ը���FreeList��С�ĺ��ʼ����Ȼ�����maxBlockSize��
	* ����allocChunkLimitС�ڵ���maxBlockSize��ALLOC_CHUNK_FRACTION��֮һ��
	*/
	allocChunkLimit = ALLOC_CHUNK_LIMIT;
	while((Size)(allocChunkLimit + ALLOC_CHUNKHDRSZ) > 
		(Size)((maxBlockSize - ALLOC_BLOCKHDRSZ) / ALLOC_CHUNK_FRACTION))
		allocChunkLimit >>= 1;

	/*
	* �ڴ������ĳ�ʼ����һ��BLOCK������㹻����һ��BLOCK��Ҫ����С�ռ䣨������Ҫһ��Blockͷ��һ��Chunkͷ��
	* ��ô�͸���minContext�Ĵ�С�������׸�Block��Ĵ�С����ʹkeeperָ��ָ������
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
			/*blocks ��ʼΪNULL*/
			block->next = blocks;
			blocks = block;
			/* keeperָ���Block�ڴ������������ڴ�������reset��ʱ�򲻻ᱻ�ͷ�*/
			keeper = block;
		}
	}
}

/**
* ����һ��AllocSetContext��ʹ��malloc����jalloc����ռ䣬ʹ��C++���ṩ��new placement��ָ���ռ���
* ��ʼ������
*
*/
AllocSetContext* AllocSetContext::allocSetContextCreate(MemoryContextData* parent, string name,
	Size minContextSize, Size initBlockSize, Size maxBlockSize)
{
	Size needed = sizeof(AllocSetContext);
	void* nodeSpace;

	if(TopMemoryContext != NULL)
	{
		/*һ������£����е��ڴ������ı���ռ�е��ڴ涼����TopMemoryContext�з����*/
		nodeSpace = TopMemoryContext->jalloc(needed);
	}
	else
	{
		/*���TopMemoryContextΪ�գ���ʹ��malloc���䣨��ʱӦ���ǳ�ʼ��TopMemoryContext��*/
		nodeSpace = malloc(needed);
		assert(nodeSpace != NULL);
	}
	/*��ʼ���ڴ����򣬱���AllocSetContext�г�Ա��ʼ��������*/
	memset(nodeSpace, 0, needed);

	return new(nodeSpace) AllocSetContext(parent, name, minContextSize, initBlockSize, maxBlockSize);
}

/**
* ���ڴ��������з���size��С���ڴ�ռ䣬���ؿռ��ָ�룬�����ڴ�ӵ����������С�
*
*/
void* AllocSetContext::jalloc(Size size)
{
	AllocBlock	block;
	AllocChunk	chunk;
	int			fidx;
	Size		chunk_size;
	Size		blksize;
	
	/*** ����������ѽ��˷��� ***/
	isReset = false;
	/*** ͳ���ܵķ������ ***/
#ifdef ALLOCSETCONTEXT_ANALY
	allocCount++;
#endif
	/*** �������Ŀռ��С����allocChunkLimit ��ô��Ϊ����������һ���������ڴ�飬��������ڴ��ӵ��ڴ�����ĵڶ���λ�� ***/
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
		 * ����ڴ�����Ϊ�վͰ���������ڴ�����ӵ��ڶ���λ�ã���֤��黹�ǻ��
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
		/** ͳ�Ʒ���һ����Block�Ĵ��� **/
#ifdef ALLOCSETCONTEXT_ANALY
		allocCountBlock++;
#endif
		return AllocChunkGetPointer(chunk);
	}

	/**
	* ��ʱ����Ŀռ��С���Ա�����һ��Chunk�����ȴ�FreeList���ҳ����ʵĿ���Ƭ��
	* �ҳ�����Ƭ�󣬷��ظ�Ƭ��������
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
	
	/*** ����һ��Chunk ***/
	chunk_size = (1 << ALLOC_MINBITS) << fidx;
	assert(chunk_size >= size);

	/*** �����ǰ��������㹻�Ŀռ䣬�ʹӻ���з��䣬�������������һ��Block������***/
	if ((block = blocks) != NULL)
	{
		Size		availspace = block->endptr - block->freeptr;

		if (availspace < (chunk_size + ALLOC_CHUNKHDRSZ))
		{
			/*
			 * ����еĿռ䲻������ɴ˴η��䣬������Ҫ���������飬����
			 * ��ǰ����п��ܴ���û��ʹ�õĿռ䣬����Щ�ռ����ӵ����������У�
			 * �����ں���������ʹ�á�
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
	 * ����һ���µĻ��,ʹ��block�пձ���Ƿ���Ҫ����block��1�����Խ����乤���������ִ�У����۵�ǰ����Ƿ��á�
	 *                                                                                                         2������Ƿ���ĵ�һ��block����Ҫ�ڴ˴����Ϊkeeper��
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
	 * ��ʱ����б�Ȼ���㹻�Ŀռ�ȥ����һ��chunk������֮
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
	 * ���realloc�Ŀռ䷴����С����ô�Ͳ��÷��£�ֱ�ӷ��ؼ��ɡ�
	 */
	if (oldsize >= size)
	{
		return pointer;
	}

	if (oldsize > allocChunkLimit)
	{
		/*
		 * ���ԭ����Chunk����һ��������Block������Ҫ�����С����ô�Ͱ������Ĵ�С����reallocһ��
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
* �ͷ�һƬ�ڴ�ռ䣬����ڴ���һ��������Block���ͻ���OS�������һ���򵥵�Chunk���ͼӵ�freeList�У��Ա��´�ʹ�á�
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
* �ͷŸ��ڴ������������е�block�������������ռ�õ��ڴ�ռ�Ҫ��TopMememoryContext���ͷš�
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
* �����ڴ������ģ�AllocSetContext����������ָ���ͷų���keeperָ��ĵ�Block�ռ䡣
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
	/** ���÷������� **/
#ifdef ALLOCSETCONTEXT_ANALY
	resetAnaly();
#endif
	/* ����������ѱ�����*/
	isReset = true;
}

/**
* ����ڴ������ĵ�ͳ����Ϣ�������ܹ�ռ�õĿռ䣬��ʹ�õĿռ䣬��δʹ�õĿռ䡣
* ����level��ʾ���ʱ������������ʹ����֮���в㼶�ṹ��
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
	/* ����������Ϣ */
	cerr	<< pad << "\tMinContextSize: " << minContextSize << "\tMaxBlockSize: " << maxBlockSize << endl
		<< pad << "\tNextBlockSize: " << nextBlockSize << "\tFreeListSize: " << ALLOC_FREELIST_SIZE << endl
		<<pad << "\tAllocChunkLimit: " << allocChunkLimit << "\tMinChunkSize: " << (1 << ALLOC_MINBITS) <<endl;
	
	/* �����ͷ���Ϣ */
	cerr << pad <<"\tTotal " << allocCount << " times alloc; " << allocCountFreeList << " times is chunk; "
		<< allocCountBlock << " times is block; " << allocCount - allocCountFreeList - allocCountBlock << " flailed !" << endl
		<<pad << "\tTotal " << freeCount << " times free; " << freeCountFreeList << " times is chunk; " << freeCountBlock << " times is block; " << endl;
#endif

	/* Ŀǰ״̬��Ϣ */
	cerr << pad << "\t" << totalspace << " total in " << nblocks << " blocks; " << freespace << " free (" 
		<< nchunks << " chunks); " << totalspace - freespace << " used" << endl; 
}

/**
* ������ñ�ʾΪ�գ�ָû�н��й��κ��ڴ���䡣��������˵��������û��ʲô�ڴ���ã�
*/
bool AllocSetContext::isEmpty()
{
	return isReset;
}

/**
* ��ʼ���������ڸ���MemoryContext�Ĺ��캯���л����init��������init���麯�������Կ�����
* �����ڴ������ĵ��ʼ��һЩ��ʼ��������AllocSetContext������Ҫ��ʲô������
*/
void AllocSetContext::init()
{

}


/* ----------
 * AllocSetFreeIndex -
 *
 *		����size������Сȷ��FreeList�е�λ�ã�ȷ������ǰsize < AllocChunkLimit
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
		* ��Ҫ����log2(tsize) + 1������ʽ��Ϊ����߼���Ч�ʣ�ʹ�ò��÷�ʽ��
		* ��LogTable��AllocSetContext.hͷ�ļ��ж���á�
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
