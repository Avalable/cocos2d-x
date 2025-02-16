/*
 Copyright (c) 2009 Dave Gamble
 
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

/* Esoteric Software: Removed everything except parsing, shorter method names, more get methods, double to float, formatted. */

#include <stdio.h>
#include <string>
#include <vector>
#include <functional>
#include "JsonAllocator.h"

#ifndef SPINE_JSON_H_
#define SPINE_JSON_H_


using namespace std;
namespace cocos2d { namespace extension {

/* Json Types: */
#define Json_False 0
#define Json_True 1
#define Json_NULL 2
#define Json_Number 3
#define Json_String 4
#define Json_Array 5
#define Json_Object 6

#ifndef SPINE_JSON_HAVE_PREV
/* Spine doesn't use the "prev" link in the Json sibling lists. */
#define SPINE_JSON_HAVE_PREV 0
#endif

/* The Json structure: */
    typedef struct Json {
        struct Json* next;
#if SPINE_JSON_HAVE_PREV
    struct Json* prev; /* next/prev allow you to walk array/object chains. Alternatively, use getSize/getItem */
#endif
        struct Json* child; /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */

        int type; /* The type of the item, as above. */
        int size; /* The number of children. */

        const char* valuestring; /* The item's string, if type==Json_String */
        int valueint; /* The item's number, if type==Json_Number */
        long valuelong;
        float valuefloat; /* The item's number, if type==Json_Number */

        const char* name; /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
    } Json;

    /* Supply a block of JSON, and this returns a Json object you can interrogate. Call Json_dispose when finished. */
    Json* Json_create (const char* value, JsonAllocator& allocator);

    /* Delete a Json entity and all subentities. */
    void Json_dispose(Json* json);

    /* Get item "string" from object. Case insensitive. */
    Json* Json_getItem (Json* json, const char* string);
    const char* Json_getString (Json* json, const char* name, const char* defaultValue);
    float Json_getFloat (Json* json, const char* name, float defaultValue);
    int Json_getInt (Json* json, const char* name, int defaultValue);
    long Json_getLong (Json* json, const char* name, long defaultValue);
    bool Json_getBool(Json *value, const char *name, bool defaultValue);
    bool Json_forEach(Json* json, const char* name, function<void(Json*)> callback);
    bool Json_forEach(Json* listJson, function<void(Json*)> callback);
    vector<string> Json_getStringArray(Json* json, const char* name);
    vector<string> Json_getStringArray(Json* json, const char* name, vector<string> defaultValue);
    vector<int> Json_getIntArray(Json* json, const char* name);
    vector<int> Json_getIntArray(Json* json, const char* name, vector<int> defaultValue);
    vector<long> Json_getLongArray(Json* json, const char* name);
    vector<long> Json_getLongArray(Json* json, const char* name, vector<long> defaultValue);

    /* Returns the number of items in an array (or object). */
    inline int Json_getSize (Json* json) { return json->size; }

    /* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
    Json* Json_getItemAt (Json* json, int item);


    /* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when Json_create() returns 0. 0 when Json_create() succeeds. */
    const char* Json_getError (void);

}} // namespace cocos2d { namespace extension {

#endif /* SPINE_JSON_H_ */
