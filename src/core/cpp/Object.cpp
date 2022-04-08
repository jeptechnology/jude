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

#include <stdlib.h>
#include <inttypes.h>

#include <jude/core/cpp/Object.h>
#include <jude/core/cpp/ObjectArray.h>
#include <jude/core/cpp/Stream.h>
#include <jude/restapi/jude_browser.h>

static const jude_field_t null_field_list = {};
static const jude_rtti_t  null_rtti = { "null", &null_field_list, 1, sizeof(jude_object_t) };

namespace
{
   std::vector<std::string> TokenizeBuffer(const char* str, char c = ' ')
   {
      std::vector<std::string> result;

      while (*str)
      {
         const char* begin = str;

         while (*str != c && *str)
         {
            str++;
         }

         result.push_back(std::string(begin, str));

         if (*str)
         {
            str++;
            if (*str == '\0')
            {
               result.push_back("");
            }
         }
      }

      return result;
   }
}

namespace jude
{
   // Used when creating a new root object
   Object::Object(const jude_rtti_t& type, 
                  std::function<void()> onChange,
                  std::function<void()> onSingleRef)
   {
      if (&type != &null_rtti)
      {
         m_sharedRoot = std::make_shared<SharedRootData>();
         m_sharedRoot->object.reset((jude_object_t*)new char[type.data_size]);
         m_sharedRoot->onChange = onChange;
         m_sharedRoot->onSingleRef = onSingleRef;

         memset(m_sharedRoot->object.get(), 0, type.data_size);
         jude_object_set_rtti(m_sharedRoot->object.get(), &type);

         m_object = m_sharedRoot->object.get();
      }
   }

   // Used when creating a sub-object
   Object::Object(Object& relative, jude_object_t &child)
      : m_sharedRoot(relative.m_sharedRoot)
      , m_object(&child)
   {
      jude_assert(m_object >= m_sharedRoot->object.get());
      jude_assert(m_object < m_sharedRoot->object.get() + m_sharedRoot->object->__rtti->data_size);
   }

   Object::Object() noexcept
      : Object(nullptr)
   {}

   Object::Object(std::nullptr_t) noexcept
      : Object(null_rtti, nullptr)
   {
      m_object = nullptr;
   }

   Object::Object(Object& rhs) noexcept
      : m_sharedRoot(rhs.m_sharedRoot)
      , m_object(rhs.m_object)
   {}

   Object::Object(Object&& rhs) noexcept
      : m_sharedRoot(std::move(rhs.m_sharedRoot))
      , m_object(rhs.m_object)
   {}

   Object& Object::operator=(Object& rhs) noexcept
   {
      ReleaseSharedData();
      m_sharedRoot = rhs.m_sharedRoot;
      m_object = rhs.m_object;
      return *this;
   }

   Object& Object::operator=(Object&& rhs) noexcept
   {
      ReleaseSharedData();
      m_sharedRoot = std::move(rhs.m_sharedRoot);
      m_object = rhs.m_object;
      return *this;
   }

   bool Object::IsTypedObject() const
   {
      return m_object->__rtti != &null_rtti;
   }

   Object::~Object()
   {
      ReleaseSharedData();
   }

   void Object::ReleaseSharedData()
   {
      if (  m_sharedRoot 
         && m_sharedRoot->onSingleRef 
         && m_sharedRoot.use_count() == 2)
      {
         // We are about to get down to one remaining instance of the shared root object
         m_sharedRoot->onSingleRef();
      }
   }

   bool Object::operator==(const Object &other) const
   {
      return 0 == jude_object_compare(other.m_object, this->m_object);
   }

   bool Object::operator!=(const Object &other) const
   {
      return !operator==(other);
   }

   Object Object::Parent()
   {
      if (auto parent = jude_object_get_parent(m_object))
      {
         return Object(*this, *parent);
      }
      return nullptr;
   }

   const Object Object::Parent() const
   {
      return const_cast<Object*>(this)->Parent();
   }

   void Object::OnEdited()
   {
      if (Options::NotifyImmediatelyOnChange)
      {
         if (m_sharedRoot->onChange && IsChanged())
         {
            m_sharedRoot->onChange();
         }

         if (auto parent = Parent())
         {
            parent.OnEdited();
         }
      }
   }

   void Object::MarkFieldSet(jude_size_t index, bool isChanged)
   {
      jude_object_mark_field_touched(m_object, index, true);
      jude_object_mark_field_changed(m_object, index, isChanged);
      OnEdited();
   }

   Object& Object::AssignId(jude_id_t id)
   {
      if (!IsIdAssigned() || (id != m_object->m_id))
      {
         m_object->m_id = id;
         MarkFieldSet(JUDE_ID_FIELD_INDEX, true);
      }
      return *this;
   }

   bool Object::IsIdAssigned() const
   {
      return Has(JUDE_ID_FIELD_INDEX);
   }

   std::string Object::GetFieldAsString(jude_size_t fieldIndex, jude_index_t arrayIndex) const
   {
      auto& field = m_object->__rtti->field_list[fieldIndex];
      
      if (jude_field_is_string(&field))
      {
         auto rawptr = jude_object_get_string_field(m_object, fieldIndex, arrayIndex);
         return rawptr ? rawptr : "";
      }

      if (jude_field_is_array(&field))
      {
         std::string path = field.label + std::string("/") + std::to_string(arrayIndex);
         return ToJSON_EmptyOnError(path.c_str());
      }
      return ToJSON_EmptyOnError(field.label);
   }

   const char* Object::FieldName(jude_size_t fieldIndex) const
   {
      if (fieldIndex >= m_object->__rtti->field_count)
      {
         return "#OutOfRange#";
      }

      return m_object->__rtti->field_list[fieldIndex].label;
   }

   int64_t Object::GetFieldValue(jude_size_t fieldIndex, jude_index_t arrayIndex) const
   {
      auto value = jude_object_get_value_in_array(m_object, fieldIndex, arrayIndex);
      if (value != nullptr)
      {
         switch (m_object->__rtti->field_list[fieldIndex].data_size)
         {
         case 1:  return *(const int8_t*)value;
         case 2:  return *(const int16_t*)value;
         case 4:  return *(const int32_t*)value;
         case 8:  return *(const int64_t*)value;
         }
      }

      return 0;
   }

   jude_size_t Object::CountField(jude_size_t fieldIndex) const
   {
      return jude_object_count_field(m_object, fieldIndex);
   }

   void Object::Clear(jude_index_t fieldIndex, jude_index_t arrayIndex)
   {
      if (jude_field_is_array(&m_object->__rtti->field_list[fieldIndex]))
      {
         jude_object_remove_value_from_array(m_object, fieldIndex, arrayIndex);
         OnEdited();
      }
      else
      {
         Clear(fieldIndex);
      }
   }

   void Object::TransferFrom(Object&& rhs)
   {
      jude_object_transfer_all(m_object, rhs.m_object);
   }

   void Object::OverwriteData(const Object& rhs, bool andClearChangeMarkers)
   {
      jude_object_overwrite_data(m_object, rhs.m_object, andClearChangeMarkers);
   }

   // Note returns true if there were changes detected
   bool Object::Patch(const Object& rhs)
   {
      auto result = jude_object_merge_data(m_object, rhs.m_object);
      OnEdited();
      return result;
   }

   bool Object::Put(const Object& rhs)
   {
      auto result = jude_object_copy_data(m_object, rhs.m_object);
      OnEdited();
      return result;
   }

   RestfulResult Object::UpdateFromJson(const std::string& JSON, UnknownFieldHandler handler, bool deltasOnly)
   {
      jude_istream_t stream;
      stream.transport = jude_decode_transport_json;
      
      jude_istream_from_buffer(&stream, reinterpret_cast<const uint8_t*>(JSON.c_str()), JSON.length());

      if (handler)
      {
         stream.state = &handler;
         stream.unknown_field_callback = [](void *user_data, const char *unknownFieldName, const char *fieldData) -> bool {
            auto handler = static_cast<UnknownFieldHandler*>(user_data);
            return (*handler)(unknownFieldName, fieldData);
         };
      }

      auto result = deltasOnly ?
                      jude_restapi_patch(jude_user_Root, m_object, "", &stream) // patch will "merge" from the JSON
                    : jude_restapi_put(jude_user_Root, m_object, "", &stream);  // put will "overwirte" from the JSON

      OnEdited();

      return RestfulResult(result);
   }

   RestfulResult Object::UpdateFromJson(InputStreamInterface& JSON, bool deltasOnly)
   {
      if (deltasOnly)
      {
         return RestPatch("", JSON, jude_user_Root);
      }
      else
      {
         return RestPut("", JSON, jude_user_Root);
      }
   }


   RestfulResult CreateResponse(jude_restapi_code_t statusCode, jude_ostream_t* output = nullptr)
   {
      if (output)
      {
         jude_ostream_flush(output);
         if (output->has_error)
         {
            return RestfulResult(statusCode, jude_ostream_get_error(output));
         }
      }
      return RestfulResult(statusCode);
   }

   RestfulResult CreateResponse(jude_restapi_code_t statusCode, jude_istream_t* input)
   {
      if (input && input->has_error)
      {
         if (jude_restapi_is_successful(statusCode))
         {
            statusCode = jude_rest_Bad_Request;
         }
         return RestfulResult(statusCode, jude_istream_get_error(input));
      }
      return RestfulResult(statusCode);
   }

   static void ReadAccessControlCallback(void* ctx, const jude_object_t* resource, jude_filter_t* filter)
   {
      if (ctx && filter)
      {
         auto accessControl = (const AccessControl*)ctx;
         accessControl->GetReadFilter(resource, *filter);
      }
   }

   std::string Object::ToJSON_WithExtraField(ExtraFieldHandler extraField, jude_user_t userLevel, jude_size_t maxSize) const
   {
      StringOutputStream output(maxSize);
      AccessControl accessControl(userLevel);
      auto outputStream = output.GetLowLevelOutputStream();
      outputStream->read_access_control = ReadAccessControlCallback;
      outputStream->read_access_control_ctx = (void*)&accessControl;
      
      // Extra field handling
      outputStream->extra_output_callback_ctx = &extraField;
      outputStream->extra_output_callback = [] (void *userData, const char **name, const char **data) -> bool {
         auto handler = static_cast<ExtraFieldHandler*>(userData);
         return (*handler)(name, data);
      };

      auto result = CreateResponse(jude_restapi_get(accessControl.GetAccessLevel(), m_object, "", outputStream), outputStream);
      if (result)
      {
         return output.GetString();
      }
      // return error string
      return "#ERROR: " + result.GetDetails();
   }

   std::vector<std::string> Object::SearchForPath(CRUD operationType, const char* pathPrefix, jude_size_t maxPaths, jude_user_t userLevel) const
   {
      std::vector<std::string> paths;

      pathPrefix = pathPrefix ? pathPrefix : "";
      auto tokens = TokenizeBuffer(pathPrefix);

      if (pathPrefix[0] == '\0' 
       || pathPrefix[0] != '/' 
       || tokens.size() == 0
       || tokens.size() >= 3
       )
      {
         return paths;
      }

      const char* pathPrefixWithoutSlash = tokens[0].c_str() + 1;

      std::string pathString(pathPrefix);

      // search path 
      auto permissionRequired = operationType == CRUD::READ ? jude_permission_Read : jude_permission_Write; 
      jude_browser_t browser = jude_browser_try_path(m_object, pathPrefixWithoutSlash, userLevel, permissionRequired);

      if (browser.remaining_suffix == nullptr)
      {
         browser.remaining_suffix = "";
      }

      auto suffixOffset = strlen(browser.remaining_suffix);

      if (!jude_browser_is_valid(&browser))
      {
         // no paths to add
      }
      else if (tokens.size()  == 2)
      {
         // The only way to search for completions on two tokens is an enum field
         if (  jude_browser_is_field(&browser) 
            && browser.x.field.iterator.current_field->type == JUDE_TYPE_ENUM)
         {
            auto& enumPrefix = tokens[1];

            // Now try to complete Enum...
            auto map = browser.x.field.iterator.current_field->details.enum_map;
            while (map->name)
            {
               if (strncmp(map->name, enumPrefix.c_str(), enumPrefix.length()) == 0)
               {
                  paths.push_back(pathString + (map->name + enumPrefix.length()));
               }
               map++;
            }
         }
      }
      else if (jude_browser_is_field(&browser))
      {
         if (browser.remaining_suffix[0] == '\0')
         {
            // with no remaining suffix... just add the prefix
            paths.push_back(pathPrefix);
         }
      }
      else if (  !pathString.empty()
               && pathString.back() != '/'
               && browser.remaining_suffix[0] == '\0')
      {
         // just complete with the slash
         paths.push_back(pathString);
      }
      else if (jude_browser_is_object(&browser))
      {
         // search for matching fields
         auto resourceType = browser.x.object->__rtti;
         for (unsigned index = 0; index < resourceType->field_count; ++index)
         {
            auto field = &resourceType->field_list[index];

            if (permissionRequired == jude_permission_Read && field->permissions.read > userLevel)
            {
               continue;
            }

            if (permissionRequired == jude_permission_Write && field->permissions.write > userLevel)
            {
               continue;
            }

            if (0 == strncmp(field->label, browser.remaining_suffix, suffixOffset))
            {
               std::string suffix = "/";
               suffix += field->label;
               paths.push_back(pathString + (suffix.c_str() + 1 + suffixOffset));
            }
         }
      }
      else // at this point we know we are an array
      {
         std::vector<jude_id_t> pathNumbers;
         if (jude_field_is_object(browser.x.array.current_field))
         {
            // this is an array of sub-resources - list the id's
            Object arrayRoot(*const_cast<Object*>(this), *browser.x.array.object);
            ObjectArray<Object> resourceArray(arrayRoot, browser.x.array.field_index);
            for (auto const& resource : resourceArray)
            {
               pathNumbers.push_back(resource.Id());
            }
         }
         else 
         {
            // this is an array of atomic type - list the indeces
            for (unsigned index = 0; index < jude_iterator_get_count(&browser.x.array); index++)
            {
               pathNumbers.push_back(index);
            }
         }

         char buffer[32];
         for (const auto& number : pathNumbers)
         {
            snprintf(buffer, sizeof(buffer), "%" PRIjudeID, number);
            if (0 == strncmp(buffer, browser.remaining_suffix, suffixOffset))
            {
               paths.push_back(pathString + (buffer + suffixOffset));
            }
         }
      }

      return paths;
   }

   RestfulResult Object::RestGet(const char* fullpath, OutputStreamInterface& output, const AccessControl& accessControl) const
   {
      auto outputStream = output.GetLowLevelOutputStream();
      outputStream->read_access_control = ReadAccessControlCallback;
      outputStream->read_access_control_ctx = (void*)&accessControl;
      return CreateResponse(jude_restapi_get(accessControl.GetAccessLevel(), m_object, fullpath, outputStream), outputStream);
   }

   static void WriteAccessControlCallback(void* ctx, const jude_object_t* resource, jude_filter_t* filter)
   {
      if (ctx && filter)
      {
         auto accessControl = (const AccessControl*)ctx;
         accessControl->GetWriteFilter(resource, *filter);
      }
   }

   RestfulResult Object::RestPost(const char* fullpath, InputStreamInterface& input, const AccessControl& accessControl)
   {
      jude_id_t newlyCreatedId;
      auto inputStream = input.GetLowLevelInputStream();
      inputStream->write_access_control = WriteAccessControlCallback;
      inputStream->write_access_control_ctx = (void*)&accessControl;

      auto statusCode = jude_restapi_post(accessControl.GetAccessLevel(), m_object, fullpath, inputStream, &newlyCreatedId);
      if (jude_restapi_is_successful(statusCode))
      {
         return RestfulResult(newlyCreatedId);
      }
      return CreateResponse(statusCode, inputStream);
   }

   RestfulResult Object::RestPatch(const char* fullpath, InputStreamInterface& input, const AccessControl& accessControl)
   {
      auto inputStream = input.GetLowLevelInputStream();
      inputStream->write_access_control = WriteAccessControlCallback;
      inputStream->write_access_control_ctx = (void*)&accessControl;
      return CreateResponse(jude_restapi_patch(accessControl.GetAccessLevel(), m_object, fullpath, inputStream), inputStream);
   }

   RestfulResult Object::RestPut(const char* fullpath, InputStreamInterface& input, const AccessControl& accessControl)
   {
      auto inputStream = input.GetLowLevelInputStream();
      inputStream->write_access_control = WriteAccessControlCallback;
      inputStream->write_access_control_ctx = (void*)&accessControl;
      return CreateResponse(jude_restapi_put(accessControl.GetAccessLevel(), m_object, fullpath, inputStream), inputStream);
   }

   RestfulResult Object::RestDelete(const char* fullpath, const AccessControl& accessControl)
   {
      return CreateResponse(jude_restapi_delete(accessControl.GetAccessLevel(), m_object, fullpath));
   }

   FieldMask Object::GetChanges() const
   {
      FieldMask filter;
      for (jude_size_t fieldIndex = 0; fieldIndex < Type().field_count; fieldIndex++)
      {
         if (IsChanged(fieldIndex))
         {
            filter.SetChanged(fieldIndex);
         }
      }
      return filter;
   }

   
   std::string Object::DebugInfo(const jude_filter_t *filterOverride) const
   {
      jude_filter_t filter;

      if (filterOverride)
      {
         filter = *filterOverride;
      }
      else
      {
         memcpy(filter.mask, m_object->__mask, sizeof(filter.mask));
      }

      std::string info = "+------------------------------+----------------------+\n"
                         "|        Field Name            |        Value         |\n"
                         "+------------------------------+----------------------+\n";

    
      for (jude_size_t fieldIndex = 0; fieldIndex < Type().field_count; fieldIndex++)
      {
         if (!jude_filter_is_touched(filter.mask, fieldIndex))
         {
            continue;
         }

         auto fieldName = Type().field_list[fieldIndex].label;
         char line[64];
         snprintf(line, 64, "|%28s%c | %-20s |\n",
                  fieldName,
                  jude_filter_is_changed(filter.mask, fieldIndex) ? '*' : ' ',
                  ToJSON(fieldName, 20, jude_user_Root).c_str());
         info += line;
      }

      info += "+------------------------------+----------------------+\n";

      return info;
   }

}

