AtomicAccessorPrototypes = ''' 
   bool Has_%NAME%() const;
   %CLASS%& Clear_%NAME%();
   %CLASS%& Set_%NAME%(%TYPE% value);
   %TYPE% Get_%NAME%() const;
   %TYPE% Get_%NAME%_or(%TYPE% defaultValue) const;
'''

AtomicArrayAccessorPrototypes = ''' 
   const Array<%TYPE%> Get_%PLURAL_NAME%() const;
   Array<%TYPE%> Get_%PLURAL_NAME%();
   auto Add_%NAME%(%TYPE% value) { return Get_%PLURAL_NAME%().Add(value); }
   %TYPE%  Get_%NAME%(jude_size_t index) const { return Get_%PLURAL_NAME%()[index]; }
'''
           
AtomicAccessorImplementations = ''' 
   // Accessors for %NAME%
   bool %CLASS%::Has_%NAME%() const
   {
      return jude_filter_is_touched(m_pData->__mask, %INDEX%); 
   }
   
   %CLASS%& %CLASS%::Clear_%NAME%()
   {
      Clear(%INDEX%);
      return *this;       
   }

   %CLASS%& %CLASS%::Set_%NAME%(%TYPE% value)
   {
      bool alwaysNotify = RTTI()->field_list[%INDEX%].always_notify; // if we have to always notify, force a "change" bit

      if (alwaysNotify || !Has_%NAME%() || (value != m_pData->%MEMBER%))
      {
         m_pData->%MEMBER% = value;
         MarkFieldSet(%INDEX%, true);
      }
      return *this;
   }

   %TYPE% %CLASS%::Get_%NAME%() const
   {
      if (!Has_%NAME%())
      {
         jude_handle_null_field_access(m_object, "%CLASS%::%NAME%");
         return {};
      }   
      return (%TYPE%)m_pData->%MEMBER%;
   }

   %TYPE% %CLASS%::Get_%NAME%_or(%TYPE% default_value) const
   {
      return Has_%NAME%() ? (%TYPE%)m_pData->%MEMBER% : default_value;
   }   
'''

AtomicArrayAccessorImplementations = ''' 
   // Accessors for %NAME%
   const Array<%TYPE%> %CLASS%::Get_%PLURAL_NAME%() const
   {
      return const_cast<%CLASS%*>(this)->Get_%PLURAL_NAME%();
   }

   Array<%TYPE%> %CLASS%::Get_%PLURAL_NAME%()
   {
      return Array<%TYPE%>(*this, %INDEX%);
   }
'''

def AtomicTemplateMap():
   return {
       'cpp': {
          'objects': {
             'single':AtomicAccessorPrototypes,
             'repeated':AtomicArrayAccessorPrototypes
          },
          'source': {
             'single':AtomicAccessorImplementations,
             'repeated':AtomicArrayAccessorImplementations
          }
       }
   }
