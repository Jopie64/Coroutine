// TestCoroutine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include <iostream>
#include <functional>

class CCoro
{
public:
	CCoro(CCoro* P_MainCoroPtr):m_MainCoroPtr(P_MainCoroPtr), m_AddressPtr(NULL), m_bEnded(false), m_YieldingCoroPtr(NULL){}
	~CCoro(){ if(m_AddressPtr != NULL)DeleteFiber(m_AddressPtr); }
	static CCoro* Cur();

	bool IsMain() const {return m_MainCoroPtr == NULL;}

	static CCoro* Main()
	{
		if(Cur()->IsMain())
			return Cur();
		return Cur()->m_MainCoroPtr;
	}

	static CCoro* Initialize();

	template<class TP_Cb>
	class FcStartFunc;

	class FcMainStartFuncDummy;

	template<class TP_Cb>
	static CCoro* Create(const TP_Cb& P_Cb)
	{
		CCoro* W_CoroPtr = new FcStartFunc<TP_Cb>(Main(), P_Cb);
		if((W_CoroPtr->m_AddressPtr = CreateFiber(0, &CCoro::StartFunc, W_CoroPtr)) == NULL)
			throw std::runtime_error("Error creating fiber.");
		return W_CoroPtr;
	}

	static void YieldTo(CCoro* P_YieldToPtr = NULL)
	{
		if(P_YieldToPtr == NULL)
			P_YieldToPtr = Main();
		P_YieldToPtr->m_YieldingCoroPtr = Cur();
		SwitchToFiber(P_YieldToPtr->m_AddressPtr);
		Cur()->OnResume();
	}



protected:
	virtual void operator()() const =0;

	void OnResume()
	{
		if(m_YieldingCoroPtr->m_bEnded)
		{
			//Coro yielding to me ended. Lets clean it up first.
			delete m_YieldingCoroPtr;
			m_YieldingCoroPtr = NULL;
		}
	}
private:
	CCoro(const CCoro&);
	CCoro& operator=(const CCoro&);

	static void CALLBACK StartFunc(void* P_FuncPtr)
	{
		CCoro* W_CoroPtr = (CCoro*)P_FuncPtr;
		W_CoroPtr->OnResume();
		(*W_CoroPtr)();
		W_CoroPtr->m_bEnded = true;
		YieldTo();//Yield to main and kill me
	}

	CCoro*	m_MainCoroPtr;
	CCoro*	m_YieldingCoroPtr;
	void*	m_AddressPtr;
	bool	m_bEnded;
};

template<class TP_Cb>
class CCoro::FcStartFunc : public CCoro
{
friend CCoro;
	FcStartFunc(CCoro* P_MainCoroPtr, const TP_Cb& P_Cb):CCoro(P_MainCoroPtr), m_Cb(P_Cb){}
	void operator()() const { m_Cb(); }
	TP_Cb m_Cb;
};

class CCoro::FcMainStartFuncDummy : public CCoro
{
friend CCoro;
	FcMainStartFuncDummy():CCoro(NULL){}
	void operator()() const { throw std::logic_error("Calling the main coro's main function right? You cannot do that!"); }
};

CCoro* CCoro::Initialize()
{
	//There should be no coro yet... Lets create the first one of this thread.
	CCoro* W_ThisPtr = new FcMainStartFuncDummy;
	W_ThisPtr->m_AddressPtr = ConvertThreadToFiber(W_ThisPtr);
	return W_ThisPtr;
}

CCoro* CCoro::Cur()
{
	CCoro* W_ThisPtr = (CCoro*)GetFiberData();
	if(W_ThisPtr == NULL)
		throw std::runtime_error("Hey, I was expecting a Coro* fiber data structure, but I found NULL for this fiber!");
		
	return W_ThisPtr;
}


using namespace std;

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

