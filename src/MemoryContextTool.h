/**
* �ڴ������Ĺ���ʹ�õ�һЩ�������ߺ궨��
*
*/

#ifndef MEMORYCONTEXTTOOL_H
#define MEMORYCONTEXTTOOL_H

#define TYPEALIGN(ALIGNVAL,LEN)  \
	(((intptr_t) (LEN) + ((ALIGNVAL) - 1)) & ~((intptr_t) ((ALIGNVAL) - 1)))

//��֤LEN�������λ�������붼��0
#define MAXALIGN(LEN)			TYPEALIGN(4, (LEN))

#endif