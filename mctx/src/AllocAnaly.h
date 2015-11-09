/*
 * =====================================================================================
 *
 *       Filename:  AllocAnaly.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年10月30日 16时25分29秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zj (), xiaojian_whu@163.com
 *        Company:  whu.sklse
 *
 * =====================================================================================
 */

#ifndef ALLOCANALY_H
#define ALLOCANALY_H

#include <string>
#include <vector>

using namespace std;

typedef size_t Size;

/**
 * AllocSetContext中freelist长度最多为16（AllocSetFreeIndex 这个函数限制），所以数组略大于这个值就够了
 * 
 * 表格输出格式如下：
 *                  
 * 		+---------------+-----+-----+------+-----+------+-------+
 * 		|   chunk_size  | 8B  | 16B | 32B  | 64B | >64B | Total |
 *		+---------------+-----+-----+------+-----+------+-------+
 *		| alloced times |  ?  |  ?  |   ?  |  ?  |   ?  |   ?   |
 *		| wasted space  |  ?  |  ?  |   ?  |  ?  |   ?  |   ?   |
 *		| average waste |  ?  |  ?  |   ?  |  ?  |   ?  |   ?   |
 *		| free times    |  ?  |  ?  |   ?  |  ?  |   ?  |   ?   |
 *		| hit rate      |  ?  |  ?  |   ?  |  ?  |   ?  |   ?   |
 *		+---------------+-----+-----+------+-----+------+-------+
 *	表头表示freelist可分配的的每一块chunk的大小，>64B 表示超过64B就会分配一个独立的块，
 *	接下来每行表示对应列大小chunk块的分配次数，浪费空间，平均每次申请浪费空间，释放次数
 *	命中率（分配一个chunk而不是一个独立的block）。
 */

class AllocAnaly
{
	public:
		AllocAnaly(Size minChunkSize, Size allocChunkLimit);
		
		void allocRecord(Size size);

		void freeRecord(Size size);

		void reset();

		void stat(int level);

		void setMinChunkSize(Size minChunkSize)
		{
			this->minChunkSize = MAXALIGN(minChunkSize);
		}

		void setAllocChunkLimit(Size allocChunkLimit);
	private:
		void calculate();

		void setColumnLength();

		void initColumn();

		int  getLength(Size value);
	private:
		/* 注： column是整个表格的cell数，freelist只是表格统计数据部分的cell数，都是从0开始计数*/
		Size	minChunkSize;		/*< 最小的chunk大小 >*/
		Size	allocChunkLimit;	/*< 分配chunk还是独立block的判断值>*/
		int	freeListSize;		/*< 空闲链表的大小 输出表格比它多了3个列>*/
		int	column[21];		/*< 每一列应输出的表格宽度>*/
		int	header[18];		/*< 表头,表示freelist中的chunk大小>*/
		int	alloced[18];		/*< 分配次数统计>*/
		int	wasted[18];		/*< 浪费空间统计>*/
		int	avgWasted[18];		/*< 平均每次分配浪费空间统计>*/
		int	freed[18];		/*< 释放次数统计>*/
		double	hitRate[18];		/*< 命中率统计>*/
		static	string rowName[6];	/*< 表格每一行的名称>*/
};

#endif
