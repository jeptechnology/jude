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

#include <jude/core/cpp/RestfulResult.h>

namespace jude
{
   void RestfulResult::SetDetailedErrorMessage(const std::string& detailedErrorMessage)
   {
      if (!IsOK())
      {
         if (detailedErrorMessage.length() == 0)
         {
            m_errorMsg = jude_restapi_code_description(m_statusCode);
         }
         else
         {
            m_errorMsg = detailedErrorMessage;
         }
      }
   }

   RestfulResult::RestfulResult(jude_restapi_code_t code, const std::string& detailedErrorMessage)
      : m_statusCode(code)
      , m_newlyCreatedId(0)
   {
      SetDetailedErrorMessage(detailedErrorMessage);
   }

   RestfulResult::RestfulResult(jude_id_t newId)
      : m_statusCode(jude_rest_Created) // new resource creation
      , m_newlyCreatedId(newId)
   {
   }
}