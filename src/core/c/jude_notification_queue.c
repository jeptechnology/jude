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

#include <stdlib.h>

#include <jude/jude_core.h>

jude_notification_queue_t* jude_notification_queue_create(size_t max_pending_notifications)
{
   return (jude_notification_queue_t *)jude_os->queue_create(max_pending_notifications, sizeof(jude_notification_t));
}

void jude_notification_queue_destroy(jude_notification_queue_t* queue)
{
   jude_os->queue_destroy((jude_queue_t*)queue);
}

void jude_notification_queue_post(jude_notification_queue_t* queue, const jude_notification_t* notification)
{
   jude_assert(queue);
   jude_assert(notification);
   jude_os->queue_send((jude_queue_t*)queue, notification);
}

bool jude_notification_queue_process(jude_notification_queue_t* queue, uint32_t max_wait_ms)
{
   if (!queue)
   {
      return false; // no notification queue!
   }

   jude_notification_t notification;

   if (!jude_os->queue_receive((jude_queue_t*)queue, &notification, max_wait_ms))
   {
      return false; // nothing to do...
   }

   if (!notification.callback)
   {
      return false; // can't process this notification without a subcriber to notify
   }

   notification.callback(notification.user_data);

   return true;
}

