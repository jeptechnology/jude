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

#include <inttypes.h>

#include <jude/jude_core.h>

#define MaxDepth 16
#define TemporaryBufferSizeForNumbers 16

typedef struct
{
   jude_ostream_t *stream;
   jude_user_t access_level;
   jude_filter_t read_access_filter;
   jude_filter_t write_access_filter;
   int count;
} jude_json_schema_generation_context_t;

static bool check_field_access(const jude_filter_t *filter, jude_size_t fieldIndex)
{
   // check if a filter is in place...
   if (filter == NULL)
      return true;

   // check if this field is in the filter bitmask
   return jude_filter_is_touched(filter->mask, fieldIndex);
}

static void jude_json_schema_updateAccessDetails(jude_json_schema_generation_context_t *details, const jude_rtti_t *type)
{
   jude_filter_clear_all(&details->read_access_filter);
   jude_filter_clear_all(&details->write_access_filter);

   for (int fieldIndex = 0; fieldIndex < type->field_count; fieldIndex++)
   {
      bool canRead  = details->access_level >= type->field_list[fieldIndex].permissions.read;
      jude_filter_set_changed(details->read_access_filter.mask, fieldIndex, canRead);
      jude_filter_set_touched(details->read_access_filter.mask, fieldIndex, canRead);

      bool canWrite = details->access_level >= type->field_list[fieldIndex].permissions.write;
      jude_filter_set_changed(details->write_access_filter.mask, fieldIndex, canWrite);
      jude_filter_set_touched(details->write_access_filter.mask, fieldIndex, canWrite);
   }
}

/*
 * To prevent warnings:
 */
static bool json_write(jude_ostream_t *stream, const char *buf)
{
   return jude_ostream_write(stream, (const uint8_t *) buf, strlen(buf));
}

static bool jude_json_schema_write_string_value(jude_ostream_t *stream, const char *name, const char *value)
{
   return   jude_ostream_write_json_tag(stream, name)
         && jude_ostream_write_json_string(stream, value, strlen(value));
}

static bool jude_json_schema_message_start(jude_ostream_t *stream)
{
   // This function writes the following to the stream:
   //   "type": "object",
   //   "properties": {

   return   jude_json_schema_write_string_value(stream, "type", "object")
         && json_write(stream, ",") 
         && jude_ostream_write_json_tag(stream, "properties")
         && json_write(stream, "{");
}

static bool jude_json_schema_array_start(jude_ostream_t *stream, const jude_field_t *field)
{
   // This function writes the following to the stream:
   //   "type": "array",
   //   "maxItems": <max_count>,
   //   "items": {

   return jude_json_schema_write_string_value(stream, "type", "array")
         && json_write(stream, ",") && jude_ostream_write_json_tag(stream, "maxItems")
         && jude_ostream_printf(stream, 64, "%" PRIu32, field->array_size)
         && json_write(stream, ",") && jude_ostream_write_json_tag(stream, "items")
         && json_write(stream, "{");
}

static bool jude_json_schema_array_end(jude_ostream_t *stream, const jude_field_t *field)
{
   return json_write(stream, "}");
}

static bool jude_json_schema_message_end(jude_ostream_t *stream)
{
   return json_write(stream, "}}");
}

static bool jude_json_schema_for_string(jude_ostream_t *stream,  const jude_field_t *field)
{
   int maxLength = field->data_size - 1; // we need to ensure space for null terminator
   return 0 < jude_ostream_printf(stream, 64, "\"type\":\"string\",\"maxLength\":%" PRId32, maxLength);
}

static bool jude_json_schema_for_bytes(jude_ostream_t *stream,  const jude_field_t *field)
{
   int maxBase64len = ((field->data_size + 2 ) / 3) * 4;
   return 0 < jude_ostream_printf(stream, 64, "\"type\":\"string\",\"maxLength\":%" PRId32, maxBase64len);
}

static bool jude_json_schema_for_unsigned(jude_ostream_t *stream,
      const jude_field_t *field)
{
   uint32_t maxValue = 0;
   switch (field->data_size)
   {
   case 1:
      maxValue = UINT8_MAX;
      break;
   case 2:
      maxValue = UINT16_MAX;
      break;
   }

   if (!json_write(stream, "\"type\":\"integer\",\"minimum\":0"))
   {
      return false;
   }

   // Only set a max if we are restricted by 8 or 16 bits (i.e. 32, 64 bits => no limit)
   if (maxValue > 0)
   {
      return jude_ostream_printf(stream, 64, ",\"maximum\":%" PRIu32, maxValue);
   }

   return true;
}

static bool jude_json_schema_for_signed(jude_ostream_t *stream, const jude_field_t *field)
{
   int32_t minValue = 0;
   int32_t maxValue = 0;
   switch (field->data_size)
   {
   case 1:
      minValue = INT8_MIN;
      maxValue = INT8_MAX;
      break;
   case 2:
      minValue = INT16_MIN;
      maxValue = INT16_MAX;
      break;
   }

   if (!jude_json_schema_write_string_value(stream, "type", "integer"))
      return false;

   // Only set a max if we are restricted by 8 or 16 bits (i.e. 32, 64 bits => no limit)
   if (minValue < 0)
   {
      if (!jude_ostream_printf(stream, 64, ",\"minimum\":%" PRId32, minValue))
      {
         return false;
      }
   }

   // Only set a max if we are restricted by 8 or 16 bits (i.e. 32, 64 bits => no limit)
   if (maxValue > 0)
   {
      if (!jude_ostream_printf(stream, 64, ",\"maximum\":%" PRId32, maxValue))
      {
         return false;
      }
   }

   return true;
}

static bool jude_json_schema_for_bool(jude_ostream_t *stream, const jude_field_t *field)
{
   return jude_json_schema_write_string_value(stream, "type", "boolean");
}

static bool jude_json_schema_for_float(jude_ostream_t *stream, const jude_field_t *field)
{
   return jude_json_schema_write_string_value(stream, "type", "number");
}

static bool jude_json_schema_for_bitmask(jude_ostream_t *stream, const jude_field_t *field)
{
   const jude_enum_map_t *enum_map = field->details.enum_map;
   bool commaIsRequired = false;

   if (!jude_json_schema_message_start(stream))
      return false;

   while (enum_map && enum_map->name)
   {
      if (commaIsRequired && !json_write(stream, ","))
         return false;

      if (!jude_ostream_write_json_tag(stream, enum_map->name) || !json_write(stream, "{")
            || !jude_json_schema_for_bool(stream, field)
            || !json_write(stream, "}"))
      {
         return false;
      }

      commaIsRequired = true;
      enum_map++;
   }

   return jude_json_schema_message_end(stream);
}

static bool jude_json_schema_for_enum(jude_ostream_t *stream, const jude_field_t *field)
{
   const jude_enum_map_t *map = field->details.enum_map;

   if (field->type == JUDE_TYPE_BITMASK)
   {
      return jude_json_schema_for_bitmask(stream, field);
   }

   if (!jude_json_schema_write_string_value(stream, "type", "string"))
      return false;

   if (map)
   {
      // If a map exists then output the start of the map and first entry...
      if (   !json_write(stream, ",") || !jude_ostream_write_json_tag(stream, "enum")
          || !json_write(stream, "[")
          || !jude_ostream_write_json_string(stream, map->name, strlen(map->name)))
         return false;

      // ...and all other entries that exist
      while ((++map)->name)
      {
         if (!json_write(stream, ",")
               || !jude_ostream_write_json_string(stream, map->name, strlen(map->name)))
            return false;
      }

      // and the final closing brace
      return json_write(stream, "]");
   }

   return true;
}

static bool jude_json_schema_for_type(jude_ostream_t *stream,  const jude_field_t *field)
{
   switch (field->type)
   {
   case JUDE_TYPE_STRING:
      return jude_json_schema_for_string(stream, field);
   
   case JUDE_TYPE_BYTES:
      return jude_json_schema_for_bytes(stream, field);

   case JUDE_TYPE_UNSIGNED:
      return jude_json_schema_for_unsigned(stream, field);

   case JUDE_TYPE_FLOAT:
      return jude_json_schema_for_float(stream, field);

   case JUDE_TYPE_SIGNED:
      return jude_json_schema_for_signed(stream, field);

   case JUDE_TYPE_BOOL:
      return jude_json_schema_for_bool(stream, field);

   case JUDE_TYPE_ENUM:
      return jude_json_schema_for_enum(stream, field);

   case JUDE_TYPE_BITMASK:
      return jude_json_schema_for_bitmask(stream, field);

   default:
      return false;
   }
}

static bool jude_json_schema_for_field(const jude_field_t* field, jude_json_schema_generation_context_t *details)
{
   const char *fieldName = field->label;
   bool commaIsRequired = false;
   bool visible = check_field_access(&details->read_access_filter, field->index);

   if (!visible)
   {
      return true;
   }

   {
      int i = 0;
      for (i = 0; i < field->index; i++)
      {
         if (check_field_access(&details->read_access_filter, i))
         {
            commaIsRequired = true;
            break;
         }
      }
   }

   if (commaIsRequired && !json_write(details->stream, ","))
      return false;

   if (!jude_ostream_write_json_tag(details->stream, fieldName)
      || !json_write(details->stream, "{"))
   {
      return false;
   }

   // Do we have write access to the field?
   if (!check_field_access(&details->write_access_filter, field->index))
   {
      if (!json_write(details->stream, "\"readOnly\":true,"))
         return false;
   }

   // If this is an array (repeated field), then start the "Array" schema
   if (   jude_field_is_array(field)
      && !jude_json_schema_array_start(details->stream, field))
      return false;

   // If this is a submessage and we are entering in, start the subschema
   if (jude_field_is_object(field))
   {
      if (  !json_write(details->stream, "\"$ref\":\"#/$defs/")
         || !json_write(details->stream, field->details.sub_rtti->name)
         || !json_write(details->stream, "\""))
      {
         return false;
      }
   }
   else
   {
      if (!jude_json_schema_for_type(details->stream, field))
         return false;
   }

   // If this was an array (repeated field), then end the "Array" schema
   if (jude_field_is_array(field)
      && !jude_json_schema_array_end(details->stream, field))
      return false;

   // Enclose the whole shcema for this element
   return json_write(details->stream, "}");
}


static bool jude_json_schema_visitor(const jude_rtti_t* type, void *user_data)
{
   jude_json_schema_generation_context_t *details =
         (jude_json_schema_generation_context_t *) user_data;

   jude_size_t field_index = 0;

   if (details->count > 0 && !json_write(details->stream, ","))
      return false;

   details->count++;

   if (  !jude_ostream_write_json_tag(details->stream, type->name)
      || !json_write(details->stream, "{"))
      return false;

   if (!jude_json_schema_message_start(details->stream))
      return false;

   jude_json_schema_updateAccessDetails(details, type);

   for (field_index = 0; field_index < type->field_count; field_index++)
   {
      if (!jude_json_schema_for_field(&type->field_list[field_index], details))
      {
         return false;
      }
   }

   if (!json_write(details->stream, "}"))
      return false;

   return true;
}

bool jude_create_default_json_schema(
      jude_ostream_t *stream,
      const jude_rtti_t *type,
      jude_user_t accessLevel)
{
   if (type == NULL)
   {
      return json_write(stream, "{}");
   }

   jude_json_schema_generation_context_t details = { .count = 0 };

   if (  !json_write(stream, "{\"type\":\"object\",\"allOf\":[{\"$ref\":\"#/$defs/")
      || !json_write(stream, type->name)
      || !json_write(stream, "\"}],")
      )
   {
      return false;
   }

   if (!json_write(stream, "\"$defs\":{"))
      return false;

   details.stream = stream;
   details.access_level = accessLevel;

   if (!jude_rtti_visit(type, jude_json_schema_visitor, &details))
   {
      return false;
   }

   if (!json_write(stream, "}}}"));

   return true;
}
