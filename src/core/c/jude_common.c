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
#include <time.h>

jude_object_t *jude_remove_const(const jude_object_t* pointer)
{
   return (jude_object_t *) pointer;
}

void jude_init()
{
}

void jude_shutdown()
{
}

static jude_id_t default_uuid_generator(void *user_data)
{
   static jude_id_t counter = 0;

   jude_id_t mycounter = ++counter;

#if (JUDE_ID_SIZE==64)
   time_t now = time(NULL);

   // 40 bits of unixtime  (seconds since 1970)
   // 24 bits of counter   (up to 16 million new UUID's per second)
   return (jude_id_t)((now & 0xFFFFF) << 24) + (mycounter & 0xFFF);
#else 
   return mycounter;
#endif
}

static jude_uuid_generator uuid_generator = default_uuid_generator;
static void* uuid_user_data;

jude_uuid_generator jude_install_custom_uuid_geneartor(void* user_data, jude_uuid_generator generator_function)
{
   jude_uuid_generator old = uuid_generator;
   uuid_generator = generator_function;
   uuid_user_data = user_data;
   return old;
}

jude_id_t jude_generate_uuid()
{
   return uuid_generator(uuid_user_data);
}

