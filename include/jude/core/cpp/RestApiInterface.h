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

#pragma once

#include <jude/restapi/jude_rest_api.h>
#include <jude/core/cpp/Stream.h>
#include <jude/core/cpp/AccessControl.h>
#include <jude/core/cpp/RestfulResult.h>
#include <string>
#include <vector>

namespace jude
{
   enum class CRUD
   {
      CREATE,
      READ,
      UPDATE,
      DELETE
   };

   class RestApiInterface
   {
   public:
      static const AccessControl accessToEverything; // gives access to all fields

      // RESTful operations to be implemented
      virtual RestfulResult RestGet   (const char* path, OutputStreamInterface& output, const AccessControl& accessControl = accessToEverything) const = 0;
      virtual RestfulResult RestPost  (const char* path, InputStreamInterface& input, const AccessControl& accessControl = accessToEverything) = 0;
      virtual RestfulResult RestPatch (const char* path, InputStreamInterface& input, const AccessControl& accessControl = accessToEverything) = 0;
      virtual RestfulResult RestPut   (const char* path, InputStreamInterface& input, const AccessControl& accessControl = accessToEverything) = 0;
      virtual RestfulResult RestDelete(const char* path, const AccessControl& accessControl = accessToEverything) = 0;

      // Convenience functions
      std::string   ToJSON(jude_user_t userLevel, size_t maxSize = 0xFFFF) const { return ToJSON("/", maxSize, userLevel); }
      std::string   ToJSON(const char* path = "/", size_t maxSize = 0xFFFF, jude_user_t userLevel = Options::DefaultAccessLevelForJSON) const;
      std::string   ToJSON_EmptyOnError(const char* path = "/", size_t maxSize = 0xFFFF, jude_user_t userLevel = Options::DefaultAccessLevelForJSON) const;
      std::string   ToJSON_WithNulls(const char* path = "/", size_t maxSize = 0xFFFF, jude_user_t userLevel = Options::DefaultAccessLevelForJSON) const;
      RestfulResult ToJSON(OutputStreamInterface& output, const AccessControl& accessControl = accessToEverything) const { return RestGet("", output, accessControl); }

      RestfulResult RestPostString(const char* path, const char* input, jude_user_t userLevel = Options::DefaultAccessLevelForJSON);
      RestfulResult RestPatchString(const char* path, const char* input, jude_user_t userLevel = Options::DefaultAccessLevelForJSON);
      RestfulResult RestPutString(const char* path, const char* input, jude_user_t userLevel = Options::DefaultAccessLevelForJSON);

      virtual std::vector<std::string> SearchForPath(CRUD operationType, const char* pathPrefix, jude_size_t maxPaths, jude_user_t userLevel = Options::DefaultAccessLevelForJSON) const = 0;

      bool PathExists(CRUD operationType, const std::string& pathPrefix, jude_user_t userLevel = Options::DefaultAccessLevelForJSON) const
      {
         return SearchForPath(operationType, pathPrefix.c_str(), 1, userLevel).size() > 0;
      }

      // Suggest to use this function each time you wish to traverse a URL path
      static std::string GetNextUrlToken(const char* urlPath, const char** resultingSuffix = nullptr, bool stripSlash = true);

      ///////////////////////////////////////////////////////////////////////////////
      // Protobuf backwards compatibility
      bool JsonEncodeTo(OutputStreamInterface& output, const AccessControl& accessControl = accessToEverything) const
      {
         return ToJSON(output, accessControl).IsOK(); // on failure, inspect output.GetOutputErrorMsg()
      }
      ///////////////////////////////////////////////////////////////////////////////
   };
}
