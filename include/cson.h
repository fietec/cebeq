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
#include <ctype.h>
#include <sys/stat.h>

#define CSON_DEF_ARRAY_CAPACITY   16
#define CSON_ARRAY_MUL_F           2
#define CSON_MAP_CAPACITY         16
#define CSON_DEF_INDENT            4
#define CSON_REGION_CAPACITY  2*1024

#define cson_ansi_rgb(r, g, b) ("\e[38;2;" #r ";" #g ";" #b "m")
#define CSON_ANSI_END "\e[0m"

#define cson_args_len(...) sizeof((typeof(__VA_ARGS__)[]){__VA_ARGS__})/sizeof(typeof(__VA_ARGS__))
#define cson_args_array(...) (typeof(__VA_ARGS__)[]){__VA_ARGS__}, cson_args_len(__VA_ARGS__)

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
        int integer;
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
#define cson_get(cson, ...) cson__get(cson, cson_args_array(__VA_ARGS__))

Cson* cson__get(Cson *cson, CsonArg args[], size_t count);

Cson* cson_new(void);
Cson* cson_new_int(int value);
Cson* cson_new_float(double value);
Cson* cson_new_bool(bool value);
Cson* cson_new_string(CsonStr value);
Cson* cson_new_cstring(char *cstr);
Cson* cson_new_array(CsonArray *value);
Cson* cson_new_map(CsonMap *value);

size_t cson_len(Cson *cson);
size_t cson_memsize(Cson *cson);

int cson_get_int(Cson *cson);
double cson_get_float(Cson *cson);
bool cson_get_bool(Cson *cson);
CsonStr cson_get_string(Cson *cson);
CsonArray* cson_get_array(Cson *cson);
CsonMap* cson_get_map(Cson *cson);

bool cson_is_int(Cson *cson);
bool cson_is_float(Cson *cson);
bool cson_is_bool(Cson *cson);
bool cson_is_string(Cson *cson);
bool cson_is_array(Cson *cson);
bool cson_is_map(Cson *cson);
bool cson_is_null(Cson *cson);

CsonArray* cson_array_new(void);
CsonError cson_array_push(CsonArray *array, Cson *value);
CsonError cson_array_pop(CsonArray *array, size_t index);
Cson* cson_array_get(CsonArray *array, size_t index);
Cson* cson_array_get_last(CsonArray *array);
size_t cson_array_memsize(CsonArray *array);

CsonMap* cson_map_new(void);
CsonError cson_map_insert(CsonMap *map, CsonStr key, Cson *value);
CsonError cson_map_remove(CsonMap *map, CsonStr key);
Cson* cson_map_get(CsonMap *map, CsonStr key);
size_t cson_map_memsize(CsonMap *map);

CsonRegion* cson__new_region(size_t capacity);
void* cson_alloc(CsonArena *arena, size_t size);
void* cson_realloc(CsonArena *arena, void *old_ptr, size_t old_size, size_t new_size);
void cson_free();
void cson_swap_arena(CsonArena *arena);
void cson_swap_and_free_arena(CsonArena *arena);

#define cson_str(string) ((CsonStr){.value=(string), .len=strlen(string)})
CsonStr cson_str_new(char *cstr);
uint32_t cson_str_hash(CsonStr str);
bool cson_str_equals(CsonStr a, CsonStr b);
size_t cson_str_memsize(CsonStr str);

#define CSON_PRINT_INDENT 4
#define cson_print(cson) do{if (cson!=NULL){cson_fprint(cson, stdout, 0); putchar('\n');}else{printf("-null-\n");}} while (0)
#define cson_array_print(array) do{if (array!=NULL){cson_array_fprint(array, stdout, 0); putchar('\n');}else{printf("-null-\n");}}while(0)
#define cson_map_print(map) do{if (map!=NULL){cson_map_fprint(map, stdout, 0); putchar('\n');}else{printf("-null-\n");}}while(0)
bool cson_write(Cson *json, char *filename);
void cson_fprint(Cson *value, FILE *file, size_t indent);
void cson_array_fprint(CsonArray *array, FILE *file, size_t indent);
void cson_map_fprint(CsonMap *map, FILE *file, size_t indent);

/* Lexer */
#define cson_lex_is_whitespace(c) ((c == ' ' || c == '\n' || c == '\t'))
#define cson_lex_check_line(lexer, c) do{if (c == '\n'){(lexer)->loc.row++; (lexer)->loc.column=1;}else{lexer->loc.column++;}}while(0)
#define cson_lex_inc(lexer) do{lexer->index++; lexer->loc.column++;}while(0)
#define cson_lex_get_char(lexer) (lexer->buffer[lexer->index])
#define cson_lex_get_pointer(lexer) (lexer->buffer + lexer->index)
#define cson_loc_expand(loc) (loc).filename, (loc).row, (loc).column
#define cson_token_args_array(...) (CsonTokenType[]){__VA_ARGS__}, cson_args_len(__VA_ARGS__)

#define CSON_LOC_FMT "%s:%d:%d"

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
bool cson__lex_expect(CsonLexer *lexer, CsonToken *token, CsonTokenType types[], size_t count);
bool cson_lex_extract(CsonToken *token, char *buffer, size_t buffer_size);
void cson_lex_trim_left(CsonLexer *lexer);
bool cson_lex_find(CsonLexer *lexer, char c);
void cson_lex_set_token(CsonToken *token, CsonTokenType type, char *t_start, char *t_end, CsonLoc loc);
bool cson_lex_is_delimeter(char c);
bool cson_lex_is_int(char *s, char *e);
bool cson_lex_is_float(char *s, char *e);

#define cson_lex_expect(lexer, token, ...) cson__lex_expect(lexer, token, cson_token_args_array(__VA_ARGS__))

/* Parser */
#define CSON_VALUE_TOKENS CsonToken_ArrayOpen, CsonToken_MapOpen, CsonToken_Int, CsonToken_Float, CsonToken_True, CsonToken_False, CsonToken_Null, CsonToken_String
#define cson_parse(buffer, buffer_size) cson_parse_buffer(buffer, buffer_size, "")
#define cson_error_unexpected(loc, actual, ...) cson__error_unexpected(loc, cson_token_args_array(__VA_ARGS__), actual, __FILE__, __LINE__)
void cson__error_unexpected(CsonLoc loc, CsonTokenType expected[], size_t expected_count, CsonTokenType actual, char *filename, size_t line);
Cson* cson_parse_buffer(char *buffer, size_t buffer_size, char *filename);
Cson* cson_read(char *filename);
bool cson__parse_map(CsonMap *map, CsonLexer *lexer);
bool cson__parse_array(CsonArray *map, CsonLexer *lexer);
bool cson__parse_value(Cson **cson, CsonLexer *lexer, CsonToken *token);

#endif // _CSON_H