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

#include <jude/jude.h>

namespace jude
{
   namespace Options
   {
      // If set to true (default), then we ensure that collection is an object with id as "key" and resources as "value".
      // If set to false, we just serialise as an array of JSON objects
      bool SerialiseCollectionAsObjectMap = true;

      // Useful for testing - forces all changes to be published immediately 
      // By default we would wait until the edit was completed before publishing
      bool NotifyImmediatelyOnChange = false; 

      // When creating a new entry in a collection, should we validate only when this has been
      // Done over the REST API (default) or even just if we create "in code"?
      bool ValidatePostOnlyForRestAPI = true; 

      bool GenerateIDsBAsedOnCollectionSize = false;

      // By default all code that does not explicitly specify an access level gets root access to the data
      // For a more security concious usage of this library, suggest using jude_user_Public here?
      RestApiSecurityLevel::Value DefaultAccessLevelForJSON = RestApiSecurityLevel::Root;
   }
}
