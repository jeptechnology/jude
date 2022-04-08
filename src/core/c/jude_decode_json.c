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

#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>

#include <jude/jude_core.h>
#include <jude/core/c/jude_internal.h>

typedef struct
{
   jude_type_t type;

   union
   {
      bool b;
      const char *string;
      int64_t sint;
      uint64_t uint;
      double fnum;
   } x;
} json_atomic_t;

/*
 * Linked list structure for storing allocated
 * entries during an array decode of a pointer
 */
typedef struct repeat_array_s
{
   void *ptr;
   struct repeat_array_s *next;
} repeat_array_t;

#define IS_WHITESPACE_CHAR(ch) (((uint8_t)(ch) <= 32) || (ch & 0x80))

#define GET_NEXT_PTR(stream, ch) jude_istream_readnext_if_not_eof((stream), ch)

#define READ_NEXT_PTR(stream, ch) \
   if (!GET_NEXT_PTR(stream, ch)) return jude_istream_error(stream, "Unexpected EOF")

#define GET_NEXT(stream, ch) GET_NEXT_PTR((stream), &ch)
#define READ_NEXT(stream, ch) READ_NEXT_PTR((stream), &ch)

#define ASSERT_NEXT_TOKEN_IS(stream, ch, expected) \
   if (!json_check_next_token_is(stream, (char*)&(ch), expected)) return false;

#define APPLY_AND_CHECK_CHANGED(type, dest, source)   \
   do {                                               \
      if (*(type*)dest != (type)source)               \
      {                                               \
         *(type*)dest = (type)source;                 \
         stream->field_got_changed = true;            \
      }                                               \
   } while (0)


/********************
 * Helper functions *
 ********************/
static bool checkreturn json_skip_whitespace(jude_istream_t *stream)
{
   while (IS_WHITESPACE_CHAR(stream->last_char))
   {
      READ_NEXT(stream, stream->last_char);
   }

   return true;
}

static bool checkreturn json_check_next_token_is(jude_istream_t *stream, char *last_char, const char *expected_chars)
{
   if (*last_char == '\0' || strchr(expected_chars, *last_char) == NULL)
   {
      READ_NEXT_PTR(stream, last_char);

      if (!json_skip_whitespace(stream))
      {
         return false;
      }
      else if (strchr(expected_chars, *last_char) == NULL)
      {
         return jude_istream_error(stream, "Expecting one of %s", expected_chars);
      }
   }
   return true;
}

static bool checkreturn json_read_null(jude_istream_t *stream, char *buf, jude_size_t count, const char* errorMsg)
{
   static const char JSON_NULL[] = "null";

   bool result = false;
   uint8_t local_buf[sizeof(JSON_NULL) + 1] = { 'n' };

   ASSERT_NEXT_TOKEN_IS(stream, stream->last_char, "n")

   if (!jude_istream_read(stream, &local_buf[1], 3))
   {
      return jude_istream_error(stream, "Unexpected EOF");
   }
   else
   {
      if (strcmp(JSON_NULL, (const char *) local_buf) == 0)
      {
         // successfully got a 'null' - to indicate this we must print into the buf
         if (count > 0)
         {
            buf[0] = '\0'; // terminate the string so it is zero-length
         }
         result = true;
         stream->field_got_nulled = true;
      }
      else
      {
         return jude_istream_error(stream, errorMsg);
      }
   }

   return result;
}

static bool checkreturn json_read_string_detail(jude_istream_t *stream, char *buf, jude_size_t count, const char *error_label, bool *dest_has_changed, bool needsEndQuote)
{
   jude_size_t bytes_read = 0;
   if (dest_has_changed) *dest_has_changed = false;

   // sanitize input!
   if (count < 1)
   {
      return false;
   }

   if (stream->last_char == 0)
   {
      // on a fresh stream, we are allowed to omit the quotes - but we need to handle that separately
      if(!json_skip_whitespace(stream))
         return false;

      // If we started with a quote, then we must end with a quote
      needsEndQuote = (stream->last_char == '\"');
   }

   if (needsEndQuote)
   {
      ASSERT_NEXT_TOKEN_IS(stream, stream->last_char, "n\"");
   }
   else
   {
      // Special handling of non-quoted string start
      if (stream->last_char != buf[bytes_read])
      {
         buf[bytes_read] = stream->last_char;
         if (dest_has_changed) *dest_has_changed = true;
      }
      bytes_read++;
   }

   if (stream->last_char == 'n')
   {
      return json_read_null(stream, buf, count, "Expected 'null' or a valid string");
   }
   else
   {
      while (bytes_read < (count - 1))
      {
         bool escaped = false;
         char c;
         if (!GET_NEXT(stream, c))
         {
            if (needsEndQuote)
            {
               return jude_istream_error(stream, "Unexpected EOF");
            }

            /* end of string - terminate */
            break;
         }

         if (c == '\\')
         {
            escaped = true;
            /* escaped char - validate next char */
            c = 0;
            ASSERT_NEXT_TOKEN_IS(stream, c, "fbnrt\\\"");
            switch (c)
            {
            case 'n': c = '\n'; break;
            case 'f': c = '\f'; break;
            case 'b': c = '\b'; break;
            case 'r': c = '\r'; break;
            case 't': c = '\t'; break;
            }
         }

         if (!escaped && needsEndQuote && c == '"')
         {
            /* end of string - terminate */
            stream->last_char = buf[bytes_read];
            break;
         }
         else if (c != buf[bytes_read])
         {
            buf[bytes_read] = c;
            if (dest_has_changed) *dest_has_changed = true;
         }

         bytes_read++;
      }

      // do we need to terminate string?
      if (buf[bytes_read] != '\0')
      {
         if (dest_has_changed) *dest_has_changed = true; // it's changed length if nothing else!
         buf[bytes_read] = '\0';
      }

      if (bytes_read >= count - 1)
      {
         if (needsEndQuote)
         {
            /* check for overflow */
            READ_NEXT(stream, stream->last_char);
            if (stream->last_char != '"')
            {
               return jude_istream_error(stream, "string overflow: %s[%d]", error_label, count);
            }
         }
         else if (GET_NEXT(stream, stream->last_char))
         {
            return jude_istream_error(stream, "string overflow: %s[%d]", error_label, count);
         }
      }
   }

   return true;
}

static bool checkreturn json_read_string_relaxed(jude_istream_t *stream, char *buf, jude_size_t count, const char *error_label, bool *dest_has_changed)
{
   // If we started with a quote, then we must end with a quote
   return json_read_string_detail(stream, buf, count, error_label, dest_has_changed, stream->last_char == '"');
}

static bool checkreturn json_read_string(jude_istream_t *stream, char *buf, jude_size_t count, const char *error_label, bool *dest_has_changed)
{
   // If we started with a quote, then we must end with a quote
   return json_read_string_detail(stream, buf, count, error_label, dest_has_changed, true);
}

#define MAX_JSON_FIELD_NAME 128

static bool json_tag_match(const char *candidateTag, const char *receivedTag)
{
   if (!candidateTag || !receivedTag)
   {
      return false;
   }

   while (*candidateTag && *receivedTag)
   {
      char candidateChar = *candidateTag;
      char receivedChar = *receivedTag;
      if (receivedChar == '.')
      {
         // [JEP] Here we are relaxing the pattern match rules to allow us to 
         // to match dots with underscores.
         // e.g. JSON field name "prefix.MyField" would match "prefix_MyField"
         receivedChar = '_';
      }

      //if (tolower(receivedChar) != tolower(candidateChar))
      if (receivedChar != candidateChar)
      {
         return false;
      }
      candidateTag++;
      receivedTag++;
   }

   return !*candidateTag && !*receivedTag;
}

static bool json_decode_tag(jude_istream_t *stream, jude_object_t *object, jude_type_t *wire_type, uint32_t *tag, bool *eof)
{
   char field_name[MAX_JSON_FIELD_NAME];

   if (!json_skip_whitespace(stream))
   {
      return false;
   }
   else if (stream->last_char == '}')
   {
      /* end of object */
      *eof = true;
      return false;
   }
   else if (!json_read_string(stream, (char *) field_name, (jude_size_t) sizeof(field_name), "tag", NULL))
   {
      return false;
   }
   else
   {
      uint8_t field_index;

      ASSERT_NEXT_TOKEN_IS(stream, stream->last_char, ":");

      *tag = JUDE_TAG_UNKNOWN;

      for (field_index = 0; field_index < object->__rtti->field_count; field_index++)
      {
         if (json_tag_match(object->__rtti->field_list[field_index].label,
               field_name))
         {
            *tag = object->__rtti->field_list[field_index].tag;
            break;
         }
      }

      READ_NEXT(stream, stream->last_char);

      /*
       * UNKONWN FIELD HANDLING
       */
      if (*tag == JUDE_TAG_UNKNOWN 
          && stream->last_char == '"' // needs to be a quote to be embedded JSON
          && stream->unknown_field_callback)
      {
         char *buffer = (char *)malloc(JUDE_MAX_UNKNOWN_FIELD_LENGTH);
         
         // Step 1 - read in the data for this field
         if(json_read_string_detail(stream, buffer, JUDE_MAX_UNKNOWN_FIELD_LENGTH, field_name, NULL, true))
         {
            // Step 2 - attempt to pass it into the handler
            if (stream->unknown_field_callback(stream->state, field_name, buffer))
            {
               *tag = JUDE_TAG_UNKNOWN_BUT_HANDLED;
            }
         }

         free(buffer);
      }
   }

   return true;
}

/* Parse the input text to generate a number, and populate the result into item.
 * last_char will be populated with the last char to be read
 */
static bool checkreturn json_read_number(jude_istream_t *stream, json_atomic_t *token)
{
   uint64_t mantissa = 0;
   bool seen_digit = false;
   int8_t sign = 1;
   int8_t scale = 0;
   int subscale = 0, signsubscale = 1;
   char digit;

   token->type = JUDE_TYPE_UNSIGNED;
   token->x.uint = 0;

   if (!json_skip_whitespace(stream))
   {
      return false;
   }

   if (stream->last_char == 'n')
   {
      // it has to be a "null" for this to be valid
      return json_read_null(stream, NULL, 0, "Expected valid number or null");
   }

   digit = stream->last_char;

   /* signed? */
   if (digit == '-')
   {
      sign = -1;
      token->type = JUDE_TYPE_SIGNED;
      READ_NEXT(stream, digit);
   }

   /* eat up zeros */
   while (digit == '0')
   {
      seen_digit = true;
      if (!GET_NEXT(stream, digit))
      {
         break;
      }
   }

   /* first part */
   if (digit >= '1' && digit <= '9')
   {
      seen_digit = true;
      do
      {
         mantissa = (mantissa * 10) + (digit - '0');
         if (!GET_NEXT(stream, digit))
         {
            break;
         }
      } while (digit >= '0' && digit <= '9'); /* Number? */
   }

   if (!seen_digit)
   {
      return jude_istream_error(stream, "expected numeric value");
   }

   if (digit == '.')
   {
      /* decimal point denotes we have a floater.. */
      token->type = JUDE_TYPE_FLOAT;

      while (GET_NEXT(stream, digit) && digit >= '0' && digit <= '9')
      {
         mantissa = (mantissa * 10) + (digit - '0');
         scale--;
      }
   } /* Fractional part? */

   if (digit == 'e' || digit == 'E') /* Exponent? */
   {
      READ_NEXT(stream, digit);

      if (digit == '+')
      {
         READ_NEXT(stream, digit);
      }
      else if (digit == '-')
      {
         signsubscale = -1; /* With sign? */
         READ_NEXT(stream, digit);
      }

      while (digit >= '0' && digit <= '9')
      {
         subscale = (subscale * 10) + (digit - '0'); /* Number? */
         if (!GET_NEXT(stream, digit))
         {
            break;
         }
      }
   }

   switch (token->type)
   {
   case JUDE_TYPE_FLOAT:
      token->x.fnum = (double) sign * (double) mantissa
            * pow(10.0, (scale + subscale * signsubscale)); /* number = +/- number.fraction * 10^+/- exponent */
      break;
   case JUDE_TYPE_SIGNED:
      token->x.sint = sign * (int64_t) mantissa;
      break;
   default:
      token->x.uint = mantissa;
      break;
   }

   stream->last_char = digit;

   return true;
}

static bool checkreturn json_skip_string(jude_istream_t *stream)
{
   if (stream->last_char == '"')
   {
      READ_NEXT(stream, stream->last_char);
   }

   while (stream->last_char != '"')
   {
      READ_NEXT(stream, stream->last_char);

      if (stream->last_char == '\\')
      {
         /* eat up escaped char */
         READ_NEXT(stream, stream->last_char);
         stream->last_char = 0;
      }
   }

   return true;
}

static bool checkreturn json_skip_field(jude_istream_t *stream, jude_type_t wire_type)
{
   int32_t curlies = 0;
   int32_t squares = 0;

   while (true)
   {
      switch (stream->last_char)
      {
      case '{':
         curlies++;
         break;
      case '[':
         squares++;
         break;
      case '}':
         curlies--;
         break;
      case ']':
         squares--;
         break;
      case '"':
         if (!json_skip_string(stream))
         {
            return false;
         }
         break;
      }

      if (stream->last_char == ',' && curlies == 0 && squares == 0)
         break;

      if (curlies < 0 || squares < 0)
         break;

      READ_NEXT(stream, stream->last_char);
   }

   return true;
}

static bool checkreturn json_dec_bool(jude_istream_t *stream, const jude_field_t *field, void *dest)
{
   static const char* ExpectedBooleanErrorMsg = "Expected true, false or null";
   char buffer[6];

   (void) field; /* unused */

   if (!json_skip_whitespace(stream))
   {
      return jude_istream_reset_error_to(stream, ExpectedBooleanErrorMsg);
   }

   if (stream->last_char == 'n')
   {
      // it has to be a "null" for this to be valid
      return json_read_null(stream, NULL, 0, ExpectedBooleanErrorMsg);
   }

   buffer[0] = (char) stream->last_char;

   if (!jude_istream_read(stream, (uint8_t*) &buffer[1], 3))
   {
      return jude_istream_reset_error_to(stream, ExpectedBooleanErrorMsg);
   }

   if (strncmp(buffer, "true", 4) == 0)
   {
      APPLY_AND_CHECK_CHANGED(bool, dest, true);
   }
   else if (jude_istream_read(stream, (uint8_t*) &buffer[4], 1)
         && strncmp(buffer, "false", 5) == 0)
   {
      APPLY_AND_CHECK_CHANGED(bool, dest, false);
   }
   else
   {
      return jude_istream_reset_error_to(stream, ExpectedBooleanErrorMsg);
   }

   // skip off the end of the bool value if we can
   if (!GET_NEXT(stream, stream->last_char))
   {
      // ignore this result;
   }

   return true;
}

static bool json_apply_atomic_number_value(jude_istream_t* stream, const jude_field_t* field, void* dest, json_atomic_t token)
{
   switch (field->type)
   {
   case JUDE_TYPE_FLOAT:
      switch (token.type)
      {
      case JUDE_TYPE_FLOAT:
         switch (field->data_size)
         {
         case 4:
            APPLY_AND_CHECK_CHANGED(float, dest, token.x.fnum);
            break;
         case 8:
            APPLY_AND_CHECK_CHANGED(double, dest, token.x.fnum);
            break;
         default:
            return jude_istream_error(stream, "bad data size");
         }
         break;
      case JUDE_TYPE_SIGNED:
      case JUDE_TYPE_UNSIGNED:
         switch (field->data_size)
         {
         case 4:
            APPLY_AND_CHECK_CHANGED(float, dest, token.x.sint);
            break;
         case 8:
            APPLY_AND_CHECK_CHANGED(double, dest, token.x.sint);
            break;
         default:
            return jude_istream_error(stream, "bad data size");
         }
         break;
      default:
         return jude_istream_error(stream, "expected float value");
      }
      break;

   case JUDE_TYPE_SIGNED:
      if (   token.type == JUDE_TYPE_BOOL
          || token.type == JUDE_TYPE_FLOAT)
      {
         return jude_istream_error(stream, "expected numeric value");
      }
      else
      {
         switch (field->data_size)
         {
         case 1:
            APPLY_AND_CHECK_CHANGED(int8_t, dest, token.x.sint);
            break;
         case 2:
            APPLY_AND_CHECK_CHANGED(int16_t, dest, token.x.sint);
            break;
         case 4:
            APPLY_AND_CHECK_CHANGED(int32_t, dest, token.x.sint);
            break;
         case 8:
            APPLY_AND_CHECK_CHANGED(int64_t, dest, token.x.sint);
            break;
         default:
            return jude_istream_error(stream, "invalid data_size");
         }
      }
      break;

   default:
      if (  token.type == JUDE_TYPE_BOOL
         || token.type == JUDE_TYPE_FLOAT
         || token.type == JUDE_TYPE_SIGNED)
      {
         return jude_istream_error(stream, "expected unsigned numeric value");
      }
      else
      {
         switch (field->data_size)
         {
         case 1:
            APPLY_AND_CHECK_CHANGED(uint8_t, dest, token.x.sint);
            break;
         case 2:
            APPLY_AND_CHECK_CHANGED(uint16_t, dest, token.x.sint);
            break;
         case 4:
            APPLY_AND_CHECK_CHANGED(uint32_t, dest, token.x.sint);
            break;
         case 8:
            APPLY_AND_CHECK_CHANGED(uint64_t, dest, token.x.sint);
            break;
         default:
            return jude_istream_error(stream, "invalid data_size: %s", field->label);
         }
      }
      break;
   }

   return true;
}

static bool json_dec_number_impl(jude_istream_t *stream, const jude_field_t *field, void *dest, bool allow_string)
{
   json_atomic_t token;

   if (!json_skip_whitespace(stream))
   {
      return false;
   }

   if (stream->last_char == 'n')
   {
      return json_read_null(stream, NULL, 0, "Expected enum value or null");
   }

   if (allow_string && (stream->last_char == '"'))
   {
      READ_NEXT(stream, stream->last_char);

      // attempt to see if the last_char is a quote
      if (json_dec_number_impl(stream, field, dest, false))
      {
         if (!json_skip_whitespace(stream))
         {
            return false;
         }
         else if (stream->last_char != '"')
         {
            return jude_istream_error(stream, "Expected ending \" character");
         }
         return true;
      }
   }

   if (field->type == JUDE_TYPE_BOOL)
   {
      return json_dec_bool(stream, field, dest);
   }
   else if (!json_read_number(stream, &token))
   {
      return false;
   }

   return json_apply_atomic_number_value(stream, field, dest, token);
}

static bool json_dec_enum_bitmask(jude_istream_t *stream, const jude_field_t *field, void *dest)
{
   const jude_enum_map_t *enum_map = field->details.enum_map;
   uint32_t mask = 0;

   switch (field->data_size)
   {
   case 1:
      mask = *(uint8_t*) dest;
      break;
   case 2:
      mask = *(uint16_t*) dest;
      break;
   case 4:
      mask = *(uint32_t*) dest;
      break;
   default:
      return jude_istream_error(stream, "Unexpected bitmask data size");
   }

   if (enum_map == NULL)
   {
      return jude_istream_error(stream, "No enum map");
   }

   READ_NEXT(stream, stream->last_char);

   while (stream->last_char != '}')
   {
      char bit_field_name[MAX_JSON_FIELD_NAME];

      if (!json_skip_whitespace(stream))
      {
         return false;
      }
      else if (stream->last_char == '}')
      {
         break;
      }
      else if (!json_read_string(stream, (char *) bit_field_name, (jude_size_t) sizeof(bit_field_name), "bitfield_name", NULL))
      {
         return false;
      }
      else
      {
         bool bitIsOn = false;
         ASSERT_NEXT_TOKEN_IS(stream, stream->last_char, ":");
         READ_NEXT(stream, stream->last_char);
         if (!json_dec_bool(stream, field, &bitIsOn))
         {
            return false;
         }

         const jude_enum_value_t *bitIndex = jude_enum_find_value(enum_map, bit_field_name);
         if (bitIndex)
         {
            if (bitIsOn)
            {
               jude_bitfield_set((jude_bitfield_t) &mask, *bitIndex);
            }
            else
            {
               jude_bitfield_clear((jude_bitfield_t) &mask, *bitIndex);
            }
         }

         ASSERT_NEXT_TOKEN_IS(stream, stream->last_char, ",}");
      }
   }

   {
      json_atomic_t token;
      token.type = JUDE_TYPE_UNSIGNED;
      token.x.uint = mask;
      return json_apply_atomic_number_value(stream, field, dest, token);
   }
}

static bool json_dec_enum_or_bitmask(jude_istream_t *stream, const jude_field_t *field, void *dest)
{
   if (!json_skip_whitespace(stream))
   {
      return false;
   }

   if (!field->details.enum_map)
   {
      return false;
   }

   // Bitmask handling
   if (JUDE_TYPE_BITMASK == field->type && stream->last_char == '{')
   {
      return json_dec_enum_bitmask(stream, field, dest);
   }

   // Handle nulls
   if (stream->last_char == 'n')
   {
      return json_read_null(stream, NULL, 0, "Expected enum value or null");
   }

   if (stream->last_char >= '0' && stream->last_char <= '9')
   {
      // must be a regular numeric enum, decode as a number and check it's valid
      json_atomic_t number;
      if (  !json_read_number(stream, &number)
         || (number.type != JUDE_TYPE_UNSIGNED))
      {
         return jude_istream_error(stream, "expected unsigned numeric value");
      }

      if (jude_enum_contains_value(field->details.enum_map, (int32_t)number.x.sint))
      {
         number.type = JUDE_TYPE_UNSIGNED;
         return json_apply_atomic_number_value(stream, field, dest, number);
      }

      return jude_istream_error(stream, "'%lld' not a value in this enum", number.x.sint);
   }
   else if (field->details.enum_map)
   {
      // decode the string for this enum and find it
      char enum_string[MAX_JSON_FIELD_NAME];

      if (!json_read_string_relaxed(stream, enum_string, sizeof(enum_string), field->label, NULL))
         return false;

      const jude_enum_value_t* value = jude_enum_find_value(field->details.enum_map, enum_string);
      if (value)
      {
         json_atomic_t token;
         token.type = JUDE_TYPE_UNSIGNED;
         token.x.uint = *value;
         return json_apply_atomic_number_value(stream, field, dest, token);
      }

      return jude_istream_error(stream, "'%s' not in this enum", enum_string);
   }

   return false;
}

/* Field decoders */
static bool checkreturn json_dec_number(jude_istream_t *stream, const jude_field_t *field, void *dest)
{
   if (field->type == JUDE_TYPE_ENUM || field->type == JUDE_TYPE_BITMASK)
      return json_dec_enum_or_bitmask(stream, field, dest);
   else
      return json_dec_number_impl(stream, field, dest, true);
}

static bool checkreturn json_dec_bytes(jude_istream_t *stream, const jude_field_t *field, void *dest)
{
   ASSERT_NEXT_TOKEN_IS(stream, stream->last_char, "n\"")

   jude_bytes_array_t *x = (jude_bytes_array_t*) dest;
   jude_size_t max_length = field->data_size - offsetof(jude_bytes_array_t, bytes);

   if (stream->last_char == 'n')
   {
      // we are expecting an unquoted 'null' if we didn't get a quote
      x->size = 0;
      return json_read_null(stream, (char *) x->bytes, max_length, "Expected base64 string or null");
   }
   else
   {
      x->size = json_base64_read(stream, x->bytes, max_length);
   }

   /* Check length, noting the space taken by the jude_size_t object. */
   if (x->size > max_length)
      return jude_istream_error(stream, "bytes overflow: %s", field->label);

   stream->last_char = '"';
   return true;
}

static bool checkreturn json_dec_string(jude_istream_t *stream, const jude_field_t *field, void *dest)
{
   return json_read_string(stream, (char*) (dest), field->data_size, field->label, &stream->field_got_changed);
}

static bool json_context_substream_open(jude_context_type_t type, jude_istream_t *stream, jude_istream_t *substream)
{
   switch (type)
   {
   case JUDE_CONTEXT_REPEATED:
      /* Expect an opening '[' */
      ASSERT_NEXT_TOKEN_IS(stream, stream->last_char, "[");
      READ_NEXT(stream, stream->last_char);
      break;

   case JUDE_CONTEXT_MESSAGE:
      /* Expect an opening '{' */
      ASSERT_NEXT_TOKEN_IS(stream, stream->last_char, "{");
      READ_NEXT(stream, stream->last_char);
      break;

   case JUDE_CONTEXT_SUBMESSAGE:
      /* Note: The SUB message opening brace is covered by the MESSAGE opening */

   case JUDE_CONTEXT_STRING:
   case JUDE_CONTEXT_DELIMITED:
      break;
   }

   if (!substream)
   {
      return false;
   }
   memcpy(substream, stream, sizeof(jude_istream_t));
   substream->bytes_read = 0;

   return true;
}

static bool json_context_substream_is_eof(jude_context_type_t type, jude_istream_t *stream)
{
   if (stream->bytes_left == 0)
   {
      return true;
   }

   switch (type)
   {
   case JUDE_CONTEXT_REPEATED:
      if (!json_skip_whitespace(stream))
      {
         /* error */
         return true;
      }
      else if (stream->last_char == ']')
      {
         // mark substream as exhausted
         stream->bytes_left = 0;
         /* end of list */
         return true;
      }
      break;
   case JUDE_CONTEXT_STRING:
      // nothing to do
      break;
   case JUDE_CONTEXT_MESSAGE:
   case JUDE_CONTEXT_SUBMESSAGE:
      if (!json_skip_whitespace(stream))
      {
         /* error */
         return true;
      }
      else if (stream->last_char == '}')
      {
         // mark substream as exhausted
         stream->bytes_left = 0;
         /* end of object */
         return true;
      }
      break;
   case JUDE_CONTEXT_DELIMITED:
      break;
   }

   return false;
}

static bool json_context_substream_next_element(jude_context_type_t type, jude_istream_t *substream)
{
   if (substream->bytes_left > 0)
   {
      switch (type)
      {
      case JUDE_CONTEXT_REPEATED:
         ASSERT_NEXT_TOKEN_IS(substream, substream->last_char, ",]");
         break;
      case JUDE_CONTEXT_STRING:
         // nothing to do
         break;
      case JUDE_CONTEXT_MESSAGE:
      case JUDE_CONTEXT_SUBMESSAGE:
         ASSERT_NEXT_TOKEN_IS(substream, substream->last_char, ",}");
         break;
      case JUDE_CONTEXT_DELIMITED:
         break;
      }

      /*
       * If we are going to next element, we should skip over the comma
       */
      if (substream->last_char == ',')
         READ_NEXT(substream, substream->last_char);

      return true;
   }
   return false;
}

static bool json_context_substream_close(jude_context_type_t type, jude_istream_t *stream, jude_istream_t *substream)
{
   switch (type)
   {
   case JUDE_CONTEXT_REPEATED:
   case JUDE_CONTEXT_STRING:
   case JUDE_CONTEXT_MESSAGE:
   case JUDE_CONTEXT_SUBMESSAGE:
   case JUDE_CONTEXT_DELIMITED:
      /*
       * A context will have created a substream
       */
      stream->state = substream->state;
      jude_buffer_transfer(&stream->buffer, &substream->buffer);
      stream->bytes_read += substream->bytes_read;
      stream->bytes_left -= substream->bytes_read;
      stream->has_error = substream->has_error;
      break;
   }

   return true;
}

static bool json_is_packed(const jude_field_t *field, jude_type_t wire_type)
{
   // JSON always packed!
   return true;
}

/* --- JSON decoding transport layer --- */
static const jude_decode_transport_t transport =
{
   .dec_bool     = &json_dec_number,
   .dec_signed   = &json_dec_number,
   .dec_unsigned = &json_dec_number,
   .dec_float    = &json_dec_number,
   .dec_enum     = &json_dec_number,
   .dec_bitmask  = &json_dec_number,
   .dec_string   = &json_dec_string,
   .dec_bytes    = &json_dec_bytes,

   .decode_tag = &json_decode_tag,
   .is_packed = &json_is_packed,
   .skip_field = &json_skip_field,

   .context.open = json_context_substream_open,
   .context.is_eof = json_context_substream_is_eof,
   .context.next_element = json_context_substream_next_element,
   .context.close = json_context_substream_close
};

const jude_decode_transport_t *jude_decode_transport_json = &transport;
