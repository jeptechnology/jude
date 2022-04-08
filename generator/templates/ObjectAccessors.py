ObjectAccessorPrototypes = '''
   bool Has_%NAME%() const { return Has(%INDEX%); }
   %CLASS%& Clear_%NAME%() { Clear(%INDEX%); return *this; }
   %TYPE% Get_%NAME%();
   const %TYPE% Get_%NAME%() const;
   %CLASS%& Set_%NAME%(const %TYPE%& value);
'''

ObjectAccessorImplementations = ''' 
   // Accessors for %NAME%
   %TYPE% %CLASS%::Get_%NAME%()
   {
      return GetChild<%TYPE%>(m_pData->%MEMBER%);
   }
   const %TYPE% %CLASS%::Get_%NAME%() const
   {
      return const_cast<%CLASS%*>(this)->Get_%NAME%();
   }
   %CLASS%& %CLASS%::Set_%NAME%(const %TYPE%& value)
   {
      bool alwaysNotify = RTTI()->field_list[%INDEX%].always_notify; // if we have to always notify, force a "change" bit

      if (alwaysNotify || !Has_%NAME%())
      {
         Get_%NAME%().OverwriteData(value);
         MarkFieldSet(%INDEX%, true);
      }
      else
      {
         auto subObj = Get_%NAME%();
         bool hasChanged = (value != subObj);
         if (hasChanged)
         {
            subObj.OverwriteData(value);
         }
         MarkFieldSet(%INDEX%, hasChanged);
      }

      return *this;
   }
'''

ObjectArrayAccessorPrototypes = '''
   ObjectArray<%TYPE%> Get_%PLURAL_NAME%();
   const ObjectArray<%TYPE%> Get_%PLURAL_NAME%() const;
   auto Add_%NAME%() { return Get_%PLURAL_NAME%().Add(); }
   auto Add_%NAME%(jude_id_t id) { return Get_%PLURAL_NAME%().Add(id); }

   %TYPE%                 Get_%NAME%(jude_size_t index) { return Get_%PLURAL_NAME%()[index]; }
   const %TYPE%           Get_%NAME%(jude_size_t index) const { return Get_%PLURAL_NAME%()[index].Clone(); }
   std::optional<%TYPE%>       Find_%NAME%(jude_id_t id) { return Get_%PLURAL_NAME%().Find(id); };
   std::optional<const %TYPE%> Find_%NAME%(jude_id_t id) const { return Get_%PLURAL_NAME%().Find(id); };
'''

ObjectArrayAccessorImplementations = ''' 
   // Accessors for %NAME%
   ObjectArray<%TYPE%> %CLASS%::Get_%PLURAL_NAME%()
   {
      return ObjectArray<%TYPE%>(*this, %INDEX%); 
   }
   const ObjectArray<%TYPE%> %CLASS%::Get_%PLURAL_NAME%() const
   {
      return const_cast<%CLASS%*>(this)->Get_%PLURAL_NAME%(); 
   }
'''

def ObjectTemplateMap():
   return {
      'cpp': {
         'objects': {
            'single':ObjectAccessorPrototypes,
            'repeated':ObjectArrayAccessorPrototypes
         },
         'source': {
            'single':ObjectAccessorImplementations,
            'repeated':ObjectArrayAccessorImplementations
         }
      }
   }
