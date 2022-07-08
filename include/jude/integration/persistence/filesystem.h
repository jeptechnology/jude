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
   This serves as an example of how to persist the data inside a jude database using the std::filesystem library of C++17
   Please treat this code as a development template. 
   You can intergrate your own persistence layer if you so wish. 
*/

#pragma once

#include <fstream>
#include <filesystem>
#include <jude/database/Database.h>
#include <jude/core/cpp/Stream.h>



namespace
{
   namespace fs = std::filesystem;

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
   class FileSystemPersistence
   {
      Database&    m_db;
      SubscriptionHandle m_unsubscriber;

      void Restore(const fs::path& filepath, const std::string datamodelPath)
      {
         std::ifstream input_file(filepath);
         if (input_file.is_open()) 
         {
            m_db.Restore(datamodelPath, input_file);
         }
      }

      void BootstrapData(const std::string &rootFolder)
      {
         fs::create_directories(fs::path(rootFolder));

         for (auto& file : fs::recursive_directory_iterator(fs::path(rootFolder)))
         {
            if (file.is_regular_file())
            {
               Restore(file.path(), file.path().string().c_str() + rootFolder.length());
            }            
         }
      }

   public:
      FileSystemPersistence(Database& db, std::string rootFolder, jude::NotifyQueue& queue = jude::NotifyQueue::Immediate)
         : m_db(db)
      {
         BootstrapData(rootFolder);
         m_unsubscriber = db.SubscribeToAllPaths(rootFolder, Callback, FieldMask::ForPersistence_DeltasOnly, queue);
      }

      ~FileSystemPersistence()
      {
         m_unsubscriber.Unsubscribe();
      }
   };
}
