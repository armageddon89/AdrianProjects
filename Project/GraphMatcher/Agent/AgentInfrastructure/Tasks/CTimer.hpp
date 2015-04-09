#pragma once

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/asio/deadline_timer.hpp>
#include <boost/function.hpp>

class CTimerService
{
public:
   CTimerService()
   {
      io_service.reset(new boost::asio::io_service);
      work.reset(new boost::asio::io_service::work(*io_service.get()));
      io_thread.reset(new boost::thread(
            boost::bind(&boost::asio::io_service::run, io_service.get())));
   }

   virtual ~CTimerService()
   {
      work.reset();
      io_thread->join();
   }

   boost::asio::io_service& get_io_service()
   {
      return *io_service;
   }

private:
   CTimerService(const CTimerService&);
   CTimerService& operator =(const CTimerService&);
   boost::scoped_ptr<boost::asio::io_service> io_service;
   boost::scoped_ptr<boost::asio::io_service::work> work;
   boost::scoped_ptr<boost::thread> io_thread;
};

struct ITimerCallBack
{
   virtual ~ITimerCallBack(){};
   virtual void timerCallBack() = 0;
};

template <class C>
class TTimerCallBackAdapter : public ITimerCallBack
{
public:
   typedef void (C::*TCallback)();
   TTimerCallBackAdapter(TCallback method, C& object)
   : mObject(&object), mMethod(method) {}

   TTimerCallBackAdapter(TCallback method, C* object)
   : mObject(object), mMethod(method) {}

   TTimerCallBackAdapter(const TTimerCallBackAdapter& ra)
   : mObject(ra.mObject), mMethod(ra.mMethod) {}

   ~TTimerCallBackAdapter() {}

   TTimerCallBackAdapter& operator = (const TTimerCallBackAdapter& rhs)
   {
      mObject = rhs.mObject;
      mMethod  = rhs.mMethod;
      return *this;
   }

   virtual void timerCallBack()
   {
      (mObject->*mMethod)();
   }

private:
   TTimerCallBackAdapter();
   C* mObject;
   TCallback mMethod;
};

class CTimer
{
public:
   CTimer(CTimerService& srv)
   : m_timer(srv.get_io_service())
   , m_periodicTime(0)
   {}

   typedef void (*TimerCallBackFuncPtr)();

   void start(long startTimeMs, long repeatTimeMs, TimerCallBackFuncPtr cb)
   {
      m_cb = cb;
      m_periodicTime = repeatTimeMs;
      m_timer.expires_from_now(boost::posix_time::milliseconds(startTimeMs));
      m_timer.async_wait(boost::bind(&CTimer::onTimer, this, _1));
   }

   void start(long startTimeMs, long repeatTimeMs, ITimerCallBack& cb)
   {
      m_cb = boost::bind(&ITimerCallBack::timerCallBack, &cb);
      m_periodicTime = repeatTimeMs;
      m_timer.expires_from_now(boost::posix_time::milliseconds(startTimeMs));
      m_timer.async_wait(boost::bind(&CTimer::onTimer, this, _1));
   }

   void stop()
   {
      m_timer.cancel();
   }

private:
   void onTimer(const boost::system::error_code& ec)
   {
      if(ec == boost::asio::error::operation_aborted)
      {
         return;
      }

      if(m_cb)
      {
         m_cb();
      }

      if(m_periodicTime)
      {
         m_timer.expires_from_now(boost::posix_time::milliseconds(m_periodicTime));
         m_timer.async_wait(boost::bind(&CTimer::onTimer, this, _1));
      }
   }

   boost::function<void ()> m_cb;
   /* deadline_timer use UTC clock - may be changed.
    * steady_timer use monotonic clock!!!
    */
   boost::asio::deadline_timer m_timer;
   long m_periodicTime;
};

class CGenericTimer
{
public:
   template<class T>
   CGenericTimer(typename TTimerCallBackAdapter<T>::TCallback cb, T* t, CTimerService& srv)
   : m_cb(new TTimerCallBackAdapter<T>(cb, t))
   , m_timer(srv)
   { }

   virtual ~CGenericTimer()
   {
      delete m_cb;
   }

   void start(long startTime, long periodicTime)
   {
      m_timer.start(startTime, periodicTime, *m_cb);
   }

   void stop()
   {
      m_timer.stop();
   }

private:
   ITimerCallBack *m_cb;
   CTimer m_timer;
};

class SingleShotTimer
{
public:
   SingleShotTimer(CTimerService& srv) : mTimer(srv) {}

   void start(long delayMs, CTimer::TimerCallBackFuncPtr cb)
   {
      mTimer.start(delayMs, 0, cb);
   }

   void start(long delayMs,ITimerCallBack& cb)
   {
      mTimer.start(delayMs, 0, cb);
   }

   void stop()
   {
      mTimer.stop();
   }

private:
   CTimer mTimer;
};

class PeriodicTimer
{
public:
   PeriodicTimer(CTimerService& srv) : mTimer(srv) {};

   void start(long periodMs, ITimerCallBack& cb)
   {
      mTimer.start(periodMs, periodMs, cb);
   }

   void start(long periodMs, CTimer::TimerCallBackFuncPtr cb)
   {
      mTimer.start(periodMs, periodMs, cb);
   }

   void stop()
   {
      mTimer.stop();
   }

private:
   CTimer mTimer;
};
