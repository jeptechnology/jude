
class EnumPrinter:

   enum_object_template = '''/* Autogenerated Code - do not edit directly */
#pragma once

#include <stdint.h>
#include <jude/core/c/jude_enum.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t %ENUM%_t;
extern const jude_enum_map_t %ENUM%_enum_map[];

#if defined(__cplusplus)
}

namespace jude
{
   namespace %ENUM%
   {
      enum Value
      {
%VALUES%,
         COUNT,

         ////////////////////////////////////////////////////////
         // Protobuf backwards compatibility
%PROTOBUF_VALUES%,
         ////////////////////////////////////////////////////////

         __INVALID_VALUE = COUNT
      };

      const char*  GetString(Value value);
      const char*  GetDescription(Value value);
      const Value* FindValue(const char* name);
            Value  GetValue(const char* name);

      // Protobuf backwards compatibility
      static auto AsText(Value value) { return GetString(value); };
   };
}

////////////////////////////////////////////////////////
// Protobuf backwards compatibility
using %ENUM%Enum = jude::%ENUM%::Value;
%USING_VALUES%
static constexpr %ENUM%Enum  %ENUM%_COUNT = jude::%ENUM%::Value::COUNT;
////////////////////////////////////////////////////////

#endif // __cplusplus

'''

   enum_source_template = '''
#include "%ENUM%.h"

extern "C" const jude_enum_map_t %ENUM%_enum_map[] = 
{
%VALUES%,
   JUDE_ENUM_MAP_END
};

namespace jude
{

   constexpr jude_size_t %ENUM%_COUNT = (jude_size_t)(sizeof(%ENUM%_enum_map) / sizeof(%ENUM%_enum_map[0]));

   const char* %ENUM%::GetString(%ENUM%::Value value)
   {
      return jude_enum_find_string(%ENUM%_enum_map, value);
   }

   const char* %ENUM%::GetDescription(%ENUM%::Value value)
   {
      return jude_enum_find_description(%ENUM%_enum_map, value);
   }

   const %ENUM%::Value* %ENUM%::FindValue(const char* name)
   {
      return (const %ENUM%::Value*)jude_enum_find_value(%ENUM%_enum_map, name);
   }

   %ENUM%::Value %ENUM%::GetValue(const char* name)
   {
      return (%ENUM%::Value)jude_enum_get_value(%ENUM%_enum_map, name);
   }

}

'''

   def __init__(self, importPrefix, name, enum_def):
      
      print("Parsing enum: ", name, "...")

      self.name = name
      self.importPrefix = importPrefix 
      self.elements = []

      for label, data in enum_def.items():

         value = 0
         description = ''
         if isinstance(data,dict):
            if not data.__contains__('value'):
               raise SyntaxError("enum value defined as dictionary but no 'value' given: " + data)
            value = (int(data['value']))
            if data.__contains__('description'):
               description = data['description']
         elif isinstance(data,int):
            value = data
         else:
            raise SyntaxError("enum element not defined as dictionary or int: " + value)
         
         # Tidy up label for code
         varname = label
         if not varname[0].isalpha():
            varname = "_" + varname
         varname = varname.replace("-", "_")

         self.elements.append((varname, label, value, description))
        
   def create_object(self):
      c_values = ',\n'.join(["         %s = %d" % (w, y) for (w,x,y,z) in self.elements])
      protobuf_values = ',\n'.join(["         %s_%s = %s" % (self.name, w, w) for (w,x,y,z) in self.elements])
      using_values = '\n'.join(["static constexpr jude::%s::Value %s_%s = jude::%s::%s;" % (self.name, self.name, w, self.name, w) for (w,x,y,z) in self.elements])
      return self.enum_object_template.replace("%VALUES%", str(c_values)) \
                                      .replace("%USING_VALUES%", str(using_values)) \
                                      .replace("%PROTOBUF_VALUES%", str(protobuf_values)) \
                                      .replace("%ENUM%", str(self.name)) \
                                      .replace("%FILE%", str(self.name).upper())
       
   def create_source(self):
      values = ',\n'.join(['   JUDE_ENUM_MAP_ENTRY(%s, %s, "%s")' % (x,y,z) for (w,x,y,z) in self.elements])
      return self.enum_source_template.replace("%VALUES%", str(values)) \
                                      .replace("%ENUM%", str(self.name)) \
                                      .replace("%FILE%", str(self.name).upper())
