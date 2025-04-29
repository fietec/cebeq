#include <cebeq.h>
#define CSON_PARSE
#define CSON_WRITE
#include <cson.h>

int main(void)
{
    char *filename = "d:/cdemeer/programming/c/cebeq/data/full_backups.json";
    Cson *cson = cson_read(filename);
    cson_print(cson);
    Cson *dup = cson_new_cson(cson);
    cson_print(dup);
    
    Cson *n = cson_get(cson, key("u32"), key("created"));
    cson__to_int(n) = 69;
    printf("------\n");
    cson_print(cson);
    cson_print(dup);
    
    cson_free();
    return 0;
}