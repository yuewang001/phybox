// ParseString.cpp: implementation of the ParseString class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning( disable : 4786 )

#include <iostream>
#include <string.h>
#include "ParseStringSTL.h"

using namespace std;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//s1: 待分割字符串
//stok: 分割符
ParseString::ParseString(char * s1,const char * stok)
{
	mstr=s1;
	strcpy(mStok,stok);
	mCount=0;
}
ParseString::ParseString()
{
	strcpy(mStok,",\n");
}

ParseString::~ParseString()
{

}

//设置待分割字符串
void ParseString::setString(char * s1){
	mstr=s1;
}
//设置分割符
void ParseString::setStoken(const char * stok){
	strcpy(mStok,stok);
}
//取得分割符
const char * ParseString::getStoken(){
	return mStok;
}
//执行分割，返回分割的字符串数目
int ParseString::exec(){
	mCount=0;
	char * s1;
	s1=strtok(mstr,mStok);
	int n=0;
	while (NULL!=s1)
	{
		if (n<mParsedstr.size()) {
			mParsedstr[n]=string(s1);
		} else {
			mParsedstr.push_back(string(s1));
		}
		s1=strtok(NULL,mStok);
		n++;
	}
	mCount=n;
	return mCount;
}
//取得分割后字符串数目
int ParseString::count(){
	return mCount;
}
//按照下标取得分割后字符串，如果n大于分割后字符串数目，返回内容不确定。
string& ParseString::operator [](int n){
	return mParsedstr[n];
}

//设置字符串并分割，相当于setString＋exec
int ParseString::set_parse(char * s1){
	setString(s1);
	return exec();
}

//去除字符串中的空格
std::string& ParseString::trim(std::string& s){
	int k;
	if (string::npos!=(k=s.find_first_not_of(' '))) s.erase(0,k);
	if ((s.length()-1)!=(k=s.find_last_not_of(' '))) s.erase(k+1);
	return s;
}
