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

#include <jude/jude_core.h>

static struct 
{
   void (*printer)(const char *message, va_list args);
   void (*null_access)(const jude_object_t *object, const char *field_name);
   void (*string_overflow)(const jude_object_t* object, const char* field_name);
   void (*array_overflow)(const jude_object_t* object, const char* field_name);
} debug =
{
   NULL, NULL, NULL, NULL
};

void jude_set_debug_printer(void (*printer_fn)(const char* format, va_list args))
{
   debug.printer = printer_fn;
}

void jude_debug(const char* format, ...)
{
   va_list args;
   va_start(args, format);
   if (debug.printer) debug.printer(format, args);
   va_end(args);
}

void jude_add_null_field_access_handler(void (*handler)(const jude_object_t* object, const char* field_name))
{
   debug.null_access = handler;
}

void jude_handle_string_overflow(const jude_object_t* object, const char* field_name)
{
   if (debug.string_overflow)
   {
      debug.string_overflow(object, field_name);
   }
   else
   {
      jude_debug("WARNING: String overflow setting field: %s\n", field_name);
   }
}

void jude_handle_null_field_access(const jude_object_t *object, const char *field_name)
{
   if (debug.null_access)
   {
      debug.null_access(object, field_name);
   }
   else
   {
      jude_debug("WARNING: Null access to field: %s\n", field_name);
   }
}

void jude_handle_index_out_of_range(const jude_object_t* object, const char* field_name, jude_size_t attempted_index, jude_size_t max_index)
{
   jude_debug("WARNING: Attempted to access %s[%d] but count is %d\n", field_name, attempted_index, max_index);
}


