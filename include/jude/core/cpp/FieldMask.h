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
#include <vector>
#include <string>
#include <functional>

namespace jude 
{
   using FieldIndex = jude_index_t;

   class FieldMask
   {
      jude_filter_t m_filter;

   public:
      using FieldMaskGenerator = std::function<FieldMask(const jude_rtti_t& type)>;

      FieldMask()
      {
         ClearAll();
      }

      FieldMask(const jude_filter_t& resourceFieldFilter)
         : m_filter(resourceFieldFilter)
      {}

      FieldMask(FieldIndex index);
      FieldMask(std::initializer_list<FieldIndex> indeces);

      const jude_filter_t& Get() const { return m_filter; }

      void ClearAll();
      void ClearAllChanged();
      void ClearAllTouched();

      void FillAll();
      void FillAllChanged();
      void FillAllTouched();

      bool IsEmpty() const;
      bool IsAnySet() const;
      bool IsAnyChanged() const;

      void Set(FieldIndex index);
      void Clear(FieldIndex index);
      bool IsSet(FieldIndex index) const;

      void SetChanged(FieldIndex index);
      void ClearChanged(FieldIndex index);
      bool IsChanged(FieldIndex index) const;

      bool operator&&(const FieldMask& rhs) const;
      FieldMask& operator &=(const FieldMask& rhs);
      FieldMask& operator |=(const FieldMask& rhs);

      std::vector<jude_index_t> AsVector() const;

      // Field Filter generator functions
      static FieldMask ForPersistence(const jude_rtti_t& type, bool deltasOnly = false);
      static FieldMask ForPersistence_DeltasOnly(const jude_rtti_t& type) { return ForPersistence(type, true); }

      static FieldMask ForUser  (const jude_rtti_t& type, jude_user_t user);      
      static FieldMask ForAdmin (const jude_rtti_t& type)  { return ForUser(type, jude_user_Admin); }
      static FieldMask ForPublic(const jude_rtti_t& type)  { return ForUser(type, jude_user_Public); }
      static FieldMask ForRoot  (const jude_rtti_t& type)  { return ForUser(type, jude_user_Root); }
      static FieldMaskGenerator ForUser(jude_user_t user) 
      { 
         return [=](const jude_rtti_t& type) { 
            return ForUser(type, user); 
         }; 
      }      
      
      static FieldMask ForFields(std::initializer_list<FieldIndex> fieldIndeces, bool deltasOnly = false);
      static FieldMask ForFields(const jude_rtti_t& type, std::vector<std::string> fieldNames, bool deltasOnly = false);
      static FieldMask ForAllChanges();

      // Backwards compatibility
      void Allow(FieldIndex index) { SetChanged(index); }
   };

   using FieldMaskGenerator = FieldMask::FieldMaskGenerator;
}
