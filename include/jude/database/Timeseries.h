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

#include <jude/jude.h>
#include "Database.h"
#include "CircularBuffer.h"
#include <algorithm>
#include <ctime>

namespace jude
{
   class TimeseriesFieldBase
   {
      // Wed Jan 01 2020 01:01:01 GMT+0000
      static constexpr time_t EpochAdjust = 1577840461;

      const jude_field_t &m_field;
      CircularBuffer<uint32_t> m_timestamps; // Seconds since Jan 1 2020. Stored as 32 bit number to save space
   
   public:
      TimeseriesFieldBase(const jude_field_t &field, size_t capacity)
         : m_field(field)
         , m_timestamps(capacity)
      {
      }

      auto GetName() const { return m_field.label; }
      auto GetField() const { return m_field; }
      
   };


   template<typename T_Value>
   class TimeseriesField
   {
      CircularBuffer<T_Value>  m_data;
      
   public:
      using const_reference = std::pair<uint32_t, T_Value>;

      explicit TimeseriesFieldBase(const jude_field_t &field, size_t capacity)
         : m_field(field)
         , m_timestamps(capacity)
         , m_data(capacity)
      {
      }

      const_reference front() const { return std::make_pair<uint32_t, T_Value>(m_timestamps.front(), m_data.front()); }
      const_reference back()  const { return std::make_pair<uint32_t, T_Value>(m_timestamps.back(), m_data.back()); }
      void clear() { m_timestamps.clear(); m_data.clear(); }
      void push_back(time_t time, const T &value)
      {
         m_timestamps.push_back(TimeToOffset(time));
         m_data.push_back(value);
      }

      size_type size() const { return m_data.size(); }
      size_type capacity() const { return m_data.capacity(); }

      bool empty() const { return m_data.empty() }
      bool full() const { return m_data.full(); }

      const_reference upper_bound(time_t timestamp)
      {
         auto upperbound = std::upper_bound(m_timestamps.begin(), m_timestamps.end(), timestamp);
         if (upperbound != m_timestamps.end())
         {

         }
      }

   private:
      uint32_t TimeToOffset(time_t timestamp) const 
      {
         return static_cast<uint32_t>(timestamp - EpochAdjust);
      }

      time_t OffsetToTime(uint32 timestamp) const
      {
         return static_cast<time_t>(timestamp) + EpochAdjust;
      }
   };
}
