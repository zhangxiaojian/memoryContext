/*
 * =====================================================================================
 *
 *       Filename:  testRow.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年11月03日 22时02分49秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zj (), xiaojian_whu@163.com
 *        Company:  whu.sklse
 *
 * =====================================================================================
 */

#include "Row.h"

int main()
{
	Row row(4);
	row.setWidth() << 4 << 5 << 6 << endl;
	int a = 5;
	unsigned int b = 10;
	row << 11 << 234 << 456 << endl;
	row << 11 << 234 << 456 << endl;
	row << 11 << 234 << 456 << endl;
	row << a << b << 23 << endl;

	return 0;
}
