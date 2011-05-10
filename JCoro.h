#pragma once
#include <windows.h>
#include <memory>
#include <functional>
#include <stdexcept>

namespace JCoro
{

class CExitException
{
};

class CCoro;

class CMainCoro
{
public:
	CCoro*	Get(){return m_MainCoroPtr.get();}
	CCoro*	operator->(){return Get();}
	CCoro&	operator*(){return *Get();}
private:
	std::tr1::shared_ptr<CCoro> m_MainCoroPtr;
	friend CCoro;
};

class CCoro
{
public:
	CCoro(CCoro* P_MainCoroPtr);
	virtual ~CCoro();

	static CMainCoro	Initialize();

	static CCoro* Cur();

	bool IsMain() const;

	static CCoro* Main();

	template<class TP_Cb>
	class FcStartFunc;

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
	void		Exit();
	bool		Ended() const;



protected:
	virtual void operator()() const =0;

	void OnResume();

private:
	CCoro(const CCoro&);
	CCoro& operator=(const CCoro&);

	static void CALLBACK StartFunc(void* P_FuncPtr);

	enum eExit { eA_No, eA_ExitInitiated, eA_ExitRunning, eA_ExitDone };

	CCoro*	m_MainCoroPtr;
	CCoro*	m_YieldingCoroPtr;
	CCoro*	m_DefaultYieldCoroPtr;
	void*	m_AddressPtr;
	bool	m_bEnded;
	eExit	m_eExit;
};

template<class TP_Cb>
class CCoro::FcStartFunc : public CCoro
{
friend CCoro;
	FcStartFunc(CCoro* P_MainCoroPtr, const TP_Cb& P_Cb):CCoro(P_MainCoroPtr), m_Cb(P_Cb){}
	void operator()() const { m_Cb(); }
	TP_Cb m_Cb;
};

CMainCoro	Initialize();
void		yield(); //No capital y, because winbase.h defines Yield() as a macro, pfff.

template<class TP_Cb>
static CCoro* Create(const TP_Cb& P_Cb) { return CCoro::Create(P_Cb); }


class CCoroutineBase
{
public:

	virtual ~CCoroutineBase()
	{
		delete m_CoroPtr;
	}

	bool IsValid() const { return m_CoroPtr != NULL; }
	void EnsureValid() const { if(!IsValid()) throw std::logic_error("Used uninitialized coroutine."); }


	typedef bool (CCoroutineBase::*T_IsValidFuncPtr)() const;
	operator T_IsValidFuncPtr () const {return IsValid() ? &CCoroutineBase::IsValid : NULL;}

protected:
	CCoroutineBase(CCoro* P_CoroPtr)
	:	m_CoroPtr(P_CoroPtr)
	{
	}

	void Init(CCoro* P_CoroPtr) { m_CoroPtr = P_CoroPtr; }

private:
	CCoroutineBase(const CCoroutineBase&);
	CCoroutineBase& operator=(const CCoroutineBase&);

protected:
	CCoro*	m_CoroPtr;
};


template<class TP_Out, class TP_In>
class CCoroutine : public CCoroutineBase
{
public:

	class self
	{
	public:
		self(CCoroutine* P_ThisPtr):m_ThisPtr(P_ThisPtr){}

		TP_In yield(const TP_Out& P_Out)
		{
			m_ThisPtr->m_Out.first = P_Out;
			m_ThisPtr->m_Out.second = true;
			JCoro::yield();
			if(!m_ThisPtr->m_In.second)
				throw std::logic_error("Unexpected yield to this coro.");
			m_ThisPtr->m_In.second = false; //invalidate
			return m_ThisPtr->m_In.first;
		}

	private:
		CCoroutine* m_ThisPtr;
	};

	typedef std::tr1::function<void (self&, TP_In)> T_Func;

	template<class TP_Cb>
	CCoroutine(const TP_Cb& P_Cb)
	: CCoroutineBase(NULL), m_In(TP_In(), false), m_Out(TP_Out(), false)
	{
		Init(P_Cb);
	}


	TP_Out operator()(const TP_In& P_In)
	{
		EnsureValid();
		m_In.first = P_In;
		m_In.second = true;
		m_CoroPtr->yield();
		if(!m_Out.second)
			throw std::logic_error("No return value received.");
		return m_Out.first;
	}

	template<class TP_Cb>
	void Init(const TP_Cb& P_Cb)
	{
		delete m_CoroPtr;
		m_Func = P_Cb;
		CCoroutineBase::Init(Create(std::tr1::bind(&CCoroutine::Start, this)));
	}

private:
	void Start()
	{
		//ASSERT(m_In.second);
		m_In.second = false;
		m_Func(self(this), m_In.first);
	}



	T_Func	m_Func;

	std::pair<TP_In, bool>	m_In;
	std::pair<TP_Out, bool>	m_Out;
};



template<class TP_Out>
class CCoroutine<TP_Out, void> : public CCoroutineBase
{
public:

	class self
	{
	public:
		self(CCoroutine* P_ThisPtr):m_ThisPtr(P_ThisPtr){}

		void yield(const TP_Out& P_Out)
		{
			m_ThisPtr->m_Out.first = P_Out;
			m_ThisPtr->m_Out.second = true;
			JCoro::yield();
		}

	private:
		CCoroutine* m_ThisPtr;
	};

	typedef std::tr1::function<void (self&)> T_Func;

	template<class TP_Cb>
	CCoroutine(const TP_Cb& P_Cb)
	: CCoroutineBase(NULL), m_Out(TP_Out(), false)
	{
		Init(P_Cb);
	}


	TP_Out operator()()
	{
		EnsureValid();
		m_CoroPtr->yield();
		if(!m_Out.second)
			throw std::logic_error("No return value received.");
		return m_Out.first;
	}

	template<class TP_Cb>
	void Init(const TP_Cb& P_Cb)
	{
		delete m_CoroPtr;
		m_Func = P_Cb;
		CCoroutineBase::Init(Create(std::tr1::bind(&CCoroutine::Start, this)));
	}

private:
	void Start()
	{
		//ASSERT(m_In.second);
		m_Func(self(this));
	}



	T_Func	m_Func;

	std::pair<TP_Out, bool>	m_Out;
};



template<>
class CCoroutine<void, void> : public CCoroutineBase
{
public:

	class self
	{
	public:
		self(CCoroutine* P_ThisPtr):m_ThisPtr(P_ThisPtr){}

		void yield()
		{
			JCoro::yield();
		}

	private:
		CCoroutine* m_ThisPtr;
	};

	typedef std::tr1::function<void (self&)> T_Func;

	template<class TP_Cb>
	CCoroutine(const TP_Cb& P_Cb)
	: CCoroutineBase(NULL)
	{
		Init(P_Cb);
	}


	void operator()()
	{
		EnsureValid();
		m_CoroPtr->yield();
	}

	template<class TP_Cb>
	void Init(const TP_Cb& P_Cb)
	{
		delete m_CoroPtr;
		m_Func = P_Cb;
		CCoroutineBase::Init(Create(std::tr1::bind(&CCoroutine::Start, this)));
	}

private:
	void Start()
	{
		//ASSERT(m_In.second);
		m_Func(self(this));
	}

	T_Func	m_Func;

};



}