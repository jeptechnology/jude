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

#include <string>
#include <set>
#include <climits>
#include <jude/database/JsonSchema.h>

namespace jude
{
   namespace jsonschema
   {
      static bool IsReadable(const jude_field_t& field, jude_user_t userLevel)
      {
         return field.permissions.read <= userLevel;
      }

      static bool IsWritable(const jude_field_t& field, jude_user_t userLevel)
      {
         return field.permissions.write <= userLevel;
      }

      static bool IsReadableOrWritable(const jude_field_t& field, jude_user_t userLevel)
      {
         return IsReadable(field, userLevel) || IsWritable(field, userLevel);
      }

      struct SchemaContext
      {
         SchemaContext(OutputStreamInterface& s) : stream(s) {}

         OutputStreamInterface& stream;
         std::string prefix;
         jude_user_t userLevel;
      };

      int64_t GetMinimum(const jude_field_t& field)
      {         
         if (field.min != LLONG_MIN)
         {
            // specified in yaml schema
            return field.min;
         }

         // Try to automate for smaller bit-width fields
         if (field.type == JUDE_TYPE_SIGNED)
         {
            switch (field.data_size)
            {
            case 1: return INT8_MIN;
            case 2: return INT16_MIN;
            }
         }

         return 0;
      }

      int64_t GetMaximum(const jude_field_t& field)
      {         
         if (field.max != LLONG_MAX)
         {
            // specified in yaml schema
            return field.max;
         }

         // Try to automate for smaller bit-width fields
         switch (field.type)
         {
            case JUDE_TYPE_SIGNED:
               switch (field.data_size)
               {
               case 1: return INT8_MAX;
               case 2: return INT16_MAX;
               }
               break;

            case JUDE_TYPE_UNSIGNED:
               switch (field.data_size)
               {
               case 1: return UINT8_MAX;
               case 2: return UINT16_MAX;
               }
               break;
         }
         return 0;
      }

      void OutputSchema(SchemaContext& context, const jude_field_t* field);

      std::string GetSchemaForActionField(const jude_field_t& field, jude_user_t userLevel)
      {
         StringOutputStream output;
         SchemaContext context(output);
         context.prefix = "              ";
         context.userLevel = userLevel;

         output.Print("\n");
         OutputSchema(context, &field);

         return output.GetString();
      }

      void OutputArraySchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream.Printf(64, "%s" "type: array\n", context.prefix.c_str());
         context.stream.Printf(64, "%s" "maxItems: %d\n", context.prefix.c_str(), field->array_size);
         context.stream.Printf(64, "%s" "items:\n", context.prefix.c_str());

         SchemaContext newContext = context;
         newContext.prefix = context.prefix + std::string("  ");

         OutputSchema(newContext, field);
      }

      void OutputObjectSchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream.Printf(256, "%s" "$ref: '#/components/schemas/%s_Schema'\n", 
            context.prefix.c_str(), 
            field->details.sub_rtti->name);
      }

      void OutputStringSchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream.Printf(64, "%s" "type: string\n", context.prefix.c_str());
         context.stream.Printf(64, "%s" "maxLength: %d\n", context.prefix.c_str(), field->data_size - 1);
      }

      void OutputBytesSchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream.Printf(64, "%s" "type: string\n", context.prefix.c_str());
         context.stream.Printf(64, "%s" "maxLength: %d\n", context.prefix.c_str(), ((field->data_size + 2) / 3) * 4);
      }

      void OutputUnsignedSchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream.Printf(64, "%s" "type: integer\n", context.prefix.c_str());
         context.stream.Printf(64, "%s" "minimum: %" PRId64 "\n", context.prefix.c_str(), GetMinimum(*field));
         auto maxValue = GetMaximum(*field);
         // Only set a max if we are restricted by 8 or 16 bits (i.e. 32, 64 bits => no limit)
         if (maxValue > 0)
         {
            context.stream.Printf(64, "%s" "maximum: %" PRId64 "\n", context.prefix.c_str(), maxValue);
         }
      }

      void OutputSignedSchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream.Printf(64, "%s" "type: integer\n", context.prefix.c_str());

         auto minValue = GetMinimum(*field);
         auto maxValue = GetMaximum(*field);

         // Only set a min if we are restricted by 8 or 16 bits (i.e. 32, 64 bits => no limit)
         if (minValue != 0)
         {
            context.stream.Printf(64, "%s" "minimum: %" PRId64 "\n", context.prefix.c_str(), minValue);
         }

         // Only set a max if we are restricted by 8 or 16 bits (i.e. 32, 64 bits => no limit)
         if (maxValue != 0)
         {
            context.stream.Printf(64, "%s" "maximum: %" PRId64 "\n", context.prefix.c_str(), maxValue);
         }
      }

      void OutputBoolSchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream.Printf(64, "%s" "type: boolean\n", context.prefix.c_str());
      }

      void OutputFloatSchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream.Printf(64, "%s" "type: number\n", context.prefix.c_str());
      }

      void OutputBitmaskSchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream.Printf(64, "%s" "type: object\n", context.prefix.c_str());
         context.stream.Printf(64, "%s" "properties:\n", context.prefix.c_str());
        
         const jude_enum_map_t* enum_map = field->details.enum_map;

         while (enum_map && enum_map->name)
         {
            context.stream.Printf(64, "%s" "  %s:\n", context.prefix.c_str(), enum_map->name);
            context.stream.Printf(64, "%s" "    type: boolean\n", context.prefix.c_str());
            enum_map++;
         }
      }

      void OutputEnumSchema(SchemaContext& context, const jude_field_t* field)
      {
         if (field->type == JUDE_TYPE_BITMASK)
         {
            OutputBitmaskSchema(context, field);
            return;
         }

         context.stream.Printf(64, "%s" "type: string\n", context.prefix.c_str());
         context.stream.Printf(64, "%s" "enum:\n", context.prefix.c_str());

         const jude_enum_map_t* enum_map = field->details.enum_map;

         while (enum_map && enum_map->name)
         {
            context.stream.Printf(64, "%s" "- '%s'\n", context.prefix.c_str(), enum_map->name);
            enum_map++;
         }
      }

      void OutputSchema(SchemaContext& context, const jude_field_t* field)
      {
         switch (field->type)
         {
         case JUDE_TYPE_STRING:
            OutputStringSchema(context, field);
            break;

         case JUDE_TYPE_BYTES:
            OutputBytesSchema(context, field);
            break;

         case JUDE_TYPE_UNSIGNED:
            OutputUnsignedSchema(context, field);
            break;

         case JUDE_TYPE_FLOAT:
            OutputFloatSchema(context, field);
            break;

         case JUDE_TYPE_SIGNED:
            OutputSignedSchema(context, field);
            break;

         case JUDE_TYPE_BOOL:
            OutputBoolSchema(context, field);
            break;

         case JUDE_TYPE_ENUM:
            OutputEnumSchema(context, field);
            break;

         case JUDE_TYPE_BITMASK:
            OutputBitmaskSchema(context, field);
            break;

         case JUDE_TYPE_OBJECT:
            OutputObjectSchema(context, field);
            break;
         default:
            break; // do nothing
         }
      }

      void GenerateObjectFieldSchema(SchemaContext& context, const jude_field_t* field, jude_user_t userLevel)
      {
         context.stream.Printf(64,  "%sallOf: [\n", context.prefix.c_str());
         context.stream.Printf(256, "%s  { $ref: '#/components/schemas/%s_Schema' }", 
            context.prefix.c_str(), 
            field->details.sub_rtti->name);

         if (!IsWritable(*field, userLevel))
         {
            context.stream.Printf(64, ",\n%s  { readOnly: true }", context.prefix.c_str());
         }
         
         if (!IsReadable(*field, userLevel))
         {
            context.stream.Printf(64, ",\n%s  { writeOnly: true }", context.prefix.c_str());
         }
         
         if (field->description && field->description[0] != '\0')
         {
            context.stream.Printf(256, ",\n%s  { description: %s ", context.prefix.c_str(), field->description);
            context.stream.Print("}");
         }

         context.stream.Printf(64, "\n%s]\n", context.prefix.c_str());
      }

      void GenerateSchema(OutputStreamInterface& output, const jude_rtti_t& rtti, jude_user_t userLevel)
      {
         SchemaContext context(output);
         context.prefix = "          ";
         context.userLevel = userLevel;

         output.Printf(64, "\n    %s_Schema:\n", rtti.name);
         output.Printf(64, "      type: object\n");

         bool propertiesHasBeenOutput = false;

         for (int index = 0; index < rtti.field_count; index++)
         {
            auto field = &rtti.field_list[index];
            
            if (!IsReadableOrWritable(*field, userLevel))
            {
               continue; // not visible in any way
            }

            if (field->is_action)
            {
               continue; // don't specify actions as part of higher level Schema
            }

            if (!propertiesHasBeenOutput)
            {
               output.Printf(64, "      properties:\n");
               propertiesHasBeenOutput = true;
            }

            output.Printf(64, "        %s:\n", field->label);
            
            if (jude_field_is_object(field) && !jude_field_is_array(field))
            {
               GenerateObjectFieldSchema(context, field, userLevel);
               continue;
            }

            if (!IsWritable(*field, userLevel))
            {
               output.Printf(64, "          readOnly: true\n");
            }
            
            if (!IsReadable(*field, userLevel))
            {
               output.Printf(64, "          writeOnly: true\n");
            }
            
            if (field->description && field->description[0] != '\0')
            {
               output.Printf(256, "          description: %s", field->description);
               output.Print("\n");
            }

            if (jude_field_is_array(field))
            {
               OutputArraySchema(context, field);
            }
            else
            {
               OutputSchema(context, field);
            }
         }
      }

      void RecursivelyOutputSchemas(OutputStreamInterface& output, std::set<const jude_rtti_t*>& schemas, const jude_rtti_t* rtti, jude_user_t userLevel)
      {
         for (const auto& s : schemas)
         {
            if (s == rtti)
            {
               return; // already done
            }
         }

         schemas.insert(rtti);

         for (int index = 0; index < rtti->field_count; index++)
         {
            if (jude_field_is_object(&rtti->field_list[index]))
            {
               RecursivelyOutputSchemas(output, schemas, rtti->field_list[index].details.sub_rtti, userLevel);
            }
         }

         GenerateSchema(output, *rtti, userLevel);
      }
   }
}
