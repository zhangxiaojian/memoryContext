/*
 * =====================================================================================
 *
 *       Filename:  Time.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年07月27日 15时58分48秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zj (), xiaojian_whu@163.com
 *        Company:  whu.sklse
 *
 * =====================================================================================
 */

#include "Time.h"

struct timeval Time::beginTime = timeval();

void Time::timeToFile(ofstream& out)
{
	time_t now;
	struct tm* fmt;

	time(&now);
	fmt = localtime(&now);

	out << "["<< fmt->tm_year + 1900 << "/" << fmt->tm_mon + 1 << "/" << fmt->tm_mday << " ";
	out << fmt->tm_hour << ":" << fmt->tm_min << ":" << fmt->tm_sec << "] ";
}

void Time::start()
{	
	gettimeofday(&beginTime,NULL);
}

void Time::stop()
{
	struct timeval endTime;
	gettimeofday(&endTime,NULL);

	size_t	sec = endTime.tv_sec - beginTime.tv_sec;
	size_t	usec;
	if(endTime.tv_usec > beginTime.tv_usec) {
		usec = endTime.tv_usec - beginTime.tv_usec;
	} else {
		usec = 1000000 + endTime.tv_usec - beginTime.tv_usec;
		sec--;
	}

	size_t	minute = sec / 60;
	size_t	ssec = sec - minute * 60;
	size_t	msec = usec / 1000;
	cout << "Time elapsed: ";
	if(minute > 0)
		cout << minute << "m";
	if(minute > 0 || ssec > 0)
		cout << ssec << "s";
	if(minute == 0) {
		if(ssec != 0 || msec != 0)
			cout << msec << "ms";
		if(ssec == 0)
			cout << usec % 1000 << "us";
	}
	cout << endl;
}

