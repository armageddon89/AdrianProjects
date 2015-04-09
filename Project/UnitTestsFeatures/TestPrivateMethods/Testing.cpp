// Testing.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <functional>
#include <unordered_map>
#include <string>
#include <memory>
#include <queue>

using namespace std;

class A	
{
public:
	virtual void f()
	{
		int x = _f();
		x;
	}

	int x;

private:
	virtual int _f()
	{
		return 0;
	}

	friend class B;
};

namespace testing
{
	auto fakeDeleter = [] (void *x) { };
}

#define PTR_FROM_VAL(x) T(&(x), testing::fakeDeleter)

class B : public A
{
public:
	typedef std::unique_ptr<void, std::function<void(void *)>> T;

	B(bool bTest = false)
	{
		m_bTest = bTest;
	}

	//asa vor fi toate functiile publice
	void f()
	{
		__super::f();
	}

	int _f()
	{
		if (m_bTest)
		{
			return __super::_f();
		}
	

		//Cod ce tine de interiorul procesului. Poate disparea intr-o clasa policy pe care sa o mostenesc
		// si doar dau copy-paste la un macro in fiecare functie privata.
		//Aici se poate adauga logica si pt celelalte primitive.
		auto functions = m_fmap.equal_range(__FUNCTION__);
		for (auto &f = functions.first; f != functions.second; f++)
		{
			f->second();
		}
		
		if (!m_retValues[__FUNCTION__].empty())
		{
			int retVal = *(int *)m_retValues[__FUNCTION__].front().get();
			m_retValues[__FUNCTION__].pop();
			return retVal;
		}

		return 0;
	}

	void DoAction(std::string functionName, std::function<void()> f)
	{
		m_fmap.emplace(functionName, f);
	}

	template<typename V>
	void WillReturn(std::string functionName, V &&value)
	{
		m_retValues[functionName].push(PTR_FROM_VAL(value));
	}

private:
	bool m_bTest;
	std::unordered_multimap<std::string, std::function<void()>> m_fmap;
	std::unordered_map<std::string, std::queue<T>> m_retValues;
};

int _tmain(int argc, _TCHAR* argv[])
{
	B b;
	b.WillReturn("B::_f", 20);
	b.DoAction("B::_f", [&] () 
	{ 
		b.x = 3;
		b.x++; 
	});
	b.DoAction("B::_f", [&] () { cout << "intrat aici"; });
	b.f();

	B b2(true);
	b2.f();
	b2._f();

	return 0;
}

