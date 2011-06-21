/*
 * Created by David Lapsley on Mon Jun 6 2011.
 *
 * Copyright 2011 MIT Haystack Observatory 
 *  
 * This file is part of mark6.
 *
 * mark6 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mark6 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mark6.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include <writer_thread_pool.h>


//-----------------------------------------------------------------------------
WriterThread::WriterThread():
  _writer_thread_pool(0),
  _running(true)
{
}
  
void WriterThread::operator() (WriterThreadPool* wtp,
			       const int sleep_time) {
  _writer_thread_pool = wtp;
  
  while (wtp->running()) {
    // Get next task.
    try {
      WriterTask w = _writer_thread_pool->next_task();
      w();
    } catch (WriterTaskTimeout &t) {
      LOG4CXX_DEBUG(logger, "Timedout waiting for next task.");
    }
  }
}

//-----------------------------------------------------------------------------
WriterThreadPool::WriterThreadPool(const boost::uint32_t task_list_size,
				   const boost::uint32_t thread_pool_size,
				   const int thread_sleep_time):
  _TASK_LIST_SIZE(task_list_size),
  _THREAD_POOL_SIZE(thread_pool_size),
  _THREAD_SLEEP_TIME(thread_sleep_time),
  _running(true)
{
}

WriterThreadPool::~WriterThreadPool() {
  while (!_threads.empty()) {
    boost::thread* t = _threads.front();
    _threads.pop_front();
    delete t;
  }
}
  
void WriterThreadPool::start()
{
  for (boost::uint32_t i=0; i<_THREAD_POOL_SIZE; ++i) {
    WriterThread w;
    _threads.push_back(new boost::thread(w, this, _THREAD_SLEEP_TIME));
  }
}

void WriterThreadPool::stop()
{
  _running = false;
}

bool WriterThreadPool::running()
{
  return _running;
}

bool WriterThreadPool::insert_task(const WriterTask& w)
{
  boost::mutex::scoped_lock lock(_task_list_mutex);    
  if (_task_list.size() < _TASK_LIST_SIZE) {
    _task_list.push_back(w);
    _task_list_cond.notify_one();
      return true;
  }
  return false;
}

WriterTask WriterThreadPool::next_task() {
  boost::mutex::scoped_lock lock(_task_list_mutex);
  
  boost::system_time timeout = boost::get_system_time() 
    + boost::posix_time::milliseconds(_THREAD_SLEEP_TIME*1000);
  
  while (_threads.empty()) {
    bool timedout = _task_list_cond.timed_wait(lock, timeout);
    if (!timedout)
      throw WriterTaskTimeout("timedout");
  }
  
  WriterTask n = _task_list.front();
  _task_list.pop_front();
  return n;
}
