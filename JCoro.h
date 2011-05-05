#pragma once
#include <windows.h>

namespace JCoro
{

class CAbortException
{
};

class CCoro
{
public:
	CCoro(CCoro* P_MainCoroPtr);
	virtual ~CCoro();

	static CCoro*	Initialize();
	static void		Deinitialize();

	static CCoro* Cur();

	bool IsMain() const;

	static CCoro* Main();

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

	static void YieldDefault();
	void		yield(CCoro* P_YieldBackCoroPtr = Cur());
	void		Abort();
	bool		Ended() const;



protected:
	virtual void operator()() const =0;

	void OnResume();

private:
	CCoro(const CCoro&);
	CCoro& operator=(const CCoro&);

	static void CALLBACK StartFunc(void* P_FuncPtr);

	enum eAbort { eA_No, eA_AbortInitiated, eA_AbortRunning, eA_AbortDone };

	CCoro*	m_MainCoroPtr;
	CCoro*	m_YieldingCoroPtr;
	CCoro*	m_DefaultYieldCoroPtr;
	void*	m_AddressPtr;
	bool	m_bEnded;
	eAbort	m_eAbort;
};

template<class TP_Cb>
class CCoro::FcStartFunc : public CCoro
{
friend CCoro;
	FcStartFunc(CCoro* P_MainCoroPtr, const TP_Cb& P_Cb):CCoro(P_MainCoroPtr), m_Cb(P_Cb){}
	void operator()() const { m_Cb(); }
	TP_Cb m_Cb;
};

CCoro*	Initialize();
void	Deinitialize();
void	yield(); //No capital y, because winbase.h defines Yield() as a macro, pfff.

template<class TP_Cb>
static CCoro* Create(const TP_Cb& P_Cb) { return CCoro::Create(P_Cb); }

}