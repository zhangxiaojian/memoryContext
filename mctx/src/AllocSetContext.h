/*
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

#ifndef ALLOCSETCONTEXT_H
#define ALLOCSETCONTEXT_H

#include "MemoryContextTool.h"
#include "MemoryContextData.h"
#include "AllocSetType.h"

using namespace std;

#define	INIT_BLOCK_SIZE	2 * 1024

/** 是否需要在统计函数中输出分析记录的数据 **/
#define ALLOCSETCONTEXT_ANALY

#define ALLOC_MINBITS 3
#define ALLOC_FREELIST_SIZE 11
#define ALLOC_CHUNK_LIMIT  (1 << (ALLOC_FREELIST_SIZE - 1 + ALLOC_MINBITS))
#define ALLOC_CHUNK_FRACTION 4

#define ALLOC_BLOCKHDRSZ	MAXALIGN(sizeof(AllocBlockData))
#define ALLOC_CHUNKHDRSZ	MAXALIGN(sizeof(AllocChunkData))

#define AllocPointerGetChunk(ptr)	\
					((AllocChunk)(((char *)(ptr)) - ALLOC_CHUNKHDRSZ))
#define AllocChunkGetPointer(chk)\
					((AllocPointer)(((char *)(chk)) + ALLOC_CHUNKHDRSZ))

typedef size_t Size;

extern MemoryContextData* TopMemoryContext;

/*
 *  AllocSetFreeIndex 中使用的查找表，为了提高效率。
 */
#define LT16(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n

static const unsigned char LogTable256[256] =
{
	0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
	LT16(5), LT16(6), LT16(6), LT16(7), LT16(7), LT16(7), LT16(7),
	LT16(8), LT16(8), LT16(8), LT16(8), LT16(8), LT16(8), LT16(8), LT16(8)
};

class AllocSetContext:public MemoryContextData
{
	public:
		virtual void* jalloc(Size size);
		
		virtual void* jrealloc(void* pointer, Size size);
		
		virtual void jfree(void* pointer);
		
		/*
		 * 删除内存上下文，包括本身数据分配的空间。
		 */
		virtual void jdelete();

		/*
		 * 重置内存上下文，具体语义由子类定义。
		 */
		virtual void jreset();

		virtual void jstats(int level);

		virtual bool isEmpty();

		virtual void init();

		/*
		 * 由此函数来分配AllocSet数据结构所占内存，避免用户使用构造函数在栈空间申请内存。
		 */
		static AllocSetContext* allocSetContextCreate(MemoryContextData* parent, string name, 
				Size minContextSize, Size initBlockSize, Size maxBlockSize);

	private:
		AllocSetContext(MemoryContextData* parent, string name,
				Size minContextSize, Size initBlockSize, Size maxBlockSize);

		static inline int AllocSetFreeIndex(Size size);
#ifdef ALLOCSETCONTEXT_ANALY
		void resetAnaly();
#endif

	private:
		AllocBlock	blocks;		/*< 内存块链表>*/
		AllocChunk	freeList[ALLOC_FREELIST_SIZE];	/*< 空闲链表>*/
		Size		minContextSize;
		Size		initBlockSize;
		Size		maxBlockSize;
		Size		nextBlockSize;
		Size		allocChunkLimit; /*< 超过这个值将会分配一个独立的块>*/
		AllocBlock	keeper;		   /*< 重置的时候不释放该空间>*/
#ifdef ALLOCSETCONTEXT_ANALY
		Size		allocCount;					/*< 总共进行了多少次内存分配>*/
		Size		allocCountFreeList;		/*< 多少次分配在FreeList中>*/
		Size		allocCountBlock;			/*< 多少次分配是一个Block>*/
		Size		freeCount;
		Size		freeCountFreeList;
		Size		freeCountBlock;
#endif
};
#endif
