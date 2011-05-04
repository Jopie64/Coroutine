#include "StdAfx.h"
#include "JCoro.h"
#include <stdexcept>

namespace JCoro
{

CCoro::CCoro(CCoro* P_MainCoroPtr)
:	m_MainCoroPtr(P_MainCoroPtr),
	m_AddressPtr(NULL),
	m_bEnded(false),
	m_YieldingCoroPtr(NULL)
{
}

CCoro::~CCoro()
{
	if(m_AddressPtr != NULL)
		DeleteFiber(m_AddressPtr);
}

bool CCoro::IsMain() const 
{
	return m_MainCoroPtr == NULL;
}

CCoro* CCoro::Main()
{
	if(Cur()->IsMain())
		return Cur();
	return Cur()->m_MainCoroPtr;
}

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


void CCoro::YieldTo(CCoro* P_YieldToPtr)
{
	if(P_YieldToPtr == NULL)
		P_YieldToPtr = Main();
	P_YieldToPtr->m_YieldingCoroPtr = Cur();
	SwitchToFiber(P_YieldToPtr->m_AddressPtr);
	Cur()->OnResume();
}

void CCoro::OnResume()
{
	if(m_YieldingCoroPtr->m_bEnded)
	{
		//Coro yielding to me ended. Lets clean it up first.
		delete m_YieldingCoroPtr;
		m_YieldingCoroPtr = NULL;
	}
}

void CALLBACK CCoro::StartFunc(void* P_FuncPtr)
{
	CCoro* W_CoroPtr = (CCoro*)P_FuncPtr;
	W_CoroPtr->OnResume();
	(*W_CoroPtr)();
	W_CoroPtr->m_bEnded = true;
	YieldTo();//Yield to main and kill me
}

}