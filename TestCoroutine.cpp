// TestCoroutine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include <iostream>
#include <functional>
#include <string>
#include "JCoro.h"
#include <memory>

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

int _tmain(int argc, _TCHAR* argv[])
{
	CMainCoro W_MainCoro = CCoro::Initialize();
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

	TestPerformance();

	char c;
	cin >> c;
	return 0;
}

