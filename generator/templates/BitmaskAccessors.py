BitMaskAccessorPrototypes = '''
   %TYPE% Get_%NAME%();
   const %TYPE% Get_%NAME%() const;
'''
   
BitMaskAccessorImplementations = """
   // Accessors for %NAME%
   %TYPE% %CLASS%::Get_%NAME%()
   {
      return %TYPE%(*this, %INDEX%);
   }
   const %TYPE% %CLASS%::Get_%NAME%() const
   {
      return const_cast<%CLASS%*>(this)->Get_%NAME%();
   }
"""

BitMaskArrayAccessorProtoypes = """
   const %TYPE% Get_%NAME%(jude_size_t arrayIndex) const;
   %TYPE% Get_%NAME%(jude_size_t arrayIndex);
"""
   
BitMaskArrayAccessorImplementations = """
   // Accessors for %NAME%
   %TYPE% %CLASS%::Get_%NAME%(jude_size_t arrayIndex)
   {
      return %TYPE%(*this, %INDEX%, arrayIndex);
   }
   const %TYPE% %CLASS%::Get_%NAME%(jude_size_t arrayIndex) const
   {
      return const_cast<%CLASS%*>(this)->Get_%NAME%(arrayIndex);
   }
"""

def BitMaskTemplateMap():
   return {
      'cpp': {
         'objects': {
            'single':BitMaskAccessorPrototypes,
            'repeated':BitMaskArrayAccessorProtoypes
         },
         'source': {
            'single':BitMaskAccessorImplementations,
            'repeated':BitMaskArrayAccessorImplementations
         }
      }
   }

