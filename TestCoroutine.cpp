// TestCoroutine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include <iostream>
#include <functional>
#include <string>
#include "JCoro.h"

using namespace std;
using namespace JCoro;

class CEndTxt
{
public:
	CEndTxt(const char* P_szTxtPtr):m_csTxt(P_szTxtPtr){}

	~CEndTxt(){ cout << m_csTxt << endl; }
	string m_csTxt;
};

void Fuck()
{
	CEndTxt W_End("Fuck, I stopped!");
	while(true)
	{
		cout << "Fuck ";
		yield();
	}
}

void You()
{
	CEndTxt W_End("You, stopped me!");
	while(true)
	{
		cout << "You!" << endl;
		yield();
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	CCoro* W_MainPtr = CCoro::Initialize();
	cout << "Hello world!" << endl;

	CCoro* W_FuckPtr = CCoro::Create(std::tr1::bind(&Fuck));
	CCoro* W_YouPtr = CCoro::Create(std::tr1::bind(&You));

	for(int i = 0; i < 10; ++i)
	{
		W_FuckPtr->yield();
		W_YouPtr->yield();
	}

	delete W_FuckPtr;
	delete W_YouPtr;

	delete W_MainPtr;

	char c;
	cin >> c;
	return 0;
}

