// TestCoroutine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include <iostream>
#include <functional>

class CCoro
{
public:
	CCoro(CCoro* P_MainCoroPtr):m_MainCoroPtr(P_MainCoroPtr), m_AddressPtr(NULL){}
	static CCoro* Cur();

	bool IsMain() const {return m_MainCoroPtr == NULL;}

	static CCoro* Main()
	{
		if(Cur()->IsMain())
			return Cur();
		return Cur()->m_MainCoroPtr;
	}

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

	void YieldTo(CCoro* P_YieldToPtr = NULL)
	{
		if(P_YieldToPtr == NULL)
			P_YieldToPtr = Main();
		SwitchToFiber(P_YieldToPtr->m_AddressPtr);
	}

protected:
	virtual void operator()() const =0;

private:
	CCoro(const CCoro&);
	CCoro& operator=(const CCoro&);

	static void CALLBACK StartFunc(void* P_FuncPtr)
	{
		CCoro* W_CoroPtr = (CCoro*)P_FuncPtr;
		(*W_CoroPtr)();
		delete W_CoroPtr;
	}

	CCoro*	m_MainCoroPtr;
	void*	m_AddressPtr;
};

template<class TP_Cb>
class CCoro::FcStartFunc : public CCoro
{
	FcStartFunc(CCoro* P_MainCoroPtr, const TP_Cb& P_Cb):CCoro(P_MainCoroPtr), m_Cb(P_Cb){}
	void operator()() const { m_Cb(); }
	TP_Cb m_Cb;
};

class CCoro::FcMainStartFuncDummy : public CCoro
{
public:
	FcMainStartFuncDummy():CCoro(NULL){}
	void operator()() const { throw std::logic_error("Calling the main coro's main function right? You cannot do that!"); }
};

CCoro* CCoro::Cur()
{
	CCoro* W_ThisPtr = (CCoro*)GetFiberData();
	if(W_ThisPtr != NULL)
		return W_ThisPtr;

	//There is no coro yet... Lets create the first of this thread.
	W_ThisPtr = new FcMainStartFuncDummy;
	W_ThisPtr->m_AddressPtr = ConvertThreadToFiber(W_ThisPtr);
	return W_ThisPtr;
}



using namespace std;
int _tmain(int argc, _TCHAR* argv[])
{

	cout << "Hello world!" << endl;

	char c;
	cin >> c;
	return 0;
}

