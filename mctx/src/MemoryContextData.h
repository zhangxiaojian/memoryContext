/*
 * =====================================================================================
 *
 *       Filename:  MemoryContextData.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年07月31日 11时14分50秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zhangjian (), xiaojian_whu@163.com
 *        Company:  whu.sklse
 *
 * =====================================================================================
 */
#ifndef MEMORYCONTEXTDATA_H
#define MEMORYCONTEXTDATA_H

#include <string>

using namespace std;

class MemoryContextData
{
	public:
		/*
		 * 分配内存的接口，具体的分配工作由实现接口的子类完成。
		 */
		virtual void* jalloc(size_t size);
		
		virtual void* jrealloc(void* point, size_t size);
		
		virtual void jfree(void* point);
		
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

		static void memoryContextInit();

		void memoryContextReset();

		void memoryContextResetChildren();

		void memoryContextDelete();

		void memoryContextDeleteChildren();

		void memoryContextStats();

		void memoryContextStatsInternal(int level);

		void memoryContextSetParent(MemoryContextData* newparent);

		string&	getName()
		{
			return	name;
		}

		MemoryContextData* getParent()
		{
			return 	parent;
		}

		void setFirstchild(MemoryContextData* first)
		{
			firstchild = first;
		}

		MemoryContextData* getFirsstchild()
		{
			return firstchild;
		}

		void setNextchild(MemoryContextData* next)
		{
			nextchild = next;
		}

		MemoryContextData* getNextchild()
		{
			return nextchild;
		}
	protected:
		MemoryContextData(MemoryContextData* prt,string mName);
		bool 	isReset;
	private:
		string	name;
		MemoryContextData* 	parent;
		MemoryContextData* 	firstchild;
		MemoryContextData* 	nextchild;
};

#endif
