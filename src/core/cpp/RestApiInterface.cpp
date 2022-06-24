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

#include <jude/core/cpp/RestApiInterface.h>
#include <sstream>

namespace jude
{
   const AccessControl RestApiInterface::accessToEverything; // gives access to all fields

   std::string RestApiInterface::ToJSON_EmptyOnError(const char* path, size_t maxSize, jude_user_t userLevel) const
   {
      std::stringstream ss;
      auto result = RestGet(path, ss, userLevel);
      if (result)
      {
         return ss.str();
      }
      // return empty string on error
      return "";
   }

   // Convenience function
   std::string RestApiInterface::ToJSON(const char *path, size_t maxSize, jude_user_t userLevel) const
   { 
      std::stringstream ss;
      auto result = RestGet(path, ss, userLevel);
      if (result)
      {
         return ss.str();
      }
      // return error string
      return "#ERROR: " + result.GetDetails();
   }

   RestfulResult RestApiInterface::RestPostString(const char* path, const char *input, jude_user_t userLevel)
   {
      std::stringstream is(input);
      return RestPost(path, is, userLevel);
   }

   RestfulResult RestApiInterface::RestPatchString(const char* path, const char* input, jude_user_t userLevel)
   {
      std::stringstream is(input);
      return RestPatch(path, is, userLevel);
   }

   RestfulResult RestApiInterface::RestPutString(const char* path, const char* input, jude_user_t userLevel)
   {
      std::stringstream is(input);
      return RestPut(path, is, userLevel);
   }

   // Suggest to use this function each time you wish to traverse a URL path
   std::string RestApiInterface::GetNextUrlToken(const char* urlPath, const char** resultingSuffix, bool stripNextSlash)
   {
      char token[MAX_REST_API_URL_PATH_TOKEN];

      auto suffix = stripNextSlash ? jude_restapi_get_next_path_token(urlPath, token, sizeof(token))
                                   : jude_restapi_get_next_path_token_no_strip(urlPath, token, sizeof(token));

      if (resultingSuffix)
      {
         *resultingSuffix = suffix;
      }
      return token;
   }
}
