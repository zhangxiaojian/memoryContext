/**
*   MemoryContextData.cpp
*	默认初始化全局的变量TopMemoryContext,也是内存上下文的根，其它内存上下文的数据结构都从这里分配。
*
*/

#include <cassert>
#include <cstdlib>
#include <cstring>
#include "MemoryContextData.h"
#include "AllocSetContext.h"

MemoryContextData* TopMemoryContext;

MemoryContextData::MemoryContextData(MemoryContextData* prt,string mName):
name(mName),
	parent(prt),
	firstchild(NULL),
	nextchild(NULL),
	isReset(true)
{
	/* Type-specific routine finishes any other essential initialization */
	init();

	/* OK to link node to parent (if any) */
	/* Could use MemoryContextSetParent here, but doesn't seem worthwhile */
	if (parent)
	{
		nextchild = parent->firstchild;
		parent->firstchild = this;
	}
}

/*
* 以下函数本应是虚函数，其功能由子类实现，为了避免没有使用子类，给出默认实现，与标准分配相同。
*/
void* MemoryContextData::jalloc(size_t size)
{
	isReset = false;
	return malloc(size);
}
		
void* MemoryContextData::jrealloc(void* point, size_t size)
{
	return realloc(point,size);
}

void MemoryContextData::jfree(void* point)
{
	free(point);
}
	
void MemoryContextData::jdelete()
{
	//do nothing
}

void MemoryContextData::jreset()
{
	isReset = true;
}

void MemoryContextData::jstats(int level)
{
	//do nothing
}

bool MemoryContextData::isEmpty()
{
	return isReset;
}

void MemoryContextData::init()
{
	//do nothing
}

/*
* --------------------------以下是MemoryContextData的函数-------------------------------------------
*/

/*
* 初始化内存上下文，应在系统初始化阶段调用。
*/
void MemoryContextData::memoryContextInit()
{
	//保证初始化工作不会重复执行
	assert(TopMemoryContext == NULL);

	//初始化内存上下文根节点，根节点的数据结构空间由malloc分配。
	TopMemoryContext = AllocSetContext::allocSetContextCreate((MemoryContextData*) NULL,
																															"TopMemoryContext",
																															0,
																															8 * 1024,
																															8 * 1024);
}

/*
* 以下操作：包括Reset，Delete，Stats，不仅对调用的上下文执行，还会对该上下文的所有孩子节点执行。
*/
void MemoryContextData::memoryContextReset()
{
	if(firstchild != NULL)
		memoryContextResetChildren();

	if(isReset == false)
	{
		jreset();
		isReset = true;
	}
}

void MemoryContextData::memoryContextResetChildren()
{
	MemoryContextData* child;
	for(child = firstchild; child != NULL; child = child->getNextchild())
		child->memoryContextReset();
}

void MemoryContextData::memoryContextDelete()
{
	assert(this != TopMemoryContext);

	memoryContextDeleteChildren();
	memoryContextSetParent(NULL);

	jdelete();
	/** 删掉上下文本身占用的空间 **/
	TopMemoryContext->jfree(this);
}

void MemoryContextData::memoryContextDeleteChildren()
{
	while(firstchild != NULL)
		firstchild->memoryContextDelete();
}

void MemoryContextData::memoryContextStats()
{
	memoryContextStatsInternal(0);
}

void MemoryContextData::memoryContextStatsInternal(int level)
{
	MemoryContextData* child;

	jstats(level);
	for (child = firstchild; child != NULL; child = child->nextchild)
		child->memoryContextStatsInternal(level + 1);
}

void MemoryContextData::memoryContextSetParent(MemoryContextData* newparent)
{
	/* Delink from existing parent, if any */
	if (parent)
	{
		MemoryContextData* tparent = parent;

		if (this == tparent->firstchild)
			tparent->firstchild = nextchild;
		else
		{
			MemoryContextData* child;

			for (child = tparent->firstchild; child; child = child->nextchild)
			{
				if (this == child->nextchild)
				{
					child->nextchild = nextchild;
					break;
				}
			}
		}
	}

	/* And relink */
	if (newparent)
	{
		parent = newparent;
		nextchild = newparent->firstchild;
		newparent->firstchild = this;
	}
	else
	{
		parent = NULL;
		nextchild = NULL;
	}
}
