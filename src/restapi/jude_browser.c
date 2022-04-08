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

#include <stdlib.h>

#include <jude/restapi/jude_browser.h>
#include <jude/core/c/jude_object.h>

jude_browser_t jude_browser_init(jude_object_t *root, jude_user_t access_level)
{
   jude_browser_t result;

   if (root)
   {
      result.type = jude_browser_OBJECT;
      result.x.object = root;
      result.code = jude_rest_OK;
   }
   else
   {
      result.type = jude_browser_INVALID;
      result.code = jude_rest_Not_Found;
   }

   result.access_level = access_level;
   result.remaining_suffix = NULL;

   return result;
}

bool jude_browser_is_valid(jude_browser_t *browser)
{
   return browser && browser->type != jude_browser_INVALID;
}

bool jude_browser_is_object(jude_browser_t *browser)
{
   return browser && browser->type == jude_browser_OBJECT;
}

bool jude_browser_is_array(jude_browser_t *browser)
{
   return browser && browser->type == jude_browser_ARRAY;
}

bool jude_browser_is_field(jude_browser_t *browser)
{
   return browser && browser->type == jude_browser_FIELD;
}

jude_object_t* jude_browser_get_object(jude_browser_t *browser)
{
   if (!jude_browser_is_object(browser)) return NULL;
   return browser->x.object;
}

jude_iterator_t* jude_browser_get_array(jude_browser_t *browser)
{
   if (!jude_browser_is_array(browser)) return NULL;
   return &browser->x.array;
}

const jude_field_t* jude_browser_get_field_type(jude_browser_t *browser)
{
   if (!jude_browser_is_field(browser)) return NULL;
   return browser->x.field.iterator.current_field;
}

void* jude_browser_get_field_data(jude_browser_t *browser)
{
   if (!jude_browser_is_field(browser)) return NULL;
   return jude_iterator_get_data(&browser->x.field.iterator, browser->x.field.array_index);
}

static bool invalidate_browser(jude_browser_t *browser, jude_restapi_code_t error_code)
{
   if (browser)
   {
      browser->type = jude_browser_INVALID;
      browser->code = error_code;
   }
   return false; // always false
}

static bool browse_into_object(jude_browser_t *browser, const char *field_name, jude_permission_t required_permission)
{
   jude_iterator_t iter = jude_iterator_begin(browser->x.object);
   if (!jude_iterator_find_by_name(&iter, field_name))
   {
      return invalidate_browser(browser, jude_rest_Not_Found);
   }

   if (  required_permission == jude_permission_Read
      && !jude_field_is_readable(iter.current_field, browser->access_level))
   {
      return invalidate_browser(browser, jude_rest_Forbidden);
   }

   if (  required_permission == jude_permission_Write
      && !jude_field_is_writable(iter.current_field, browser->access_level))
   {
      return invalidate_browser(browser, jude_rest_Forbidden);
   }

   if (jude_iterator_is_array(&iter))
   {
      browser->type = jude_browser_ARRAY;
      browser->x.array = iter;
   }
   else if (jude_iterator_is_subresource(&iter))
   {
      browser->type = jude_browser_OBJECT;
      browser->x.object = (jude_object_t*)jude_iterator_get_data(&iter, 0);
   }
   else
   {
      browser->type = jude_browser_FIELD;
      browser->x.field.iterator = iter;
      browser->x.field.array_index = 0;
   }

   return true;
}

// Allow search criteria on browse token 
static bool browse_into_array_of_objects_via_search(jude_browser_t *browser, const char *path_token)
{
   char key[64];

   if (!jude_field_is_object(browser->x.array.current_field))
   {
      return false;
   }

   char *value = strchr(path_token, '=');
   if (value == NULL)
   {
      return false; // no '=' after 'key'
   }
   
   size_t key_length = value - path_token;
   if (key_length >= sizeof(key))
   {
      return false; // key too long
   }

   strncpy(key, path_token, key_length);
   key[key_length] = 0;

   value++;
   if (value[0] == '\0')
   {
      return false; // no value
   }

   const jude_field_t *key_field = jude_rtti_find_field(browser->x.array.current_field->details.sub_rtti, key);
   if (!key_field)
   {
      return false;
   }

   jude_size_t count = jude_iterator_get_count(&browser->x.array);
   for (unsigned array_index = 0; array_index < count; array_index++)
   {
      jude_object_t* subresource = (jude_object_t*)jude_iterator_get_data(&browser->x.array, array_index);
      if (!jude_filter_is_touched(subresource->__mask, 0))
      {
         continue;
      }

      jude_iterator_t subresource_iter = jude_iterator_begin(subresource);
      jude_iterator_go_to_index(&subresource_iter, key_field->index);
      const char *subresource_value = jude_get_string(subresource_iter.current_field, subresource_iter.details.data, 0);
      if (!subresource_value || (strcmp(subresource_value, value) != 0))
      {
         continue;
      }

      browser->type = jude_browser_OBJECT;
      browser->x.object = subresource;
      return true;
   }

   return false;
}

static bool browse_into_array(jude_browser_t *browser, const char *path_token)
{
   if (path_token[0] == '*')
   {
      // Allow search criteria via: "/*key=value/" on the path token
      return browse_into_array_of_objects_via_search(browser, &path_token[1]);
   }

   char *ptr;
   jude_id_t id_or_index = (jude_id_t)strtoll(path_token, &ptr, 10);
   if (ptr == path_token)
   {
      return invalidate_browser(browser, jude_rest_Bad_Request); // bad number
   }

   if (jude_field_is_object(browser->x.array.current_field))
   {
	  // this is an array of sub-objects - browse into array via id
	  jude_object_t *subresource = jude_object_find_subresource(browser->x.array.object, browser->x.array.field_index, id_or_index);
	  if (subresource == NULL)
	  {
        return invalidate_browser(browser, jude_rest_Not_Found);
	  }

	  browser->type = jude_browser_OBJECT;
	  browser->x.object = subresource;

	  return true;
   }

   // this is an array of fields - browse into array via index
   jude_index_t index = (jude_index_t)id_or_index;
   if (jude_iterator_get_count(&browser->x.array) <= index)
   {
      return invalidate_browser(browser, jude_rest_Not_Found); // index out of range
   }

   browser->type = jude_browser_FIELD;
   browser->x.field.iterator = browser->x.array;
   browser->x.field.array_index = index;

   return true;
}

bool jude_browser_follow_path(jude_browser_t *browser, const char *path_token, jude_permission_t required_permission)
{
   if (path_token == NULL)
   {
      return invalidate_browser(browser, jude_rest_Bad_Request);
   }

   if (!jude_browser_is_valid(browser))
   {
      return false; // browser is already invalid
   }

   if (jude_browser_is_field(browser))
   {
      return invalidate_browser(browser, jude_rest_Bad_Request); // a field is a leaf node so we can go no further!
   }
   
   if (jude_browser_is_object(browser))
   {
	   return browse_into_object(browser, path_token, required_permission);
   }

   return browse_into_array(browser, path_token);
}

jude_browser_t jude_browser_try_path(jude_object_t* root, const char* fullpath, jude_user_t access_level, jude_permission_t required_permission)
{
   jude_browser_t current = jude_browser_init(root, access_level);
   current.remaining_suffix = fullpath ? fullpath : "";

   while (jude_browser_is_valid(&current) && current.remaining_suffix[0] != '\0')
   {
      char token[MAX_REST_API_URL_PATH_TOKEN];

      jude_browser_t next;
      memcpy(&next, &current, sizeof(next));

      next.remaining_suffix = jude_restapi_get_next_path_token(current.remaining_suffix, token, sizeof(token));

      if (jude_browser_is_field(&current))
      {
         // a field is a leaf node so we can go no further!
         invalidate_browser(&current, jude_rest_Not_Found);
         break;
      }
      else if (jude_browser_is_object(&current))
      {
         if (!browse_into_object(&next, token, required_permission))
         {
            break;
         }
      }
      else if (!browse_into_array(&next, token))
      {
         break;
      }

      // Successful traversal, copy next into current and try again...
      memcpy(&current, &next, sizeof(next));
   }

   return current;
}

