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

	//����ռ�
	void* memory = myContext->jalloc(1024);
	void* error = errorContext->jalloc(2098);
	void* normalerror = normalErrorContext->jalloc(1025);

	//�ͷſռ�
	errorContext->jfree(error);
	errorContext->jstats(0);
	//���ͳ����Ϣ
	TopMemoryContext->memoryContextStats();

	//�ͷſռ�
	//myContext->jfree(memory);

	myContext->jstats(0);
	//�����ڴ�������
	myContext->jreset();
	myContext->jstats(0);
	//ɾ���ڴ��������е��ڴ�
	myContext->jdelete();

	return 0;
}