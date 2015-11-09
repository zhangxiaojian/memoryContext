/*
 * =====================================================================================
 *
 *       Filename:  Row.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年11月03日 20时40分25秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zj (), xiaojian_whu@163.com
 *        Company:  whu.sklse
 *
 * =====================================================================================
 */

#include "Row.h"

/**
 * @param num 一共将会输出多少行
 * @param lev 输出的等级，用来表示每行的缩进（不指定则为0）
 */
Row::Row(uint num, uint lev):
	rowNum(num),
	rowRecord(1),
	size(0),
	column(0),
	level(lev),
	header(true),
	isExceed(true),
	isLack(true),
	r_left(true),
	r_centre(false),
	r_right(false),
	r_width(false)
{

}

/**输出无符号整形值
 *
 */
Row &Row::operator << (uint value)
{
	/*设置cell宽度状态*/
	if(r_width == true)
	{
		widths.push_back(value);
		return *this;
	}
	
	/* 如果是第一行，就先输出一行作为表格头*/
	if(rowRecord == 1 && header)
	{
		splitLine();
		header = false;
	}

	/* 如果是第一列，先输出缩进量*/
	if(column == 0)
	{
		cout << getPad(level);
	}

	/* 如果输出的列超过了colunm初始化的列，是否需要截断后面的输出*/
	if(column >= size)
	{
		if(!isExceed)
			cout << " | " << value;
		return *this;
	}

	/*根据不同的状态输出值，默认左对齐，可以设置居中对齐或者右对齐*/
	uint valuewidth = getWidth(value);
	uint cellwidth = widths[column++];
	uint space = cellwidth - valuewidth;

	if(size == 0 || valuewidth >= cellwidth)
	{
		cout << " | " << value;
	}else if(r_right == true)
	{
		cout << " | " << stringSpace(space) << value;
	}else if(r_centre == true)
	{
		cout << " | " << stringSpace(space / 2) << value 
			<< stringSpace(space - (space / 2));
	}else
	{
		cout << " | " << value << stringSpace(space);
	}

	return *this;
}	

/**输出string类型数据
 *
 */
Row &Row::operator << (string value)
{
	if(r_width == true)
	{
		cerr << "we need a number to set cell width" << endl;
		return *this;
	}
	
	if(rowRecord == 1 && header)
	{
		splitLine();
		header = false;
	}

	if(column == 0)
	{
		cout << getPad(level);
	}

	if(column >= size)
	{
		if(!isExceed)
			cout << " | " << value;
		return *this;
	}

	/*输出值*/
	uint valuewidth = value.size();
	uint cellwidth = widths[column++];
	uint space = cellwidth - valuewidth;

	if(size == 0 || valuewidth >= cellwidth)
	{
		cout << " | " << value;
	}else if(r_right == true)
	{
		cout << " | " << stringSpace(space) << value;
	}else if(r_centre == true)
	{
		cout << " | " << stringSpace(space / 2) << value 
			<< stringSpace(space - (space / 2));
	}else
	{
		cout << " | " << value << stringSpace(space);
	}

	return *this;
}

/**输出endl
 * 1 表示设置宽度的结束
 * 2 如果输入的值不够colunm初始化的数量 是否空白补齐
 * 3 输出endl到cout
 * 4 如果是最后一行 输出一行作为表格结束
 *
 */
Row &Row::operator << (ostream &(pf)(ostream&))
{
	if(r_width == true)
	{
		size = widths.size();
		r_width = false;
		return *this;
	}


	while(column < size && isLack)
	{
		(*this) << stringSpace(widths[column++]);
	}

	column = 0;
	cout << " |" << endl;

	if(rowRecord == 1 || rowRecord == rowNum)
		splitLine();
	rowRecord++;

	return	*this;
}

/**获得一个数值的宽度
 *
 */
uint	Row::getWidth(uint value)
{
	uint width = 0;
	while(value > 0)
	{
		value = value / 10;
		width++;
	}

	return width == 0 ? 1 : width;
}

/**根据缩进层级得到一个空白字符的字符串，用来输出缩进
 *
 */
string Row::getPad(int level)
{
	string pad;
	for(int i = 0; i < level; ++i)
		pad = pad + "  ";
	return pad;
}

/**返回一定数量的空格字符
 *
 * @num		空格数量
 * @string 	全是空格的字符串
 */
string	Row::stringSpace(uint num)
{
	string space;
	while(num--)
		space += " ";
	return space;
}

/**输出一条分割线 +----+------+
 *
 */
void	Row::splitLine()
{
	if(size == 0)
		return;

	string line = getPad(level) + " +";
	for(uint i = 0; i < size; ++i)
	{
		uint count = widths[i] + 2;
		while(count--)
			line += "-";
		line += "+";
	}
	cout << line << endl;
}
