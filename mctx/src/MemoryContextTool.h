/**
* 内存上下文公共使用的一些函数或者宏定义
*
*/

#ifndef MEMORYCONTEXTTOOL_H
#define MEMORYCONTEXTTOOL_H

#define TYPEALIGN(ALIGNVAL,LEN)  \
	(((intptr_t) (LEN) + ((ALIGNVAL) - 1)) & ~((intptr_t) ((ALIGNVAL) - 1)))

//保证LEN的最后两位二进制码都是0
#define MAXALIGN(LEN)			TYPEALIGN(4, (LEN))

#endif