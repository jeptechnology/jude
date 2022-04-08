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

#include <string>
#include <jude/jude_core.h>
#include <jude/restapi/jude_rest_api.h>

namespace jude
{
   class RestfulResult
   {
      std::string         m_errorMsg;
      jude_restapi_code_t m_statusCode;
      jude_id_t           m_newlyCreatedId; // for POST operations

      void SetDetailedErrorMessage(const std::string& detailedErrorMessage);

   public:
      RestfulResult(jude_restapi_code_t code = jude_rest_OK, const std::string& detailedErrorMessage = "");
      explicit RestfulResult(jude_id_t newId);

      bool IsOK() const { return jude_restapi_is_successful(m_statusCode); }
      operator bool() const { return IsOK(); }
      jude_restapi_code_t GetCode() const { return m_statusCode; }
      const std::string& GetDetails() const { return m_errorMsg; }
      jude_id_t GetCreatedObjectId() const { return m_newlyCreatedId; }
   };

}
