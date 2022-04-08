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
#include <jude/core/cpp/Notification.h>
#include <jude/core/cpp/NotifyQueue.h>
#include <jude/core/cpp/FieldMask.h>
#include <string>
#include <functional>

namespace jude
{
   class Object;

   class SubscriptionHandle
   {
      std::function<void()> m_unsubscriber; // function returned by all subscriptions - call this to unsubscribe
   
   public:
      SubscriptionHandle(std::function<void()>&& unsubscribe) : m_unsubscriber(std::move(unsubscribe)) {}
      SubscriptionHandle(std::nullptr_t) : m_unsubscriber(nullptr) {} 
      SubscriptionHandle() : m_unsubscriber(nullptr) {} 

      void Unsubscribe() 
      { 
         if (m_unsubscriber) 
         {
            m_unsubscriber(); 
            m_unsubscriber = nullptr;
         }
      }

      operator bool() const { return m_unsubscriber.operator bool(); }
   };

   /*
    * Publish / Subscribe interface
    * 
    * NOTE: The absence of a "publish" method is deliberate. 
    * It is sometimes necessary to protect who can publish changes and when
    * It is up to the implementing sub class when to publish changes based on its own knowledge
    */
   template<class T_Object = Object>
   class PubSubInterface
   {
   public:
      using T_Subscriber = std::function<void(const Notification<T_Object>&)>;

      // To implement...
      virtual SubscriptionHandle OnChange(T_Subscriber callback,
                                    FieldMask    resourceFieldFilter,
                                    NotifyQueue& queue = NotifyQueue::Default) = 0;

      // OnChange to all field changes
      virtual SubscriptionHandle OnChange(T_Subscriber callback,
                                    NotifyQueue& queue = NotifyQueue::Default)
      {
         return OnChange(callback, FieldMask::ForAllChanges().Get(), queue);
      }

      SubscriptionHandle OnAdded(T_Subscriber callback,
                           NotifyQueue& queue = NotifyQueue::Default)
      {
         return OnChange([=] (const Notification<T_Object>& info)
            {
               if (info.IsNew()) callback(info);
            },
            { JUDE_ID_FIELD_INDEX },
            queue);
      }

      SubscriptionHandle OnDeleted(T_Subscriber callback,
                             NotifyQueue& queue = NotifyQueue::Default)
      {
         return OnChange([=] (const Notification<T_Object>& info)
            {
               if (info.IsDeleted()) callback(info);
            },
            { JUDE_ID_FIELD_INDEX },
            queue); 
      }
   };

   template<>
   class PubSubInterface<Object>
   {
   public:
      using Subscriber = std::function<void(const Notification<Object>&)>;

      // To implement...
      virtual SubscriptionHandle OnChangeToPath(const std::string& subscriptionPath, // subscribe inside 
                                           Subscriber callback,
                                           FieldMask  resourceFieldFilter = FieldMask::ForAllChanges(),
                                           NotifyQueue& queue = NotifyQueue::Default) = 0;

      virtual SubscriptionHandle OnChangeToObject(Subscriber callback,
                                            FieldMask  resourceFieldFilter = FieldMask::ForAllChanges(),
                                            NotifyQueue& queue = NotifyQueue::Default)
      {
         return OnChangeToPath("", callback, resourceFieldFilter, queue);
      }

      SubscriptionHandle OnObjectAdded(Subscriber callback,
                                 const std::string& subscriptionPath = "",
                                 NotifyQueue& queue = NotifyQueue::Default)
      {
         return OnChangeToPath(
            subscriptionPath,
            [=] (const Notification<Object>& info) {
               if (info.IsNew()) callback(info);
            }, 
            { JUDE_ID_FIELD_INDEX }, 
            queue); 
      }

      SubscriptionHandle OnObjectDeleted(Subscriber callback,
                                   const std::string& subscriptionPath = "",
                                   NotifyQueue& queue = NotifyQueue::Default)
      {
         return OnChangeToPath(
            subscriptionPath,
            [=] (const Notification<Object>& info) {
               if (info.IsDeleted()) callback(info);
            }, 
            { JUDE_ID_FIELD_INDEX }, 
            queue); 
      }
   };
}
