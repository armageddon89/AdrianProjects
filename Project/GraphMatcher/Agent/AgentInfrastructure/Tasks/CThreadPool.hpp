/*
 * ThreadPool.hpp
 *
 *  Created on: Jul 17, 2014
 *      Author: adobrescu
 */

#pragma once

#include "boost/unordered_set.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/thread/condition_variable.hpp"
#include "boost/thread/lock_types.hpp"
#include "boost/function.hpp"
#include "boost/bind.hpp"
#include "boost/thread.hpp"

#include "CLogger.hpp"
USE_LOGGER(DefaultLogger);

class CThreadPool
{
public:

	CThreadPool(int maxNumberOfThreads) :
		m_maxNumberOfThreads(maxNumberOfThreads)
	{
		PROFILE_FUNC;
	}

	void joinFinishedThreads()
	{
		PROFILE_FUNC;

		for (size_t i = 0; i < m_threadsToJoin.size(); i++)
		{
			if (!m_threadsToJoin[i])
			{
				TRACE_LOG("Shouldn't arrive here, NULL");
				continue;
			}

			if(m_threadsToJoin[i]->joinable())
			{
				try
				{
					m_threadsToJoin[i]->join();
				}
				catch(boost::thread_resource_error &ex)
				{
					TRACE_LOG("Error on join - %s", ex.what());
				}
			}

			boost::unordered_set<boost::thread *>::iterator it = m_threads.find(m_threadsToJoin[i]);
			if (it != m_threads.cend())
			{
				m_threads.erase(it);
			}

			delete m_threadsToJoin[i];
		}

		m_threadsToJoin.clear();
	}

	void joinAllThreads()
	{
		PROFILE_FUNC;

		for (boost::unordered_set<boost::thread *>::iterator it = m_threads.begin();
			 it != m_threads.end(); it++)
		{
			if (!(*it))
			{
				TRACE_LOG("Shouldn't arrive here. Current thread is null");
				continue;
			}

			if ((*it)->joinable())
			{
				try
				{
					(*it)->join();
				}
				catch(boost::thread_resource_error &ex)
				{
					TRACE_LOG("Error on join - %s", ex.what());
				}
			}
		}

		try
		{
			boost::unique_lock<boost::mutex> lock(m_mutex);
			for (boost::unordered_set<boost::thread *>::iterator it = m_threads.begin();
						 it != m_threads.end(); it++)
			{
				delete *it;
			}
			m_threads.clear();
			m_threadsToJoin.clear();
		}
		catch(boost::lock_error &ex)
		{
			TRACE_LOG("Exception - %s", ex.what());
		}
	}

	template <typename Functor, typename Arg>
	void addNewThread(const Functor &functor,
					  Arg &arg)
	{
		PROFILE_FUNC_TPL;
		try
		{
			boost::unique_lock<boost::mutex> lock(m_mutex);

			while (static_cast<int>(m_threads.size()) == m_maxNumberOfThreads)
			{
				if (!m_threadsToJoin.empty())
				{
					joinFinishedThreads();
				}
				else
				{
					try
					{
						m_maxThreadsCondition.wait(lock);
					}
					catch(boost::condition_error &ex)
					{
						TRACE_LOG("Error on waiting condition - %s", ex.what());
					}
					joinFinishedThreads();
				}
			}

			boost::thread *th = new (std::nothrow)
				boost::thread(&CThreadPool::_threadWrapper<Functor, Arg>,
					this,
					functor,
					boost::ref(arg));
			if(!th)
			{
				TRACE_LOG("Could not allocate a new thread");
				return;
			}

			m_threads.insert(th);
		}
		catch(boost::lock_error &ex)
		{
			TRACE_LOG("Exception - %s", ex.what());
		}
	}

	template <typename Functor, typename Arg>
	void addNewThreadValue(const Functor &functor,
					  Arg arg)
	{
		PROFILE_FUNC_TPL;
		try
		{
			boost::unique_lock<boost::mutex> lock(m_mutex);

			while (static_cast<int>(m_threads.size()) == m_maxNumberOfThreads)
			{
				if (!m_threadsToJoin.empty())
				{
					joinFinishedThreads();
				}
				else
				{
					try
					{
						m_maxThreadsCondition.wait(lock);
					}
					catch(boost::condition_error &ex)
					{
						TRACE_LOG("Error on waiting condition - %s", ex.what());
					}
					joinFinishedThreads();
				}
			}

			boost::thread *th = new (std::nothrow)
				boost::thread(&CThreadPool::_threadWrapperValue<Functor, Arg>,
					this,
					functor,
					arg);
			if(!th)
			{
				TRACE_LOG("Could not allocate a new thread");
				return;
			}

			m_threads.insert(th);
		}
		catch(boost::lock_error &ex)
		{
			TRACE_LOG("Exception - %s", ex.what());
		}
	}

private:

	/*
	 * arg is intentionally non-const to cover all the situations.
	 * functor will wrap the right constness
	 */
	template <typename Functor, typename Arg>
	void _threadWrapper(const Functor &functor, Arg &arg)
	{
		PROFILE_FUNC_TPL;
		functor(arg);

		try
		{
			boost::unique_lock<boost::mutex> lock(m_mutex);

			boost::thread::id threadId = boost::this_thread::get_id();
			for (boost::unordered_set<boost::thread *>::iterator it = m_threads.begin();
				 it != m_threads.end(); it++)
			{
				if ((*it)->get_id() == threadId)
				{
					m_threadsToJoin.push_back(*it);

					if (static_cast<int>(m_threads.size()) == m_maxNumberOfThreads)
					{
						m_maxThreadsCondition.notify_one();
					}

					break;
				}
			}
		}
		catch(boost::lock_error &ex)
		{
			TRACE_LOG("Error on lock - %s", ex.what());
		}
	}

	template <typename Functor, typename Arg>
	void _threadWrapperValue(const Functor &functor, Arg arg)
	{
		PROFILE_FUNC_TPL;
		functor(arg);

		try
		{
			boost::unique_lock<boost::mutex> lock(m_mutex);

			boost::thread::id threadId = boost::this_thread::get_id();
			for (boost::unordered_set<boost::thread *>::iterator it = m_threads.begin();
				 it != m_threads.end(); it++)
			{
				if ((*it)->get_id() == threadId)
				{
					m_threadsToJoin.push_back(*it);

					if (static_cast<int>(m_threads.size()) == m_maxNumberOfThreads)
					{
						m_maxThreadsCondition.notify_one();
					}

					break;
				}
			}
		}
		catch(boost::lock_error &ex)
		{
			TRACE_LOG("Error on lock - %s", ex.what());
		}
	}

	boost::condition_variable m_maxThreadsCondition;
	boost::unordered_set<boost::thread *> m_threads;
	std::vector<boost::thread *> m_threadsToJoin;
	boost::mutex m_mutex;

	int m_maxNumberOfThreads;
};
