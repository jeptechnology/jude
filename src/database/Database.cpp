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

#include <jude/database/Database.h>
#include <jude/database/Swagger.h>
#include <string>
#include <memory>
#include <mutex>
#include <utility>
#include <set>
#include <map>
#include <vector>
#include <optional>
#include <iomanip>

using namespace std;

namespace
{
   std::optional<std::string> VerifyAndTrimPath(const std::string& proposedPath)
   {
      auto fullpath = proposedPath.c_str();
      // prune leading slashes...
      while (fullpath && *fullpath == '/') fullpath++;

      std::string path = fullpath;

      // prune off the trailing slashes...
      if (auto trailing_slash = strchr(fullpath, '/'))
      {
         path = path.substr(0, trailing_slash - fullpath);
         if (*(trailing_slash++) != 0)
         {
            return nullopt;
         }
      }
      return path;
   }
}

namespace jude
{
   bool NotifyImmediatelyOnChange = false;

   Database::Database(std::string name, jude_user_t accessLevel, std::shared_ptr<jude::Mutex> mutex)
      : DatabaseEntry(mutex)
      , m_name(name)
      , m_accessLevel(accessLevel)
      , m_allowGlobalRestGet(true) // by default, we can perform a GET on the entire DB - turn this off if you think this is too much data
   {
   }

   bool Database::InstallDatabaseEntry(DatabaseEntry& entry)
   {
      std::lock_guard<jude::Mutex> lock(*m_mutex);

      auto adjustedPath = VerifyAndTrimPath(entry.GetName());
      if (!adjustedPath)
      {
         return false;
      }

      if (m_entries.find(*adjustedPath) != m_entries.end())
      {
         return false;
      }

      // If we are part of this database then we have to share the same mutex locking
      if (entry.m_mutex != this->m_mutex)
      {
         entry.m_mutex = this->m_mutex;
      }

      m_entries[*adjustedPath] = &entry;

      return true;
   }

   const DatabaseEntry* Database::FindEntryForPath(const char** fullpath, jude_user_t accessLevel, bool recurse) const
   {
      auto token = RestApiInterface::GetNextUrlToken(*fullpath, fullpath);
      if (token == "")
      {
         if (fullpath)
         {
            *fullpath = nullptr; // indicate to caller that the path was a root path
         }
         return nullptr;
      }

      const auto& it = m_entries.find(token);
      if (it == m_entries.end())
      {
         return nullptr;
      }

      if (accessLevel < it->second->GetAccessLevel(CRUD::READ))
      {
         return nullptr; // no permission
      }

      if (!recurse || it->second->GetEntryType() != DBEntryType::DATABASE)
      {
         return it->second;
      }
      
      return reinterpret_cast<const Database*>(it->second)->FindEntryForPath(fullpath, accessLevel, recurse);
   }

   DatabaseEntry* Database::FindEntryForPath(const char** fullpath, jude_user_t accessLevel, bool recurse)
   {
      return const_cast<DatabaseEntry*>(const_cast<const Database*>(this)->FindEntryForPath(fullpath, accessLevel, recurse));
   }

   std::vector<std::string> Database::SearchForPath(CRUD operationType, const char* pathPrefix, jude_size_t maxPaths, jude_user_t userLevel) const
   {
      pathPrefix = pathPrefix ? pathPrefix : "";

      if (pathPrefix[0] == '\0' || pathPrefix[0] != '/')
      {
         return {};
      }

      std::vector<std::string> paths;

      const char* remainingSuffix;
      std::string token = RestApiInterface::GetNextUrlToken(pathPrefix, &remainingSuffix, false);

      if (strlen(remainingSuffix) == 0)
      {
         for (const auto& entry : m_entries)
         {
            if (0 == strncmp(entry.first.c_str(), token.c_str(), token.length()))
            {
               paths.push_back("/" + token + (entry.first.c_str() + token.length()) /*+ "/" */);
            }
         }
      }
      else
      {
         const char* tokenStr = token.c_str();
         const auto ref = FindEntryForPath(&tokenStr, userLevel);
         if (!ref)
         {
            return { };
         }

         auto subpaths = ref->SearchForPath(operationType, remainingSuffix, maxPaths, userLevel);
         for (auto& subpath : subpaths)
         {
            paths.push_back("/" + token + subpath);
         }
      }

      return paths;
   }

   size_t Database::SubscriberCount() const
   { 
      std::lock_guard<jude::Mutex> lock(*m_mutex);
      
      size_t count = 0;
      for (auto& entry : m_entries)
      {
         count += entry.second->SubscriberCount();
      }
      return count;
   }

   void Database::ClearAllDataAndSubscribers()
   {
      std::lock_guard<jude::Mutex> lock(*m_mutex);

      for (auto& entry : m_entries)
      {
         entry.second->ClearAllDataAndSubscribers();
      }
   }


   std::string Database::GetName() const
   {
      return m_name;
   }

   const char* Database::GetNameForSchema() const
   {
      if (m_name.length() > 0)
      {
         return m_name.c_str();
      }
      return "Global";
   }


   jude_user_t Database::GetAccessLevel(CRUD type) const
   {
      return m_accessLevel;
   }

   std::string Database::DebugInfo() const
   {
      std::string output;
      for (const auto& entry : m_entries)
      {
         output += entry.first + " :\n";
         output += entry.second->DebugInfo();
         output += "\n----\n";
      }
      return output;
   }

   RestfulResult Database::RestGet(const char* fullpath, std::ostream& output, const AccessControl& accessControl) const
   {      
      if ( m_allowGlobalRestGet && 
          (fullpath == nullptr || fullpath[0] == 0 || std::string(fullpath) == "/")
         )
      {
         output << '{';
         bool first = true;
         for (auto const& entry : m_entries)
         {
            if (entry.second->GetAccessLevel(CRUD::READ) <= accessControl.GetAccessLevel())
            {
               if (!first)
               {
                  output << ',';
               }

               output << std::quoted(entry.first) << ':';
               entry.second->RestGet("/", output, accessControl);

               first = false;
            }
         }
         output << '}';
         
         return jude_rest_OK;
      }

      if (auto entry = FindEntryForPath(&fullpath, accessControl.GetAccessLevel()))
      {
         return entry->RestGet(fullpath, output, accessControl);
      }
      return fullpath ? jude_rest_Not_Found : jude_rest_Method_Not_Allowed; // method not allowed on root db object
   }

   RestfulResult Database::RestPost(const char* fullpath, std::istream& input, const AccessControl& accessControl)
   {
      if (auto entry = FindEntryForPath(&fullpath, accessControl.GetAccessLevel()))
      {
         return entry->RestPost(fullpath, input, accessControl);
      }
      return fullpath ? jude_rest_Not_Found : jude_rest_Method_Not_Allowed; // method not allowed on root db object
   }

   RestfulResult Database::RestPatch(const char* fullpath, std::istream& input, const AccessControl& accessControl)
   {
      if (auto entry = FindEntryForPath(&fullpath, accessControl.GetAccessLevel()))
      {
         return entry->RestPatch(fullpath, input, accessControl);
      }
      return fullpath ? jude_rest_Not_Found : jude_rest_Method_Not_Allowed; // method not allowed on root db object
   }

   RestfulResult Database::RestPut(const char* fullpath, std::istream& input, const AccessControl& accessControl)
   {
      if (auto entry = FindEntryForPath(&fullpath, accessControl.GetAccessLevel()))
      {
         return entry->RestPut(fullpath, input, accessControl);
      }
      return fullpath ? jude_rest_Not_Found : jude_rest_Method_Not_Allowed; // method not allowed on root db object
   }

   RestfulResult Database::RestDelete(const char* fullpath, const AccessControl& accessControl)
   {
      if (auto entry = FindEntryForPath(&fullpath, accessControl.GetAccessLevel()))
      {
         return entry->RestDelete(fullpath, accessControl);
      }
      return fullpath ? jude_rest_Not_Found : jude_rest_Method_Not_Allowed; // method not allowed on root db object
   }

   SubscriptionHandle Database::OnChangeToPath(
         const std::string& subscriptionPath,
         Subscriber callback,
         FieldMask resourceFieldFilter,
         NotifyQueue& queue)
   {
      std::lock_guard<jude::Mutex> lock(*m_mutex);

      const char* path = subscriptionPath.c_str();
      if (auto entry = FindEntryForPath(&path, jude_user_Root))
      {
         return entry->OnChangeToPath(path, callback, resourceFieldFilter, queue);
      }

      return nullptr;
   }

   void Database::OutputAllSchemasInYaml(std::ostream& output, std::set<const jude_rtti_t*>& alreadyDone, jude_user_t userLevel) const
   {
      for (const auto& entry : m_entries)
      {
         entry.second->OutputAllSchemasInYaml(output, alreadyDone, userLevel);
      }

      // Now if the global read is available, we output the schema for this:
      if (m_allowGlobalRestGet)
      {
         output << "\n"
                << "    " << GetNameForSchema() << "_Schema:\n"
                << "      type: object\n"
                << "      properties:\n";

         for (const auto& entry : m_entries)
         {
            output << entry.second->GetSwaggerReadSchema(userLevel);
         }
      }
   }

   void Database::OutputAllSwaggerPaths(std::ostream& output, const std::string& prefix, jude_user_t userLevel) const
   {
      // output swagger end points descriptions for the entire resource...
      std::string subPrefix = m_name.length() ? (prefix + "/" + m_name) : prefix;
      bool newLineRequired = false;

      if (m_allowGlobalRestGet && userLevel >= m_accessLevel)
      {
         char buffer[1024];
         if (m_name.length() == 0 && prefix.length() == 0)
         {
            // global top level path
            output << "  /:"; // global
            snprintf(buffer, std::size(buffer), swagger::GetTemplate, "entire DB", GetNameForSchema(), GetNameForSchema());
            output << buffer;
         }
         else
         {
            output << "  " << prefix << "/" << m_name << "/:";
            snprintf(buffer, std::size(buffer), swagger::GetTemplate, m_name.c_str(), m_name.c_str(), m_name.c_str(), m_name.c_str());
            output << buffer;
         }
         newLineRequired = true;
      }

      for (const auto& entry : m_entries)
      {
         if (userLevel < entry.second->GetAccessLevel(CRUD::CREATE)
            && userLevel < entry.second->GetAccessLevel(CRUD::READ)
            && userLevel < entry.second->GetAccessLevel(CRUD::UPDATE)
            && userLevel < entry.second->GetAccessLevel(CRUD::DELETE))
         {
            // Cannot possibly access *any* API endpoints in this resource so skip completely
            continue;
         }

         if (newLineRequired)
         {
            output << "\n";
         }
         entry.second->OutputAllSwaggerPaths(output, subPrefix, userLevel);         
         newLineRequired = true;
      }
   }

   void Database::GenerateYAMLforSwaggerOAS3(std::ostream& output, jude_user_t userLevel) const
   {
      char buffer[1024];

      // Opening "info" and "servers"...
      snprintf(buffer, 1024, swagger::HeaderTemplate, m_name.c_str(), "data/v2");
      output << buffer;

      // All paths
      output << "\n\npaths:\n";
      OutputAllSwaggerPaths(output, "", userLevel);

      // Common components (errors, parameters, etc)
      output << "\n";
      output << swagger::ComponentsTemplate;
      output << "\n  schemas:\n";

      // Schemas in use in this API
      std::set<const jude_rtti_t*> alreadyDone;
      OutputAllSchemasInYaml(output, alreadyDone, userLevel);
   }

   std::string Database::GetSwaggerReadSchema(jude_user_t userLevel) const
   {
      if (userLevel < m_accessLevel)
      {
         return "";
      }

      std::stringstream output;
      const char* name = GetNameForSchema();
      output << "        " << name << ":\n";
      output << "          $ref: '#/components/schemas/" << name << "_Schema'\n";
      return output.str();
   }

   SubscriptionHandle Database::SubscribeToAllPaths(std::string prefix, PathNotifyCallback callback, FieldMaskGenerator filterGenerator, NotifyQueue& queue)
   {
      std::vector<SubscriptionHandle> unsubscribers;
      
      for (const auto& entry : m_entries)
      {
         auto newPath = prefix + "/" + entry.first;
         unsubscribers.push_back(entry.second->SubscribeToAllPaths(newPath, callback, filterGenerator, queue));         
      }

      return SubscriptionHandle([unsubscribers = std::move(unsubscribers)] { 
         for (auto handle : unsubscribers)
         {
            handle.Unsubscribe();
         }
      });
   }

   bool Database::Restore(std::string path, std::istream& input)
   {
      const char *next = path.c_str();
      if (auto entry = FindEntryForPath(&next, jude_user_Root))
      {
         return entry->Restore(next, input);
      }
      return false;
   }
}


