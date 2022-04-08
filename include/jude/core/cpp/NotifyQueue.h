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

#pragma once

#include <jude/jude_core.h>
#include <string>
#include <list>
#include <functional>

#include "Notification.h"

namespace jude
{
   class NotifyQueue
   {
      std::string m_name;
      jude_notification_queue_t *m_queue;
      std::vector<std::function<void()>> m_pausedNotifications;
      bool m_paused {false};

      NotifyQueue(std::nullptr_t);  // null queue - only use this for immediate dispatch of notifications

   public:
      // Use this queue if you want notifications to be processed immediately rather than posted to a queue
      static NotifyQueue Immediate; 
      
      // Unless specified, we will always use the default queue for subscriptions
      static NotifyQueue Default;

      static void SetDefaultQueue(const std::string& name, size_t maxDepth);
      static void SetDefaultQueueAsImmediate();

      explicit NotifyQueue(const std::string& name, size_t maxDepth = 128);
      ~NotifyQueue();

      // temporarily stop events being processed
      void Pause();
      void Play();

      bool operator==(const NotifyQueue& rhs)
      {
         return m_queue == rhs.m_queue;
      }

      bool IsImmediate() const { return m_queue == nullptr; }
      
      void Send(std::function<void()>&& callback);

      // Wait for up to "maxWaitMs" milliseconds for a new item in the queue and process it
      // returns true when notifications are processed
      bool Process(uint32_t maxWaitMs = 0);
   };
}

