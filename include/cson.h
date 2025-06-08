/* 
    =========================================
    cson.h <https://github.com/fietec/cson.h>
    =========================================
    Copyright (c) 2025 Constantijn de Meer

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef _CSON_H
#define _CSON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <sys/stat.h>

#include <cebeq.h>

#define CSON_DEF_ARRAY_CAPACITY   16
#define CSON_ARRAY_MUL_F           2
#define CSON_MAP_CAPACITY         16
#define CSON_DEF_INDENT            4
#define CSON_REGION_CAPACITY  2*1024

#define cson_ansi_rgb(r, g, b) ("\e[38;2;" #r ";" #g ";" #b "m")
#define CSON_ANSI_END "\e[0m"

#define cson_drop_first(arg, ...) __VA_ARGS__
#define cson_args_len(...) sizeof((typeof(__VA_ARGS__)[]){__VA_ARGS__})/sizeof(typeof(__VA_ARGS__))
#define cson_args_array(...) (typeof(__VA_ARGS__)[]){cson_drop_first(__VA_ARGS__)}, cson_args_len(__VA_ARGS__)-1

#define cson_info(msg, ...) (printf("%s%s:%d: " msg CSON_ANSI_END "\n", cson_ansi_rgb(196, 196, 196), __FILE__, __LINE__, ## __VA_ARGS__))
#define cson_warning(msg, ...) (fprintf(stderr, "%s%s:%d: [WARNING] " msg CSON_ANSI_END "\n", cson_ansi_rgb(196, 64, 0), __FILE__, __LINE__, ## __VA_ARGS__))
#define cson_error(error, msg, ...) (fprintf(stderr, "%s%s:%d [ERROR] (%s): " msg CSON_ANSI_END "\n", cson_ansi_rgb(196, 0, 0), __FILE__, __LINE__, (CsonErrorStrings[(error)]), ## __VA_ARGS__))

#define cson_assert(state, msg, ...) do{if (!(state)) {cson_error(0, msg, ##__VA_ARGS__); exit(1);}} while (0)
#define cson_assert_alloc(alloc) cson_assert((alloc)!=NULL, "Memory allocation failed! (need more RAM :/)")

#define cson_arr_len(arr) ((arr)!= NULL ? sizeof((arr))/sizeof((arr)[0]):0)

typedef struct Cson Cson;
typedef struct CsonArg CsonArg;
typedef struct CsonArray CsonArray;
typedef struct CsonMap CsonMap;
typedef struct CsonMapItem CsonMapItem;
typedef struct CsonArg CsonArg;
typedef struct CsonStr CsonStr;
typedef struct CsonArena CsonArena;
typedef struct CsonRegion CsonRegion;

typedef enum {
    Cson_Int,
    Cson_Float,
    Cson_Bool,
    Cson_Null,
    Cson_String,
    Cson_Array,
    Cson_Map,
    Cson__TypeCount
} CsonType;

static const char* const CsonTypeStrings[] = {
    [Cson_Int] = "int",
    [Cson_Float] = "float",
    [Cson_Bool] = "bool",
    [Cson_Null] = "null",
    [Cson_String] = "String",
    [Cson_Array] = "Array",
    [Cson_Map] = "Map"
};

_Static_assert(Cson__TypeCount == cson_arr_len(CsonTypeStrings), "CsonType count has changed!");

typedef enum {
    CsonError_Success,
    CsonError_InvalidParam,
    CsonError_InvalidType,
    CsonError_Alloc,
    CsonError_FileNotFound,
    CsonError_UnexpectedToken,
    CsonError_EndOfBuffer,
    CsonError_Unimplemented,
    CsonError_UnclosedString,
    CsonError_IndexError,
    CsonError_KeyError,
    CsonError_Any,
    CsonError_None,
    Cson__ErrorCount
} CsonError;

static const char* const CsonErrorStrings[] = {
    [CsonError_Success] = "Success",
    [CsonError_InvalidParam] = "InvalidArguments",
    [CsonError_FileNotFound] = "FileNotFound",
    [CsonError_InvalidType] = "InvalidType",
    [CsonError_Alloc] = "Allocation",
    [CsonError_UnexpectedToken] = "UnexpectedToken",
    [CsonError_UnclosedString] = "UnclosedString",
    [CsonError_EndOfBuffer] = "EndOfBuffer",
    [CsonError_IndexError] = "IndexError",
    [CsonError_KeyError] = "KeyError",
    [CsonError_Unimplemented] = "UNIMPLEMENTED",
    [CsonError_Any] = "Undefined",
    [CsonError_None] = ""
};

_Static_assert(Cson__ErrorCount == cson_arr_len(CsonErrorStrings), "CsonError count has changed!");

typedef enum {
    CsonArg_Key,
    CsonArg_Index,
    Cson__ArgCount
} CsonArgType;

static const char* const CsonArgStrings[] = {
    [CsonArg_Key] = "Key",
    [CsonArg_Index] = "Index"
};

_Static_assert(Cson__ArgCount == cson_arr_len(CsonArgStrings), "CsonArgType count has changed!");

struct CsonStr{
    char *value;
    size_t len;
};

struct CsonArray{
    Cson **items;
    size_t size;
    size_t capacity;
};

struct CsonMap{
    CsonMapItem **items;
    size_t size;
    size_t capacity;
};

struct CsonMapItem{
    CsonStr key;
    Cson *value;
    CsonMapItem *next;
};

struct CsonArg{
    CsonArgType type;
    union{
        CsonStr key;
        size_t index;
    } value;
};

struct Cson{
    union{
        int64_t integer;
        double floating;
        bool boolean;
        CsonStr string;
        CsonArray *array;
        CsonMap *map;
        void *null;
    } value;
    CsonType type;
};

struct CsonArena{
    CsonRegion *first, *last;
    size_t region_size;
};

struct CsonRegion{
    size_t size;
    size_t capacity;
    CsonRegion *next;
    uintptr_t data[];
};

#define key(kstr) ((CsonArg) {.value.key=cson_str(kstr), .type=CsonArg_Key})
#define index(istr) ((CsonArg) {.value.index=(size_t)(istr), .type=CsonArg_Index})

#define cson__to_int(cson)    (cson)->value.integer
#define cson__to_float(cson)  (cson)->value.floating
#define cson__to_bool(cson)   (cson)->value.boolean
#define cson__to_string(cson) (cson)->value.string
#define cson__to_array(cson)  (cson)->value.array
#define cson__to_map(cson)    (cson)->value.map

// macros for multi-level searching
#define cson_get(cson, ...) cson__get(cson, cson_args_array((CsonArg){0}, ##__VA_ARGS__))
#define cson_get_int(cson, ...) cson__get_int(cson_get(cson, ##__VA_ARGS__))
#define cson_get_float(cson, ...) cson__get_float(cson_get(cson, ##__VA_ARGS__))
#define cson_get_bool(cson, ...) cson__get_bool(cson_get(cson, ##__VA_ARGS__))
#define cson_get_string(cson, ...) cson__get_string(cson_get(cson, ##__VA_ARGS__))
#define cson_get_cstring(cson, ...) cson__get_cstring(cson_get(cson, ##__VA_ARGS__))
#define cson_get_array(cson, ...) cson__get_array(cson_get(cson, ##__VA_ARGS__))
#define cson_get_map(cson, ...) cson__get_map(cson_get(cson, ##__VA_ARGS__))

CBQLIB Cson* cson__get(Cson *cson, CsonArg args[], size_t count);

CBQLIB Cson* cson_new(void);
CBQLIB Cson* cson_new_int(int32_t value);
CBQLIB Cson* cson_new_float(double value);
CBQLIB Cson* cson_new_bool(bool value);
CBQLIB Cson* cson_new_string(CsonStr value);
CBQLIB Cson* cson_new_cstring(char *cstr);
CBQLIB Cson* cson_new_array(CsonArray *value);
CBQLIB Cson* cson_new_map(CsonMap *value);
CBQLIB Cson* cson_new_cson(Cson *cson);
CBQLIB Cson* cson_new_null();

CBQLIB size_t cson_len(Cson *cson);
CBQLIB size_t cson_memsize(Cson *cson);

CBQLIB int64_t cson__get_int(Cson *cson);
CBQLIB double cson__get_float(Cson *cson);
CBQLIB bool cson__get_bool(Cson *cson);
CBQLIB CsonStr cson__get_string(Cson *cson);
CBQLIB char* cson__get_cstring(Cson *cson);
CBQLIB CsonArray* cson__get_array(Cson *cson);
CBQLIB CsonMap* cson__get_map(Cson *cson);

CBQLIB bool cson_is_int(Cson *cson);
CBQLIB bool cson_is_float(Cson *cson);
CBQLIB bool cson_is_bool(Cson *cson);
CBQLIB bool cson_is_string(Cson *cson);
CBQLIB bool cson_is_array(Cson *cson);
CBQLIB bool cson_is_map(Cson *cson);
CBQLIB bool cson_is_null(Cson *cson);

CBQLIB Cson* cson_array_new(void);
CBQLIB CsonError cson_array_push(Cson *array, Cson *value);
CBQLIB CsonError cson_array_pop(Cson *array, size_t index);
CBQLIB Cson* cson_array_get(Cson *array, size_t index);
CBQLIB Cson* cson_array_get_last(Cson *array);
CBQLIB Cson* cson_array_dup(Cson *array);
CBQLIB size_t cson_array_memsize(Cson *array);

CBQLIB Cson* cson_map_new(void);
CBQLIB CsonError cson_map_insert(Cson *map, CsonStr key, Cson *value);
CBQLIB CsonError cson_map_remove(Cson *map, CsonStr key);
CBQLIB Cson* cson_map_get(Cson *map, CsonStr key);
CBQLIB Cson *cson_map_keys(Cson *map);
CBQLIB Cson* cson_map_dup(Cson *map);
CBQLIB size_t cson_map_memsize(Cson *map);

extern CsonArena *cson_current_arena;

CsonRegion* cson__new_region(size_t capacity);
CBQLIB void* cson_alloc(CsonArena *arena, size_t size);
CBQLIB void* cson_realloc(CsonArena *arena, void *old_ptr, size_t old_size, size_t new_size);
CBQLIB void cson_free();
CBQLIB void cson_swap_arena(CsonArena *arena);
CBQLIB void cson_swap_and_free_arena(CsonArena *arena);

#define cson_str(string) ((CsonStr){.value=(string), .len=strlen(string)})
CBQLIB CsonStr cson_str_new(char *cstr);
CBQLIB CsonStr cson_str_dup(CsonStr str);
CBQLIB uint32_t cson_str_hash(CsonStr str);
CBQLIB bool cson_str_equals(CsonStr a, CsonStr b);
CBQLIB size_t cson_str_memsize(CsonStr str);

#define CSON_PRINT_INDENT 4
#define cson_print(cson) do{if (cson!=NULL){cson_fprint(cson, stdout, 0); putchar('\n');}else{printf("-null-\n");}} while (0)
#define cson_array_print(array) do{if (array!=NULL){cson_array_fprint(array, stdout, 0); putchar('\n');}else{printf("-null-\n");}}while(0)
#define cson_map_print(map) do{if (map!=NULL){cson_map_fprint(map, stdout, 0); putchar('\n');}else{printf("-null-\n");}}while(0)
CBQLIB bool cson_write(Cson *json, char *filename);
CBQLIB void cson_fprint(Cson *value, FILE *file, size_t indent);
CBQLIB void cson_array_fprint(CsonArray *array, FILE *file, size_t indent);
CBQLIB void cson_map_fprint(CsonMap *map, FILE *file, size_t indent);

/* Lexer */
#define cson_lex_is_whitespace(c) ((c == ' ' || c == '\n' || c == '\t'))
#define cson_lex_check_line(lexer, c) do{if (c == '\n'){(lexer)->loc.row++; (lexer)->loc.column=1;}else{lexer->loc.column++;}}while(0)
#define cson_lex_inc(lexer) do{lexer->index++; lexer->loc.column++;}while(0)
#define cson_lex_get_char(lexer) (lexer->buffer[lexer->index])
#define cson_lex_get_pointer(lexer) (lexer->buffer + lexer->index)
#define cson_loc_expand(loc) (loc).filename, (loc).row, (loc).column
#define cson_token_args_array(...) (CsonTokenType[]){__VA_ARGS__}, cson_args_len(__VA_ARGS__)

#define CSON_LOC_FMT "%s:%u:%u"

typedef enum{
    CsonToken_MapOpen,
    CsonToken_MapClose,
    CsonToken_ArrayOpen,
    CsonToken_ArrayClose,
    CsonToken_Sep,
    CsonToken_MapSep,
    CsonToken_String,
    CsonToken_Int,
    CsonToken_Float,
    CsonToken_True,
    CsonToken_False,
    CsonToken_Null,
    CsonToken_Invalid,
    CsonToken_End,
    CsonToken__Count
} CsonTokenType;

static const char* const CsonTokenTypeNames[] = {
    [CsonToken_MapOpen] = "MapOpen",
    [CsonToken_MapClose] = "MapClose",
    [CsonToken_ArrayOpen] = "ArrayOpen",
    [CsonToken_ArrayClose] = "ArrayClose",
    [CsonToken_Sep] = "Sep",
    [CsonToken_MapSep] = "MapSep",
    [CsonToken_String] = "String",
    [CsonToken_Int] = "Int",
    [CsonToken_Float] = "Float",
    [CsonToken_True] = "True",
    [CsonToken_False] = "False",
    [CsonToken_Null] = "Null",
    [CsonToken_Invalid] = "Invalid",
    [CsonToken_End] = "--END--"
};

_Static_assert(CsonToken__Count == cson_arr_len(CsonTokenTypeNames), "CsonTokenType count has changed!");

typedef struct{
    char *filename;
    size_t row;
    size_t column;
} CsonLoc;

typedef struct{
    CsonTokenType type;
    char *t_start;
    char *t_end;
    size_t len;
    CsonLoc loc;
} CsonToken;

typedef struct{
    char *buffer;
    size_t buffer_size;
    size_t index;
    CsonLoc loc;
} CsonLexer;

CsonLexer cson_lex_init(char *buffer, size_t buffer_size, char *filename);
bool cson_lex_next(CsonLexer *lexer, CsonToken *token);
bool cson__lex_expect(CsonLexer *lexer, CsonToken *token, CsonTokenType types[], size_t count, char *file, size_t line);
bool cson_lex_extract(CsonToken *token, char *buffer, size_t buffer_size);
void cson_lex_trim_left(CsonLexer *lexer);
bool cson_lex_find(CsonLexer *lexer, char c);
void cson_lex_set_token(CsonToken *token, CsonTokenType type, char *t_start, char *t_end, CsonLoc loc);
bool cson_lex_is_delimeter(char c);
bool cson_lex_is_int(char *s, char *e);
bool cson_lex_is_float(char *s, char *e);

#define cson_lex_expect(lexer, token, ...) cson__lex_expect(lexer, token, cson_token_args_array(__VA_ARGS__), __FILE__, __LINE__)

/* Parser */
#define CSON_VALUE_TOKENS CsonToken_ArrayOpen, CsonToken_MapOpen, CsonToken_Int, CsonToken_Float, CsonToken_True, CsonToken_False, CsonToken_Null, CsonToken_String
#define cson_parse(buffer, buffer_size) cson_parse_buffer(buffer, buffer_size, "")
#define cson_error_unexpected(loc, actual, ...) cson__error_unexpected(loc, cson_token_args_array(__VA_ARGS__), actual, __FILE__, __LINE__)
void cson__error_unexpected(CsonLoc loc, CsonTokenType expected[], size_t expected_count, CsonTokenType actual, char *filename, size_t line);
CBQLIB Cson* cson_parse_buffer(char *buffer, size_t buffer_size, char *filename);
CBQLIB Cson* cson_read(char *filename);
bool cson__parse_map(Cson *map, CsonLexer *lexer);
bool cson__parse_array(Cson *array, CsonLexer *lexer);
bool cson__parse_value(Cson **cson, CsonLexer *lexer, CsonToken *token);

#endif // _CSON_H
