// TestCoroutine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include <iostream>
#include <functional>
#include <string>
#include "jstd\JCoro.h"
#include <memory>
#include "jstd\JStd.h"

using namespace std;
using namespace JStd::Coro;

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

int G_Nr = 0;

const int G_YieldEvery  = 10;
#ifdef _DEBUG
const int G_LogEvery	= 100000;
const int G_Iter		= 500000;
#else
const int G_LogEvery	= 10000000;
const int G_Iter		= 50000000;
#endif

void IncNr(bool doYield)
{
	int incCount = 0;
	int iLogEvery = G_LogEvery;
//	if(!doYield)
//		iLogEvery *= 10;
	do
	{
		++G_Nr;
		++incCount;
		if(G_Nr % iLogEvery == 0)
			cout << "Reached " << G_Nr << " at iteration " << incCount << endl;
		if(doYield && incCount % G_YieldEvery == 0)
			yield();
	}while(doYield);
}

void TestPerformance()
{
	cout << "Testing coro performance..." << endl;

	std::tr1::shared_ptr<CCoro> W_Inc1Ptr(CCoro::Create(std::tr1::bind(&IncNr, true)));
	std::tr1::shared_ptr<CCoro> W_Inc2Ptr(CCoro::Create(std::tr1::bind(&IncNr, true)));

	for(int i=0; i<G_Iter/G_YieldEvery; ++i)
	{
		W_Inc1Ptr->yield();
		W_Inc2Ptr->yield();
	}

	G_Nr = 0;
	cout << "Now testing without coro's..." << endl;
	for(int i=0; i<G_Iter; ++i)
	{
		IncNr(false);
		IncNr(false);
	}

	cout << "Done." << endl;
}


typedef CCoroutine<int, std::wstring> CTestCoro;
void DoSome(CTestCoro::self& P_Self, std::wstring P_csStr)
{
	while(true)
		P_csStr = P_Self.yield(MessageBox(NULL, P_csStr.c_str(), P_csStr.c_str(), 0));
}

void CountChars(CTestCoro::self& P_Self, std::wstring P_csStr)
{
	while(true)
		P_csStr = P_Self.yield(P_csStr.size());
}


typedef CCoroutine<void, void> CVoidCoro;

void DoSomeElse(CVoidCoro::self& P_Self)
{
	while(true)
	{
		MessageBox(NULL, L"Nothing", L"Hmmm... Nothing.", 0);
		P_Self.yield();
	}
}

typedef CCoroutine<std::string, void> CStringCoro;

void ReturnSomeStuff(CStringCoro::self& P_Self)
{
	while(true)
	{
		MessageBox(NULL, L"Return something", L"Hmmm... Something.", 0);
		P_Self.yield("Return!");
	}
}
class CBla
{
public:
	CBla(){cout << "hello" << endl;}
	CBla(const std::string& P_s){cout << "hello: " << P_s << endl;}
	~CBla(){cout << "bye" << endl;}
};


template<class TP_Return>
class CFuture
{
public:
	TP_Return m_Val;
};

int _tmain(int argc, _TCHAR* argv[])
{
	{
		JStd::COptional<CBla> W_1;
		JStd::COptional<CBla> W_2;
		JStd::COptional<CBla> W_3;
		JStd::COptional<CBla> W_4;
		W_2();
		W_3("hoi");

		if(W_1)
			cout << "W1 is ok" << endl;
		if(W_2)
			cout << "W2 is ok" << endl;
	}

	CMainCoro W_MainCoro = CCoro::Initialize();

	cout << "Hello world!" << endl;

	using namespace std::tr1::placeholders;
	CTestCoro W_Coro(std::tr1::bind(&DoSome, _1, _2));

	cout << "User said " << W_Coro(L"Hello") << " on hello" << endl;
	cout << "User said " << W_Coro(L"World") << " on world" << endl;


	CTestCoro W_LengthCoro(&CountChars);

	cout
		<< "Length of hi: " << W_LengthCoro(L"hi") << endl
		<< "Length of bye: " << W_LengthCoro(L"bye") << endl;

	(CVoidCoro(&DoSomeElse))();
//	CVoidCoro W_VoidCoro(&DoSomeElse);
//	W_VoidCoro();

	cout << "Coro said: " << (CStringCoro(&ReturnSomeStuff))() << endl;

	CCoro* W_FuckPtr = CCoro::Create(std::tr1::bind(&Fuck));
	CCoro* W_YouPtr = CCoro::Create(std::tr1::bind(&You));

	for(int i = 0; i < 10; ++i)
	{
		W_FuckPtr->yield();
		W_YouPtr->yield();
	}

	delete W_FuckPtr;
	delete W_YouPtr;

	TestPerformance();

	char c;
	cin >> c;
	return 0;
}

