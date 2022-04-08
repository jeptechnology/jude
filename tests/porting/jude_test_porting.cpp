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

#include "jude/porting/jude_porting.h"
#include "jude/jude.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern "C" jude_os_interface_t jude_porting_interface_cpp11;

namespace Mutex
{
   size_t lockTime = 0;
   bool   allowCreation = true;
   size_t count = 0;
}

namespace Semaphore
{
   size_t takeTime = 0;
   bool   allowCreation = true;
   size_t count = 0;
}

namespace Queue
{
   size_t receiveTime = 0;
   bool   allowCreation = true;
   size_t count = 0;
}

static jude_mutex_t *mutex_create()
{
   if (!Mutex::allowCreation)
   {
      return nullptr;
   }
   auto result = jude_porting_interface_cpp11.mutex_create();
   if (result)
   {
      Mutex::count++;
   }
   return result;
}

static void mutex_destroy(jude_mutex_t *mutex)
{
   jude_porting_interface_cpp11.mutex_destroy(mutex);
   Mutex::count--;
}

static bool mutex_lock(jude_mutex_t *mutex, uint32_t timeoutMs)
{
   if (Mutex::lockTime > timeoutMs)
   {
      return false;
   }

   return jude_porting_interface_cpp11.mutex_lock(mutex, timeoutMs);
}

static void mutex_unlock(jude_mutex_t *mutex)
{
   jude_porting_interface_cpp11.mutex_unlock(mutex);
}

static jude_semaphore_t *semaphore_create(uint32_t initial, uint32_t max)
{
   if (!Semaphore::allowCreation)
   {
      return nullptr;
   }
   auto sem = jude_porting_interface_cpp11.semaphore_create(initial, max);
   if (sem)
   {
      Semaphore::count++;
   }
   return sem;
}

static void semaphore_destroy(jude_semaphore_t *sem)
{
   jude_porting_interface_cpp11.semaphore_destroy(sem);
   Semaphore::count--;
}

static bool semaphore_take(jude_semaphore_t *sem, uint32_t maxWaitMs)
{
   if (Semaphore::takeTime > maxWaitMs)
   {
      return false;
   }
   return jude_porting_interface_cpp11.semaphore_take(sem, maxWaitMs);
}

static void semaphore_give(jude_semaphore_t *sem)
{
   return jude_porting_interface_cpp11.semaphore_give(sem);
}

static jude_queue_t *queue_create(size_t maxElements, size_t elementSize)
{
   if (!Queue::allowCreation)
   {
      return nullptr;
   }
   auto q = jude_porting_interface_cpp11.queue_create(maxElements, elementSize);
   if (q)
   {
      Queue::count++;
   }
   return q;
}

static void queue_destroy(jude_queue_t *q)
{
   jude_porting_interface_cpp11.queue_destroy(q);
   Queue::count--;
}

static void queue_send(jude_queue_t *q, const void *e)
{
   jude_porting_interface_cpp11.queue_send(q, e);
}

static bool queue_receive(jude_queue_t *q, void *e, uint32_t maxWaitMs)
{
   if (Queue::receiveTime > maxWaitMs)
   {
      return false;
   }
   return jude_porting_interface_cpp11.queue_receive(q, e, maxWaitMs);
}

jude_os_interface_t jude_porting_test_interface =
{
   jude_porting_interface_cpp11.fatal,

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

// during tests we will use our test interface
jude_os_interface_t *jude_os = &jude_porting_test_interface;
