/*
 * =====================================================================================
 *
 *       Filename:  Time.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年11月21日 20时46分24秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zj (), xiaojian_whu@163.com
 *        Company:  whu.sklse
 *
 * =====================================================================================
 */

#ifndef TIME_H
#define TIME_H

#include <sys/time.h>
#include <stdio.h>
#include <iostream>
#include <ctime>
#include <string>
#include <fstream>



using namespace std;

/*
 * 时间类，用于获取当前的系统时间，目前只实现一个函数，为了给输出到文件的log添加时间戳，
 * 之所以写成一个类，而不是在log中添加一个函数，是为了系统以后其它地方用到需要获取时间时比较方便。
 * 也可以扩充几个函数，以不同的格式输出时间。
 */
class Time
{
	private:
		static struct timeval beginTime;
	public:
		static void timeToFile(ofstream& out);

		static void start();

		static void stop();
};



#endif
