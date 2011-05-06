#include "StdAfx.h"
#include "JCoro.h"
#include <stdexcept>

namespace JCoro
{

DWORD G_dwCoroSlotIx = 0;

DWORD GetCoroSlotIx()
{
	if(G_dwCoroSlotIx == 0)
		G_dwCoroSlotIx = FlsAlloc(NULL);

	return G_dwCoroSlotIx;
}

CCoro* GetCurCoroPtr()
{
	return (CCoro*)FlsGetValue(GetCoroSlotIx());
}

void SetCurCoroPtr(CCoro* P_CoroPtr)
{
	FlsSetValue(GetCoroSlotIx(), P_CoroPtr);
}

bool IsFiber()
{
	void* W_FiberPtr = GetCurrentFiber();
	return W_FiberPtr != NULL && (int)W_FiberPtr != 0x00001E00; //Magic number when not in fiber mode. It is in that case the version of threads or something. The windows API will provide its own IsFiber() function in the future.
}

bool IsCoro()
{
	return GetCurCoroPtr() != NULL;
}

CCoro::CCoro(CCoro* P_MainCoroPtr)
:	m_MainCoroPtr(P_MainCoroPtr),
	m_AddressPtr(NULL),
	m_bEnded(false),
	m_YieldingCoroPtr(NULL),
	m_eExit(eA_No),
	m_DefaultYieldCoroPtr(NULL)
{
}

CCoro::~CCoro()
{
	if(IsMain())
		return;

	if(!m_bEnded)
		Exit();

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

class CMainCoroutine : public CCoro, public std::tr1::enable_shared_from_this<CMainCoroutine>
{
friend CCoro;
public:
	CMainCoroutine():CCoro(NULL), m_bWasAlreadyFiber(IsFiber()){}
	virtual ~CMainCoroutine()
	{
		if(!m_bWasAlreadyFiber)
			ConvertFiberToThread();
	}

	bool m_bWasAlreadyFiber;
	void operator()() const { throw std::logic_error("Calling the main coro's main function right? You cannot do that!"); }
};

CMainCoro CCoro::Initialize()
{
	//Lets check if there already was a main coro
	CMainCoro W_Coro;
	if(IsCoro())
	{
		//Yea we already got one.
		W_Coro.m_MainCoroPtr = ((CMainCoroutine*)Main())->shared_from_this();
		return W_Coro;
	}

	//There should be no coro yet... Lets create the first one of this thread.
	std::tr1::shared_ptr<CMainCoroutine> W_ThisPtr(new CMainCoroutine);
	if(W_ThisPtr->m_bWasAlreadyFiber)
	{
		W_ThisPtr->m_AddressPtr = GetCurrentFiber();
	}
	else if((W_ThisPtr->m_AddressPtr = ConvertThreadToFiber(NULL)) == NULL)
	{
		throw std::runtime_error("Cannot initialize coroutines.");
	}
	SetCurCoroPtr(W_ThisPtr.get());
	W_Coro.m_MainCoroPtr = W_ThisPtr;
	return W_Coro;
}



CCoro* CCoro::Cur()
{
	CCoro* W_ThisPtr = GetCurCoroPtr();
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

void CCoro::Exit()
{
	if(Ended())
		throw std::logic_error("Exiting an ended coroutine.");

	m_eExit = eA_ExitInitiated;
	yield();
}

void CCoro::OnResume()
{
	if(m_eExit == eA_ExitInitiated)
	{
		//Someone wants me to exit... Well lets obey that then.
		m_eExit = eA_ExitRunning;
		throw CExitException();
	}
}

void CALLBACK CCoro::StartFunc(void* P_FuncPtr)
{
	CCoro* W_CoroPtr = (CCoro*)P_FuncPtr;
	SetCurCoroPtr(W_CoroPtr);
	try
	{
		W_CoroPtr->OnResume();
		(*W_CoroPtr)();
	}
	catch(CExitException&)
	{
		W_CoroPtr->m_eExit = eA_ExitDone;
	}
	W_CoroPtr->m_bEnded = true;
	YieldDefault();//Yield to default. I've now become useless...
}


void yield()
{
	CCoro::YieldDefault();
}

CMainCoro Initialize()
{
	return CCoro::Initialize();
}


}