StringAccessorPrototypes = ''' 
   bool Has_%NAME%() const { return Has(%INDEX%); }
   %CLASS%& Clear_%NAME%() { Clear(%INDEX%); return *this; }
   const std::string Get_%NAME%() const;
   const char *Get_%NAME%_Pointer() const;
   const std::string Get_%NAME%_or(const std::string& defaultValue) const;
   %CLASS%& Set_%NAME%(const std::string& %NAME%) { return Set_%NAME%(%NAME%.c_str()); }
   %CLASS%& Set_%NAME%(const char* %NAME%); 
'''
        
StringAccessorImplementations = ''' 
   // Accessors for %NAME%
   const std::string %CLASS%::Get_%NAME%() const
   {
      return std::string(Get_%NAME%_Pointer());
   }

   const char * %CLASS%::Get_%NAME%_Pointer() const
   {
      if (!Has_%NAME%()) 
      { 
         jude_handle_null_field_access(m_object, "%NAME%"); 
         return ""; 
      }
      return m_pData->%MEMBER%;
   }

   const std::string %CLASS%::Get_%NAME%_or(const std::string& defaultValue) const
   {
      if (!Has_%NAME%()) { return defaultValue; }
      return std::string(m_pData->%MEMBER%);
   }

   %CLASS%& %CLASS%::Set_%NAME%(const char *input%NAME%)
   {
      bool alwaysNotify = RTTI()->field_list[%INDEX%].always_notify; // if we have to always notify, force a "change" bit
      
      jude_object_set_string_field(m_object, %INDEX%, 0, input%NAME%);
      MarkFieldSet(%INDEX%, alwaysNotify || IsChanged(%INDEX%));
      return *this;
   }
'''

StringArrayAccessorPrototypes = ''' 
   StringArray Get_%PLURAL_NAME%();
   auto Add_%NAME%(const std::string& value) { return Get_%PLURAL_NAME%().Add(value); }
   const StringArray Get_%PLURAL_NAME%() const;
   const char *Get_%NAME%(jude_size_t index) const { return Get_%PLURAL_NAME%()[index]; }
'''
           
StringArrayAccessorImplementations = ''' 
   // Accessors for %NAME%
   StringArray %CLASS%::Get_%PLURAL_NAME%()
   {
      return StringArray(*this, %INDEX%); 
   }

   const StringArray %CLASS%::Get_%PLURAL_NAME%() const 
   {
      return const_cast<%CLASS%*>(this)->Get_%PLURAL_NAME%();
   }     
'''

def StringTemplateMap():
   return {
      'cpp': {
         'objects': {
            'single':StringAccessorPrototypes,
            'repeated':StringArrayAccessorPrototypes
         },
         'source': {
            'single':StringAccessorImplementations,
            'repeated':StringArrayAccessorImplementations
         }
      }
   }
   