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

//s1: ���ָ��ַ���
//stok: �ָ��
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

//���ô��ָ��ַ���
void ParseString::setString(char * s1){
	mstr=s1;
}
//���÷ָ��
void ParseString::setStoken(const char * stok){
	strcpy(mStok,stok);
}
//ȡ�÷ָ��
const char * ParseString::getStoken(){
	return mStok;
}
//ִ�зָ���طָ���ַ�����Ŀ
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
//ȡ�÷ָ���ַ�����Ŀ
int ParseString::count(){
	return mCount;
}
//�����±�ȡ�÷ָ���ַ��������n���ڷָ���ַ�����Ŀ���������ݲ�ȷ����
string& ParseString::operator [](int n){
	return mParsedstr[n];
}

//�����ַ������ָ�൱��setString��exec
int ParseString::set_parse(char * s1){
	setString(s1);
	return exec();
}

//ȥ���ַ����еĿո�
std::string& ParseString::trim(std::string& s){
	int k;
	if (string::npos!=(k=s.find_first_not_of(' '))) s.erase(0,k);
	if ((s.length()-1)!=(k=s.find_last_not_of(' '))) s.erase(k+1);
	return s;
}
