// ParseString.h: interface for the ParseString class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PARSESTRING_H__30D700F9_5CE0_474D_98D0_AE84E46A1E3A__INCLUDED_)
#define AFX_PARSESTRING_H__30D700F9_5CE0_474D_98D0_AE84E46A1E3A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
//�����趨�ķָ�������ַ����ָ�Ϊ��Ρ�
//ʹ��string
#include <string>
#include <vector>

typedef std::vector<std::string> VStr;

class ParseString  
{
public:
	//s1: ���ָ��ַ���
	//stok: �ָ��
	ParseString(char* s1, const char* stok=",\n");
	ParseString();
	virtual ~ParseString();
	//���ô��ָ��ַ���
	void setString(char* s1);
	//���÷ָ��
	void setStoken(const char* stok);
	//ȡ�÷ָ��
	const char* getStoken();
	//ִ�зָ���طָ���ַ�����Ŀ
	int exec();
	//ȡ�÷ָ���ַ�����Ŀ
	int count();
	//�����±�ȡ�÷ָ���ַ��������n���ڷָ���ַ�����Ŀ�����ؿ����ݡ�
	std::string& operator [](int n);
	//�����ַ������ָ�൱��setString��exec
	int set_parse(char* s1);
	//ȥ���ַ����еĿո�
	static std::string& trim(std::string& s);

private:
	//���ָ��ַ���
	char* mstr;
	//�ָ��
	char mStok[10];
	//�洢�ָ���ַ���
	VStr mParsedstr;
	//�ָ���ַ�����Ŀ
	int mCount;
};

#endif // !defined(AFX_PARSESTRING_H__30D700F9_5CE0_474D_98D0_AE84E46A1E3A__INCLUDED_)
