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

#include <jude/porting/jude_porting.h>
#include <iostream>

namespace jude
{
   class Mutex
   {
      jude_mutex_t* m_impl;
      int m_lockDepth{ 0 };
      char holdingThread[16];

      Mutex(Mutex&&) = delete;
      Mutex(const Mutex&) = delete;
      Mutex& operator=(const Mutex&) = delete;

   public:
      Mutex()
      {
         holdingThread[0] = '\0';
         m_impl = jude_os->mutex_create();
      }

      ~Mutex()
      {
         jude_os->mutex_destroy(m_impl);
      }

      // for BasicLockable interface
      void lock() 
      { 
         jude_os->mutex_lock(m_impl, -1); 
         m_lockDepth++;
      }

      void unlock() 
      { 
         if (m_lockDepth <= 0)
         {
            jude_fatal("Attmpt to unlock mutex that is not locked");
         }
         m_lockDepth--;
         jude_os->mutex_unlock(m_impl);
      }

      auto GetLockDepth() const { return m_lockDepth; }
   };
}
