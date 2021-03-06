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
#include <jude/core/cpp/AtomicArray.h>
#include <jude/core/cpp/Object.h>

namespace jude
{
   class StringArray : public BaseArray
   {
   public:
      StringArray(Object& object, jude_size_t fieldIndex);

      bool Add(const char *value);
      bool Insert(jude_size_t array_index, const char *value);
      bool Set(jude_size_t array_index, const char *value);
      const char *Get(jude_size_t array_index) const;
      const char *operator[] (jude_size_t index) const { return Get(index); }

      bool Add(const std::string& value) { return Add(value.c_str()); }
      bool Insert(jude_size_t array_index, std::string& value) { return Insert(array_index, value.c_str()); }
      bool Set(jude_size_t array_index, std::string& value) { return Set(array_index, value.c_str()); }
   };
}
