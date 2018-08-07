// ParseString.h: interface for the ParseString class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PARSESTRING_H__30D700F9_5CE0_474D_98D0_AE84E46A1E3A__INCLUDED_)
#define AFX_PARSESTRING_H__30D700F9_5CE0_474D_98D0_AE84E46A1E3A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
//按照设定的分割符，将字符串分割为多段。
//使用string
#include <string>
#include <vector>

typedef std::vector<std::string> VStr;

class ParseString  
{
public:
	//s1: 待分割字符串
	//stok: 分割符
	ParseString(char* s1, const char* stok=",\n");
	ParseString();
	virtual ~ParseString();
	//设置待分割字符串
	void setString(char* s1);
	//设置分割符
	void setStoken(const char* stok);
	//取得分割符
	const char* getStoken();
	//执行分割，返回分割的字符串数目
	int exec();
	//取得分割后字符串数目
	int count();
	//按照下标取得分割后字符串，如果n大于分割后字符串数目，返回空内容。
	std::string& operator [](int n);
	//设置字符串并分割，相当于setString＋exec
	int set_parse(char* s1);
	//去除字符串中的空格
	static std::string& trim(std::string& s);

private:
	//待分割字符串
	char* mstr;
	//分割符
	char mStok[10];
	//存储分割后字符串
	VStr mParsedstr;
	//分割后字符串数目
	int mCount;
};

#endif // !defined(AFX_PARSESTRING_H__30D700F9_5CE0_474D_98D0_AE84E46A1E3A__INCLUDED_)
