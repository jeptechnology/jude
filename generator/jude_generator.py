#!/usr/bin/env python3

jude_version = "jude-0.0.1"
try:
    from functools import reduce
except:
    pass

import pprint
import json
import yaml

import time
import os.path

import sys
import re

pp = pprint.PrettyPrinter(indent=2)

# ---------------------------------------------------------------------------
#                Generation of single fields
# ---------------------------------------------------------------------------

from templates.AtomicAccessors import AtomicTemplateMap 
from templates.BitmaskAccessors import BitMaskTemplateMap 
from templates.BytesAccessors import BytesTemplateMap 
from templates.StringAccessors import StringTemplateMap 
from templates.ObjectAccessors import ObjectTemplateMap
from printers.enum_printer import EnumPrinter
from printers.bitmask_printer import BitmaskPrinter

templateMap = {
   'atomic':     AtomicTemplateMap(),
   'bitmask':    BitMaskTemplateMap(),
   'bytes':      BytesTemplateMap(),
   'string':     StringTemplateMap(),
   'subobject':  ObjectTemplateMap()
}

atomic_types = {
   'char'  : { 'ctype' : 'char'    , 'judetype': 'UNSIGNED' },
   'bool'  : { 'ctype' : 'bool'    , 'judetype': 'BOOL' },
   'float' : { 'ctype' : 'float'   , 'judetype': 'FLOAT' },
   'double': { 'ctype' : 'double'  , 'judetype': 'FLOAT' },
   'i8'    : { 'ctype' : 'int8_t'  , 'judetype': 'SIGNED' }, 
   'i16'   : { 'ctype' : 'int16_t' , 'judetype': 'SIGNED' },   
   'i32'   : { 'ctype' : 'int32_t' , 'judetype': 'SIGNED' }, 
   'i64'   : { 'ctype' : 'int64_t' , 'judetype': 'SIGNED' }, 
   'u8'    : { 'ctype' : 'uint8_t' , 'judetype': 'UNSIGNED' }, 
   'u16'   : { 'ctype' : 'uint16_t', 'judetype': 'UNSIGNED' },
   'u32'   : { 'ctype' : 'uint32_t', 'judetype': 'UNSIGNED' },
   'u64'   : { 'ctype' : 'uint64_t', 'judetype': 'UNSIGNED' }, 
   'id'    : { 'ctype' : 'jude_id_t', 'judetype': 'UNSIGNED' }, 
}

hpp_file_template = '''
/*****************************************************************************
 * 
 * Auto-generated file: PLEASE DO NOT MODIFY DIRECTLY
 *
 ****************************************************************************/
#pragma once

#ifndef __cplusplus
#error "This file must only be used by C++ compiler"
#endif /* __cplusplus */

#include <stdint.h>
#include <string>
#include <vector>
#include <optional>

#include <jude/jude.h>
#include <jude/core/cpp/Validatable.h>
%HEADERS%

%CLASS_DECLARATION%

'''

cpp_file_template = '''
/*****************************************************************************
 * 
 * Auto-generated file: PLEASE DO NOT MODIFY DIRECTLY
 *
 ****************************************************************************/
#include <stdint.h>

#include "%FILE%.h"

%CLASS_DEFINITION%

'''

cpp_class_template = '''
namespace jude {

   %CLASS% %CLASS%::Clone() const
   {
      return CloneAs<%CLASS%>();
   }

   %CLASS%::%CLASS%() :
     Object(%OBJECT%_rtti)
   {
      m_pData = (%STRUCT%*)RawData();     
   }  

   %CLASS%::%CLASS%(%CLASS%&& move_ref) :
     Object(std::move(move_ref))
   {
      m_pData = (%STRUCT%*)RawData();     
   }  

   %CLASS%::%CLASS%(%CLASS%& copy_ref) :
     Object(copy_ref)
   {
      m_pData = (%STRUCT%*)RawData();     
   }  

   %CLASS%& %CLASS%::operator= (%CLASS% &rhs)
   {
      Object::operator=(rhs);
      m_pData = (%STRUCT%*)RawData();     
      return *this;
   }

   %CLASS%& %CLASS%::operator= (%CLASS% &&rhs)
   {
      Object::operator=(std::move(rhs));
      m_pData = (%STRUCT%*)RawData();     
      return *this;
   }

   %CLASS%& %CLASS%::operator= (std::nullptr_t)
   {
      m_pData = nullptr;
      return operator=(%CLASS%(nullptr));
   }
   
%ACCESSORS%

}

'''


cpp_class_declarataion_template = '''

namespace jude {

class %CLASS% : public Object
{
   %STRUCT% *m_pData;

   friend class Object;

   %CLASS%(Object& relative, jude_object_t& data) 
      : Object(relative, data)
      , m_pData((%STRUCT% *)&data)
   {}

   %CLASS%(Object& relative, %STRUCT%& data) 
      : %CLASS%(relative, (jude_object_t&)data)
   {}

public:
   /*
   * Attribute Indeces
   */
   class Index {
   public:
%INDEX_ENUM%   
   };

   // [JEP] TODO: Make this private when possible so that we force new objects to be created with factory function New()
   %CLASS%();

   static %CLASS% New() { return %CLASS%(); }

   %CLASS%(std::nullptr_t) : m_pData(nullptr) {}
   %CLASS%(%CLASS%&& move_me); 
   %CLASS%(%CLASS%& copy_me); 
   %CLASS%& operator= (%CLASS% &rhs);
   %CLASS%& operator= (%CLASS% &&rhs);
   %CLASS%& operator= (std::nullptr_t);

   const %CLASS% ConstCopyConstruct(const %CLASS% &rhs);
   
   bool operator== (const Object &rhs) const { return Object::operator==(rhs); }
   bool operator!= (const Object &rhs) const { return !operator==(rhs); }
   
   %CLASS% Clone() const;

   virtual ~%CLASS%() {}

%ACCESSORS%

   const %STRUCT% *TypedRawData() const { return m_pData; }

   static constexpr const jude_rtti_t* RTTI() { return &%OBJECT%_rtti; }; 

   ///////////////////////////////////////////////////////////////////////////////
   // Protobuf backwards compatibility
   auto FormLockGuard() { return *this; }
   ///////////////////////////////////////////////////////////////////////////////
};

}
'''

h_file_template = '''
/*****************************************************************************
 * 
 * Auto-generated file: PLEASE DO NOT MODIFY DIRECTLY
 *
 ****************************************************************************/
#pragma once

#include <jude/jude.h>
%C_TYPE_HEADERS%

#ifdef __cplusplus
extern "C" {
#endif

struct %CLASS%_indeces 
{
%INDEX_NAMES%
};

#ifdef __cplusplus
}
#endif
'''

c_file_template = '''
/*****************************************************************************
 * 
 * Auto-generated file: PLEASE DO NOT MODIFY DIRECTLY
 *
 ****************************************************************************/
#include <string.h>
#include <jude/jude_core.h>

%FIELD_INDICES%

static void init(%STRUCT%* _this)
{
   memset(_this, 0, sizeof(%STRUCT%));
   jude_object_set_rtti((jude_object_t*)_this, &%CLASS%_rtti);
}

static void destroy(%STRUCT%* _this)
{
   // TODO: destroy all subscribers...
}

%IMPLEMENTATIONS%

'''

database_hpp_template = '''
/*****************************************************************************
 * 
 * Auto-generated file: PLEASE DO NOT MODIFY DIRECTLY
 *
 ****************************************************************************/
#pragma once

#ifndef __cplusplus
#error "This file must only be used by C++ compiler"
#endif /* __cplusplus */

#include "jude/jude.h"
#include "jude/database/Database.h"
#include "jude/database/Resource.h"
#include "jude/database/Collection.h"
%HEADERS%

namespace jude {

class %DATABASE% : public jude::Database
{
public:
   %MEMBER_DECL%

   %DATABASE%(
      const std::string& name = "", 
      jude_user_t access = jude_user_Public, 
      std::shared_ptr<jude::Mutex> sharedMutex = std::make_shared<jude::Mutex>())
      : jude::Database(name, access, sharedMutex)
      %MEMBER_INITIALISERS%
   {
      %MEMBER_ACCESS_LEVELS%
   }

   //////////////////////////////////////////////////////////////////////////////
   // Start of Protobuf compatibility layer - we want to remove this eventually
   //////////////////////////////////////////////////////////////////////////////
   class LockGuard
   {
      std::lock_guard<jude::Mutex> m_lock;
      %DATABASE% *m_data;
   
   public:
      LockGuard(%DATABASE%& data, jude::Mutex& mutex) 
         : m_lock(mutex)
         , m_data(&data)
      {}

      %LOCKGUARD_ACCESSORS%

      void RendezvousWithEventManager() { /* TODO */ }
   };

   auto operator ->() { return this; }

   auto FormLockGuard()
   {
      return LockGuard(*this, *m_mutex);
   }
   //////////////////////////////////////////////////////////////////////////////
   // End of Protobuf compatibility layer
   //////////////////////////////////////////////////////////////////////////////

};

} // namespace jude
'''


Globals = {}
Constants = {}

def pluralOf(name):
   if (name[-1:] == "s"):
     return name
   else:
     return name + "s"

class Names:
   '''Keeps a set of nested names and formats them to C identifier.'''
   def __init__(self, parts = ()):
      if isinstance(parts, Names):
         parts = parts.parts
      self.parts = tuple(parts)
   
   def __str__(self):
      return '_'.join(self.parts)

   def __add__(self, other):
      if isinstance(other, (str, unicode)):
         return Names(self.parts + (other,))
      elif isinstance(other, tuple):
         return Names(self.parts + other)
      else:
         raise ValueError("Name parts should be of type str")
   
   def __eq__(self, other):
      return isinstance(other, Names) and self.parts == other.parts
   
def names_from_type_name(type_name):
   '''Parse Names() from FieldDescriptorProto type_name'''
   if type_name[0] != '.':
      raise NotImplementedError("Lookup of non-absolute type names is not supported")
   return Names(type_name[1:].split('.'))

def strip_prefix(prefix, name):
   return name.replace(prefix + '_','',1)

def interpretName(name_text):
   parts = name_text.split('[')
   
   if len(parts) == 1:
      parts = name_text.split('(')
      
      if len(parts) == 1:
         return str(parts[0]), None, False

      if len(parts) > 2 or len(str(parts[1].split(')')[0])):
         raise SyntaxError("Field action name '" + "' should be of the form 'action_name()")

      actionType = True

      return str(parts[0]).strip(), None, actionType

   if len(parts) > 2:
      raise SyntaxError("Field array name '" + "' should be of the form 'array_name[array_length]")
   return str(parts[0]).strip(), str(parts[1].split(']')[0]), False

class Field:
   
   def get(self, name, default):
      return self.field_data.get(name, self._class.get(name, default))


   def calculate_types(self):          
      self.alias     = self.get('alias', None)
      self.max_value = self.get('max', None)
      self.min_value = self.get('min', None)
      self.type      = self.get('type', None)
      self.default   = self.get('default', None)
      self.persist   = self.get('persist', True)

      # permissions
      auth = self.get('auth', 'Public')
      if isinstance(auth, dict):
         self.auth_read = str(auth.get('read', 'Public'))
         self.auth_write = str(auth.get('write', 'Public'))
      else:
         self.auth_read = str(auth)
         self.auth_write = str(auth)

      # Force id's to be writable only by root
      if self.name == 'id':
         self.auth_write = 'Root'

      validAuth = ['Root', 'Admin', 'Public']
      if (self.auth_read not in validAuth):
         raise SyntaxError("Field '" + self.name + "' has invalid read auth '" + self.auth_read + "': needs to be one of " + str(validAuth) )
      if (self.auth_write not in validAuth):
         raise SyntaxError("Field '" + self.name + "' has invalid write auth '" + self.auth_write + "': needs to be one of " + str(validAuth) )

      # An action type has to override some settings 
      if self.isActionType:
         self.auth_read = 'Root'
         self.persist = False         

      if self.type.startswith('string') or self.type.startswith('bytes'):
         parts = self.type.split(':')
         if len(parts) != 2:
            raise SyntaxError("Field type '" + self.type + "' should be of the form 'string:n' where n is a number > 0")
         self.max_size = parts[1]
        
         if parts[0] == 'string':
            self.ctype = "char"
            self.judetype = 'STRING'
         elif parts[0] == 'bytes':
            self.ctype = "JUDE_BYTES_ARRAY_T(%s)" % self.max_size
            self.judetype = 'BYTES'

      else:
         self.max_size = None
         # Decide the C data type to use in the struct.
         if self.type in atomic_types.keys():
            self.ctype = atomic_types[self.type]['ctype']
            self.judetype = atomic_types[self.type]['judetype']
         else:
            self.ctype = self.type + '_t'
            self.subobjname = self.type
            if self.type in self.model['enums'].keys() or self.type in self.model['external']['enums'].keys():
               self.judetype = 'ENUM'
            elif self.type in self.model['bitmasks'].keys() or self.type in self.model['external']['bitmasks'].keys():
               self.judetype = 'BITMASK'
            else:
               self.judetype = 'OBJECT'

      if not self.array_size is None:
         if not self.max_size is None and self.judetype != 'BYTES':
            self.array_decl = '[%s][%s]' % (self.max_size, self.array_size)
         else:   
            self.array_decl = '[%s]' % self.array_size
      elif not self.max_size is None and self.judetype != 'BYTES':
         self.array_decl = '[%s]' % (self.max_size)
      else:   
         self.array_decl = ''

      if self.judetype in ['STRING']:
         self.cpptype = 'std::string'
      elif self.judetype == 'BYTES':
         self.cpptype = 'std::vector<uint8_t>'
      elif self.judetype == 'OBJECT':
         self.cpptype = str(self.ctype)[:-2] + Globals.get('ObjectSuffix', '')
      elif self.judetype == 'BITMASK':
         self.cpptype = str(self.ctype)[:-2]
      elif self.judetype == 'ENUM':
         self.cpptype = str(self.ctype)[:-2] + "::Value"
      else:
         self.cpptype = str(self.ctype)

      # print(self)

   def __init__(self, object_name, name, field_data, model):      

      self.name, self.array_size, self.isActionType = interpretName(name)
      self.judetype = 'NULL'
      self.model = model

      if not self.name.isidentifier():
         raise SyntaxError("FATAL: '" + self.name + "' is not a valid variable name")

      self.object_name = object_name.replace('_', ' ').title().replace(' ','')
      self.struct_name = object_name + '_t'
      self.member = 'm_' + self.name

      if not isinstance(field_data, dict):
         self.field_data = { 'type': field_data }
      else:
         self.field_data = field_data

      classname = self.field_data.get('class', 'default')
      self._class = self.model['classes'].get(classname, self.model['external']['classes'].get(classname, {}))

      self.tag       = self.get('tag', None)
      self.alwaysNotify = self.field_data.get('alwaysNotify', self._class.get('alwaysNotify', None))

   def __cmp__(self, other):
      return cmp(self.name, other.name)
   
   def __str__(self):
      result = ''
      if self.array_size is not None:
         result += '   jude_size_t ' + self.member + '_count;\n'
      result += '   %s %s%s;' % (self.ctype, self.member, self.array_decl)
      return result

   def getTypeAndPlurality(self):
     if self.array_size is not None:
       plurality = 'repeated'
     else:
       plurality = 'single'

     if self.judetype == 'STRING':
       type = 'string'
     elif self.judetype == 'BYTES':
       type = 'bytes'
     elif self.judetype == 'OBJECT':
       type = 'subobject'
     elif self.judetype == 'BITMASK':
       type = 'bitmask'
     else:
       type = 'atomic'

     return (type, plurality)
      
      
   # We need to generate the correct template for the type of attribute
   def getAccessorTemplate(self):
     type, plurality = self.getTypeAndPlurality() 
     return templateMap[type]['cpp']['source'][plurality]

   def getAccessorDeclTemplate(self):
     type, plurality = self.getTypeAndPlurality() 
     return templateMap[type]['cpp']['objects'][plurality]

   def getCallbacksTemplate(self):
     type, plurality = self.getTypeAndPlurality() 
     return templateMap[type]['c']['callbacks'][plurality]

   def getImplementationsTemplate(self):
     type, plurality = self.getTypeAndPlurality() 
     return templateMap[type]['c']['source'][plurality]
         
   def getPrototypesTemplate(self):
     type, plurality = self.getTypeAndPlurality() 
     return templateMap[type]['c']['objects'][plurality]
   
   def tags(self):
      '''Return the #define for the tag number of this field.'''
      identifier = '%s_%s_tag' % (self.object_name, self.name)
      return '#define %-40s %d\n' % (identifier, self.tag)

   def tag_id(self, index):
      '''Return the enum for the tag ID of this field.'''
      identifier = '%s' % (self.name)
      return '     %-25s = %d' % (identifier, self.tag)

   def tag_index(self, index):
      '''Return the enum for the tag ID of this field.'''
      identifier = '%s' % (self.name)
      return '%-25s = %d' % (identifier, index)

   def masks(self):
      '''Return the enum for the tag mask of this field.'''
      identifier = '%s_%s_bit' % (self.object_name, self.name)
      return '   %-40s = %d,\n' % (identifier, (1<<(self.tag - 1)))

   def mask_id(self):
      '''Return the enum for the tag ID of this field.'''
      identifier = '%s_MASK' % (self.title())
      return '     %-25s = %d' % (identifier, (1<<(self.tag - 1)))
   
   def jude_field_t(self, prev_field_name, index):
      '''Return the jude_field_t initializer to use in the constant array.
      prev_field_name is the name of the previous field or None.
      '''

      result =  '   {\n'
      result += '      .label = "%s",\n' % self.name
      result += '      .description = "%s",\n' % self.field_data.get('description','')
      result += '      .tag   = %d,\n' % self.tag
      result += '      .index = %d,\n' % index
      result += '      .type  = JUDE_TYPE_%s,\n' % self.judetype

      if not prev_field_name: 
         result += '      .data_offset = JUDE_DATAOFFSET_FIRST(%s, %s, %s),\n' % (self.struct_name, self.member, "NULL")
      else:
         result += '      .data_offset = JUDE_DATAOFFSET_OTHER(%s, %s, %s),\n' % (self.struct_name, self.member, prev_field_name) 

      if self.array_size:
         result += '      .size_offset = jude_delta(%s, %s_count, %s),\n' % (self.struct_name, self.member, self.member)
         result += '      .data_size   = jude_membersize(%s, %s[0]),\n' % (self.struct_name, self.member)
         result += '      .array_size  = %s,\n' % self.array_size
      else:
         result += '      .data_size   = jude_membersize(%s, %s),\n' % (self.struct_name, self.member)
         result += '      .array_size  = 0,\n'

      result += '      .persist  = %s,\n' % ("true" if self.persist else "false")
      result += '      .always_notify = %s,\n' % ("true" if self.alwaysNotify or self.isActionType else "false")
      result += '      .is_action = %s,\n' % ("true" if self.isActionType else "false")
      result += '      .min = %s,\n' % self.field_data.get('min','LLONG_MIN')
      result += '      .max = %s,\n' % self.field_data.get('max','LLONG_MAX')

      result += '      .permissions = {\n'
      result += '         .read  = jude_user_%s,\n' % (self.auth_read)
      result += '         .write = jude_user_%s\n' % (self.auth_write)
      result += '      },\n'


      if self.judetype == 'OBJECT':
         result += '      .details = { &%s_rtti }\n' % self.subobjname
      elif self.judetype == 'ENUM':
         result += '      .details = { &%s_enum_map }\n' % str(self.ctype)[:-2]
      elif self.judetype == 'BITMASK':
         result += '      .details = { &%s_bitmask_map }\n' % str(self.ctype)[:-2]
      else:
         result += '      .details = { NULL }\n'
      
      result += '   }'

      return result
   
   def title(self):
      name = str(self.name)

      if Globals.get('legacy', False):
         return name[:1].upper() + name[1:];
      else:
         return name

   def accessor(self):
      accessorTemplate = self.getAccessorTemplate()

      # legacy code requires this change
      if Globals.get('legacy', False):
         accessorTemplate = accessorTemplate.replace('Has_%NAME%', 'Is%NAME%Set') \
                                            .replace('Clear_%', 'Clear%') \
                                            .replace('Set_%', 'Set%') \
                                            .replace('Add_%', 'Add%') \
                                            .replace('Get_%NAME%_or', 'Get%NAME%Or') \
                                            .replace('Get_%', 'Get%') \
                                            .replace('Find_%', 'Find%') \

      return accessorTemplate \
                  .replace('%NAME%', str(self.title())) \
                  .replace('%PLURAL_NAME%', pluralOf(self.title())) \
                  .replace('%TYPE%', self.cpptype) \
                  .replace('%MEMBER%', str(self.member)) \
                  .replace('%OBJECT%', str(self.object_name)) \
                  .replace('%ID%', 'ID::%s' % (self.title())) \
                  .replace('%INDEX%', 'Index::%s' % (self.name)) \
                  
   def accessorDecl(self):
      accessorTemplate = self.getAccessorDeclTemplate()

      # legacy code requires this change
      if Globals.get('legacy', False):
         accessorTemplate = accessorTemplate.replace('Has_%NAME%', 'Is%NAME%Set') \
                                            .replace('Clear_%', 'Clear%') \
                                            .replace('Set_%', 'Set%') \
                                            .replace('Add_%', 'Add%') \
                                            .replace('Get_%NAME%_or', 'Get%NAME%Or') \
                                            .replace('Get_%', 'Get%') \
                                            .replace('Find_%', 'Find%') \

      
      return accessorTemplate \
                  .replace('%NAME%', str(self.title())) \
                  .replace('%PLURAL_NAME%', pluralOf(self.title())) \
                  .replace('%TYPE%', self.cpptype) \
                  .replace('%MEMBER%', str(self.member)) \
                  .replace('%OBJECT%', str(self.object_name)) \
                  .replace('%ID%', 'ID::%s' % (self.title())) \
                  .replace('%INDEX%', 'Index::%s' % (self.name)) \
   
   def c_callbacks(self):
      template = self.getCallbacksTemplate()
      type = str(self.ctype)
      return template \
                  .replace('%NAME%', str(self.title())) \
                  .replace('%PLURAL_NAME%', pluralOf(self.title())) \
                  .replace('%TYPE%', type) \
                  
   def c_implementations(self):
      template = self.getImplementationsTemplate()
      type = str(self.ctype)
      return template \
                  .replace('%NAME%', str(self.title())) \
                  .replace('%PLURAL_NAME%', pluralOf(self.title())) \
                  .replace('%TYPE%', type) \
                  .replace('%MEMBER%', str(self.member)) \
                  .replace('%STRUCT%', str(self.struct_name)) \
                  .replace('%INDEX%', "Index_Of_%s" % (self.name)) \
                  
   def c_prototypes(self):
      template = self.getPrototypesTemplate()

      type = str(self.ctype)

      return template \
                  .replace('%NAME%', str(self.title())) \
                  .replace('%PLURAL_NAME%', pluralOf(self.title())) \
                  .replace('%TYPE%', type) \
                  .replace('%MEMBER%', str(self.member)) \
                  .replace('%STRUCT%', str(self.struct_name)) \

# ---------------------------------------------------------------------------
#               Generation of objects (structures)
# ---------------------------------------------------------------------------

class Object:

   def calculate_types(self):
      for field in self.fields_and_id:
         field.calculate_types()         

   def __init__(self, importPrefix, name, schema, model):
      self.name = name
      self.importPrefix = importPrefix
      self.struct_name = self.name + '_t'
      self.fields = []
      self.model = model

      if schema is None:
         schema = {} # default schema is empty

      if not isinstance(schema, dict):
         raise SyntaxError("Object definition should be a dictionary: " + str(schema))

      id_field = None

      for f in schema:
         field = Field(name, f, schema[f], model)
         if field.name == "id":
            id_field = field
            id_field.tag = 1000
         else:
            self.fields.append(field)

      # sort out tags - note that "id" is always tag 1000
      unused_tags = set(range(2,len(self.fields) + 2))
      used_tags = set()
      used_tags.add(1000)
      for f in self.fields:
         if f.tag is not None:
            if f.tag in used_tags:
               raise SyntaxError("Tag " + f.tag + " is already in use")
            used_tags.add(f.tag)
            if (f.tag in unused_tags):
               unused_tags.remove(f.tag)
      
      for f in self.fields:
         if f.tag is None:
            f.tag = unused_tags.pop()
      
      self.packed = True
      self.ordered_fields = sorted(self.fields, key=lambda x: x.tag)

      # id field is always present as a u16:
      self.fields_and_id = []
      if id_field == None:
         id_field = Field(name, "id", {'type':'id','tag':1000, 'persist':False, 'auth':'Root'}, model)
      self.fields_and_id.append(id_field)
      self.fields_and_id.extend(self.ordered_fields)

      # TODO: Order fields in most packed way

   def get_dependencies(self):
      '''Get list of type names that this structure refers to.'''
      deps = []
      for f in self.fields:
         deps.append(str(f.ctype))
      return deps
   
   def structure_definition(self):
      result = 'typedef struct %s \n{\n' % self.struct_name
      # bitmask_size = (len(self.ordered_fields * 2) + 7)//8      
      result += '   JUDE_HEADER_DECL(%s);\n' % max(1, len(self.fields_and_id))
      result += '\n'.join([str(f) for f in self.ordered_fields])
      result += '\n}'
      
      result += ' %s;' % self.struct_name
      
      return result

   def c_prototypes_definition(self):      
      result = '\n'.join([f.c_prototypes() for f in self.ordered_fields])
      return result

   def c_implementations(self):
      result = '\n'.join([f.c_implementations() for f in self.ordered_fields])
      return result

   def c_callbacks(self):
      result = '\n'.join([f.c_callbacks() for f in self.ordered_fields])
      return result
             
   def count_all_fields(self):
      return len(self.fields_and_id)

   def fields_declaration(self):
      result = 'extern const jude_rtti_t %s_rtti;' % (self.name)
      return result

   def fields_definition(self):
      result = 'static const jude_field_t %s_fields[%d] =\n{\n' % (self.name, self.count_all_fields() + 1)
      
      prev = None
      index = 0
      for field in self.fields_and_id:
         result += field.jude_field_t(prev, index)
         result += ',\n'
         prev = field.member
         index = index + 1
      
      result += '   JUDE_LAST_FIELD\n};'
      result += '\n\n'
      result += 'const jude_rtti_t %s_rtti =\n{\n' % (self.name)
      result += '   .name        =  "%s",\n' % (self.name)
      result += '   .field_list  =  %s_fields,\n' % (self.name)
      result += '   .field_count =  %d,\n' % (self.count_all_fields())
      result += '   .data_size   =  sizeof(%s)\n' % (self.struct_name)
      result += '};\n\n'


      return result

   def field_tag_enum(self):
      index = 0
      result = ''
      for field in self.fields_and_id:
         result += field.tag_id(index)
         result += ',\n'
         index = index + 1
         
      return result

   def field_index_enum(self):
      index = 0
      result = ''
      hasCustomId = False
      for field in self.fields_and_id:
         result += '   static const jude_size_t '
         result += field.tag_index(index)
         result += ';\n'
         if field.name == "Id": 
            hasCustomId = True
         index = index + 1
         
      if not hasCustomId:
         result += '\n   // For protobuf backwards compatibility\n'
         result += '   static const jude_size_t Id = id;\n'

      return result

   def field_indeces_c_object(self):
      result = ''
      if self.fields_and_id:
         for field in self.fields_and_id:
            result += '     const jude_size_t %s;\n' % (field.name)
      else:
         result += '     const jude_size_t dummy_index;\n'
         
      return result

   def field_indeces_c_source(self):
      index = 0
      result = ''
      if self.fields_and_id:
         for field in self.fields_and_id:
            result += '     .%s = %s,\n' % (field.name, str(index))
            index = index + 1
      else:
            result += '     .dummy_index = 0\n'

      return result

   def field_indeces_c_defines(self):
      index = 0
      result = ''
      for field in self.fields_and_id:
         result += '#define Index_Of_%s %s\n' % (field.name, str(index))
         index = index + 1
         
      return result

   

   def class_definition(self):
      accessors = ''
      className = str(self.name) + Globals.get('ObjectSuffix', '')
      for field in self.ordered_fields:
         accessors += field.accessor().replace('%CLASS%', className)
      
      return cpp_class_template.replace('%STRUCT%', str(self.struct_name)) \
                         .replace('%CLASS%', className) \
                         .replace('%ACCESSORS%', str(accessors)) \
                         .replace('%NUM_FIELDS%', str(len(self.ordered_fields)))
                         


   def class_declaration(self):
      accessors = ''
      className = str(self.name) + Globals.get('ObjectSuffix', '')
      for field in self.ordered_fields:
         accessors += '   // Accessors for ' + str(field.name) + '\n\n'
         accessors += field.accessorDecl().replace('%CLASS%', className)
         accessors += '\n\n'
         
      return cpp_class_declarataion_template.replace('%STRUCT%', str(self.struct_name)) \
                         .replace('%ACCESSORS%', str(accessors)) \
                         .replace('%INDEX_ENUM%', str(self.field_index_enum())) \
                         .replace('%NUM_FIELDS%', str(len(self.ordered_fields))) \
                         .replace('%CLASS%', className)
   
   def class_object_name(self, type):
      resource  = str(type)
      return "" + resource[:1].title() + resource[1:] + '.h'

   def class_source_name(self, type):
      source = str(type)
      return "" + source[:1].title() + source[1:] + '.cpp'
  
   def object_dependants(self):
      '''Get list of C++ objects that this class needs.'''
      result = ''
      done = []
      done.extend(atomic_types.keys())
      for field in self.ordered_fields:
         if field.ctype != self.name and \
            field.type not in done and \
            (field.judetype != 'BYTES') and \
            (field.judetype != 'STRING'):
            done.append(field.type)
            if field.judetype == 'ENUM' or field.judetype == 'BITMASK':
               result += '' # no need for Enums
            else:
               resourceType = self.model['objects'].get(field.type, self.model['external']['objects'].get(field.type, self))
               includePath = resourceType.importPrefix
               result += '#include "../' + str(includePath) + "/" + str(field.type) + '.h"\n'
            
      result += '#include "../' + Globals['objectbasename'] + '"\n'
      
      return result

   def c_object_dependants(self):
      '''Get list of c objects that this class needs.'''
      result = ''
      done = []
      done.extend(atomic_types)
      for field in self.ordered_fields:
         if field.ctype != self.name and not field.ctype in done and (field.judetype != 'BYTES'):
            done.append(field.ctype)
            if field.judetype == 'ENUM':
               result += '' # no need for Enums
            else:
               result += '' #'#include "' + str(field.subobjname) + '.h"\n'

      result += '#include "../../' + Globals['objectbasename'] + '"\n'
      
      return result
   
   def class_object_file(self):
      return hpp_file_template \
                        .replace('%HEADERS%', self.object_dependants()) \
                        .replace('%STRUCTURE_DEF%', self.structure_definition()) \
                        .replace('%CLASS_DECLARATION%', self.class_declaration()) \
                        .replace('%OBJECT%', str(self.name)) \

   def class_source_file(self):
      return cpp_file_template \
                        .replace('%FILE%', str(self.name)) \
                        .replace('%HEADERS%', ('#include "../' + Globals['objectbasename'] + '"')) \
                        .replace('%STRUCTURE_DEF%', self.structure_definition()) \
                        .replace('%CLASS_DEFINITION%', self.class_definition()) \
                        .replace('%OBJECT%', str(self.name)) \

   def c_object_file(self):
      return h_file_template \
                       .replace('%C_TYPE_HEADERS%', self.c_object_dependants()) \
                       .replace('%CLASS%', str(self.name)) \
                       .replace('%STRUCT%', str(self.struct_name)) \
                       .replace("%INDEX_NAMES%", self.field_indeces_c_object()) \
                       .replace('%PROTOTYPES%', self.c_prototypes_definition()) 

   def c_source_file(self):
      return c_file_template.replace('%IMPLEMENTATIONS%',  self.c_implementations()) \
                       .replace('%INDEX_VALUES%', self.field_indeces_c_source()) \
                       .replace('%CLASS%', str(self.name)) \
                       .replace('%STRUCT%', str(self.struct_name)) \
                       .replace('%CALLBACKS%', self.c_callbacks()) \
                       .replace('%FIELD_INDICES%', self.field_indeces_c_defines()) \

# ---------------------------------------------------------------------------
#               Generation of objects (structures)
# ---------------------------------------------------------------------------

class DatabaseEntry:
   
   def __init__(self, name, field_data, model):      

      self.name, self.max_size, self.isActionType = interpretName(name)
      self.model = model

      if not self.name.isidentifier():
         raise SyntaxError("FATAL: '" + self.name + "' is not a valid variable name")

      if not isinstance(field_data, dict):
         self.field_data = { 'type': field_data }
      else:
         self.field_data = field_data

      self.type = self.field_data['type']

      self.is_collecton = (self.max_size is not None)
      self.is_sub_database = (self.type in model['databases'] or self.type in model['external']['databases'])
      self.is_individual = (not self.is_sub_database) and self.max_size is None

      classname = self.field_data.get('class', 'default')
      self._class = self.model['classes'].get(classname, self.model['external']['classes'].get(classname, {}))
      self.calculate_crud_auth()

   def get(self, name, default):
      return self.field_data.get(name, self._class.get(name, default))

   def calculate_crud_auth(self):          
      # permissions
      auth = self.get('auth', 'Public')
      if isinstance(auth, dict):
         self.auth_create = str(auth.get('create', 'Public'))
         self.auth_read   = str(auth.get('read',   'Public'))
         self.auth_update = str(auth.get('update', 'Public'))
         self.auth_delete = str(auth.get('delete', 'Public'))
      else:
         self.auth_create = str(auth)
         self.auth_read   = str(auth)
         self.auth_update = str(auth)
         self.auth_delete = str(auth)
      
      validAuth = ['Root', 'Admin', 'Public']
      if (self.auth_read not in validAuth):
         raise SyntaxError("Field '" + self.name + "' has invalid read auth '" + self.auth_read + "': needs to be one of " + str(validAuth) )
      if (self.auth_create not in validAuth):
         raise SyntaxError("Field '" + self.name + "' has invalid write auth '" + self.auth_write + "': needs to be one of " + str(validAuth) )
      if (self.auth_update not in validAuth):
         raise SyntaxError("Field '" + self.name + "' has invalid write auth '" + self.auth_write + "': needs to be one of " + str(validAuth) )
      if (self.auth_delete not in validAuth):
         raise SyntaxError("Field '" + self.name + "' has invalid write auth '" + self.auth_write + "': needs to be one of " + str(validAuth) )

   def GetDeclaration(self):
      if self.is_collecton:
         return "jude::Collection<jude::" + self.type + Globals.get('ObjectSuffix', '') + "> " + self.name + ';'

      if self.is_individual:
         return "jude::Resource<jude::" + self.type + Globals.get('ObjectSuffix', '') + "> " + self.name + ';'

      return "jude::" + self.type + " " + self.name + ';'

   def GetLockGuardAccessor(self):
      if self.is_collecton:
         result  = 'auto& Get' + self.name + 's() { return m_data->' + self.name + '; }\n'
         result += '      const auto& Get' + self.name + 's() const { return m_data->' + self.name + '; }\n'
         result += '      auto Find' + self.name + 'ById(jude_id_t id) { return m_data->' + self.name + '.WriteLock(id); }\n'
         result += '      auto Add' + self.name + '(jude_id_t id = JUDE_AUTO_ID) { id = m_data->' + self.name + '.Post(id).Commit().GetCreatedObjectId(); return Find' + self.name + 'ById(id); }\n'
         result += '      void Remove' + self.name + '(jude_id_t id) { m_data->' + self.name + '.Delete(id); }\n'
         
         return result

      if self.is_individual:
         return 'auto Get' + self.name + '() { return m_data->' + self.name + '.WriteLock(); }'

      return ''

   def GetInitialiser(self):
      if self.is_collecton:
         return '%s("%s", %s, jude_user_%s, sharedMutex)' % (self.name, self.name, self.max_size, self.auth_read)

      return '%s("%s", jude_user_%s, sharedMutex)' % (self.name, self.name, self.auth_read)

   def GetInstallation(self):
      result = "InstallDatabaseEntry(" + self.name + ");"
      if self.auth_create != "Public":
         result += '\n      %s.SetAccessLevel(CRUD::CREATE, jude_user_%s);' % (self.name, self.auth_create)
      if self.auth_read != "Public":
         result += '\n      %s.SetAccessLevel(CRUD::READ, jude_user_%s);' % (self.name, self.auth_read)
      if self.auth_update != "Public":
         result += '\n      %s.SetAccessLevel(CRUD::UPDATE, jude_user_%s);' % (self.name, self.auth_update)
      if self.auth_delete != "Public":
         result += '\n      %s.SetAccessLevel(CRUD::DELETE, jude_user_%s);' % (self.name, self.auth_delete)

      return result

class Database:

   def calculate_types(self):
      for field in self.fields_and_id:
         field.calculate_types()         

   def __init__(self, importPrefix, name, schema, model):
      self.name = name
      self.importPrefix = importPrefix
      self.entries = []
      self.model = model

      if schema is None:
         schema = {} # default schema is empty

      if not isinstance(schema, dict):
         raise SyntaxError("Object definition should be a dictionary: " + str(schema))

      # Parse the database entries...     
      for e in schema:                  
         entry = DatabaseEntry(e, schema[e], model)
         self.entries.append(entry)
      

   def header_name(self):
      return self.name + ".h"

   def get_dependencies(self):
      '''Get list of type names that this structure refers to.'''
      deps = []
      return deps
   
   # def required_headers(self):
   #    return '\n'.join([str('#include "' + e.field_data['type'] + '.h"') for e in self.entries])

   def member_declarations(self):
      return '\n   '.join( [ e.GetDeclaration() for e in self.entries] )

   def lock_guard_accessors(self):
      return '\n      '.join( [ e.GetLockGuardAccessor() for e in self.entries] )
      
   def member_initialisers(self):
      return '\n      '.join( [ ", " + e.GetInitialiser() for e in self.entries] )

   def member_access_levels(self):
      return '\n      '.join( [ e.GetInstallation() for e in self.entries] )

   def object_dependants(self):
      '''Get list of C++ objects that this class needs.'''
      result = ''
      done = []
      for entry in self.entries:
         print("Database EntryType is ", entry.type)
         if entry.type not in done:
            done.append(entry.type)
            resourceType = self.model['databases'] \
                              .get(entry.type, self.model['external']['databases'] \
                              .get(entry.type, self.model['objects'] \
                              .get(entry.type, self.model['external']['objects'] \
                              .get(entry.type, None))))

            includePath = resourceType.importPrefix
            print("Database Entry", entry.type, " has importPrefix ", includePath)
            result += '#include "../' + str(includePath) + "/" + str(entry.type) + '.h"\n'
            
      result += '#include "../' + Globals['objectbasename'] + '"\n'
      
      return result

   def header_file(self):
      return database_hpp_template.replace('%DATABASE%', self.name) \
                         .replace('%HEADERS%', self.object_dependants()) \
                         .replace('%MEMBER_DECL%', self.member_declarations()) \
                         .replace('%LOCKGUARD_ACCESSORS%', self.lock_guard_accessors()) \
                         .replace('%MEMBER_INITIALISERS%', self.member_initialisers()) \
                         .replace('%MEMBER_ACCESS_LEVELS%', self.member_access_levels())

   
# ---------------------------------------------------------------------------
#               Processing of entire .proto files
# ---------------------------------------------------------------------------

def toposort2(data):
   '''Topological sort.
   From http://code.activestate.com/recipes/577413-topological-sort/
   This function is under the MIT license.
   '''
   for k, v in data.items():
      v.discard(k) # Ignore self dependencies
   extra_items_in_deps = reduce(set.union, data.values(), set()) - set(data.keys())
   data.update(dict([(item, set()) for item in extra_items_in_deps]))
   while True:
      ordered = set(item for item,dep in data.items() if not dep)
      if not ordered:
         break
      for item in sorted(ordered):
         yield item
      data = dict([(item, (dep - ordered)) for item,dep in data.items()
            if item not in ordered])
   assert not data, "A cyclic dependency exists amongst %r" % data

def sort_dependencies(objects):
   '''Sort a list of Objects based on dependencies.'''
   dependencies = {}
   object_by_name = {}
   for obj_name in objects:
      resource = objects[obj_name]
      dependencies[str(resource.struct_name)] = set(resource.get_dependencies())
      object_by_name[str(resource.struct_name)] = resource
   
   for objname in toposort2(dependencies):
      if objname in object_by_name:
         yield object_by_name[objname]

def generate_header(headername, model):
   '''Generate content for a resource file.
      Generates strings, which should be concatenated and stored to file.
   '''
   
   yield '/* Automatically generated jude resource model resource */\n'
   yield '/* Generated by %s at %s. */\n\n' % (jude_version, time.asctime())
   yield '#pragma once\n\n'
   yield '#include <stdint.h>\n'
   yield '#include <jude/jude_core.h>\n'
   yield '\n'
   
   yield '/* Constants */\n'
   yield '#include "%s/%s_constants.h"\n'%(Globals['modelname'], Globals['modelname'])
   yield '\n\n'  

   if len(model['imports']) > 0: 
      yield '/* External definitions */\n'
      for external_import in model['imports']:
         print(external_import)
         yield '#include "%s.model.h"\n'%(external_import.split('.')[0])
      yield '\n'

   if len(model['enums'].keys()) > 0: 
      yield '/* Enum definitions */\n'
      for enum in model['enums']:
         yield '#include "%s/%s.h"\n'%(Globals['modelname'], enum)

   if len(model['bitmasks'].keys()) > 0: 
      yield '/* Bitmask definitions */\n'
      for bitmask in model['bitmasks']:
         yield '#include "%s/%s.h"\n'%(Globals['modelname'], bitmask)

   yield '\n\n'
   yield '#ifdef __cplusplus\nextern "C" {\n#endif\n\n'

   yield '\n/* Object definitions */\n'
   objects = sort_dependencies(model.get('objects', {}))
   
   for obj in objects:
      yield obj.fields_declaration()
      yield '\n\n' + obj.structure_definition() + '\n\n'
     
   yield '/* Field tags (for use in manual encoding/decoding) */\n'
   for obj in objects:
      yield '\n// %s Tags\n' % obj.name
      for field in obj.fields:
         yield field.tags()
      yield '\n'

   yield '/* Struct field encoding specification for jude */\n'
   for obj in objects:
      yield obj.fields_declaration() + '\n'
   yield '\n'
   
   yield '#ifdef __cplusplus\n} // __cplusplus\n#endif\n\n'

def generate_source(headername, model):
   '''Generate content for a source file.'''
   
   yield '/* Automatically generated jude constant definitions */\n'
   yield '/* Generated by %s at %s. */\n\n' % (jude_version, time.asctime())
   yield '#include "%s"\n' % (headername)
   yield '#include <limits.h>\n'
   yield '\n'
     
   objects = sort_dependencies(model.get('objects', {}))
 
   yield '\n\n'
   
   for obj in objects:
      yield obj.fields_definition() + '\n\n'
   
   yield '\n'

def generate_constants_header(constants):
   '''Generate content for a header file.'''
   
   yield '/* Automatically generated jude constant definitions */\n'
   yield '/* Generated by %s at %s. */\n\n' % (jude_version, time.asctime())
   yield '#pragma once\n'
   yield '\n'

   for constant in constants:
      yield '#define %s (%s)\n'%(constant, constants[constant])


# ---------------------------------------------------------------------------
#                   Command line interface
# ---------------------------------------------------------------------------

import sys
import os.path   
from optparse import OptionParser

optparser = OptionParser(
   usage = "Usage: jude_generator.py [options] file.yaml [output_dir]",
   epilog = "Output will be written to file.jude.h and file.jude.c.")

def parse_enum(importPrefix, name, enum_def, constants):

   if not isinstance(enum_def,dict):
      raise SyntaxError("Enum definition should be a dictionary: " + enum_def)
   print("Parsing enum: ", importPrefix, name, "...")
   return EnumPrinter(importPrefix, name, enum_def)

def parse_bitmask(importPrefix, name, bitmask_def, constants):

   if not isinstance(bitmask_def,dict):
      raise SyntaxError("Bitmask definition should be a dictionary: " + bitmask_def)
   print("Parsing bitmask: ", importPrefix, name, "...")
   return BitmaskPrinter(importPrefix, name, bitmask_def)

def parse_object(importPrefix, name, obj_def, model):
   
   if obj_def is None:
      obj_def = {}

   if not isinstance(obj_def,dict):
      raise SyntaxError("Object definition should be a dictionary: " + str(obj_def))
   print("Parsing resource: ", importPrefix, name, "...")
   return Object(importPrefix, name, obj_def, model)

def parse_database(importPrefix, name, obj_def, model):
   
   if obj_def is None:
      obj_def = {}

   if not isinstance(obj_def,dict):
      raise SyntaxError("Database definition should be a dictionary: " + str(obj_def))
   print("Parsing database definition: ", importPrefix, name, "...")
   return Database(importPrefix, name, obj_def, model)


def guarantee_directory_exists(dir_path):
   while not os.path.exists(dir_path):
      try:
         os.makedirs(dir_path)
      except OSError as e:
         if e.errno != 17:  # 17 is "Cannot create a file when that file already exists" - we want to suppress this as it simply means another thread has created it at the same time as us.
            raise

def interpret_key(k):
   x = k.split()
   if (len(x) == 2):
      return (x[0], x[1])

   if (len(x) == 1 and x[0] == 'Import'):
      return (x[0], "Import")

   raise SyntaxError("Error parsing: '" + str(k) + "': Expected one of: 'Import', 'Constant', 'Enum', 'Bitmask', 'Object'")

def load_model_definitions(filename, importer):
   yaml_data = {}

   importPrefix = os.path.splitext(os.path.basename(importer))[0]

   myFile = open(filename, 'r')
   if myFile.mode == 'r':
      yaml_data = yaml.safe_load(myFile.read())
      ##pp = pprint.PrettyPrinter(indent=2)
      ##pp.pprint(yaml_data)
      myFile.close()
      #exit(-1)

   model = { 
      'imports':[], 
      'constants':{},
      'classes':{},
      'enums':{}, 
      'bitmasks':{}, 
      'objects':{},
      'databases':{},
      'external': { 'constants':{}, 'classes':{}, 'enums':{}, 'bitmasks':{}, 'objects':{}, 'databases':{} }
   }

   # Parse the file
   for a in yaml_data.keys():
      #print("Processing %s" % (a))
      (deftype, name) = interpret_key(a)
      
      if deftype == "Import":
         
         import_filenames = yaml_data[a]

         if not isinstance(import_filenames, list):
            import_filenames = [import_filenames]

         for import_filename in import_filenames:

            if import_filename in model['imports']:
               continue

            model['imports'].append(import_filename)
            abs_import_filepath = os.path.join(os.path.dirname(os.path.abspath(filename)), import_filename)
            # load import immediately so we can use the definitions
            external_model = load_model_definitions(abs_import_filepath, import_filename)
            model['external']['constants'].update(external_model['constants'])
            model['external']['classes'].update(external_model['classes'])
            model['external']['enums'].update(external_model['enums'])
            model['external']['bitmasks'].update(external_model['bitmasks'])
            model['external']['objects'].update(external_model['objects'])
            model['external']['databases'].update(external_model['databases'])
         
            model['external']['constants'].update(external_model['external']['constants'])
            model['external']['classes'].  update(external_model['external']['classes'])
            model['external']['enums'].    update(external_model['external']['enums'])
            model['external']['bitmasks']. update(external_model['external']['bitmasks'])
            model['external']['objects'].  update(external_model['external']['objects'])
            model['external']['databases'].  update(external_model['external']['databases'])

      elif deftype == "Constant":
         model['constants'][name] = yaml_data[a]
      elif deftype == "Class":
         model['classes'][name] = yaml_data[a]
      elif deftype == "Enum":
         model['enums'][name] = parse_enum(importPrefix, name, yaml_data[a], model['constants'])
      elif deftype == "Bitmask":
         model['bitmasks'][name] = parse_bitmask(importPrefix, name, yaml_data[a], model['constants'])
      elif deftype == "Object":
         model['objects'][name] = parse_object(importPrefix, name, yaml_data[a], model)
      elif deftype == "Database":
         model['databases'][name] = parse_database(importPrefix, name, yaml_data[a], model)
      else:
         raise SyntaxError("Unexpected key: " + a)

   return model

def process_file(filename, output_dir):

   model = load_model_definitions(filename, filename)
   # print(model)

   # Decide the file names
   Globals['modelname'] = os.path.splitext(os.path.basename(filename))[0]
   prefix = os.path.join(output_dir, Globals['modelname'])
   directory = prefix
   headername = prefix + '.model.h'
   sourcename = prefix + '.model.c'
   Globals['objectbasename'] = os.path.basename(headername)

   results = []

   guarantee_directory_exists(directory)

   constants_header = directory + '/' + Globals['modelname'] + '_constants.h'
   constants_text = ''.join(generate_constants_header(model.get('constants', [])))
   results.append( (constants_header, constants_text) ) 

   for enum in model['enums']:
      enum_object = directory + '/' + str(enum) + '.h'
      enum_source = directory + '/' + str(enum) + '.cpp'
      results.append( (enum_object, model['enums'][enum].create_object()) ) 
      results.append( (enum_source, model['enums'][enum].create_source()) ) 

   for bitmask in model['bitmasks']:
      bitmask_object = directory + '/' + str(bitmask) + '.h'
      bitmask_source = directory + '/' + str(bitmask) + '.cpp'
      results.append( (bitmask_object, model['bitmasks'][bitmask].create_object()) ) 
      results.append( (bitmask_source, model['bitmasks'][bitmask].create_source()) ) 

   for obj_name in model['objects']:
      #print (obj_name)
      obj = model['objects'][obj_name]
      obj.calculate_types()
      obj_filename = directory + '/' + obj.class_object_name(obj.name)
      obj_filetext = obj.class_object_file()
      results.append( (obj_filename, obj_filetext) ) 

      obj_filename = directory + '/' + obj.class_source_name(obj.name)
      obj_filetext = obj.class_source_file()
      results.append( (obj_filename, obj_filetext) ) 

   for db_name in model['databases']:
      #print (obj_name)
      db = model['databases'][db_name]
      db_filename = directory + '/' + db.header_name()
      db_filetext = db.header_file()
      results.append( (db_filename, db_filetext) ) 

   objectdata = ''.join(generate_header(Globals['objectbasename'], model))
   results.append( (headername, objectdata) ) 

   sourcedata = ''.join(generate_source(Globals['objectbasename'], model))
   results.append( (sourcename, sourcedata) ) 

   return results
   
def main():
   optparser.add_option("-l", "--legacy", action="store_true", dest="legacy", help="legacy mode code generation")

   options, filenames = optparser.parse_args()
   
   print("Note: Launching with options = ",options)

   if options.legacy:
      Globals['legacy'] = True
      Globals['ObjectSuffix'] = 'Accessor'

   if not filenames:
      optparser.print_help()
      sys.exit(1)
   
   jude_yaml_file = filenames[0]
   
   if len(filenames) > 1:
      output_dir = filenames[1]
   else:
      output_dir = os.path.dirname(jude_yaml_file)
 
   guarantee_directory_exists(output_dir)
   
   results = process_file(jude_yaml_file, output_dir)

   for name, data in results:
      guarantee_directory_exists(os.path.dirname(name))
      sys.stdout.write("Creating file: " + os.path.abspath(name) + " ... ")  
      file = open(name, 'w')
      file.write(data)
      file.close()
      os.chmod(name, 0o777)
      if (os.path.exists(name)):
       sys.stdout.write("OK\n")
      else:
       sys.stdout.write("Failed!\n")      

if __name__ == '__main__':
   main()