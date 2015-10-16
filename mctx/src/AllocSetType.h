/*
 * =====================================================================================
 *
 *       Filename:  AllocSetType.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年07月31日 11时52分39秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zj (), xiaojian_whu@163.com
 *        Company:  whu.sklse
 *
 * =====================================================================================
 */
#ifndef ALLOCSETTYPE_H
#define ALLOCSETTYPE_H

#include <cstddef>
#include "MemoryContextTool.h"

class AllocSetContext;

typedef struct AllocBlockData
{
	AllocSetContext* aset;  /*< 内存块属于哪一个内存上下文>*/
	AllocBlockData*	next;   /*< 链接到下一个内存块>*/
	char* freeptr;		/*< 可用空间的起始位置>*/
	char* endptr;		/*< 内存块的末尾位置>*/
}AllocBlockData;

typedef AllocBlockData* AllocBlock;

typedef struct AllocChunkData
{
	void* 	aset;  		/*< 内存片属于哪一个内存上下文, 如果在空闲链表中，使用这个值来链接空闲链表>*/
	size_t 	size;		/*< 内存片大小>*/
}AllocChunkData;

typedef AllocChunkData*	AllocChunk;

typedef void* AllocPointer;

#endif

