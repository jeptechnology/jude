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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PLATFORM REQUIREMENTS */

#define JUDE_WAIT_FOREVER ((uint32_t)-1)

typedef struct jude_mutex_t jude_mutex_t;
typedef struct jude_semaphore_t jude_semaphore_t;
typedef struct jude_queue_t jude_queue_t;

// mockable OS interface for jude
typedef struct
{
   void (*fatal)(const char *file, int line, const char *message);

   // mutex
   jude_mutex_t* (*mutex_create)();
   void (*mutex_destroy)(jude_mutex_t *);
   bool (*mutex_lock)(jude_mutex_t *, uint32_t milliseconds);
   void (*mutex_unlock)(jude_mutex_t *);

   // semaphore
   jude_semaphore_t* (*semaphore_create)(uint32_t initial, uint32_t max);
   void (*semaphore_destroy)(jude_semaphore_t *);
   bool (*semaphore_take)(jude_semaphore_t *, uint32_t milliseconds);
   void (*semaphore_give)(jude_semaphore_t *);

   // queues
   jude_queue_t *(*queue_create)(size_t maxElements, size_t elementSize);
   void (*queue_destroy)(jude_queue_t *);
   void (*queue_send)(jude_queue_t *queue, const void *element); // a copy of element is made
   bool (*queue_receive)(jude_queue_t *queue, void *buffer, uint32_t milliseconds);

} jude_os_interface_t;

extern jude_os_interface_t *jude_os;

#ifdef __cplusplus
}
#endif
