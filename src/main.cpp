#include "MemoryContextData.h"
#include "AllocSetContext.h"

int main()
{
	MemoryContextData::memoryContextInit();

	MemoryContextData* myContext = 
				AllocSetContext::allocSetContextCreate(TopMemoryContext, "myContext", 1000, 4000, 8000);

	MemoryContextData* errorContext = 
				AllocSetContext::allocSetContextCreate(TopMemoryContext, "errorContext", 1024, 2048, 8092);

	MemoryContextData* normalErrorContext =
				AllocSetContext::allocSetContextCreate(errorContext, "normalErrorContext", 1024, 2048, 4096);

	//分配空间
	void* memory = myContext->jalloc(1024);
	void* error = errorContext->jalloc(2098);
	void* normalerror = normalErrorContext->jalloc(1025);

	//释放空间
	errorContext->jfree(error);
	errorContext->jstats(0);
	//输出统计信息
	TopMemoryContext->memoryContextStats();

	//释放空间
	//myContext->jfree(memory);

	myContext->jstats(0);
	//重置内存上下文
	myContext->jreset();
	myContext->jstats(0);
	//删除内存上下文中的内存
	myContext->jdelete();

	return 0;
}