#include <cson.h>

Cson* cson__get(Cson *cson, CsonArg args[], size_t count)
{
    if (cson == NULL) return NULL;
    Cson *next = cson;
    for (size_t i=0; i<count; ++i){
        CsonArg arg = args[i];
        if (arg.type == CsonArg_Key && next->type == Cson_Map){
            next = cson_map_get(next, arg.value.key);
            if (next == NULL){
                cson_error(CsonError_KeyError, "No such key in map: \"%s\"", arg.value.key.value);
                return NULL;
            }
            continue;
        }
        else if (arg.type == CsonArg_Index && next->type == Cson_Array){
            next = cson_array_get(next, arg.value.index);
            if (next == NULL){
                cson_error(CsonError_IndexError, "Index out of bounds for array of size %u: %u", cson_len(next), arg.value.index);
                return NULL;
            }
            continue;
        }
        else{
            cson_error(CsonError_InvalidType, "Cannot access %s via %s!", CsonTypeStrings[next->type], CsonArgStrings[arg.type]);
        }
        return NULL;
    }
    return next;
}

size_t cson_len(Cson *cson)
{
    if (cson == NULL) return 0;
    switch (cson->type){
        case Cson_Array: return cson->value.array->size;
        case Cson_Map: return cson->value.map->size;
        default:{
            cson_error(CsonError_InvalidType, "value of type %s does not have a length.", CsonTokenTypeNames[cson->type]);
            return 0;
        }
    }
}

size_t cson_memsize(Cson *cson)
{
    if (cson == NULL) return 0;
    size_t total = sizeof(Cson);
    switch (cson->type){
        case Cson_Array: total += cson_array_memsize(cson); break;
        case Cson_Map: total += cson_map_memsize(cson); break;
        case Cson_String: total += cson_str_memsize(cson->value.string); break;
        default: break;
    }
    return total;
}

/* Cson constructors */

Cson* cson_new(void)
{
    Cson *cson = cson_alloc(cson_current_arena, sizeof(*cson));
    cson_assert_alloc(cson);
    return cson;
}

Cson* cson_new_int(int32_t value)
{
    Cson *cson = cson_alloc(cson_current_arena, sizeof(*cson));
    cson_assert_alloc(cson);
    cson->type = Cson_Int;
    cson->value.integer = value;
    return cson;
}

Cson* cson_new_float(double value)
{
    Cson *cson = cson_alloc(cson_current_arena, sizeof(*cson));
    cson_assert_alloc(cson);
    cson->type = Cson_Float;
    cson->value.floating = value;
    return cson;
}

Cson* cson_new_bool(bool value)
{
    Cson *cson = cson_alloc(cson_current_arena, sizeof(*cson));
    cson_assert_alloc(cson);
    cson->type = Cson_Bool;
    cson->value.boolean = value;
    return cson;
}

Cson* cson_new_string(CsonStr value)
{
    Cson *cson = cson_alloc(cson_current_arena, sizeof(*cson));
    cson_assert_alloc(cson);
    cson->type = Cson_String;
    cson->value.string = cson_str_new(value.value);
    return cson;
}

Cson* cson_new_cstring(char *cstr)
{
    if (cstr == NULL){
        cson_error(CsonError_InvalidParam, "String may not be null!");
        return NULL;
    }
    Cson *cson = cson_alloc(cson_current_arena, sizeof(*cson));
    cson_assert_alloc(cson);
    cson->type = Cson_String;
    cson->value.string = cson_str_new(cstr);
    return cson;
}

Cson* cson_new_array(CsonArray *value)
{
    Cson *cson = cson_alloc(cson_current_arena, sizeof(*cson));
    cson_assert_alloc(cson);
    cson->type = Cson_Array;
    cson->value.array = value;
    return cson;
}

Cson* cson_new_map(CsonMap *value)
{
    Cson *cson = cson_alloc(cson_current_arena, sizeof(*cson));
    cson_assert_alloc(cson);
    cson->type = Cson_Map;
    cson->value.map = value;
    return cson;
}

Cson* cson_new_null(void)
{
    Cson *cson = cson_alloc(cson_current_arena, sizeof(*cson));
    cson_assert_alloc(cson);
    cson->type = Cson_Null;
    cson->value.null = NULL;
    return cson;
}

Cson* cson_new_cson(Cson *cson)
{
    if (cson == NULL) return NULL;
    switch (cson->type){
        case Cson_Int: return cson_new_int(cson__to_int(cson));
        case Cson_Float: return cson_new_float(cson__to_float(cson));
        case Cson_Bool: return cson_new_bool(cson__to_bool(cson));
        case Cson_String: return cson_new_string(cson_str_dup(cson__to_string(cson)));
        case Cson_Array: return cson_array_dup(cson);
        case Cson_Map: return cson_map_dup(cson);
        default: {
            cson_error(CsonError_InvalidType, "Cson is of an invalid type: %d!", cson->type);
            return NULL;
        }
    }
}

int64_t cson__get_int(Cson *cson) {
    if (cson == NULL) {
        cson_error(CsonError_InvalidParam, "Cannot extract %s from null pointer!", CsonTypeStrings[Cson_Int]);
        return 0;
    }
    if (cson->type != Cson_Int) {
        cson_error(CsonError_InvalidType, "Cannot convert %s to %s!", CsonTypeStrings[cson->type], CsonTypeStrings[Cson_Int]);
        return 0;
    }
    return cson->value.integer;
}

double cson__get_float(Cson *cson) {
    if (cson == NULL) {
        cson_error(CsonError_InvalidParam, "Cannot extract %s from null pointer!", CsonTypeStrings[Cson_Float]);
        return 0.0;
    }
    if (cson->type != Cson_Float) {
        cson_error(CsonError_InvalidType, "Cannot convert %s to %s!", CsonTypeStrings[cson->type], CsonTypeStrings[Cson_Float]);
        return 0.0;
    }
    return cson->value.floating;
}

bool cson__get_bool(Cson *cson) {
    if (cson == NULL) {
        cson_error(CsonError_InvalidParam, "Cannot extract %s from null pointer!", CsonTypeStrings[Cson_Bool]);
        return false;
    }
    if (cson->type != Cson_Bool) {
        cson_error(CsonError_InvalidType, "Cannot convert %s to %s!", CsonTypeStrings[cson->type], CsonTypeStrings[Cson_Bool]);
        return false;
    }
    return cson->value.boolean;
}

CsonStr cson__get_string(Cson *cson) {
    if (cson == NULL) {
        cson_error(CsonError_InvalidParam, "Cannot extract %s from null pointer!", CsonTypeStrings[Cson_String]);
        return (CsonStr){0};
    }
    if (cson->type == Cson_Null){
        return (CsonStr) {.value=NULL, .len=0};
    }
    if (cson->type != Cson_String) {
        cson_error(CsonError_InvalidType, "Cannot convert %s to %s!", CsonTypeStrings[cson->type], CsonTypeStrings[Cson_String]);
        return (CsonStr){0};
    }
    return cson->value.string;
}

char* cson__get_cstring(Cson *cson){
    if (cson == NULL) {
        cson_error(CsonError_InvalidParam, "Cannot extract %s from null pointer!", CsonTypeStrings[Cson_String]);
        return NULL;
    }
    if (cson->type == Cson_Null){
        return NULL;
    }
    if (cson->type != Cson_String) {
        cson_error(CsonError_InvalidType, "Cannot convert %s to %s!", CsonTypeStrings[cson->type], CsonTypeStrings[Cson_String]);
        return NULL;
    }
    return cson->value.string.value;
}

CsonArray* cson__get_array(Cson *cson) {
    if (cson == NULL) {
        cson_error(CsonError_InvalidParam, "Cannot extract %s from null pointer!", CsonTypeStrings[Cson_Array]);
        return NULL;
    }
    if (cson->type != Cson_Array) {
        cson_error(CsonError_InvalidType, "Cannot convert %s to %s!", CsonTypeStrings[cson->type], CsonTypeStrings[Cson_Array]);
        return NULL;
    }
    return cson->value.array;
}

CsonMap* cson__get_map(Cson *cson) {
    if (cson == NULL) {
        cson_error(CsonError_InvalidParam, "Cannot extract %s from null pointer!", CsonTypeStrings[Cson_Map]);
        return NULL;
    }
    if (cson->type != Cson_Map) {
        cson_error(CsonError_InvalidType, "Cannot convert %s to %s!", CsonTypeStrings[cson->type], CsonTypeStrings[Cson_Map]);
        return NULL;
    }
    return cson->value.map;
}

bool cson_is_int(Cson *cson)
{
    if (cson == NULL) return false;
    return cson->type == Cson_Int;
}

bool cson_is_float(Cson *cson)
{
    if (cson == NULL) return false;
    return cson->type == Cson_Float;
}

bool cson_is_bool(Cson *cson)
{
    if (cson == NULL) return false;
    return cson->type == Cson_Bool;
}

bool cson_is_string(Cson *cson)
{
    if (cson == NULL) return false;
    return cson->type == Cson_String;
}

bool cson_is_array(Cson *cson)
{
    if (cson == NULL) return false;
    return cson->type == Cson_Array;
}

bool cson_is_map(Cson *cson)
{
    if (cson == NULL) return false;
    return cson->type == Cson_Map;
}

bool cson_is_null(Cson *cson)
{
    if (cson == NULL) return false;
    return cson->type == Cson_Null;
}

/* Array implementation */

Cson* cson_array_new(void)
{
    CsonArray *array = cson_alloc(cson_current_arena, sizeof(*array) + CSON_DEF_ARRAY_CAPACITY*sizeof(Cson*));
    cson_assert_alloc(array);
    array->size = 0;
    array->capacity = CSON_DEF_ARRAY_CAPACITY;
    array->items = (Cson**) (array+1);
    return cson_new_array(array);
}

CsonError cson_array_push(Cson *array, Cson *value)
{
    if (array == NULL || value == NULL) return CsonError_InvalidParam;
    if (array->type != Cson_Array) return CsonError_InvalidType;
    CsonArray *arr = array->value.array;
    if (arr->size >= arr->capacity){
        size_t new_capacity = arr->capacity * CSON_ARRAY_MUL_F;
        arr->items = cson_realloc(cson_current_arena, arr->items, arr->capacity*sizeof(Cson*), new_capacity*sizeof(Cson));
        arr->capacity = new_capacity;
    }
    arr->items[arr->size++] = value;
    return CsonError_Success;
}

Cson* cson_array_get(Cson *array, size_t index)
{
    if (array == NULL || array->type != Cson_Array || index >= cson_len(array)) return NULL;
    return cson__to_array(array)->items[index];
}

Cson* cson_array_get_last(Cson *array)
{
    if (array == NULL || array->type != Cson_Array || cson_len(array) == 0) return NULL;
    CsonArray *arr = cson__to_array(array);
    return arr->items[arr->size-1];
}

CsonError cson_array_pop(Cson *array, size_t index)
{
    if (array == NULL) return CsonError_InvalidParam;
    if (array->type != Cson_Array) return CsonError_InvalidType;
    CsonArray *arr = cson__to_array(array);
    if (index >= arr->size) return CsonError_IndexError;
    for (size_t i=index; i<arr->capacity-1; ++i){
        arr->items[i] = arr->items[i+1];
    }
    return CsonError_Success;
}

Cson* cson_array_dup(Cson *array)
{
    if (array == NULL || array->type != Cson_Array) return NULL;
    Cson *new_array = cson_array_new();
    for (size_t i=0; i<cson_len(array); ++i){
        cson_array_push(new_array, cson_new_cson(cson_array_get(array, i)));
    }
    return new_array;
}

size_t cson_array_memsize(Cson *array)
{
    if (array == NULL || array->type != Cson_Array) return 0;
    size_t total = sizeof(CsonArray);
    CsonArray *arr = cson__to_array(array);
    for (size_t i=0; i<arr->size; ++i){
        total += cson_memsize(arr->items[i]);
    }
    return total;
}

/* Map implementation */

Cson* cson_map_new(void)
{
    CsonMap *map = cson_alloc(cson_current_arena, sizeof(*map) + CSON_MAP_CAPACITY*sizeof(CsonMapItem*));
    cson_assert_alloc(map);
    map->size = 0;
    map->capacity = CSON_MAP_CAPACITY;
    map->items = (CsonMapItem**) (map+1);
    cson_assert_alloc(map->items);
    return cson_new_map(map);
}

CsonMapItem* cson_map_item_new(CsonStr key, Cson *value)
{
    CsonMapItem *item = cson_alloc(cson_current_arena, sizeof(*item));
    cson_assert_alloc(item);
    item->key = key;
    item->value = value;
    item->next = NULL;
    return item;
}

CsonError cson_map_insert(Cson *map, CsonStr key, Cson *value)
{
    if (map == NULL || key.value == NULL || value == NULL) return CsonError_InvalidParam;
    if (map->type != Cson_Map) return CsonError_InvalidType;
    CsonMap *i_map = cson__to_map(map);
    size_t index = cson_str_hash(key) % i_map->capacity;
    CsonMapItem *item = i_map->items[index];
    if (item != NULL){
        do {
            if (cson_str_equals(item->key, key)){
                item->value = value;
                return CsonError_Success;
            }
            if (item->next == NULL) break;
            item = item->next;
        }while(true);
        item->next = cson_map_item_new(key, value);
    }
    else{
        i_map->items[index] = cson_map_item_new(key, value);
    }
    i_map->size++;
    return CsonError_Success;
}

CsonError cson_map_remove(Cson *map, CsonStr key)
{
    if (map == NULL || key.value == NULL) return CsonError_InvalidParam;
    if (map->type != Cson_Map) return CsonError_InvalidType;
    CsonMap *i_map = cson__to_map(map);
    size_t index = cson_str_hash(key) % i_map->capacity;
    CsonMapItem *item = i_map->items[index];
    if (item == NULL) return CsonError_KeyError;
    if (cson_str_equals(item->key, key)){
        i_map->items[index] = item->next;
        i_map->size--;
        return CsonError_Success;
    }
    while (item->next != NULL){
        if (cson_str_equals(item->next->key, key)){
            item->next = item->next->next;
            i_map->size--;
            return CsonError_Success;
        }
        item = item->next;
    }
    return CsonError_KeyError;
}

Cson* cson_map_get(Cson *map, CsonStr key)
{
    if (map == NULL || key.value == NULL) return NULL;
    if (map->type != Cson_Map) return NULL;
    CsonMap *i_map = cson__to_map(map);
    size_t index = cson_str_hash(key) % i_map->capacity;
    CsonMapItem *item = i_map->items[index];
    while (item != NULL){
        if (cson_str_equals(item->key, key)) return item->value;
        item = item->next;
    }
    return NULL;
}

Cson* cson_map_dup(Cson *map)
{
    if (map == NULL || map->type != Cson_Map) return NULL;
    Cson* new_map = cson_map_new();
    Cson* keys = cson_map_keys(map);
    for (size_t i=0; i<cson_len(keys); ++i){
        CsonStr key = cson__to_string(cson_array_get(keys, i));
        cson_map_insert(new_map, key, cson_new_cson(cson_map_get(map, key)));
    }
    return new_map;
}

size_t cson_map_memsize(Cson *map)
{
    if (map == NULL || map->type != Cson_Map) return 0;
    CsonMap *i_map = cson__to_map(map);
    size_t total = sizeof(CsonMap);
    for (size_t i=0; i<i_map->capacity; ++i){
        CsonMapItem *item = i_map->items[i];
        while (item != NULL){
            total += (sizeof(CsonMapItem) + cson_str_memsize(item->key) + cson_memsize(item->value));
            item = item->next;
        }
    }
    return total;
}

Cson *cson_map_keys(Cson *map)
{
    if (map == NULL || map->type != Cson_Map) return NULL;
    CsonMap *i_map = cson__to_map(map);
    Cson *array = cson_array_new();
    for (size_t i=0; i<i_map->capacity; ++i){
        CsonMapItem *item = i_map->items[i];
        while (item != NULL){
            cson_array_push(array, cson_new_string(cson_str_dup(item->key)));
            item = item->next;
        }
    }
    return array;
}

/* Memory management */

CsonRegion* cson__new_region(size_t capacity)
{
    size_t size = sizeof(CsonRegion) + sizeof(uintptr_t)*capacity;
    CsonRegion *region = calloc(1, size);
    cson_assert(region != NULL, "Failed to allocate new region!");
    region->size = 0;
    region->capacity = capacity;
    region->next = NULL;
    return region;
}

void cson__free(CsonArena *arena)
{
    if (arena == NULL) return;
    CsonRegion *next = arena->first;
    while (next != NULL){
        CsonRegion *temp = next;
        next = temp->next;
        free(temp);
    }
    arena->first = NULL;
    arena->last = NULL;
}

void* cson_alloc(CsonArena *arena, size_t size)
{
    if (arena == NULL) return NULL;
    size_t all_size = (size + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);
    size_t default_capacity = (arena->region_size != 0)? arena->region_size:CSON_REGION_CAPACITY;
    if (arena->first == NULL){
        cson_assert(arena->last == NULL, "Invalid arena state!: first:%p, last:%p", arena->first, arena->last);
        size_t capacity = (all_size > default_capacity)? all_size:default_capacity;
        CsonRegion *region = cson__new_region(capacity);
        arena->first = region;
        arena->last = region;
    }
    CsonRegion *last = arena->last;
    if (last->size + all_size > last->capacity){
        cson_assert(arena->last == NULL, "Invalid arena state!: size:%u, capacity:%u, next:%p", last->size, last->capacity, last->next);
        size_t capacity = (all_size > default_capacity)? all_size:default_capacity;
        last->next = cson__new_region(capacity);
        arena->last = last->next;
    }
    void *result = &arena->last->data[arena->last->size];
    arena->last->size += all_size;
    return result;
}

void* cson_realloc(CsonArena *arena, void *old_ptr, size_t old_size, size_t new_size)
{
    if (arena == NULL) return NULL;
    if (old_size >= new_size) return old_ptr;
    void *new_ptr = cson_alloc(arena, new_size);
    cson_assert_alloc(new_ptr);
    char *rc = (char*) old_ptr;
    char *wc = (char*) new_ptr;
    for (size_t i=0; i<old_size; ++i){
        *wc++ = *rc++;
    }
    return new_ptr;
}

void* cson_dup(CsonArena *arena, void *old_ptr, size_t old_size, size_t new_size)
{
    if (arena == NULL) return NULL;
    void *new_ptr = cson_alloc(arena, new_size);
    cson_assert_alloc(new_ptr);
    char *rc = (char*) old_ptr;
    char *wc = (char*) new_ptr;
    for (size_t i=0; i<new_size; ++i){
        *wc++ = (i<old_size)?*rc++:'\0';
    }
    return new_ptr;
}

void cson_swap_arena(CsonArena *arena)
{
    if (arena == NULL){
        cson_current_arena = &cson_default_arena;
    }
    else{
        cson_current_arena = arena;
    }
}

void cson_swap_and_free_arena(CsonArena *arena)
{
    cson__free(cson_current_arena);
    if (arena == NULL){
        cson_current_arena = &cson_default_arena;
    }
    else{
        cson_current_arena = arena;
    }
}

void cson_free()
{
    cson__free(cson_current_arena);
}

/* Further utilities */
uint32_t cson_hash(void *p, size_t n)
{
    // DJB2 hashing
    uint32_t hash = 5381;
    while (n-- > 0){
        hash = ((hash << 5) + hash) + *((char*)p++);
    }
    return hash;
}

uint64_t cson_file_size(const char* filename){
    struct stat file;
    if (stat(filename, &file) == -1){
        cson_error(CsonError_FileNotFound, "Failed to read file: \"%s\"", filename);
        return 0;
    }
    return (uint64_t) file.st_size;
}

/* String Utilities */

CsonStr cson_str_new(char *cstr)
{
    size_t len = strlen(cstr);
    char *value = cson_dup(cson_current_arena, cstr, len, len+1);
    return (CsonStr) {.value=value, .len=len};
}

CsonStr cson_str_dup(CsonStr str)
{
    return (CsonStr) {.value=cson_dup(cson_current_arena, str.value, str.len, str.len+1), .len=str.len};
}

uint32_t cson_str_hash(CsonStr str)
{
    return cson_hash(str.value, str.len);
}

bool cson_str_equals(CsonStr a, CsonStr b)
{
    if (a.len != b.len) return false;
    char *pa = a.value;
    char *pb = b.value;
    size_t len = a.len;
    while (len-- > 0){
        if (*pa++ != *pb++) return false;
    }
    return true;
}

size_t cson_str_memsize(CsonStr string)
{
    size_t total = sizeof(CsonStr);
    if (string.value != NULL){
        total += string.len + 1;
    }
    return total;
}

#define cson_print_indent(file, indent) (fprintf((file), "%*s", (int)(indent)*CSON_PRINT_INDENT, ""))

void cson_fprint(Cson *value, FILE *file, size_t indent)
{
    if (value == NULL || file == NULL) return;
    switch (value->type){
        case Cson_Int:{
            fprintf(file, "%"PRId64, value->value.integer);
        }break;
        case Cson_Float:{
            fprintf(file, "%lf", value->value.floating);
        }break;
        case Cson_Bool:{
            fprintf(file, "%s", value->value.boolean? "true":"false");
        }break; 
        case Cson_String:{
            fprintf(file, "\"%s\"", value->value.string.value);
        }break;
        case Cson_Null:{
            fprintf(file, "null");
        }break;
        case Cson_Array:{
            cson_array_fprint(value->value.array, file, indent);
        }break;
        case Cson_Map:{
            cson_map_fprint(value->value.map, file, indent);
        }break;
        default:{
            cson_error(CsonError_InvalidType, "Invalid value type: %s", CsonErrorStrings[value->type]);
            return;
        }
    }
}

void cson_array_fprint(CsonArray *array, FILE *file, size_t indent)
{
    fprintf(file, "[\n");
    for (size_t i=0; i<array->size; ++i){
        cson_print_indent(file, indent+1);
        cson_fprint(array->items[i], file, indent+1);
        fprintf(file, "%c\n", (i+1 == array->size)? ' ':',');
    }
    cson_print_indent(file, indent);
    fputc(']', file);
}

void cson_map_fprint(CsonMap *map, FILE *file, size_t indent)
{
    fprintf(file, "{\n");
    size_t size = map->size;
    for (size_t i=0; i<map->capacity; ++i){
        CsonMapItem *value = map->items[i];
        while (value){
            cson_print_indent(file, indent+1);
            fprintf(file, "\"%s\": ", value->key.value);
            cson_fprint(value->value, file, indent+1);
            fprintf(file, "%c\n", (--size > 0)? ',':' ');
            value = value->next;
        }
    }
    cson_print_indent(file, indent);
    fputc('}', file);
}

bool cson_write(Cson *json, char *filename)
{
    if (json == NULL || filename == NULL) return false;
    FILE *file = fopen(filename, "w");
    if (file == NULL){
        cson_error(CsonError_FileNotFound, "Could not find file: \"%s\"", filename);
        return false;
    }
    cson_fprint(json, file, 0);
    fclose(file);
    return true;
}

CsonLexer cson_lex_init(char *buffer, size_t buffer_size, char *filename)
{
    return (CsonLexer) {.buffer=buffer, .buffer_size=buffer_size, .index=0, .loc=(CsonLoc){.filename=filename, .row=1, .column=1}};
}

bool cson_lex_next(CsonLexer *lexer, CsonToken *token)
{
    if (lexer == NULL || token == NULL || lexer->index > lexer->buffer_size) return false;
    cson_lex_trim_left(lexer);
    char *t_start = cson_lex_get_pointer(lexer);
    CsonLoc t_loc = lexer->loc;
    switch (cson_lex_get_char(lexer)){
        case '{':{
            cson_lex_set_token(token, CsonToken_MapOpen, t_start, t_start+1, t_loc);
            break;
        }
        case '}':{
            cson_lex_set_token(token, CsonToken_MapClose, t_start, t_start+1, t_loc);
            break;
        }
        case '[':{
            cson_lex_set_token(token, CsonToken_ArrayOpen, t_start, t_start+1, t_loc);
            break;
        }
        case ']':{
            cson_lex_set_token(token, CsonToken_ArrayClose, t_start, t_start+1, t_loc);
            break;
        }
        case ',':{
            cson_lex_set_token(token, CsonToken_Sep, t_start, t_start+1, t_loc);
            break;
        }
        case ':':{
            cson_lex_set_token(token, CsonToken_MapSep, t_start, t_start+1, t_loc);
            break;
        }
        case '"':{
            // lex string
            cson_lex_inc(lexer);
            char *s_start = cson_lex_get_pointer(lexer);
            if (!cson_lex_find(lexer, '"')){
                cson_error(CsonError_UnclosedString, "Missing closing delimeter for '\"' at " CSON_LOC_FMT "\n", cson_loc_expand(t_loc));
                return false;
            }
            char *s_end = cson_lex_get_pointer(lexer);
            cson_lex_set_token(token, CsonToken_String, s_start, s_end, t_loc);
            break;
        }
        case '\0':
        case EOF:{
            cson_lex_set_token(token, CsonToken_End, t_start, t_start+1, t_loc);
            cson_lex_inc(lexer);
            return false;
        }
        default:{
            // multi-character literal
            // find end of literal
            char c;
            while (lexer->index < lexer->buffer_size && !cson_lex_is_delimeter(c = cson_lex_get_char(lexer))){
                cson_lex_check_line(lexer, c);
                lexer->index++;
            }
            char *t_end = cson_lex_get_pointer(lexer);
            size_t t_len = t_end-t_start;
            // check for known literals
            if (memcmp(t_start, "true", t_len) == 0){
                cson_lex_set_token(token, CsonToken_True, t_start, t_end, t_loc);
                return true;
            }
            if (memcmp(t_start, "false", t_len) == 0){
                cson_lex_set_token(token, CsonToken_False, t_start, t_end, t_loc);
                return true;
            }
            if (memcmp(t_start, "null", t_len) == 0){
                cson_lex_set_token(token, CsonToken_Null, t_start, t_end, t_loc);
                return true;
            }
            if (cson_lex_is_int(t_start, t_end)){
                cson_lex_set_token(token, CsonToken_Int, t_start, t_end, t_loc);
                return true;
            }
            if (cson_lex_is_float(t_start, t_end)){
                cson_lex_set_token(token, CsonToken_Float, t_start, t_end, t_loc);
                return true;
            }
            cson_error(CsonError_InvalidType, "Invalid literal \"%.*s\" at "CSON_LOC_FMT, (int)t_len, t_start, cson_loc_expand(t_loc));
            cson_lex_set_token(token, CsonToken_Invalid, t_start, t_end, t_loc);
            return false;
        }
    }
    cson_lex_inc(lexer);
    return true;
}

bool cson__lex_expect(CsonLexer *lexer, CsonToken *token, CsonTokenType types[], size_t count, char *file, size_t line)
{
    if (lexer == NULL || token == NULL) return false;
    cson_lex_next(lexer, token);
    for (size_t i=0; i<count; ++i){
        if (token->type == types[i]) return true;
    }
    cson__error_unexpected(token->loc, types, count, token->type, file, line);
    return false;  
}

bool cson_lex_extract(CsonToken *token, char *buffer, size_t buffer_size)
{
    if (token == NULL || buffer == NULL || buffer_size == 0) return false;
    if (token->len >= buffer_size) return false;
    if (token->type == CsonToken_String){
        char temp_buffer[token->len+1];
        memset(temp_buffer, 0, token->len+1);
        char *r = token->t_start;
        char *w = temp_buffer;
        while (r != token->t_end){
            if (*r == '\\'){
                switch(*++r){
                    case '\'': *w = 0x27; break;
                    case '"':  *w = 0x22; break;
                    case '?':  *w = 0x3f; break;
                    case '\\': *w = 0x5c; break;
                    case 'a':  *w = 0x07; break;
                    case 'b':  *w = 0x08; break;
                    case 'f':  *w = 0x0c; break;
                    case 'n':  *w = 0x0a; break;
                    case 'r':  *w = 0x0d; break;
                    case 't':  *w = 0x09; break;
                    case 'v':  *w = 0x0b; break;
                    default:{
                        *w++ = '\\';
                        *w = *r;
                    }
                }
            }
            else{
                *w = *r;
            }
            r++;
            w++;
        }
        sprintf(buffer, "%.*s", (int) (w-temp_buffer+1), temp_buffer);
    }
    else{
        sprintf(buffer, "%.*s", (int)token->len, token->t_start);
    }
    return true;
}

void cson_lex_print(CsonToken token)
{
    cson_info(CSON_LOC_FMT": %s: '%.*s'\n", cson_loc_expand(token.loc), CsonTokenTypeNames[token.type], (int)(token.t_end-token.t_start), token.t_start);
}

void cson_lex_set_token(CsonToken *token, CsonTokenType type, char *t_start, char *t_end, CsonLoc loc)
{
    if (token == NULL) return;
    token->type = type;
    token->t_start = t_start;
    token->t_end = t_end,
    token->len = t_end-t_start,
    token->loc = loc;
}

bool cson_lex_find(CsonLexer *lexer, char c)
{
    char rc;
    while (lexer->index < lexer->buffer_size){
        if ((rc = cson_lex_get_char(lexer)) == c) return true;
        cson_lex_check_line(lexer, rc);
        lexer->index++;
    }
    return false;
}

void cson_lex_trim_left(CsonLexer *lexer)
{
    char c;
    while (lexer->index <= lexer->buffer_size && cson_lex_is_whitespace((c = cson_lex_get_char(lexer)))){
        cson_lex_check_line(lexer, c);
        lexer->index++;
    }
}

bool cson_lex_is_delimeter(char c)
{
    switch (c){
        case '{':
        case '}':
        case '[':
        case ']':
        case ',':
        case ':':
        case ' ':
        case '\n':
        case '\t':{
            return true;
        }
        default: return false;
    }
}

bool cson_lex_is_int(char *s, char *e)
{
    if (!s || !e || e-s < 1) return false;
    if (*s == '-' || *s == '+') s++;
    while (s < e){
        if (!isdigit(*s++)) return false;
    }
    return true;
}

bool cson_lex_is_float(char *s, char *e)
{
    char* ep = NULL;
    strtod(s, &ep);
    return (ep && ep == e);
}

void cson__error_unexpected(CsonLoc loc, CsonTokenType expected[], size_t expected_count, CsonTokenType actual, char *filename, size_t line)
{
    if (expected_count == 0) return;
    fprintf(stderr, "%s%s:%u [ERROR] (%s): Expected [", cson_ansi_rgb(196, 0, 0), filename, line, CsonErrorStrings[CsonError_UnexpectedToken]);
    size_t i;
    for (i=0; i<expected_count-1; ++i){
        fprintf(stderr, "%s, ", CsonTokenTypeNames[expected[i]]);
    }
    fprintf(stderr, "%s], but got [%s] at "CSON_LOC_FMT CSON_ANSI_END"\n", CsonTokenTypeNames[expected[i]], CsonTokenTypeNames[actual], cson_loc_expand(loc));
}

/* Parser implementation */

bool cson__parse_map(Cson *map, CsonLexer *lexer)
{
    if (map == NULL || map->type != Cson_Map || lexer == NULL) return false;
    CsonToken token;
    while (true){
        cson_lex_next(lexer, &token);
        switch (token.type){
            case CsonToken_String: break;
            case CsonToken_MapClose:{
                if (cson_len(map) > 0){
                    cson_error_unexpected(token.loc, token.type, CSON_VALUE_TOKENS);
                    return false;
                }
                return true;
            }
            default: {
                cson_error_unexpected(token.loc, token.type, CsonToken_String, CsonToken_MapClose);
                return false;
            }
        }
        size_t key_size = token.len+1;
        char key_buffer[key_size];
        cson_lex_extract(&token, key_buffer, key_size);
        if (!cson_lex_expect(lexer, &token, CsonToken_MapSep)) return false;
        Cson *cson = NULL;
        if (!cson_lex_expect(lexer, &token, CSON_VALUE_TOKENS)) return false;
        if (!cson__parse_value(&cson, lexer, &token)) return false;
        cson_map_insert(map, cson_str_new(key_buffer), cson);
        if (!cson_lex_expect(lexer, &token, CsonToken_Sep, CsonToken_MapClose)) return false;
        switch (token.type){
            case CsonToken_Sep:break;
            case CsonToken_MapClose:return true;
            default: {}
        }
    }
}

bool cson__parse_array(Cson *array, CsonLexer *lexer)
{
    if (array == NULL || array->type != Cson_Array || lexer == NULL) return false;
    CsonToken token;
    while (true){
        if (!cson_lex_expect(lexer, &token, CSON_VALUE_TOKENS, CsonToken_ArrayClose)) return false;
        if (token.type == CsonToken_ArrayClose){
            if (cson_len(array) > 0){
                cson_error_unexpected(token.loc, token.type, CSON_VALUE_TOKENS);
                return false;
            }
            return true;;
        }
        Cson *cson = NULL;
        if (!cson__parse_value(&cson, lexer, &token)) return false;
        cson_array_push(array, cson);
        if (!cson_lex_expect(lexer, &token, CsonToken_Sep, CsonToken_ArrayClose)) return false;
        switch (token.type){
            case CsonToken_Sep: break;
            case CsonToken_ArrayClose: return true;
            default: {}
        }
    }
    return false;
}

bool cson__parse_value(Cson **cson, CsonLexer *lexer, CsonToken *token)
{
    if (cson == NULL || lexer == NULL || token == NULL) return false;
    size_t buff_size = token->len+1;
    char buffer[buff_size];
    cson_lex_extract(token, buffer, buff_size);
    switch (token->type){
        case CsonToken_ArrayOpen:{
            Cson *array = cson_array_new();
            if (!cson__parse_array(array, lexer)) return false;
            *cson = array;
        }break;
        case CsonToken_MapOpen:{
            Cson *map = cson_map_new();
            if (!cson__parse_map(map, lexer)) return false;
            *cson = map;
        }break;
        case CsonToken_Int:{
            *cson = cson_new_int(atoll(buffer));
        }break;
        case CsonToken_Float:{
            *cson = cson_new_float(atof(buffer));
        }break;
        case CsonToken_String:{
            *cson = cson_new_cstring(buffer);
        }break;
        case CsonToken_True:{
            *cson = cson_new_bool(true);
        }break;
        case CsonToken_False:{
            *cson = cson_new_bool(false);
        }break;
        case CsonToken_Null:{
            *cson = cson_new_null();
        }break;
        default: {}
    }
    return true;
}

#define cson_parse(buffer, buffer_size) cson_parse_buffer(buffer, buffer_size, "")

Cson* cson_parse_buffer(char *buffer, size_t buffer_size, char *filename)
{
    if (buffer == NULL || buffer_size == 0) return NULL;
    CsonLexer lexer = cson_lex_init(buffer, buffer_size, filename);
    CsonToken token;
    if (!cson_lex_next(&lexer, &token)){
        if (token.type == CsonToken_End){
            cson_error(CsonError_EndOfBuffer, "file is empty: \"%s\"", filename);
        }
        return NULL;
    }
    Cson *cson = NULL;
    switch(token.type){
        case CsonToken_ArrayOpen:{
            Cson *array = cson_array_new();
            if (cson__parse_array(array, &lexer)){
                cson = array;
            }
        }break;
        case CsonToken_MapOpen:{
            Cson *map = cson_map_new();
            if (cson__parse_map(map, &lexer)){
                cson = map;
            }
        }break;
        default:{
            cson_error(CsonError_UnexpectedToken, CSON_LOC_FMT": json object may only start with [%s, %s] and not [%s]", cson_loc_expand(token.loc), CsonTokenTypeNames[CsonToken_ArrayOpen], CsonTokenTypeNames[CsonToken_MapOpen], CsonTokenTypeNames[token.type]);
            return NULL;
        }
    }
    if (cson != NULL && !cson_lex_expect(&lexer, &token, CsonToken_End)){
        cson_error(CsonError_UnexpectedToken, "json object may not have trailing values after closing of parent %s!", CsonTypeStrings[cson->type]);
        return NULL;
    }
    return cson;
}

Cson* cson_read(char *filename){
    FILE *file = fopen(filename, "r");
    if (file == NULL){
        cson_error(CsonError_FileNotFound, "Could open file: \"%s\"", filename);
        return NULL;
    }
    uint64_t file_size = cson_file_size(filename);
    char *file_content = (char*) calloc(file_size+1, sizeof(*file_content));
    cson_assert_alloc(file_content);
    fread(file_content, 1, file_size, file);
    fclose(file);
    Cson *cson = cson_parse_buffer(file_content, file_size, filename);
    free(file_content);
    return cson;
}
