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

#include <vector>
#include <jude/jude_core.h>
#include <jude/core/cpp/Object.h>
#include <jude/core/cpp/AtomicArray.h>

namespace jude
{
   class BytesArray : public BaseArray
   {
   public:
      BytesArray(Object& object, jude_size_t fieldIndex);

      bool Add(const uint8_t *value, jude_size_t size);
      bool Insert(jude_size_t array_index, const uint8_t *value, jude_size_t size);
      bool Set(jude_size_t array_index, const uint8_t *value, jude_size_t size);
      const std::vector<uint8_t> Get(jude_size_t array_index) const;
      const std::vector<uint8_t> operator[](jude_size_t array_index) const { return Get(array_index); }

      bool Add(const std::vector<uint8_t>& value) { return Add(value.data(), (jude_size_t)value.size()); }
      bool Insert(jude_size_t array_index, const std::vector<uint8_t>& value) { return Insert(array_index, value.data(), (jude_size_t)value.size()); }
      bool Set(jude_size_t array_index, const std::vector<uint8_t>& value) { return Set(array_index, value.data(), (jude_size_t)value.size()); }
   };
}
