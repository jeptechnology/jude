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
#include <initializer_list>

namespace jude
{
   namespace RestApiSecurityLevel
   {
      using Value = jude_user_t;

      static constexpr Value Local  = jude_user_Public;
      static constexpr Value Cloud  = jude_user_Cloud;
      static constexpr Value Admin  = jude_user_Admin;
      static constexpr Value Root   = jude_user_Root;
   };

   class AccessControl
   {
      // Apply the access level to all levels of the document hierarchy
      RestApiSecurityLevel::Value   m_accessLevel;   // what's my access level? Applies to all levels of hierachy
      bool          m_onlyPersisted; // only read/write what is persisted

      // Apply these only to the root object...
      bool          m_rootDeltasOnly;  // what fields in the root object have changed?
      jude_filter_t m_rootFieldFilter; // what fields in the root object am I interested in?

      void ApplyTopLevelFilter(jude_filter_t& filter) const;
      void ApplyDeltasOnlyFilter(jude_filter_t& filter) const;
      void GetFilter(const jude_object_t* resource, bool forReading, jude_filter_t& filter) const;

   public:
      AccessControl(RestApiSecurityLevel::Value accessLevel = jude_user_Root, 
                    const jude_filter_t* rootFieldFilter = nullptr, 
                    bool deltasOnly = false, 
                    bool persistentOnly = false);

      // convenience functions
      static AccessControl Make(RestApiSecurityLevel::Value accessLevel, jude_filter_t* rootFieldFilter = nullptr);
      static AccessControl Make_forDeltas(RestApiSecurityLevel::Value accessLevel, jude_filter_t* rootFieldFilter = nullptr);
      static AccessControl Make_forPersistence(RestApiSecurityLevel::Value accessLevel, jude_filter_t* rootFieldFilter = nullptr);
      static AccessControl Make_forPersistenceDeltas(RestApiSecurityLevel::Value accessLevel, jude_filter_t* rootFieldFilter = nullptr);
      static AccessControl Make_forFields(std::initializer_list<jude_index_t> fields);

      RestApiSecurityLevel::Value GetAccessLevel() const { return m_accessLevel; }

      virtual void GetReadFilter(const jude_object_t* resource, jude_filter_t& filter) const;
      virtual void GetWriteFilter(const jude_object_t* resource, jude_filter_t& filter) const;

      static AccessControl ForPersistence(const jude_rtti_t* topLevelType = nullptr);
   };
}
