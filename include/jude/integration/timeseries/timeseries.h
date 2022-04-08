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

/*
   This serves as an example of how to gather a timeseries from the database
   Please treat this code as a development template. 
   You can intergrate your own persistence layer if you so wish. 
*/

#pragma once

#include <thread>
#include <jude/database/Database.h>
#include <jude/core/cpp/Stream.h>

namespace
{
   void Callback(const std::string& pathThatChanged, const jude::Notification<jude::Object>& info)
   {
      fs::path path(pathThatChanged);

      if (info.IsDeleted())
      {
         fs::remove(path);
         return;
      }

      if (info.IsNew())
      {
         fs::create_directories(path.parent_path());
      }

      auto json = info->ToJSON(); 
      std::ofstream fileout(pathThatChanged);
      fileout << json;
   }
}

namespace jude 
{
   struct TelemetryConfig
   { 
      std::string              path;         // JSON path into data model
      std::chrono::seconds     minInterval;  // The min period between each publish 
      std::chrono::seconds     maxInterval;  // The max period between each 
      size_t                   batchSize;    // Batch values up into groups of this size when publishing (optional - default is 1)
      size_t                   capacity;     // Total history cpacity - will cycle round when full
      std::vector<std::string> onChange;     // Take new values when these When these change, take the new 
      std::vector<std::string> outputFilter; // output these fields
   }

   struct Eventseries
   { 
      std::string          path;
      std::chrono::seconds interval;
   }

   class SeriesHandling
   {
      std::unique_ptr<std::thread> m_thread;

   public:
      SeriesHandling(Database& db)
         : m_db(db)
      {
      }


   };
}
