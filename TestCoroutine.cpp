// TestCoroutine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include <iostream>
#include <functional>
#include "JCoro.h"

using namespace std;
using namespace JCoro;

void Fuck()
{
	for(int i = 0; i < 10; ++i)
	{
		cout << "Fuck ";
		CCoro::YieldTo();
	}
}

void You()
{
	for(int i = 0; i < 10; ++i)
	{
		cout << "You!" << endl;
		CCoro::YieldTo();
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	CCoro::Initialize();
	cout << "Hello world!" << endl;

	CCoro* W_FuckPtr = CCoro::Create(std::tr1::bind(&Fuck));
	CCoro* W_YouPtr = CCoro::Create(std::tr1::bind(&You));

	for(int i = 0; i < 10; ++i)
	{
		CCoro::YieldTo(W_FuckPtr);
		CCoro::YieldTo(W_YouPtr);
	}


	char c;
	cin >> c;
	return 0;
}

