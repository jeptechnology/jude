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

#include <jude/restapi/jude_browser.h>
#include <jude/restapi/jude_rest_api.h>

static jude_enum_map_t restapi_result_map[] =
{
   JUDE_ENUM_MAP_ENTRY(OK                , 200, "OK"),
   JUDE_ENUM_MAP_ENTRY(Created           , 201, "Created, OK"),
   JUDE_ENUM_MAP_ENTRY(No_Content        , 204, "No Content, OK"),

   JUDE_ENUM_MAP_ENTRY(Bad_Request       , 400, "Bad Request"),
   JUDE_ENUM_MAP_ENTRY(Unauthorized      , 401, "Unauthorized"),
   JUDE_ENUM_MAP_ENTRY(Forbidden         , 403, "Forbidden"),
   JUDE_ENUM_MAP_ENTRY(Not_Found         , 404, "Not Found"),
   JUDE_ENUM_MAP_ENTRY(Method_Not_Allowed, 405, "Method Not Allowed"),
   JUDE_ENUM_MAP_ENTRY(Conflict          , 409, "Conflict"),

   JUDE_ENUM_MAP_ENTRY(Internal_Server_Error, 500, "Internal Server Error" ),

   JUDE_ENUM_MAP_END
};

const char *jude_restapi_code_description(jude_restapi_code_t result)
{
   return jude_enum_find_description(restapi_result_map, result);
}

bool jude_restapi_is_successful(jude_restapi_code_t result)
{
   return result >= 200 && result < 300; // essentially just checks the result is 2xx
}

static bool path_token_is_valid(const char *path_token)
{
   return path_token && (path_token[0] != 0);
}

static void invalidate_path_token(char *path_token)
{
   if (path_token)
   {
      path_token[0] = 0;
   }
}

const char* jude_restapi_get_next_path_token_no_strip(const char* fullpath, char* token, size_t max_token_length)
{
   invalidate_path_token(token);

   if (fullpath == NULL)
   {
      return "";
   }

   // skip prefixed '/' characters
   while (*fullpath == '/')
   {
      fullpath++;
   }

   size_t length_of_token = 0;
   const char *start_of_next_token = strchr(fullpath, '/');
   if (start_of_next_token)
   {
      length_of_token = start_of_next_token - fullpath;
   }
   else
   {
      length_of_token = strlen(fullpath);
      start_of_next_token = "";
   }

   if (length_of_token >= max_token_length)
   {
      return ""; // fullpath token is too long
   }

   strncpy(token, fullpath, length_of_token); 
   token[length_of_token] = 0;

   return start_of_next_token;
}

const char* jude_restapi_get_next_path_token(const char* fullpath, char* token, size_t max_token_length)
{
   const char* start_of_next_token = jude_restapi_get_next_path_token_no_strip(fullpath, token, max_token_length);
   
   while (*start_of_next_token == '/')
   {
      start_of_next_token++;
   }

   return start_of_next_token;
}


static jude_browser_t browse_to_path(jude_object_t *root, const char *fullpath, jude_user_t access_level, jude_permission_t requiredPermissions)
{
   char token[MAX_REST_API_URL_PATH_TOKEN];

   jude_browser_t browser = jude_browser_init(root, access_level);
   fullpath = jude_restapi_get_next_path_token(fullpath, token, sizeof(token));

   while (jude_browser_is_valid(&browser) && path_token_is_valid(token))
   {
      jude_browser_follow_path(&browser, token, requiredPermissions);
      fullpath = jude_restapi_get_next_path_token(fullpath, token, sizeof(token));
   }

   return browser;
}

static jude_restapi_code_t get_field(jude_browser_t *browser, jude_ostream_t *output)
{
   output->suppress_first_tag = true;
   
   if (!jude_iterator_is_touched(&browser->x.field.iterator))
   {
      // Field is not set!
      return jude_rest_Not_Found;
   }

   if (jude_encode_single_value(output, &browser->x.field.iterator))
   {
      return jude_rest_OK;
   }

   return jude_rest_Internal_Server_Error;
}

static jude_restapi_code_t get_array(jude_browser_t *browser, jude_ostream_t *output)
{
   if (!jude_iterator_is_touched(&browser->x.array))
   {
      // Field is not set!
      return jude_rest_Not_Found;
   }

   output->suppress_first_tag = true;
   if (jude_encode_single_value(output, &browser->x.array))
   {
      return jude_rest_OK;
   }
   return jude_rest_Internal_Server_Error;
}

static jude_restapi_code_t get_object(jude_browser_t *browser, jude_ostream_t *output)
{
   if (jude_encode(output, browser->x.object))
   {
      return jude_rest_OK;
   }
   return jude_rest_Internal_Server_Error;
}

static jude_restapi_code_t delete_object(jude_browser_t* browser)
{
   if (jude_object_is_top_level(browser->x.object))
   {
      return jude_rest_Forbidden; // cannot delete root object
   }

   // clear this object
   jude_object_clear_touch_markers(browser->x.object);

   jude_object_t* parent = jude_object_get_parent(browser->x.object);
   jude_size_t child_index = jude_object_get_child_index(browser->x.object);

   // if its not inside an array, we can remove this object from its parent
   if (!jude_field_is_array(&(parent->__rtti->field_list[child_index])))
   {
      jude_filter_set_touched(parent->__mask, child_index, false);
      jude_filter_set_changed(parent->__mask, child_index, true);
   }

   return jude_rest_OK;
}

static jude_restapi_code_t delete_array(jude_browser_t* browser)
{
   jude_iterator_t* iterator = &browser->x.array;
   jude_size_t* count = jude_iterator_get_count_reference(iterator);
   if (count == NULL)
   {
      return jude_rest_Internal_Server_Error;
   }
   *count = 0;
   jude_iterator_clear_touched(iterator);
   jude_iterator_set_changed(iterator);
   return jude_rest_OK;
}

static jude_restapi_code_t delete_field(jude_browser_t* browser)
{
   jude_iterator_t* iterator = &browser->x.field.iterator;

   jude_size_t* count = jude_iterator_get_count_reference(iterator);
   if (count)
   {
      // delete a field inside an array -> we just delete this element
      if (jude_object_remove_value_from_array(iterator->object, iterator->field_index, browser->x.field.array_index))
      {
         return jude_rest_OK;
      }
   }
   else
   {
      // singleton field - just remove this
      if (jude_iterator_is_touched(iterator))
      {
         jude_iterator_clear_touched(iterator);
         jude_iterator_set_changed(iterator);
         return jude_rest_OK;
      }
   }

   return jude_rest_Not_Found;

}

static jude_restapi_code_t post_field(jude_browser_t* browser, jude_istream_t* input, jude_id_t *id_or_index)
{
   // we can only "post" (i.e. create) inside an array...
   return jude_rest_Method_Not_Allowed;
}

static jude_restapi_code_t post_new_object(jude_browser_t* browser, jude_istream_t* input, jude_id_t* id)
{
   bool array_was_changed = jude_iterator_is_changed(&browser->x.array);
   jude_object_t* created_obj = jude_object_add_subresource(browser->x.array.object, browser->x.array.field_index, JUDE_AUTO_ID);
   if (created_obj != NULL)
   {
      if (jude_decode_noinit(input, created_obj))
      {
         if (id)
         {
            *id = created_obj->m_id;
         }
         return jude_rest_OK;
      }
      else
      {
         // decoding went wrong... remove the new object
         jude_object_remove_subresource(browser->x.array.object, browser->x.array.field_index, created_obj->m_id);
         if (!array_was_changed)
         {
            // field wasn't "changed" to begin with so mark it as "unchanged" as we removed our new object
            jude_object_mark_field_changed(browser->x.array.object, browser->x.array.field_index, false);
         }
      }
   }
   return jude_rest_Bad_Request;
}

static jude_restapi_code_t post_new_element(jude_browser_t* browser, jude_istream_t* input, jude_index_t* new_index)
{
   jude_iterator_t* iterator = &browser->x.array;
   jude_size_t* count = jude_iterator_get_count_reference(iterator);
   if (count == NULL)
   {
      return jude_rest_Internal_Server_Error;
   }

   if (!jude_object_insert_value_into_array(iterator->object, iterator->field_index, *count, NULL))
   {
      return jude_rest_Bad_Request;
   }

   jude_index_t last_index = *count - 1;
   if (new_index)
   {
      *new_index = last_index; // our new index is now the value of count
   }

   if (decode_field_element(input, &browser->x.array, last_index))
   {
      return jude_rest_OK;
   }

   return jude_rest_Bad_Request;
}

static jude_restapi_code_t post_array(jude_browser_t* browser, jude_istream_t* input, jude_id_t* id_or_index)
{
   if (jude_iterator_is_subresource(&browser->x.array))
   {
      return post_new_object(browser, input, id_or_index);
   }
   else
   {
      jude_index_t index;
      jude_restapi_code_t result = post_new_element(browser, input, &index);
      *id_or_index = index;
      return result;
   }
}

static jude_restapi_code_t post_object(jude_browser_t* browser, jude_istream_t* input, jude_id_t* id_or_index)
{
   // we can only "post" (i.e. create) inside an array...
   return jude_rest_Method_Not_Allowed;
}

static jude_restapi_code_t patch_field(jude_browser_t* browser, jude_istream_t* input)
{
   bool field_was_touched = jude_iterator_is_touched(&browser->x.field.iterator);

   // // Special case for Patching an enum
   // if (browser->x.field.iterator.current_field->type == JUDE_TYPE_ENUM && input->last_char != '"')
   // {
   //    input->last_char = '"';
   // }

   if (decode_field_element(input, &browser->x.field.iterator, browser->x.field.array_index))
   {
      if (input->field_got_nulled)
      {
         jude_iterator_clear_touched(&browser->x.field.iterator);
         if (field_was_touched)
         {
            jude_iterator_set_changed(&browser->x.field.iterator);
         }
      }
      else
      {
         jude_iterator_set_touched(&browser->x.field.iterator);
         if (input->field_got_changed || !field_was_touched)
         {
            jude_iterator_set_changed(&browser->x.field.iterator);
         }
      }

      return jude_rest_OK;
   }

   return jude_rest_Bad_Request;
}

static jude_restapi_code_t patch_array(jude_browser_t* browser, jude_istream_t* input)
{
   if (jude_decode_single_field(input, &browser->x.array))
   {
      //if (input->field_got_nulled)
      //{
      //   return delete_field(browser);
      //}
      //jude_iterator_set_touched(&browser->x.array);
      //jude_iterator_set_changed(&browser->x.array, input->field_got_changed);

      return jude_rest_OK;
   }

   return jude_rest_Bad_Request;
}

static jude_restapi_code_t patch_object(jude_browser_t* browser, jude_istream_t* input)
{
   // clear everything but the ID
   jude_id_t id = browser->x.object->m_id;
   bool idNeedsReinstating = jude_filter_is_touched(browser->x.object->__mask, 0);
   bool decodeResult = jude_decode_noinit(input, browser->x.object);

   // reinstate the id of an object - this is not "patch-able"
   if (idNeedsReinstating)
   {
      browser->x.object->m_id = id;
      jude_filter_set_touched(browser->x.object->__mask, 0, true);
      jude_filter_set_changed(browser->x.object->__mask, 0, false);
   }

   return decodeResult ? jude_rest_OK : jude_rest_Bad_Request;
}

static jude_restapi_code_t put_field(jude_browser_t* browser, jude_istream_t* input)
{
   // put will clear the field first
   jude_iterator_clear_touched(&browser->x.field.iterator);
   jude_restapi_code_t result = patch_field(browser, input);
   if (result == jude_rest_Not_Found && input->field_got_nulled)
   {
      result = jude_rest_OK; // field was nulled by us first so no wonder it couldn't find it do delete it
   }
   return result;
}

static jude_restapi_code_t put_array(jude_browser_t* browser, jude_istream_t* input)
{
   // put will clear the field first
   jude_iterator_clear_touched(&browser->x.array);
   return patch_array(browser, input);
}

static void clear_all_fields_except_id(jude_object_t *object)
{
   jude_id_t id = object->m_id;
   bool idNeedsReinstating = jude_filter_is_touched(object->__mask, 0);
   jude_object_clear_all(object);

   if (idNeedsReinstating)
   {
      object->m_id = id;
      jude_filter_set_touched(object->__mask, 0, true);
      jude_filter_set_changed(object->__mask, 0, false);
   }
}

static jude_restapi_code_t put_object(jude_browser_t* browser, jude_istream_t* input)
{
   clear_all_fields_except_id(browser->x.object);
   // then patch    
   return patch_object(browser, input);
}

jude_restapi_code_t jude_restapi_get(jude_user_t user, const jude_object_t* root, const char* fullpath, jude_ostream_t* output)
{
   jude_browser_t browser = browse_to_path(jude_remove_const(root), fullpath, user, jude_permission_Read);
   if (!jude_browser_is_valid(&browser))
   {
      return browser.code;
   }

   switch (browser.type)
   {
   case jude_browser_OBJECT:
      return get_object(&browser, output);
      break;

   case jude_browser_FIELD:
      return get_field(&browser, output);
      break;

   case jude_browser_ARRAY:
      return get_array(&browser, output);
      break;

   default:
      return jude_rest_Internal_Server_Error;
   }
}

jude_restapi_code_t jude_restapi_post(jude_user_t user, jude_object_t *root, const char *fullpath, jude_istream_t *input, jude_id_t *id)
{
   jude_browser_t browser = browse_to_path(root, fullpath, user, jude_permission_Write);
   if (!jude_browser_is_valid(&browser))
   {
      return browser.code;
   }

   switch (browser.type)
   {
   case jude_browser_OBJECT:
      return post_object(&browser, input, id);
      break;

   case jude_browser_FIELD:
      return post_field(&browser, input, id);
      break;

   case jude_browser_ARRAY:
      return post_array(&browser, input, id);
      break;

   default:
      return jude_rest_Internal_Server_Error;
   }
}

jude_restapi_code_t jude_restapi_patch(jude_user_t user, jude_object_t *root, const char *fullpath, jude_istream_t *input)
{
   jude_browser_t browser = browse_to_path(root, fullpath, user, jude_permission_Write);
   if (!jude_browser_is_valid(&browser))
   {
      return browser.code;
   }

   switch (browser.type)
   {
   case jude_browser_OBJECT:
      return patch_object(&browser, input);
      break;

   case jude_browser_FIELD:
      return patch_field(&browser, input);
      break;

   case jude_browser_ARRAY:
      return patch_array(&browser, input);
      break;

   default:
      return jude_rest_Internal_Server_Error;
   }
}

jude_restapi_code_t jude_restapi_put(jude_user_t user, jude_object_t *root, const char *fullpath, jude_istream_t *input)
{
   jude_browser_t browser = browse_to_path(root, fullpath, user, jude_permission_Write);
   if (!jude_browser_is_valid(&browser))
   {
      return browser.code;
   }

   switch (browser.type)
   {
   case jude_browser_OBJECT:
      return put_object(&browser, input);
      break;

   case jude_browser_FIELD:
      return put_field(&browser, input);
      break;

   case jude_browser_ARRAY:
      return put_array(&browser, input);
      break;

   default:
      return jude_rest_Internal_Server_Error;
   }
}

jude_restapi_code_t jude_restapi_delete(jude_user_t user, jude_object_t *root, const char *fullpath)
{
   jude_browser_t browser = browse_to_path(root, fullpath, user, jude_permission_Write);
   if (!jude_browser_is_valid(&browser))
   {
      return browser.code;
   }

   switch (browser.type)
   {
   case jude_browser_OBJECT:
      return delete_object(&browser);
      break;

   case jude_browser_FIELD:
      return delete_field(&browser);
      break;

   case jude_browser_ARRAY:
      return delete_array(&browser);
      break;

   default:
      return jude_rest_Internal_Server_Error;
   }
}

