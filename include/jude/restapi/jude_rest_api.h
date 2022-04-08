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

#include <jude/jude_core.h>

#ifdef __cplusplus
extern "C" {
#endif

// URL paths will not be able to have single tokens more than this length (e.g. field names, collecton names, etc should be less than 128)
#define MAX_REST_API_URL_PATH_TOKEN 128

typedef enum
{
   jude_rest_OK                    = 200,
   jude_rest_Created               = 201, // result of POST
   jude_rest_No_Content            = 204, // result of PATCH to write-only

   jude_rest_Bad_Request           = 400,
   jude_rest_Unauthorized          = 401,
   jude_rest_Forbidden             = 403,
   jude_rest_Not_Found             = 404,
   jude_rest_Method_Not_Allowed    = 405,
   jude_rest_Conflict              = 409,

   jude_rest_Internal_Server_Error = 500
} jude_restapi_code_t;

/*
 * RESTful API operations on jude objects
 */
jude_restapi_code_t jude_restapi_get   (jude_user_t user_level, const jude_object_t *, const char *path, jude_ostream_t *output); // const operation
jude_restapi_code_t jude_restapi_post  (jude_user_t user_level, jude_object_t *, const char *path, jude_istream_t *input, jude_id_t *id);
jude_restapi_code_t jude_restapi_patch (jude_user_t user_level, jude_object_t *, const char *path, jude_istream_t *input);
jude_restapi_code_t jude_restapi_put   (jude_user_t user_level, jude_object_t *, const char *path, jude_istream_t *input);
jude_restapi_code_t jude_restapi_delete(jude_user_t user_level, jude_object_t *, const char *path);

const char *jude_restapi_code_description(jude_restapi_code_t result);
bool        jude_restapi_is_successful   (jude_restapi_code_t result); // essentially just checks the result is SUCCESS
const char* jude_restapi_get_next_path_token(const char* urlpath, char* token_found, size_t max_token_length);
const char* jude_restapi_get_next_path_token_no_strip(const char* urlpath, char* token_found, size_t max_token_length);

#ifdef __cplusplus
}
#endif

