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
#include <jude/database/DatabaseEntry.h>
#include <array>
#include <map>

namespace jude
{
   class DatabaseHandle;

   class Database : public DatabaseEntry
   {
      std::string m_name;
      jude_user_t m_accessLevel;
      bool m_allowGlobalRestGet;
      std::map<std::string, DatabaseEntry*> m_entries;
      
      const char* GetNameForSchema() const;

   protected:

      // NOTE: top level database entries live forever
      //  - They can only be "installed" by subsclass of Database
      //  - The subclass should be the "owner" of entries
      //  - If you need to add/remove resources dynamically please use a collection instead of an individual resource.
      //  - You cannot remove top level database entries from a DB once they are present.
      bool InstallDatabaseEntry(DatabaseEntry& entry);

      Database(const Database&) = delete;

   public:

      explicit Database(std::string name, jude_user_t accessLevel, std::shared_ptr<jude::Mutex> mutex);

      void SetAllowGlobalRestGet(bool allowed) { m_allowGlobalRestGet = allowed; }

      const jude_rtti_t* GetType() const { return nullptr; } // database has no single type!

      virtual size_t SubscriberCount() const override;
      virtual void ClearAllDataAndSubscribers() override;

      // RestApiInterface
      virtual RestfulResult RestGet(const char* path, std::ostream& output, const AccessControl& accessControl = accessToEverything) const override;
      virtual RestfulResult RestPost(const char* path, std::istream& input, const AccessControl& accessControl = accessToEverything) override;
      virtual RestfulResult RestPatch(const char* path, std::istream& input, const AccessControl& accessControl = accessToEverything) override;
      virtual RestfulResult RestPut(const char* path, std::istream& input, const AccessControl& accessControl = accessToEverything) override;
      virtual RestfulResult RestDelete(const char* path, const AccessControl& accessControl = accessToEverything) override;

      virtual std::vector<std::string> SearchForPath(CRUD operationType, const char* pathPrefix, jude_size_t maxPaths, jude_user_t userLevel = jude_user_Root) const override;

      DatabaseEntry* FindEntryForPath(const char** fullpath, jude_user_t accessLevel, bool recurse = false);
      const DatabaseEntry* FindEntryForPath(const char** fullpath, jude_user_t accessLevel, bool recurse = false) const;
      virtual DBEntryType GetEntryType() const override { return DBEntryType::DATABASE; }
      
      // From DatabaseEntry - a Database is itself also a Database entry - it can be a sub-Database
      virtual std::string GetName() const override;
      virtual jude_user_t GetAccessLevel(CRUD type) const override;
      virtual std::string DebugInfo() const override;

      virtual bool Restore(std::string path, std::istream& input) override;

      virtual void OutputAllSchemasInYaml(std::ostream& output, std::set<const jude_rtti_t*>& alreadyDone, jude_user_t userLevel) const override;
      virtual void OutputAllSwaggerPaths(std::ostream& output, const std::string& prefix, jude_user_t userLevel) const override;
      void GenerateYAMLforSwaggerOAS3(std::ostream& output, jude_user_t userLevel) const;
      virtual std::string GetSwaggerReadSchema(jude_user_t userLevel) const override;

      //
      virtual SubscriptionHandle SubscribeToAllPaths(std::string prefix, PathNotifyCallback callback, FieldMaskGenerator filterGenerator, NotifyQueue& queue) override;

      // From PubSubInterface...
      virtual SubscriptionHandle OnChangeToPath(
         const std::string& subscriptionPath,
         Subscriber callback,
         FieldMask resourceFieldFilter = FieldMask::ForAllChanges(),
         NotifyQueue& queue = NotifyQueue::Default) override;

   };
}
