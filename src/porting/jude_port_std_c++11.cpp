/*
 * The MIT License (MIT)
 * Copyright © 2022 James Parker
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal 
 * in the Software without restriction, including without limitation the rights 
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <assert.h>

#include <jude/jude.h>
#include <jude/porting/jude_porting.h>

static void fatal_impl(const char *file, int line, const char *message)
{
   fprintf(stderr, "JUDE_FATAL:%s:%d:%s\n", file, line, message);
   exit(-1);
}

// mutex
struct jude_mutex_t
{
   std::recursive_timed_mutex native;
};

static jude_mutex_t *mutex_create()
{
   return new jude_mutex_t;
}

static void mutex_destroy(jude_mutex_t *mutex)
{
   jude_assert(mutex != nullptr);
   delete mutex;
}

static bool mutex_lock(jude_mutex_t *mutex, uint32_t timeoutMs)
{
   jude_assert(mutex != nullptr);
   return mutex->native.try_lock_for(std::chrono::milliseconds(timeoutMs));
}

static void mutex_unlock(jude_mutex_t *mutex)
{
   jude_assert(mutex != nullptr);
   mutex->native.unlock();
}

// semaphore
struct jude_semaphore_t
{
   size_t max;
   size_t count;
   std::condition_variable cv;
   std::mutex mut;
};

static jude_semaphore_t *semaphore_create(uint32_t initial, uint32_t max)
{
   auto sem = new jude_semaphore_t;
   sem->count = initial;
   sem->max = max;
   return sem;
}

static void semaphore_destroy(jude_semaphore_t *sem)
{
   jude_assert(sem != nullptr);
   delete sem;
}

static bool semaphore_take(jude_semaphore_t *sem, uint32_t maxWaitMs)
{
   jude_assert(sem != nullptr);

   std::unique_lock<std::mutex> lck(sem->mut);

   if(sem->count == 0)
   {
      sem->cv.wait_for(lck, std::chrono::milliseconds(maxWaitMs));
   }

   if (sem->count)
   {
      --sem->count;
      return true;
   }

   return false;
}

static void semaphore_give(jude_semaphore_t *sem)
{
   jude_assert(sem != nullptr);

   std::unique_lock<std::mutex> lck(sem->mut);

   if (sem->count < sem->max)
   {
      sem->count++;
      sem->cv.notify_one();
   }
}

// queues
struct jude_queue_t
{
   size_t elementSize;
   size_t max;
   std::queue<std::vector<char>> m_queue;
   std::condition_variable cv;
   std::mutex mut;
};

static jude_queue_t *queue_create(size_t maxElements, size_t elementSize)
{
   auto q = new jude_queue_t;
   q->elementSize = elementSize;
   q->max = maxElements;
   return q;
}

static void queue_destroy(jude_queue_t *q)
{
   delete q;
}

static void queue_send(jude_queue_t *q, const void *e)
{
   std::unique_lock<std::mutex> lck(q->mut);
   if (q->m_queue.size() < q->max)
   {
      q->m_queue.emplace((char *)e, (char*)e + q->elementSize);
      q->cv.notify_one();
   }
}

static bool queue_receive(jude_queue_t *q, void *e, uint32_t maxWaitMs)
{
   jude_assert(q != nullptr);

   std::unique_lock<std::mutex> lck(q->mut);

   if(q->m_queue.size() == 0)
   {
      q->cv.wait_for(lck, std::chrono::milliseconds(maxWaitMs));
   }

   if (q->m_queue.size() == 0)
   {
      return false;
   }

   std::vector<char>& data = q->m_queue.front();
   memcpy(e, data.data(), q->elementSize);
   q->m_queue.pop();

   return true;
}

extern "C" 
{
   jude_os_interface_t jude_porting_interface_cpp11 =
   {
      fatal_impl,

      // mutex
      mutex_create,
      mutex_destroy,
      mutex_lock,
      mutex_unlock,

      // semaphore
      semaphore_create,
      semaphore_destroy,
      semaphore_take,
      semaphore_give,

      // queue
      queue_create,
      queue_destroy,
      queue_send,
      queue_receive
   };
}

#if JUDE_USE_STDLIB
jude_os_interface_t *jude_os = &jude_porting_interface_cpp11;
#endif
