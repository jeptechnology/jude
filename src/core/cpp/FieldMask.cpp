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

#include <jude/core/cpp/FieldMask.h>

namespace jude 
{
   FieldMask::FieldMask(FieldIndex index)
   {
      ClearAll();
      SetChanged(index);
   }

   FieldMask::FieldMask(std::initializer_list<FieldIndex> fieldIndeces)
   {  
      ClearAll();
      for (const auto& index : fieldIndeces)
      {
         SetChanged(index);
      }
   }

   void FieldMask::ClearAll()
   {
      jude_filter_clear_all(&m_filter);
   }

   void FieldMask::ClearAllChanged()
   {
      jude_filter_clear_all_changed(&m_filter);
   }

   void FieldMask::ClearAllTouched()
   {
      jude_filter_clear_all_touched(&m_filter);
   }

   void FieldMask::FillAll()
   {
      jude_filter_fill_all(&m_filter);
   }

   void FieldMask::FillAllChanged()
   {
      jude_filter_fill_all_changed(&m_filter);
   }

   void FieldMask::FillAllTouched()
   {
      jude_filter_fill_all_touched(&m_filter);
   }
   
   bool FieldMask::IsEmpty() const
   {
      return jude_filter_is_empty(&m_filter);
   }

   bool FieldMask::IsAnyChanged() const
   {
      return jude_filter_is_any_changed(&m_filter);
   }
   bool FieldMask::IsAnySet() const
   {
      return jude_filter_is_any_touched(&m_filter);
   }
   bool FieldMask::operator&&(const FieldMask& rhs) const
   {
      return jude_filter_is_overlapping(&m_filter, &rhs.m_filter);
   }
   void FieldMask::SetChanged(FieldIndex index)
   {
      return jude_filter_set_changed(m_filter.mask, index, true);
   }
   void FieldMask::Set(FieldIndex index)
   {
      return jude_filter_set_touched(m_filter.mask, index, true);
   }
   bool FieldMask::IsChanged(FieldIndex index) const
   {
      return jude_filter_is_changed(m_filter.mask, index);
   }
   bool FieldMask::IsSet(FieldIndex index) const
   {
      return jude_filter_is_touched(m_filter.mask, index);
   }

   FieldMask& FieldMask::operator &=(const FieldMask& rhs)
   {
      jude_filter_and_equals(&m_filter, &rhs.m_filter);
      return *this;
   }

   FieldMask& FieldMask::operator |=(const FieldMask& rhs)
   {
      jude_filter_or_equals(&m_filter, &rhs.m_filter);
      return *this;
   }

   std::vector<jude_index_t> FieldMask::AsVector() const
   {
      std::vector<jude_index_t> changedFieldIndicies;
      for (jude_index_t index = 0; index < JUDE_MAX_FIELDS_PER_MESSAGE; index++)
      {
         if (IsChanged(index)) changedFieldIndicies.push_back(index);
      }
      return changedFieldIndicies;
   }

   FieldMask FieldMask::ForPersistence(const jude_rtti_t& type, bool deltasOnly)
   {
      FieldMask filter;

      for (int i = 0; i < type.field_count; i++)
      {
         if (type.field_list[i].persist)
         {
            if (!deltasOnly)
            {
               filter.Set(i);
            }
            filter.SetChanged(i);
         }
      }

      return filter;
   }

   FieldMask FieldMask::ForUser(const jude_rtti_t& type, RestApiSecurityLevel::Value user)
   {
      FieldMask filter;

      for (int i = 0; i < type.field_count; i++)
      {
         if (type.field_list[i].permissions.read <= user)
         {
            filter.SetChanged(i);
         }
      }
       return filter;
   }

   FieldMask FieldMask::ForFields(std::initializer_list<FieldIndex> fieldIndeces, bool deltasOnly)
   {
      FieldMask filter;

      for (const auto& index : fieldIndeces)
      {
         filter.SetChanged(index);
         if (!deltasOnly)
         {
            filter.Set(index);
         }
      }
      return filter;
   }

   FieldMask ForFields(const jude_rtti_t& type, std::vector<std::string> fieldNames, bool deltasOnly)
   {
      FieldMask filter;

      for (const auto& name : fieldNames)
      {
         if (auto field = jude_rtti_find_field(&type, name.c_str()))
         {
            filter.SetChanged(field->index);
            if (!deltasOnly)
            {
               filter.Set(field->index);
            }
         }
      }
      return filter;
   }

   FieldMask FieldMask::ForAllChanges()
   {
      FieldMask filter;
      jude_filter_fill_all_changed(&filter.m_filter);
      return filter;
   }


}
