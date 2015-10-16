#include <iostream>
#include "../MemoryContextData.h"
#include "../AllocSetContext.h"

using namespace std;

int main()
{
	//初始化工作
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

	//释放errorContext空间
	errorContext->jfree(error);
	//输出统计信息
	cout << "-----------------------------所有内存上下文统计信息--------------------------------"<< endl;
	TopMemoryContext->memoryContextStats();


	cout << "-----------------------------myContext 统计信息------------------------------------"<< endl;
	myContext->jstats(0);
	//重置内存上下文
	myContext->jreset();
	cout << "-----------------------------重置内存上下文后--------------------------------------"<< endl;
	myContext->jstats(0);
	//删除内存上下文中的内存
	myContext->jdelete();
	cout << "-----------------------------删除内存上下文后--------------------------------------"<< endl;
	myContext->jstats(0);

	return 0;
}
