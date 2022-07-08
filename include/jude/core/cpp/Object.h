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

#include <string>
#include <functional>
#include <optional>
#include <vector>
#include <memory>

#include <jude/jude_core.h>
#include <jude/core/cpp/FieldMask.h>
#include <jude/core/cpp/RestApiInterface.h>


namespace jude
{

   using UnknownFieldHandler = std::function<bool(const char *unknownField, const char *fieldData)>;
   using ExtraFieldHandler = std::function<bool(const char **extraField, const char **fieldData)>;

   // Shared Object behave like Java references
   // - You can assign 

   class Object : public RestApiInterface
   {
      friend class BaseArray;
      friend class GenericObjectArray;
      friend class GenericResource;
      friend class CollectionBase;

      struct SharedRootData
      {
         std::unique_ptr<jude_object_t> object;
         std::function<void()> onChange;    // called when object or subpath changes
         std::function<void()> onSingleRef; // called when reference count of this shared data is about to be one
      };

      std::shared_ptr<SharedRootData> m_sharedRoot;
      
      void ReleaseSharedData();
      void OnEdited();

   protected:
      jude_object_t *m_object; // if not null, always points to somewhere inside m_sharedRoot

      void MarkFieldSet(jude_size_t index, bool isChanged);
      void MarkObjectAsNew() { jude_filter_set_changed(m_object->__mask, JUDE_ID_FIELD_INDEX, true); }

      // Used when creating a new object
      explicit Object(const jude_rtti_t& type, 
                     std::function<void()> onChange = nullptr,
                     std::function<void()> onSingleRef = nullptr);

      // Used when creating a subobject
      // relative can be any object with the same root
      explicit Object(Object& relative, jude_object_t &child);

      // Used to safely construct a child object from this one
      template<class T_Object, typename T_Struct>
      T_Object GetChild(T_Struct &child)
      {
         return T_Object(*this, (jude_object_t &)child);
      }


   public:
      Object() noexcept;          // default constructor is a null resource
      Object(std::nullptr_t) noexcept; // default constructor is a null resource
      Object(Object&) noexcept;   // can only copy non-const objects
      Object(Object&&) noexcept;  // can only copy non-const objects
      Object& operator=(Object&) noexcept;
      Object& operator=(Object&&) noexcept;
      Object& operator=(std::nullptr_t) noexcept { return operator=(Object(nullptr));  }

      virtual ~Object();

      void DestroyAndPreventCallbacks()
      {
         // This prevents any callbacks on this object 
         m_sharedRoot->onChange = nullptr;
         m_sharedRoot->onSingleRef = nullptr;
         m_sharedRoot.reset();
      }

      Object Clone(bool andClearChangeMarkers = true, 
                  std::function<void()> onChange = nullptr,
                  std::function<void()> onSingleRef = nullptr) const 
      {
         if (m_object == nullptr)
         {
            return nullptr;
         }

         Object clone(*m_object->__rtti, onChange, onSingleRef);
         clone.OverwriteData(*this, andClearChangeMarkers);

         // we no longer have a parent.
         clone.m_sharedRoot->object->__parent_offset = 0;

         return std::move(clone);
      }

      template<class T_Object>
      T_Object As()
      {
         if (IsOK())
         {
            return T_Object(*this, *m_object);
         }
         return nullptr;
      }

      template<class T_Object = Object>
      T_Object CloneAs(bool andClearChangeMarkers = true) const
      {
         return Clone(andClearChangeMarkers).As<T_Object>();
      }

      size_t RefCount() const { return m_sharedRoot.use_count(); }

      Object Parent();
      const Object Parent() const;

      bool operator==(const Object &other) const;
      bool operator!=(const Object &other) const;
      bool IsOK() const { return m_object != nullptr; }
      operator bool() const { return IsOK(); }
      bool IsTypedObject() const;
      bool IsType(const jude_rtti_t* type) const { return m_object->__rtti == type; }

      // The "id" is a useful "index" into collections and arrays:
      // - the id is unique to an instance of the micro document engine (but not globally across multiple instances of the document engine)
      // - a missing id will indicate that this object is an "empty slot" and can be used by "Add", "Insert" or "Post" to the object array
      jude_id_t Id() const { return m_object->m_id; }
      bool      IsIdAssigned() const;
      Object&   AssignId(jude_id_t newId);

      // Only valid for Objects inside a collection
      bool IsNew() const { return IsChanged(JUDE_ID_FIELD_INDEX); }
      bool IsDeleted() const { return !IsIdAssigned(); }

      bool Has(jude_index_t fieldIndex) const { return jude_filter_is_touched(m_object->__mask, fieldIndex); }
      // Is anything changed?
      bool IsChanged() const { return jude_object_is_changed(m_object); }
      // Is fieldIndex changed?
      bool IsChanged(jude_index_t fieldIndex) const { return jude_filter_is_changed(m_object->__mask, fieldIndex); }
      FieldMask GetChanges() const; // return all changes and clears the change flags
      void ClearChangeMarkers() { jude_object_clear_change_markers(m_object); } 
      void NotifyThatFieldChanged(jude_index_t fieldIndex) { MarkFieldSet(fieldIndex, true); }

      // Clear All
      void Clear() { jude_object_clear_touch_markers(m_object); OnEdited(); }
      // Clear a field
      void Clear(jude_index_t fieldIndex) { jude_object_mark_field_touched(m_object, fieldIndex, false); OnEdited(); }
      // Clear an element from an array item
      void Clear(jude_index_t fieldIndex, jude_index_t arrayIndex);

      bool IsEmpty() const { return !jude_object_is_touched(m_object); }
      jude_size_t CountField(jude_size_t fieldIndex) const;
      std::string GetFieldAsString(jude_size_t fieldIndex, jude_index_t arrayIndex = 0) const;
      int64_t GetFieldValue(jude_size_t fieldIndex, jude_index_t arrayIndex = 0) const;
      const char* FieldName(jude_size_t fieldIndex) const;

      std::string GetField(const std::string& field) const { return ToJSON_EmptyOnError(field.c_str()); }
      void SetField(const std::string& field, const std::string& value) { RestPatchString(field.c_str(), value.c_str()); }

      template<typename T>
      void SetFieldAsNumber(jude_size_t fieldIndex, T newValue, jude_index_t arrayIndex = 0)
      {
         static_assert(std::is_arithmetic<T>::value, "T must be numeric");
      
         // NOTE: This is a safe operation (no memory corruption) but it will produce undefined values in the target field if the field width is wrong
         jude_object_set_value_in_array(m_object, fieldIndex, arrayIndex, &newValue);
      }

      template<typename T>
      T GetFieldAsNumber(jude_size_t fieldIndex, jude_index_t arrayIndex = 0) const
      {
         static_assert(std::is_arithmetic<T>::value, "T must be numeric");
         return static_cast<T>(GetFieldValue(fieldIndex, arrayIndex));
      }

      void OverwriteData(const Object& rhs, bool andClearChangeMarkers = true); // all data is overwritten, no changes are detected, no subscriptions moved
      void TransferFrom(Object&& rhs); // all data and change markers transferred from rhs
      bool Put(const Object& rhs); // returns true when there are changes detected, no subscriptions moved
      bool Patch(const Object& rhs); // returns true when there are changes detected, no subscriptions moved
      RestfulResult UpdateFromJson(const std::string& JSON, bool deltasOnly = true) { return UpdateFromJson(JSON, nullptr, deltasOnly); }
      RestfulResult UpdateFromJson(const std::string& JSON, UnknownFieldHandler handler, bool deltasOnly = true);
      RestfulResult UpdateFromJson(std::istream& JSON, bool deltasOnly = true);

      // REST API interface for this resource
      virtual RestfulResult RestGet   (const char* path, std::ostream& output, const AccessControl& accessControl = accessToEverything) const override;
      virtual RestfulResult RestPost  (const char* path, std::istream& input, const AccessControl& accessControl = accessToEverything) override;
      virtual RestfulResult RestPatch (const char* path, std::istream& input, const AccessControl& accessControl = accessToEverything) override;
      virtual RestfulResult RestPut   (const char* path, std::istream& input, const AccessControl& accessControl = accessToEverything) override;
      virtual RestfulResult RestDelete(const char* path, const AccessControl& accessControl = accessToEverything) override;

      virtual std::vector<std::string> SearchForPath(CRUD operationType, const char* pathPrefix, jude_size_t maxPaths, jude_user_t userLevel = jude_user_Root) const override;

      virtual std::string ToString(jude_size_t fieldIndex, jude_size_t maxSize = 0xFFFF, jude_user_t userLevel = jude_user_Root) const
      {
         return ToJSON_EmptyOnError(FieldName(fieldIndex), maxSize, userLevel);
      }
      std::string ToJSON_WithExtraField(ExtraFieldHandler extraField, jude_user_t userLevel, jude_size_t maxSize = 0xFFFF) const;

      std::string operator[](std::string path) const
      {
         return ToJSON_EmptyOnError(path.c_str());
      }

      std::string DebugInfo(const jude_filter_t* filterOverride = nullptr) const;
      jude_size_t CountSubscribers(bool recursive = false) const;

      // For internal use
      auto RawData() { return m_object; }
      auto RawData() const { return m_object; }

      const jude_rtti_t& Type() const { return *m_object->__rtti; }

      static constexpr const jude_rtti_t* RTTI() { return nullptr; };

      ///////////////////////////////////////////////////////////////////////////////
      // Protobuf backwards compatibility
      void ClearFieldSet(jude_index_t index) { Clear(index); }
      bool IsFieldSet(jude_index_t fieldIndex) const { return Has(fieldIndex); } 
      bool IsFieldChanged(jude_index_t fieldIndex) const { return IsChanged(fieldIndex); } 
      // NOTE: This is unhelpful when you speficy a "custom" id field with different capitalisation in the schema, e.g. "Id" or "ID"
      // This would then render this function invisible when accessing through the derived type-safe Object.
      // To fix, we should ensure all genuine attempts to get the fixed id field of objects use the method Id() instead. Remove this when we can.
      jude_id_t GetId() const { return Id();}
      bool IsIdSet() const { return IsIdAssigned(); }
      
      template<typename T>
      std::optional<T> GetFieldAsNumber(const std::string& name, jude_size_t arrayIndex = 0) const
      {
         if (auto field = jude_rtti_find_field(m_object->__rtti, name.c_str()))
         {
            return GetFieldAsNumber<T>(field->index, arrayIndex);
         }
         return std::nullopt;
      }   

      bool IsFieldChanged(const std::string& name) const 
      { 
         if (auto field = jude_rtti_find_field(m_object->__rtti, name.c_str()))
         {
            return IsFieldChanged(field->index);
         }
         return false;
      }

      ///////////////////////////////////////////////////////////////////////////////
   };
}

