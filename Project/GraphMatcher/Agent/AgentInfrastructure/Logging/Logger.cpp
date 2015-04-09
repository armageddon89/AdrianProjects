#include "Logger.hpp"
#include <algorithm>

bool DefaultLogger::m_bStarted = false;
DEFINE_LOGGER("/tmp/agent", DefaultLogger);

void BasicLogger::increaseTabStack(const tString &funcName)
{
	boost::unique_lock<boost::mutex> lock(m_mutex);

	boost::thread::id threadId = boost::this_thread::get_id();
	tThreadMap::iterator found = m_numberOfTabsMap.find(threadId);
	struct mallinfo mi;
	mi = mallinfo();
	if (found == m_numberOfTabsMap.end())
	{
		m_numberOfTabsMap.insert(tThreadMap::value_type(threadId, 0));
		tStackType stack;
		stack.push(funcName);
		tIntStackType memoryStack;
		memoryStack.push(mi.uordblks);

		m_callStack.insert(tCallStackMap::value_type(threadId, stack));
		m_memoryMap.insert(tMemoryMap::value_type(threadId, memoryStack));
	}
	else
	{
		m_numberOfTabsMap[threadId]++;
		m_callStack[threadId].push(funcName);
		m_memoryMap[threadId].push(mi.uordblks);
	}
}

void BasicLogger::decreaseTabStack(const tString &funcName, int timeMs)
{
	boost::unique_lock<boost::mutex> lock(m_mutex);

	boost::thread::id threadId = boost::this_thread::get_id();
	m_numberOfTabsMap[threadId]--;
	if(!m_callStack.empty())
	{
		tString func;
		func = m_callStack[threadId].top();
		int prevMem = m_memoryMap[threadId].top();

		m_callStack[threadId].pop();
		m_memoryMap[threadId].pop();

		struct mallinfo mi;
		mi = mallinfo();

		int memIncrease = (int)mi.uordblks - prevMem ;

		tConsumptionMap::iterator found = m_consumptionMap.find(func);
		if (found != m_consumptionMap.end())
		{
			found->second += memIncrease;
		}
		else
		{
			m_consumptionMap.insert(tConsumptionMap::value_type(func, memIncrease));
		}

		m_debugStream << "During function " << func << " " << memIncrease << " blocks were consumed" << std::endl;
	}
}

tString BasicLogger::getCurrentTime()
{
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	tString sTime = asctime(timeinfo);
	sTime = sTime.substr(0, sTime.size() - 1);
	
	struct timeval detail_time;
	gettimeofday(&detail_time,NULL);
	sTime.append(".");
	char millis[3];
	sprintf(millis, "%0*s", 3, boost::lexical_cast<tString>(detail_time.tv_usec / 1000).c_str());
	sTime.append(millis);
	
	return sTime;
}

DefaultLogger::DefaultLogger(const tString &fileName) :
	m_fileName(fileName),
	m_bEnabled(false),
	m_periodicTimer(m_timerService),
	m_periodicCB(&DefaultLogger::timerHandler, this)
{
	printf("Constructing logger\n");

	m_fileName.append("_");
	m_fileName.append(boost::lexical_cast<tString>(getpid()));

	tString memoryFile = fileName;
	memoryFile.append("Dump");
	memoryFile.append("_");
	memoryFile.append(boost::lexical_cast<tString>(getpid()));

	m_outStream.open(m_fileName.c_str(), std::ios::out);
	m_debugStream.open(memoryFile.c_str(), std::ios::out);

	m_periodicTimer.start(10000, m_periodicCB);

	boost::unique_lock<boost::mutex> lock(m_mutex);
	m_bStarted = true;
}

DefaultLogger::~DefaultLogger()
{
	dumpLeaks();
	m_bStarted = false;
	m_outStream.close();
	m_debugStream.close();
}

static bool sortFunc(const std::pair<tString, int> &a, const std::pair<tString, int> &b)
{
	return a.second > b.second;
}

void DefaultLogger::timerHandler()
{
	boost::unique_lock<boost::mutex> lock(m_mutex);

	m_outStream << "...............\nDumping memory consumption:\n.......................\n";

	std::vector<std::pair<tString, int>, __gnu_cxx::malloc_allocator<std::pair<tString, int> > > sortedVec;
	for (tConsumptionMap::iterator it = m_consumptionMap.begin(); it != m_consumptionMap.end(); it++)
	{
		sortedVec.push_back(std::pair<tString, int>(it->first, it->second));
	}

	std::sort(sortedVec.begin(), sortedVec.end(), sortFunc);

	for (size_t i = 0; i < sortedVec.size(); i++)
	{
		m_outStream << sortedVec[i].second << " -> " << sortedVec[i].first << std::endl;
	}

	m_outStream << "..................................................\n";

	lock.unlock();
	dumpLeaks();
	lock.lock();
}

void DefaultLogger::dumpLeaks()
{
	boost::unique_lock<boost::mutex> lock(m_mutex);
	if (!m_outStream)
	{
		return;
	}

	m_debugStream << "..........................\nDumping memory leaks (" + boost::lexical_cast<tString>(m_addressMap.size()) + " total)" + " :\n........... " <<
		getCurrentTime() << " ................" << std::endl;
	for (tAddressMap::iterator it = m_addressMap.begin(); it != m_addressMap.end(); it++)
	{
		m_debugStream << "Address " << it->first << " leaked " << it->second.first << " bytes allocated from " << it->second.second << std::endl;
	}
	
	m_debugStream << "..............................\n";
}

void DefaultLogger::traceNew(void *ptr, int size)
{
	boost::unique_lock<boost::mutex> lock(m_mutex);

	if (!m_bEnabled)
	{
		return;
	}

	std::pair<int, tString> pair;
	pair.first = size;
	boost::thread::id threadId = boost::this_thread::get_id();
	tString function;
	tCallStackMap::iterator found = m_callStack.find(threadId);
	if (found != m_callStack.end())
	{
		if (!found->second.empty())
		{
			function = found->second.top();
		}
	}
	pair.second = function;
	m_addressMap.insert(tAddressMap::value_type(reinterpret_cast<size_t>(ptr), pair));
}

void DefaultLogger::traceDelete(void *ptr)
{
	boost::unique_lock<boost::mutex> lock(m_mutex);

	if (!m_bEnabled)
	{
		return;
	}

	boost::thread::id threadId = boost::this_thread::get_id();
	tCallStackMap::iterator found = m_callStack.find(threadId);
	tString function;
	if (found != m_callStack.end())
	{
		if (!found->second.empty())
		{
			function = found->second.top().c_str();
		}
	}

	tAddressMap::iterator addr = m_addressMap.find(reinterpret_cast<size_t>(ptr));
	if (addr == m_addressMap.end())
	{
		//TODO
	}
	else
	{
		m_addressMap.erase(addr);
	}
}

void DefaultLogger::enable(bool bEnable)
{
	boost::unique_lock<boost::mutex> lock(m_mutex);
	m_bEnabled = bEnable;

	if (bEnable)
	{
		for (size_t i = 0; i < m_lostLogs.size(); i++)
		{
			m_outStream << m_lostLogs[i] << std::endl;
		}
	}
}

void DefaultLogger::log(const tString &logData)
{
	boost::unique_lock<boost::mutex> lock(m_mutex);

	if(!m_bEnabled)
	{
		if ((int)m_lostLogs.size() < kMaxLogsKept)
		{
			m_lostLogs.push_back(logData);
		}
		return;
	}

	boost::thread::id threadId = boost::this_thread::get_id();

	if (threadId != m_lastThread)
	{
		m_outStream << "Switch to thread - " << threadId << std::endl;
		m_outStream << "................................................" << std::endl;
		m_lastThread = threadId;
	}

	m_outStream << getCurrentTime() << " == " <<
			tString(3 * m_numberOfTabsMap[threadId], ' ') << logData << std::endl;
	m_outStream.flush();
}
