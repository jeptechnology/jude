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
#include <iostream>
#include <jude/jude_core.h>
#include <jude/core/cpp/AccessControl.h>

namespace jude
{
   namespace swagger
   {
      // For Swagger structure
      extern const char* HeaderTemplate;
      extern const char* ComponentsTemplate;

      // For collections
      extern const char* PostTemplate;
      extern const char* GetAllTemplate;
      extern const char* GetWithIdTemplate;
      extern const char* PatchWithIdTemplate;
      extern const char* PutWithIdTemplate;
      extern const char* DeleteWithIdTemplate;
      extern const char* PatchActionWithIdTemplate;

      // For individuals
      extern const char* GetTemplate;
      extern const char* PatchTemplate;
      extern const char* PutTemplate;
      extern const char* PatchActionTemplate;

      std::string GetSchemaForActionField(const jude_field_t& field, RestApiSecurityLevel::Value userLevel);
      void RecursivelyOutputSchemas(std::ostream& output, std::set<const jude_rtti_t*>& schemas, const jude_rtti_t* rtti, RestApiSecurityLevel::Value userLevel);
   }
}
