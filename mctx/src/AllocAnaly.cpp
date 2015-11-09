/*
 * =====================================================================================
 *
 *       Filename:  AllocAnaly.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年10月30日 17时17分25秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zj (), xiaojian_whu@163.com
 *        Company:  whu.sklse
 *
 * =====================================================================================
 */

#include <cstring>
#include <cassert>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "MemoryContextTool.h"
#include "AllocAnaly.h"
#include "AllocSetContext.h"
#include "Row.h"


string	AllocAnaly::rowName[6] = { 
				"Chunk Size",
				"Alloced Times",
				"Wasted Space",
				"Average Wasted",
				"Free Times",
				"Hit Rate"
			       };

/**
 * 构造函数，获得最小的Chunk大小和allocChunkLimit，同时初始化类中的数组。
 *
 */
AllocAnaly::AllocAnaly(Size minChunkSize, Size allocChunkLimit)
{
	memset(this, 0, sizeof(AllocAnaly));
	this->minChunkSize = MAXALIGN(minChunkSize);
	this->allocChunkLimit = MAXALIGN(allocChunkLimit);
	freeListSize = AllocSetContext::AllocSetFreeIndex(allocChunkLimit);
}

/**记录一次内存分配行为，每次成功分配内存时调用
 *
 * @param size 内存分配的实际大小
 */
void AllocAnaly::allocRecord(Size size)
{
	if(size > allocChunkLimit)
	{
		alloced[freeListSize + 1]++;
	}else
	{
		/* 在对应的chunk大小中增加一次分配次数，迭加浪费的空间*/
		int tids = AllocSetContext::AllocSetFreeIndex(size);
		Size chunk_size = minChunkSize << tids;
		alloced[tids]++;
		wasted[tids] += chunk_size - size;
	}
}

/**记录一次内存释放行为，每次成功释放内存时调用
 *
 * @param size 内存释放的实际大小
 */
void AllocAnaly::freeRecord(Size size)
{
	if(size > allocChunkLimit)
	{
		freed[freeListSize + 1]++;
	}else
	{
		int tids = AllocSetContext::AllocSetFreeIndex(size);
		freed[tids]++;
	}
}

/**重置数据，当内存上下文重置的时候调用。因为其它数据都是根据分配次数，释放次数，
 * 浪费空间大小计算而来，所以只需要重置这几个数据即可。
 *
 */
void AllocAnaly::reset()
{
	memset(alloced, 0, sizeof(alloced));
	memset(wasted, 0, sizeof(wasted));
	memset(freed, 0, sizeof(freed));
}

/**输出统计数据，因为数据在不断变化，表格大小随时在变，所以每次输出统计数据都计算一遍。
 *
 */
void AllocAnaly::stat(int level)
{
	calculate();
	setColumnLength();

	Row row(6, level);

	for(int i = 0; i <= freeListSize + 3; ++i)
	{
		row.setWidth() << column[i];
	}
	row << endl;

	row.centre() << rowName[0];
	for(int i = 0; i <= freeListSize; ++i)
	{
		stringstream str;
		str << (minChunkSize << i) << "B";
		row << str.str();
	}
	stringstream str;
	str << ">" << (minChunkSize << freeListSize) << "B";
	row << str.str();
	row << "Total" << endl;;

	row.left() << rowName[1];
	for(int i = 0; i <= freeListSize + 2; ++i)
	{
		row << alloced[i];
	}
	row << endl;

	row.left() << rowName[2];
	for(int i = 0; i <= freeListSize + 2; ++i)
	{
		row << wasted[i];
	}
	row << endl;
	
	row.left() << rowName[3];
	for(int i = 0; i <= freeListSize + 2; ++i)
	{
		row << avgWasted[i];
	}
	row << endl;
	
	row.left() << rowName[4];
	for(int i = 0; i <= freeListSize + 2; ++i)
	{
		row << freed[i];
	}
	row << endl;
	
	row.left() << rowName[5];
	for(int i = 0; i <= freeListSize + 2; ++i)
	{
		ostringstream out;
		uint temp = (hitRate[i] * 100);
		out << ((double)temp / 100);
		row << out.str();
	}
	row << endl;
}

/**计算表格中的数据，填充每一项。
 * 
 */
void AllocAnaly::calculate()
{
	Size allocTotal = 0;
	Size wastedTotal = 0;
	Size freedTotal = 0;

	for(int i = 0; i <= freeListSize; ++i)
		header[i] = minChunkSize << i;
	
	/* 算出分配，释放，浪费空间大小值的和 */
	for(int i = 0; i <= (freeListSize + 1); ++i)
	{
		allocTotal += alloced[i];
		wastedTotal += wasted[i];
		freedTotal += freed[i];
		alloced[i] == 0 ? avgWasted[i] = 0 : (avgWasted[i] = wasted[i] / alloced[i]);
	}

	/* 计算命中率 */
	for(int i = 0; i <= freeListSize + 1; ++i)
	{
		alloced[i] == 0 ? hitRate[i] = 0 : (hitRate[i] = (double)alloced[i] / (double)allocTotal);
	}

	/* 初始化最后一列 */
	Size end = freeListSize + 2;
	alloced[end] = allocTotal;
	wasted[end] = wastedTotal;
	freed[end] = freedTotal;
	wastedTotal == 0 ? avgWasted[end] = 0 : (avgWasted[end] = wastedTotal / allocTotal);
	
	if(alloced[end - 1] == alloced[end])
	{
		hitRate[end] = 0;
	}else
	{
		hitRate[end] = (double)(allocTotal - alloced[end - 1]) / (double)allocTotal;
	}
}

/**计算输出表格每一列的宽度，值为每一列数据的最大宽度，作为输出时表格宽度所用。
 *
 */
void	AllocAnaly::setColumnLength()
{
	initColumn();
	for(int i = 0; i <= freeListSize + 2; ++i)
	{
		if(getLength(header[i]) > column[i + 1])
			column[i + 1] = getLength(header[i]);
		if(getLength(alloced[i]) > column[i + 1])
			column[i + 1] = getLength(alloced[i]);
		if(getLength(wasted[i]) > column[i + 1])
			column[i + 1] = getLength(wasted[i]);
		if(getLength(avgWasted[i]) > column[i + 1])
			column[i + 1] = getLength(avgWasted[i]);
		if(getLength(freed[i]) > column[i + 1])
			column[i + 1] = getLength(freed[i]);
		if(hitRate[i] > 0 && column[i + 1] < 4)
			column[i + 1] = 4;	/* 命中率保留小数点后两位宽度为4*/

	}
}

/**初始化每一列表格的宽度，初始化值为表头宽度大小。
 *
 */
void	AllocAnaly::initColumn()
{
	/* 根据表头数值大小初始化每一列 */
	column[0] = 14;			/* average wasted */
	column[freeListSize + 3] = 5; 	/* total */

	for(int i = 1; i <= freeListSize + 1; ++i)
	{
		column[i] = getLength(minChunkSize << (i - 1)) + 1;
	}
	column[freeListSize + 2] = column[freeListSize + 1] + 1; /* 多了一个 > 号*/
}

/**获得一个数值的宽度
 *
 */
int	AllocAnaly::getLength(Size value)
{
	int length = 0;
	while(value > 0)
	{
		value = value / 10;
		length++;
	}

	return length == 0 ? 1 : length;
}

void	AllocAnaly::setAllocChunkLimit(Size allocChunkLimit)
{
	this->allocChunkLimit = MAXALIGN(allocChunkLimit);
	freeListSize = AllocSetContext::AllocSetFreeIndex(allocChunkLimit);
}
