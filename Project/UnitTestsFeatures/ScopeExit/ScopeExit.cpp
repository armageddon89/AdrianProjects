#include <Windows.h>
#include <utility>
#include <tchar.h>

template <typename Function>
class ScopeGuard
{
public:
	ScopeGuard(Function f) :
		m_f(std::move(f)),
		m_bActive(true)
	{}

	ScopeGuard(ScopeGuard&& other) :
		m_f(std::move(other.m_f)),
		m_bActive(other.m_bActive)
	{
		other.dismiss();
	}

	~ScopeGuard()
	{
		if (m_bActive)
		{
			m_f();
		}
	}

	__forceinline void dismiss() { m_bActive = false; }

private:
	Function m_f;
	bool m_bActive;

	//= delete when will be available
	ScopeGuard() {};
	ScopeGuard(const ScopeGuard &) {};
	ScopeGuard& operator=(const ScopeGuard&) {};
};

namespace cleanup
{
	//enum class ScopeHandler {}; //pentru VS2012
	struct ScopeHandler { enum _ScopeHandler { }; };

	template <typename Function>
	ScopeGuard<Function> operator+(ScopeHandler, Function &&f)
	{
		return ScopeGuard<Function>(std::forward<Function>(f));
	}
}

#define CONCATENATE_IMPL(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)

#ifdef __COUNTER__
#define ANONYMOUS_VARIABLE(str) CONCATENATE(str, __COUNTER__)
#else
#define ANONYMOUS_VARIABLE(str) CONCATENATE(str, _LINE_)
#endif

#define SCOPE_EXIT \
	auto ANONYMOUS_VARIABLE(SCOPE_EXIT_ID) = ::cleanup::ScopeHandler() + [&]()

#define SCOPE_ROLLBACK ::cleanup::ScopeHandler() + [&]()

void ExternApiWhoThrows()
{
	throw 404;
}

struct RandomPlugin
{
	__forceinline int Init() { return false; }
	__forceinline void Uninit() { }
};

RandomPlugin *plugin = nullptr;
void f()
{
	HANDLE hn = ::CreateFile(L"coolFile.txt", GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE, 	nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,	nullptr);
	SCOPE_EXIT //Some scenario that must be done, mandatory before exit
	{ 
		//verify hn validity
		::CloseHandle(hn);
		if(!::DeleteFile(L"coolFile")) { /*Logs */ }
	};

	plugin = new RandomPlugin();
	plugin->Init();
	auto rollback = SCOPE_ROLLBACK { plugin->Uninit(); }; //only if a chain of things will fail

	WIN32_FIND_DATA fileInfo;
	HANDLE hFind = ::FindFirstFile(L"coolFolderForMyPlugin", &fileInfo);
	SCOPE_EXIT { ::FindClose(hFind); }; //again mandatory
	
	if(INVALID_HANDLE_VALUE == hFind)
	{
		return; //rollback and both scope_exits are called
	}
	rollback.dismiss(); //everything ok. Plugin is good, no need for rollback

	ExternApiWhoThrows(); //scope_exit is called

	//unreachable without SCOPE_EXIT
	::CloseHandle(hn);
	::FindClose(hFind);
}

int _tmain(int argc, _TCHAR* argv[])
{
	try 
	{
		f();
	}
	catch(...)
	{

	}

	return 0;
}

