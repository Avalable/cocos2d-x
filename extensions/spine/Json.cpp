/*
 Copyright (c) 2009, Dave Gamble
 Copyright (c) 2013, Esoteric Software

 Permission is hereby granted, dispose of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

/* Json */
/* JSON parser in C. */

#ifndef _DEFAULT_SOURCE
/* Bring strings.h definitions into string.h, where appropriate */
#define _DEFAULT_SOURCE
#endif

#ifndef _BSD_SOURCE
/* Bring strings.h definitions into string.h, where appropriate */
#define _BSD_SOURCE
#endif

#include "Json.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h> /* strtod (C89), strtof (C99) */
#include <string.h> /* strcasecmp (4.4BSD - compatibility), _stricmp (_WIN32) */
#include <spine/extension.h>
#include "JsonAllocator.h"

#ifndef SPINE_JSON_DEBUG
/* Define this to do extra NULL and expected-character checking */
#define SPINE_JSON_DEBUG 0
#endif

namespace cocos2d { namespace extension
{
    static const char* ep;

    const char* Json_getError (void) {
        return ep;
    }

    static int Json_strcasecmp (const char* s1, const char* s2) {
        /* TODO we may be able to elide these NULL checks if we can prove
         * the graph and input (only callsite is Json_getItem) should not have NULLs
         */
        if (s1 && s2) {
#if defined(_WIN32)
    return _stricmp(s1, s2);
#else
            return strcasecmp( s1, s2 );
#endif
        } else {
            if (s1 < s2)
                return -1; /* s1 is null, s2 is not */
            else if (s1 == s2)
                return 0; /* both are null */
            else
                return 1; /* s2 is nul	s1 is not */
        }
    }

/* Internal constructor. */
    static Json *Json_new(JsonAllocator& allocator) {
        Json* json = (Json*)allocator.allocate(sizeof(Json));//(Json*)MALLOC(Json, 1);
        memset(json, 0, sizeof(Json));
        return json;
    }

/* Delete a Json structure. */
    void Json_dispose(Json *c) {

        /*
        Json *next;
        while (c) {
            next = c->next;
            if (c->child) Json_dispose(c->child, allocator);
            if (c->valuestring) FREE(c->valuestring);
            if (c->name) FREE(c->name);
            FREE(c);
            c = next;
        }
        */
    }

/* Parse the input text to generate a number, and populate the result into item. */
    static const char* parse_number (Json *item, const char* num) {
        char * endptr;
        float n;

        /* Using strtod and strtof is slightly more permissive than RFC4627,
         * accepting for example hex-encoded floating point, but either
         * is often leagues faster than any manual implementation.
         *
         * We also already know that this starts with [-0-9] from parse_value.
         */
#if __STDC_VERSION__ >= 199901L
        n = strtof(num, &endptr);
#else
n = (float)strtod( num, &endptr );
#endif
        
        char * longEndptr;
        long l;
        l = strtol(num, &longEndptr, 10);
        
        /* ignore errno's ERANGE, which returns +/-HUGE_VAL */
        /* n is 0 on any other error */

        if (endptr != num) {
            /* Parse success, number found. */
            item->valuefloat = n;
            item->valueint = (int)n;
            item->valuelong = (long)l;
            item->type = Json_Number;
            return endptr;
        } else {
            /* Parse failure, ep is set. */
            ep = num;
            return 0;
        }
    }

/* Parse the input text into an unescaped cstring, and populate item. */
    static const unsigned char firstByteMark[7] = {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
    static const char* parse_string(Json *item, const char* str, JsonAllocator& allocator) {
        const char* ptr = str + 1;
        char* ptr2;
        char* out;
        int len = 0;
        unsigned uc, uc2;
        if (*str != '\"') { /* TODO: don't need this check when called from parse_value, but do need from parse_object */
            ep = str;
            return 0;
        } /* not a string! */

        while (*ptr != '\"' && *ptr && ++len)
            if (*ptr++ == '\\') ptr++; /* Skip escaped quotes. */

        out = (char*)allocator.allocate(sizeof(char)*(len + 1));//MALLOC(char, len + 1); /* The length needed for the string, roughly. */
        if (!out) return 0;

        ptr = str + 1;
        ptr2 = out;
        while (*ptr != '\"' && *ptr) {
            if (*ptr != '\\')
                *ptr2++ = *ptr++;
            else {
                ptr++;
                switch (*ptr) {
                    case 'b':
                        *ptr2++ = '\b';
                        break;
                    case 'f':
                        *ptr2++ = '\f';
                        break;
                    case 'n':
                        *ptr2++ = '\n';
                        break;
                    case 'r':
                        *ptr2++ = '\r';
                        break;
                    case 't':
                        *ptr2++ = '\t';
                        break;
                    case 'u': /* transcode utf16 to utf8. */
                        sscanf(ptr + 1, "%4x", &uc);
                        ptr += 4; /* get the unicode char. */

                        if ((uc >= 0xDC00 && uc <= 0xDFFF) || uc == 0) break; /* check for invalid.	*/

                        /* TODO provide an option to ignore surrogates, use unicode replacement character? */
                        if (uc >= 0xD800 && uc <= 0xDBFF) /* UTF16 surrogate pairs.	*/
                        {
                            if (ptr[1] != '\\' || ptr[2] != 'u') break; /* missing second-half of surrogate.	*/
                            sscanf(ptr + 3, "%4x", &uc2);
                            ptr += 6;
                            if (uc2 < 0xDC00 || uc2 > 0xDFFF) break; /* invalid second-half of surrogate.	*/
                            uc = 0x10000 + (((uc & 0x3FF) << 10) | (uc2 & 0x3FF));
                        }

                        len = 4;
                        if (uc < 0x80)
                            len = 1;
                        else if (uc < 0x800)
                            len = 2;
                        else if (uc < 0x10000) len = 3;
                        ptr2 += len;

                        switch (len) {
                            case 4:
                                *--ptr2 = ((uc | 0x80) & 0xBF);
                                uc >>= 6;
                                /* fallthrough */
                            case 3:
                                *--ptr2 = ((uc | 0x80) & 0xBF);
                                uc >>= 6;
                                /* fallthrough */
                            case 2:
                                *--ptr2 = ((uc | 0x80) & 0xBF);
                                uc >>= 6;
                                /* fallthrough */
                            case 1:
                                *--ptr2 = (uc | firstByteMark[len]);
                        }
                        ptr2 += len;
                        break;
                    default:
                        *ptr2++ = *ptr;
                        break;
                }
                ptr++;
            }
        }
        *ptr2 = 0;
        if (*ptr == '\"') ptr++; /* TODO error handling if not \" or \0 ? */
        item->valuestring = out;
        item->type = Json_String;
        return ptr;
    }

/* Predeclare these prototypes. */
    static const char* parse_value(Json *item, const char* value, JsonAllocator& allocator);
    static const char* parse_array(Json *item, const char* value, JsonAllocator& allocator);
    static const char* parse_object(Json *item, const char* value, JsonAllocator& allocator);

/* Utility to jump whitespace and cr/lf */
    static const char* skip (const char* in) {
        if (!in) return 0; /* must propagate NULL since it's often called in skip(f(...)) form */
        while (*in && (unsigned char)*in <= 32)
            in++;
        return in;
    }

/* Parse an object - create a new root, and populate. */
    Json *Json_create(const char* value, JsonAllocator& allocator) {

        Json *c;
        ep = 0;
        if (!value) return 0; /* only place we check for NULL other than skip() */
        c = Json_new(allocator);
        if (!c) return 0; /* memory fail */

        value = parse_value(c, skip(value), allocator);
        if (!value) {
            //Json_dispose(c);
            return 0;
        } /* parse failure. ep is set. */

        return c;
    }

/* Parser core - when encountering text, process appropriately. */
    static const char* parse_value(Json *item, const char* value, JsonAllocator& allocator) {
        /* Referenced by Json_create(), parse_array(), and parse_object(). */
        /* Always called with the result of skip(). */
#if SPINE_JSON_DEBUG /* Checked at entry to graph, Json_create, and after every parse_ call. */
if (!value) return 0; /* Fail on null. */
#endif

        switch (*value) {
            case 'n': {
                if (!strncmp(value + 1, "ull", 3)) {
                    item->type = Json_NULL;
                    return value + 4;
                }
                break;
            }
            case 'f': {
                if (!strncmp(value + 1, "alse", 4)) {
                    item->type = Json_False;
                    item->valueint = 0;
                    /* calloc prevents us needing item->type = Json_False or valueint = 0 here */
                    return value + 5;
                }
                break;
            }
            case 't': {
                if (!strncmp(value + 1, "rue", 3)) {
                    item->type = Json_True;
                    item->valueint = 1;
                    return value + 4;
                }
                break;
            }
            case '\"':
                return parse_string(item, value, allocator);
            case '[':
                return parse_array(item, value, allocator);
            case '{':
                return parse_object(item, value, allocator);
            case '-': /* fallthrough */
            case '0': /* fallthrough */
            case '1': /* fallthrough */
            case '2': /* fallthrough */
            case '3': /* fallthrough */
            case '4': /* fallthrough */
            case '5': /* fallthrough */
            case '6': /* fallthrough */
            case '7': /* fallthrough */
            case '8': /* fallthrough */
            case '9':
                return parse_number(item, value);
            default:
                break;
        }

        ep = value;
        return 0; /* failure. */
    }

/* Build an array from input text. */
    static const char* parse_array(Json *item, const char* value, JsonAllocator& allocator) {
        Json *child;

#if SPINE_JSON_DEBUG /* unnecessary, only callsite (parse_value) verifies this */
if (*value != '[') {
    ep = value;
    return 0;
} /* not an array! */
#endif

        item->type = Json_Array;
        value = skip(value + 1);
        if (*value == ']') return value + 1; /* empty array. */

        item->child = child = Json_new(allocator);
        if (!item->child) return 0; /* memory fail */
        value = skip(parse_value(child, skip(value), allocator)); /* skip any spacing, get the value. */
        if (!value) return 0;
        item->size = 1;

        while (*value == ',') {
            Json *new_item = Json_new(allocator);
            if (!new_item) return 0; /* memory fail */
            child->next = new_item;
#if SPINE_JSON_HAVE_PREV
    new_item->prev = child;
#endif
            child = new_item;
            value = skip(parse_value(child, skip(value + 1), allocator));
            if (!value) return 0; /* parse fail */
            item->size++;
        }

        if (*value == ']') return value + 1; /* end of array */
        ep = value;
        return 0; /* malformed. */
    }

/* Build an object from the text. */
    static const char* parse_object(Json *item, const char* value, JsonAllocator& allocator)
    {
        Json *child;

#if SPINE_JSON_DEBUG /* unnecessary, only callsite (parse_value) verifies this */
if (*value != '{') {
    ep = value;
    return 0;
} /* not an object! */
#endif

        item->type = Json_Object;
        value = skip(value + 1);
        if (*value == '}') return value + 1; /* empty array. */

        item->child = child = Json_new(allocator);
        if (!item->child) return 0;
        value = skip(parse_string(child, skip(value), allocator));
        if (!value) return 0;
        child->name = child->valuestring;
        child->valuestring = 0;
        if (*value != ':') {
            ep = value;
            return 0;
        } /* fail! */
        value = skip(parse_value(child, skip(value + 1), allocator)); /* skip any spacing, get the value. */
        if (!value) return 0;
        item->size = 1;

        while (*value == ',') {
            Json *new_item = Json_new(allocator);
            if (!new_item) return 0; /* memory fail */
            child->next = new_item;
#if SPINE_JSON_HAVE_PREV
    new_item->prev = child;
#endif
            child = new_item;
            value = skip(parse_string(child, skip(value + 1), allocator));
            if (!value) return 0;
            child->name = child->valuestring;
            child->valuestring = 0;
            if (*value != ':') {
                ep = value;
                return 0;
            } /* fail! */
            value = skip(parse_value(child, skip(value + 1), allocator)); /* skip any spacing, get the value. */
            if (!value) return 0;
            item->size++;
        }

        if (*value == '}') return value + 1; /* end of array */
        ep = value;
        return 0; /* malformed. */
    }

    Json *Json_getItem(Json *object, const char *string)
    {
        Json *c = object->child;
        while (c && Json_strcasecmp(c->name, string))
            c = c->next;
        return c;
    }

    const char *Json_getString(Json *object, const char *name, const char *defaultValue)
    {
        object = Json_getItem(object, name);
        if (object && object->valuestring)
            return object->valuestring;
        else
            return defaultValue;
    }
    
    vector<string> Json_getStringArray(Json* json, const char* name){
        vector<string> result;
        return Json_getStringArray(json, name, result);
    }
    
    vector<string> Json_getStringArray(Json* json, const char* name, vector<string> defaultValue){
        vector<string> result;
        bool success = Json_forEach(json, name, [&](Json* json){
            result.push_back(json->valuestring);
        });
        
        if (success)
            return result;
        else
            return defaultValue;
    }
    
    vector<int> Json_getIntArray(Json* json, const char* name){
        vector<int> result;
        return Json_getIntArray(json, name, result);
    }
    
    vector<int> Json_getIntArray(Json* json, const char* name, vector<int> defaultValue){
        vector<int> result;
        bool success = Json_forEach(json, name, [&](Json* json){
            result.push_back(json->valueint);
        });
        
        if (success)
            return result;
        else
            return defaultValue;
    }
    
    vector<long> Json_getLongArray(Json* json, const char* name){
        vector<long> result;
        return Json_getLongArray(json, name, result);
    }
    
    vector<long> Json_getLongArray(Json* json, const char* name, vector<long> defaultValue){
        vector<long> result;
        bool success = Json_forEach(json, name, [&](Json* json){
            result.push_back(json->valuelong);
        });
        
        if (success)
            return result;
        else
            return defaultValue;
    }
    
    bool Json_forEach(Json* json, const char* name, function<void(Json*)> callback)
    {
        if (!callback)
            return false;
        
        Json* listJson = Json_getItem(json, name);
        return Json_forEach(listJson, callback);
    }
    
    bool Json_forEach(Json* listJson, function<void(Json*)> callback)
    {
        if (!listJson)
            return false;
        
        if (listJson->child)
        {
            Json* item = listJson->child;
            do{
                callback(item);
            }while((item = item->next));
        }
        
        return true;
    }

    float Json_getFloat(Json *value, const char *name, float defaultValue)
    {
        value = Json_getItem(value, name);
        
        string numStr = (value && value->valuestring && value->valuefloat == 0) ? value->valuestring : "";
        if (numStr.size() > 0)
            parse_number(value, numStr.c_str());
        
        return value ? value->valuefloat : defaultValue;
    }

    int Json_getInt(Json *value, const char *name, int defaultValue)
    {
        value = Json_getItem(value, name);
        
        string numStr = (value && value->valuestring && value->valueint == 0) ? value->valuestring : "";
        if (numStr.size() > 0)
            parse_number(value, numStr.c_str());
        
        return value ? value->valueint : defaultValue;
    }
    
    long Json_getLong(Json *value, const char *name, long defaultValue)
    {
        value = Json_getItem(value, name);
        
        string numStr = (value && value->valuestring && value->valuelong == 0) ? value->valuestring : "";
        if (numStr.size() > 0)
            parse_number(value, numStr.c_str());
        
        return value ? value->valuelong : defaultValue;
    }

    bool Json_getBool(Json *value, const char *name, bool defaultValue)
    {
        return (bool)Json_getInt(value, name, (int)defaultValue);
    }

    Json *Json_getItemAt (Json *array, int item) {
    Json *c = array->child;
    while (c && item > 0)
        item--, c = c->next;
    return c;
    }

}} // namespace cocos2d { namespace extension {
