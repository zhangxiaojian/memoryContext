/*
 * =====================================================================================
 *
 *       Filename:  testAnaly.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年10月30日 20时29分27秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zj (), xiaojian_whu@163.com
 *        Company:  whu.sklse
 *
 * =====================================================================================
 */

#include <iostream>
#include "AllocAnaly.h"

using namespace std;

int main()
{
	AllocAnaly analy(8, 128);
	for(int i = 1; i < 200; ++i)
		analy.allocRecord(i);
	for(int i = 1; i < 200; ++i)
		analy.freeRecord(i);
	
	analy.stat(10);
//	analy.reset();
//	analy.stat();
	//string a = "123456";	
	//cout << "sizeof('12345')" << sizeof(a.c_str()) << endl;
	//cout << "sizeof(1234567890)" << sizeof(1234567890) << endl;
	//cout << "sizeof('国家')" << sizeof("国家") << endl;

	return 0;
}
