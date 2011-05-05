#include "StdAfx.h"
#include "JCoro.h"
#include <stdexcept>

//TODO: Ensure cooperation with other fiber using libraries (.NET for example) will work.

namespace JCoro
{

CCoro::CCoro(CCoro* P_MainCoroPtr)
:	m_MainCoroPtr(P_MainCoroPtr),
	m_AddressPtr(NULL),
	m_bEnded(false),
	m_YieldingCoroPtr(NULL),
	m_eAbort(eA_No),
	m_DefaultYieldCoroPtr(NULL)
{
}

CCoro::~CCoro()
{
	if(IsMain())
	{
		ConvertFiberToThread();
		return;
	}

	if(!m_bEnded)
		Abort();

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
	if((W_ThisPtr->m_AddressPtr = ConvertThreadToFiber(W_ThisPtr)) == NULL)
	{
		delete W_ThisPtr;
		throw std::runtime_error("Cannot initialize coroutines.");
	}
	return W_ThisPtr;
}

void CCoro::Deinitialize()
{
	delete Main();
}


CCoro* CCoro::Cur()
{
	CCoro* W_ThisPtr = (CCoro*)GetFiberData();
	if(W_ThisPtr == NULL)
		throw std::runtime_error("Hey, I was expecting a Coro* fiber data structure, but I found NULL for this fiber!");
		
	return W_ThisPtr;
}


void CCoro::yield(CCoro* P_YieldBackCoroPtr)
{
	if(this == Cur())
	{
		//Yielded to self. This would be an error if the user realy wanted to do this because we are already in self.
		//Asuming the user intended to yield the default because is has the same function name but not in this class.
		YieldDefault();
		return;
	}

	if(m_bEnded)
		throw std::logic_error("Yielding to an ended coroutine.");

	if(P_YieldBackCoroPtr != NULL && P_YieldBackCoroPtr->Ended())
		throw std::logic_error("Cannot set yieldback to an ended coroutine.");

	if(m_DefaultYieldCoroPtr != NULL && P_YieldBackCoroPtr != NULL && m_DefaultYieldCoroPtr != P_YieldBackCoroPtr)
		throw std::logic_error("Canot set yieldback, because coroutine is already set to yield back to a different coroutine.");

	if(!Cur()->Ended())
		m_YieldingCoroPtr = Cur();

	if(P_YieldBackCoroPtr != NULL)
		m_DefaultYieldCoroPtr = P_YieldBackCoroPtr;

	SwitchToFiber(m_AddressPtr);
	Cur()->OnResume();
}

void CCoro::YieldDefault()
{
	CCoro* W_CoroPtr = NULL;
	if(Cur()->m_DefaultYieldCoroPtr != NULL)
	{
		W_CoroPtr = Cur()->m_DefaultYieldCoroPtr;
		Cur()->m_DefaultYieldCoroPtr = NULL;
	}
	else
		W_CoroPtr = Main();

	W_CoroPtr->yield(NULL);
}

bool CCoro::Ended() const
{
	return m_bEnded;
}

void CCoro::Abort()
{
	if(Ended())
		throw std::logic_error("Aborting an ended coroutine.");

	m_eAbort = eA_AbortInitiated;
	yield();
}

void CCoro::OnResume()
{
	if(m_eAbort == eA_AbortInitiated)
	{
		//Someone wants me to abort... Well lets obey that then.
		m_eAbort = eA_AbortRunning;
		throw CAbortException();
	}
}

void CALLBACK CCoro::StartFunc(void* P_FuncPtr)
{
	CCoro* W_CoroPtr = (CCoro*)P_FuncPtr;
	try
	{
		W_CoroPtr->OnResume();
		(*W_CoroPtr)();
	}
	catch(CAbortException&)
	{
		W_CoroPtr->m_eAbort = eA_AbortDone;
	}
	W_CoroPtr->m_bEnded = true;
	YieldDefault();//Yield to default and kill me
}


void yield()
{
	CCoro::YieldDefault();
}

CCoro* Initialize()
{
	return CCoro::Initialize();
}

void Deinitialize()
{
	CCoro::Deinitialize();
}


}