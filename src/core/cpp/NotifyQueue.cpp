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

#include <jude/core/cpp/NotifyQueue.h>

namespace jude
{
   NotifyQueue::NotifyQueue(std::nullptr_t)
      : m_name("ImmediateQueue")
      , m_queue(nullptr)
   {
   }
         
   NotifyQueue::NotifyQueue(const std::string& name, size_t maxDepth)
      : m_name(name)
      , m_queue(jude_notification_queue_create(maxDepth))
   {
   }

   NotifyQueue::~NotifyQueue()
   {
      if (m_queue)
      {
         jude_notification_queue_destroy(m_queue);
      }
   }

   void NotifyQueue::Pause()
   {
      m_paused = true;
   }

   void NotifyQueue::Play()
   {
      if (!m_paused)
      {
         return;
      }

      for (auto& callback : m_pausedNotifications)
      {
         callback();
      }
      m_pausedNotifications.clear();
      m_paused = false;
   }


   void NotifyQueue::Send(std::function<void()>&& callback)
   {
      if (m_paused)
      {
         m_pausedNotifications.push_back(callback);
         return;
      }

      if (m_queue == nullptr)
      {
         callback(); // immediate
         return;
      }

      jude_notification_t notification;
      notification.user_data = new std::function<void()>(std::move(callback));
      notification.callback = [](void* data)
      {
         auto func = reinterpret_cast<std::function<void()>*>(data);
         (*func)();
         delete func;
      };
      jude_notification_queue_post(m_queue, &notification);
   }

   bool NotifyQueue::Process(uint32_t maxWaitMs)
   {
      if (m_queue)
      {
         return jude_notification_queue_process(m_queue, maxWaitMs);
      }
      return false;
   }

   NotifyQueue NotifyQueue::Immediate(nullptr);
   NotifyQueue NotifyQueue::Default(nullptr); // default queue is immediate unless specified in SetDefaultQueue()

   void NotifyQueue::SetDefaultQueue(const std::string& name, size_t maxDepth)
   {
      Default.m_name = name;
      if (Default.m_queue)
      {
         jude_notification_queue_destroy(Default.m_queue);
      }
      Default.m_queue = jude_notification_queue_create(maxDepth);
   }

   void NotifyQueue::SetDefaultQueueAsImmediate()
   {
      Default.m_name = "Immediate";
      if (Default.m_queue)
      {
         jude_notification_queue_destroy(Default.m_queue);
      }
      Default.m_queue = nullptr;
   }

}

