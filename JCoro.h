#pragma once
#include <windows.h>

namespace JCoro
{

class CCoro
{
public:
	CCoro(CCoro* P_MainCoroPtr);
	virtual ~CCoro();

	static CCoro* Initialize();

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

	static void YieldTo(CCoro* P_YieldToPtr = NULL);



protected:
	virtual void operator()() const =0;

	void OnResume();

private:
	CCoro(const CCoro&);
	CCoro& operator=(const CCoro&);

	static void CALLBACK StartFunc(void* P_FuncPtr);

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

}