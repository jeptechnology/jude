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

#include <jude/core/cpp/AccessControl.h>
#include <jude/core/cpp/FieldMask.h>

namespace jude
{
   AccessControl::AccessControl(
      jude_user_t accessLevel,
      const jude_filter_t* rootFieldFilter,
      bool deltasOnly,
      bool persistentOnly)
      : m_accessLevel(accessLevel)
      , m_onlyPersisted(persistentOnly)
      , m_rootDeltasOnly(deltasOnly)
   {
      if (rootFieldFilter)
      {
         memcpy(&m_rootFieldFilter, rootFieldFilter, sizeof(jude_filter_t));
      }
      else
      {
         jude_filter_fill_all(&m_rootFieldFilter);
      }
   }

   void AccessControl::ApplyTopLevelFilter(jude_filter_t& filter) const
   {
      // start with "everything" filter 
      jude_filter_and_equals(&filter, &m_rootFieldFilter);
   }

   void AccessControl::ApplyDeltasOnlyFilter(jude_filter_t& filter) const
   {
      if (m_rootDeltasOnly)
      {
         jude_filter_clear_all_touched(&filter);
      }
   }

   AccessControl AccessControl::Make(jude_user_t accessLevel, jude_filter_t* rootFieldFilter)
   {
      return AccessControl(accessLevel, rootFieldFilter, false, false);
   }

   AccessControl AccessControl::Make_forDeltas(jude_user_t accessLevel, jude_filter_t* rootFieldFilter)
   {
      return AccessControl(accessLevel, rootFieldFilter, true, false);
   }

   AccessControl AccessControl::Make_forPersistence(jude_user_t accessLevel, jude_filter_t* rootFieldFilter)
   {
      return AccessControl(accessLevel, rootFieldFilter, false, true);
   }

   AccessControl AccessControl::Make_forPersistenceDeltas(jude_user_t accessLevel, jude_filter_t* rootFieldFilter)
   {
      return AccessControl(accessLevel, rootFieldFilter, true, true);
   }

   AccessControl AccessControl::Make_forFields(std::initializer_list<jude_index_t> fields)
   {
      auto mask = FieldMask::ForFields(fields);
      return AccessControl(jude_user_Root, &mask.Get());
   }

   void AccessControl::GetFilter(const jude_object_t* resource, bool forReading, jude_filter_t& filter) const
   {
      jude_filter_fill_all(&filter);

      if (jude_object_is_top_level(resource))
      {
         ApplyTopLevelFilter(filter);
         ApplyDeltasOnlyFilter(filter);
      }

      // construct an access control filter based on our settings
      jude_filter_t access_control_filter;
      jude_filter_clear_all(&access_control_filter);
      auto lastField = &resource->__rtti->field_list[resource->__rtti->field_count];
      for (auto field = resource->__rtti->field_list; field != lastField; field++)
      {
         bool isAccessible =    (forReading && jude_field_is_readable(field, m_accessLevel))
                             || (!forReading && jude_field_is_writable(field, m_accessLevel));
         isAccessible &= !m_onlyPersisted || jude_field_is_persisted(field);
         jude_filter_set_changed(access_control_filter.mask, field->index, isAccessible);
         jude_filter_set_touched(access_control_filter.mask, field->index, isAccessible);
      }

      jude_filter_and_equals(&filter, &access_control_filter);
   }

   void AccessControl::GetReadFilter(const jude_object_t* resource, jude_filter_t& filter) const
   {
      GetFilter(resource, true, filter);
   }

   void AccessControl::GetWriteFilter(const jude_object_t* resource, jude_filter_t& filter) const
   {
      GetFilter(resource, false, filter);
   }
};
