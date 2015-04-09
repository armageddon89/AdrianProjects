#if defined(USE_LOGGER)

#pragma once

#include <sys/types.h>
#include <unistd.h>

#include "boost/unordered_map.hpp"
#include "boost/thread.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/scope_exit.hpp"
#include "boost/date_time.hpp"
#include "ext/malloc_allocator.h"
#include "boost/chrono.hpp"

#include <iomanip>
#include <fstream>
#include <string>
#include <stack>
#include <memory>

#include "CTimer.hpp"

typedef std::basic_string<char, std::char_traits<char>, __gnu_cxx::malloc_allocator<char> > tString;
class BaseLog
{
public:

	void increaseTabStack(const tString &funcName);
	void decreaseTabStack(const tString &funcName, int timeMs);

	virtual void log(const tString &logData) = 0;
	virtual void enable(bool bEnable) = 0;
	tString getCurrentTime();

protected:

	typedef boost::unordered_map<boost::thread::id, int, boost::hash<boost::thread::id>, std::equal_to<boost::thread::id>, __gnu_cxx::malloc_allocator<boost::thread::id> > tThreadMap;
	tThreadMap m_numberOfTabsMap;
	typedef std::stack<tString, std::deque<tString, __gnu_cxx::malloc_allocator<tString> > > tStackType;
	typedef std::stack<size_t, std::deque<size_t, __gnu_cxx::malloc_allocator<size_t> > > tIntStackType;
	typedef boost::unordered_map<boost::thread::id, tStackType, boost::hash<boost::thread::id>, std::equal_to<boost::thread::id>, __gnu_cxx::malloc_allocator<boost::thread::id> > tCallStackMap;
	tCallStackMap m_callStack;
	typedef boost::unordered_map<size_t, std::pair<int, tString>, boost::hash<size_t>, std::equal_to<size_t>, __gnu_cxx::malloc_allocator<size_t> > tAddressMap;
	tAddressMap m_addressMap;
	typedef boost::unordered_map<boost::thread::id, tIntStackType, boost::hash<boost::thread::id>, std::equal_to<boost::thread::id>, __gnu_cxx::malloc_allocator<boost::thread::id> > tMemoryMap;
	tMemoryMap m_memoryMap;
	typedef boost::unordered_map<tString, int, boost::hash<tString>, std::equal_to<tString>, __gnu_cxx::malloc_allocator<tString> > tConsumptionMap;
	tConsumptionMap m_consumptionMap;
	boost::thread::id m_lastThread;
	boost::mutex m_mutex;
	std::ofstream m_debugStream;
};

class DefaultLogger : public BaseLog
{
public:
	DefaultLogger(const tString &fileName);
	virtual ~DefaultLogger();
	void dumpLeaks();
	void traceNew(void *ptr, int size);
	void traceDelete(void *ptr);
	void enable(bool bEnable);
	void log(const tString &logData);
	
	static bool isInitialized()
	{
		return m_bStarted;
	}

private:

	void timerHandler();

	tString m_fileName;
	std::ofstream m_outStream;
	std::vector<tString, __gnu_cxx::malloc_allocator<tString> > m_lostLogs;
	static const int kMaxLogsKept = 2000;
	bool m_bEnabled;
	static bool m_bStarted;
	CTimerService m_timerService;
	PeriodicTimer m_periodicTimer;
	TTimerCallBackAdapter<DefaultLogger> m_periodicCB;
};

#define DEFINE_LOGGER(fileName, Logger) Logger g_logger(fileName); 
#define USE_LOGGER(Logger) extern Logger g_logger

#define PROFILE_FUNC \
	tString anonymousFunc = __PRETTY_FUNCTION__; \
	g_logger.increaseTabStack(anonymousFunc); \
	g_logger.log(tString("--> ") + anonymousFunc); \
	boost::chrono::steady_clock::time_point __start = boost::chrono::steady_clock::now(); \
	BOOST_SCOPE_EXIT(anonymousFunc, __start)	\
	{ \
		int millisec = boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::steady_clock::now() - __start).count(); \
		g_logger.log(tString("<-- ") + anonymousFunc + " (" + boost::lexical_cast<tString>(millisec) + " mSec)"); \
		g_logger.decreaseTabStack(anonymousFunc, millisec); \
	}	\
	BOOST_SCOPE_EXIT_END
	
#define PROFILE_FUNC_TPL \
	tString anonymousFunc = __PRETTY_FUNCTION__; \
	g_logger.increaseTabStack(anonymousFunc); \
	g_logger.log(tString("--> ") + anonymousFunc); \
	boost::chrono::steady_clock::time_point __start = boost::chrono::steady_clock::now(); \
	BOOST_SCOPE_EXIT_TPL(anonymousFunc, __start)	\
	{ \
		int millisec = boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::steady_clock::now() - __start).count(); \
		g_logger.log(tString("<-- ") + anonymousFunc + " (" + boost::lexical_cast<tString>(millisec) + " mSec)"); \
		g_logger.decreaseTabStack(anonymousFunc, millisec); \
	}	\
	BOOST_SCOPE_EXIT_END

#define TRACE_LOG(...) \
{ \
	char buffer[2048] = {0}; \
	sprintf(buffer, __VA_ARGS__); \
	g_logger.log(g_logger.getCurrentTime() + " == " + \
				 tString("file: ") + __FILE__ + ", line:" + boost::lexical_cast<tString>(__LINE__) + " ==> " + buffer); \
}

#define TRACE_NEW(address, size) g_logger.traceNew(address, size)
#define TRACE_DELETE(address) g_logger.traceDelete(address)

#define TRACE_ENABLE(bEnable) g_logger.enable(bEnable)

#else
	#define DEFINE_LOGGER(fileName, Logger)
	#define USE_LOGGER(Logger)
	#define PROFILE_FUNC
	#define PROFILE_FUNC_TPL
	#define TRACE_ENABLE
	#define TRACE_LOG(...)
	#define TRACE_NEW(address, size)
	#define TRACE_DELETE(address)
#endif
