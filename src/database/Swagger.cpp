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
#include <sstream>
#include <set>
#include <climits>
#include <jude/database/Swagger.h>

namespace jude
{
   namespace swagger
   {
      const char* HeaderTemplate = R"(openapi: 3.0.0

info:
  description: This describes the REST API
  version: '1.0.0'
  title: %s API

servers:
  - description: Local API
    url: http://{host}:{port}/data/{version}
    variables:
      host:
        default: '192.168.0.123' # This should be set to your hub's local IP
      port:
        enum:
          - '80'
          - '8080'
          - '443'
          - '8443'
        default: '80'
      version: 
        enum:
          - v1
          - v2
        default: v2
)";

      const char* ComponentsTemplate = R"(
components:
  
  responses:
    400BadRequest:
      description: Bad Request
      content:
        application/json:
          schema:
            type: object
            properties:
              Error:
                  type: string
  
  parameters :
    idParam:
      in: path
      name: id
      schema:
        type: integer
      required : true
      description : Numeric ID of the resource
)";

      const char* PostTemplate = R"(
    post:
      summary: Create a new entry in the %s collection
      tags:
        - %s
      requestBody:
        description: Object to add to collection
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/%s_Schema'
      responses:
        '201':
           description: %s created
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
)";

      const char* GetAllTemplate = R"(
    get:
      summary: Get all entries in the %s collection
      tags:
        - %s
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 type: array
                 items:
                   $ref: '#/components/schemas/%s_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found
)";

       const char* GetWithIdTemplate = R"(
    get:
      summary: Get entry in the %s collection with given id
      tags:
        - %s
      parameters:
        - $ref: '#/components/parameters/idParam'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/%s_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found
)";

       const char* PatchWithIdTemplate = R"(
    patch:
      summary: Update the entry in the %s collection with given id
      tags:
        - %s
      parameters:
        - $ref: '#/components/parameters/idParam'
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/%s_Schema'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/%s_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found
)";

       const char* PutWithIdTemplate = R"(
    put:
      summary: Replace the entry in the %s collection with given id
      tags:
        - %s
      parameters:
        - $ref: '#/components/parameters/idParam'
      requestBody:
        description: JSON object representing fields you wish to replace with
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/%s_Schema'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/%s_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found
)";

       const char* DeleteWithIdTemplate = R"(
    delete:
      summary: Delete the entry in the %s collection with given id
      tags:
        - %s
      parameters:
        - $ref: '#/components/parameters/idParam'
      responses:
        '204':
           description: Object deleted
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found
)";

       const char* PatchActionWithIdTemplate = R"(
    patch:
      summary: Invoke the action %s() on the %s resource
      tags:
        - %s
      parameters:
        - $ref: '#/components/parameters/idParam'
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: %s
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/%s_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found
)";

       const char* GetTemplate = R"(
    get:
      summary: Get the %s resource data
      tags:
        - %s
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/%s_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found
)";

       const char* PatchTemplate = R"(
    patch:
      summary: Update the data in the %s resource 
      tags:
        - %s
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/%s_Schema'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/%s_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found
)";

       const char* PutTemplate = R"(
    put:
      summary: Replace all the data in the %s resource 
      tags:
        - %s
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/%s_Schema'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/%s_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found
)";

       const char* PatchActionTemplate = R"(
    patch:
      summary: Invoke the action %s() on the %s resource
      tags:
        - %s
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: %s
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/%s_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found
)";

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
         SchemaContext(std::ostream& s) : stream(s) {}

         std::ostream& stream;
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
         std::stringstream output;
         SchemaContext context(output);
         context.prefix = "              ";
         context.userLevel = userLevel;

         output << "\n";
         OutputSchema(context, &field);

         return output.str();
      }

      void OutputArraySchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream << context.prefix << "type: array\n";
         context.stream << context.prefix << "maxItems: " << field->array_size << "\n";
         context.stream << context.prefix << "items:\n";

         SchemaContext newContext = context;
         newContext.prefix = context.prefix + std::string("  ");

         OutputSchema(newContext, field);
      }

      void OutputObjectSchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream << context.prefix << "$ref: '#/components/schemas/" << field->details.sub_rtti->name << "_Schema'\n";
      }

      void OutputStringSchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream << context.prefix << "type: string\n";
         context.stream << context.prefix << "maxLength: " << (field->data_size - 1) << "\n";
      }

      void OutputBytesSchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream << context.prefix << "type: string\n";
         context.stream << context.prefix << "maxLength: " << (((field->data_size + 2) / 3) * 4) << "\n";
      }

      void OutputUnsignedSchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream << context.prefix << "type: integer\n";
         context.stream << context.prefix << "minimum: "  << GetMinimum(*field) << "\n";
         auto maxValue = GetMaximum(*field);
         // Only set a max if we are restricted by 8 or 16 bits (i.e. 32, 64 bits => no limit)
         if (maxValue > 0)
         {
            context.stream << context.prefix << "maximum: " << maxValue << "\n";
         }
      }

      void OutputSignedSchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream << context.prefix << "type: integer\n";

         auto minValue = GetMinimum(*field);
         auto maxValue = GetMaximum(*field);

         // Only set a min if we are restricted by 8 or 16 bits (i.e. 32, 64 bits => no limit)
         if (minValue != 0)
         {
            context.stream << context.prefix << "minimum: " << minValue << "\n";
         }

         // Only set a max if we are restricted by 8 or 16 bits (i.e. 32, 64 bits => no limit)
         if (maxValue != 0)
         {
            context.stream << context.prefix << "maximum: " << maxValue << "\n";
         }
      }

      void OutputBoolSchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream << context.prefix << "type: boolean\n";
      }

      void OutputFloatSchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream << context.prefix << "type: number\n";
      }

      void OutputBitmaskSchema(SchemaContext& context, const jude_field_t* field)
      {
         context.stream << context.prefix << "type: object\n";
         context.stream << context.prefix << "properties:\n";
        
         const jude_enum_map_t* enum_map = field->details.enum_map;

         while (enum_map && enum_map->name)
         {
            context.stream << context.prefix << "  " << enum_map->name << ":\n";
            context.stream << context.prefix << "    type: boolean\n";
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

         context.stream << context.prefix << "type: string\n";
         context.stream << context.prefix << "enum:\n";

         const jude_enum_map_t* enum_map = field->details.enum_map;

         while (enum_map && enum_map->name)
         {
            context.stream << context.prefix << "- '" << enum_map->name << "'\n";
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
         context.stream << context.prefix << "allOf: [\n";
         context.stream << context.prefix << "  { $ref: '#/components/schemas/" << field->details.sub_rtti->name << "_Schema' }";

         if (!IsWritable(*field, userLevel))
         {
            context.stream << ",\n";
            context.stream << context.prefix << "  { readOnly: true }";
         }
         
         if (!IsReadable(*field, userLevel))
         {
            context.stream << ",\n";
            context.stream << context.prefix << "  { writeOnly: true }";
         }
         
         if (field->description && field->description[0] != '\0')
         {
            context.stream << ",\n";
            context.stream << context.prefix << "  { description: " << field->description << " }";
         }

         context.stream << "\n";
         context.stream << context.prefix << "]\n";
      }

      void GenerateSchema(std::ostream& output, const jude_rtti_t& rtti, jude_user_t userLevel)
      {
         SchemaContext context(output);
         context.prefix = "          ";
         context.userLevel = userLevel;

         output << "\n    " << rtti.name << "_Schema:\n";
         output << "      type: object\n";

         bool propertiesHasBeenOutput = false;

         for (jude_size_t index = 0; index < rtti.field_count; index++)
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
               output << "      properties:\n";
               propertiesHasBeenOutput = true;
            }

            output << "        " << field->label << ":\n";
            
            if (jude_field_is_object(field) && !jude_field_is_array(field))
            {
               GenerateObjectFieldSchema(context, field, userLevel);
               continue;
            }

            if (!IsWritable(*field, userLevel))
            {
               output << "          readOnly: true\n";
            }
            
            if (!IsReadable(*field, userLevel))
            {
               output << "          writeOnly: true\n";
            }
            
            if (field->description && field->description[0] != '\0')
            {
               output << "          description: " << field->description << "\n";
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

      void RecursivelyOutputSchemas(std::ostream& output, std::set<const jude_rtti_t*>& schemas, const jude_rtti_t* rtti, jude_user_t userLevel)
      {
         for (const auto& s : schemas)
         {
            if (s == rtti)
            {
               return; // already done
            }
         }

         schemas.insert(rtti);

         for (jude_size_t index = 0; index < rtti->field_count; index++)
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
