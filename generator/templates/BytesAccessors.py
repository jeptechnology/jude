BytesAccessorPrototypes = '''
   bool Has_%NAME%() const { return Has(%INDEX%); }
   %CLASS%& Clear_%NAME%() { Clear(%INDEX%); return *this; }
   const std::vector<uint8_t> Get_%NAME%() const;
   %CLASS%& Set_%NAME%(const std::vector<uint8_t>& %NAME%) { return Set_%NAME%(%NAME%.data(), (jude_size_t)%NAME%.size()); }
   %CLASS%& Set_%NAME%(const uint8_t* %NAME%, jude_size_t size);
'''

BytesAccessorImplementations = '''
   // Accessors for %NAME%
   const std::vector<uint8_t> %CLASS%::Get_%NAME%() const
   {
      if (!Has_%NAME%()) { return std::vector<uint8_t>(); }
      return std::vector<uint8_t>(m_pData->%MEMBER%.bytes, m_pData->%MEMBER%.bytes + m_pData->%MEMBER%.size);
   }

   %CLASS%& %CLASS%::Set_%NAME%(const uint8_t* value, jude_size_t size)
   {
      bool alwaysNotify = RTTI()->field_list[%INDEX%].always_notify; // if we have to always notify, force a "change" bit

      if (jude_object_set_bytes_field(m_object, %INDEX%, 0, value, size))
      {
         if (alwaysNotify || IsChanged(%INDEX%))
         {
            MarkFieldSet(%INDEX%, true);
         }
      }
      return *this;
   }
'''

BytesArrayAccessorPrototypes = ''' 
   const BytesArray Get_%PLURAL_NAME%() const;
   BytesArray Get_%PLURAL_NAME%();
'''
           
BytesArrayAccessorImplementations = ''' 
   // Accessors for %NAME%
   const BytesArray %CLASS%::Get_%PLURAL_NAME%() const 
   {
      return const_cast<%CLASS%*>(this)->Get_%PLURAL_NAME%();
   }   
   BytesArray %CLASS%::Get_%PLURAL_NAME%()
   {
      return BytesArray(*this, %INDEX%); 
   }   
'''

def BytesTemplateMap():
   return {
      'cpp': {
         'objects': {
            'single':BytesAccessorPrototypes,
            'repeated':BytesArrayAccessorPrototypes
         },
         'source': {
            'single':BytesAccessorImplementations,
            'repeated':BytesArrayAccessorImplementations
         }
      }
   }
