#define CSON_PARSE
#define CSON_WRITE
#include <time.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#ifdef _WIN32
    #include <windows.h>
#endif // _WIN32

#include <cson.h>
#include <syncit.h>
#include <cwalk.h>
#include <flib.h>

#define array(cson) (cson)->value.array 
#define map(cson) (cson)->value.map

int copy_dir(const char *from, const char *to)
{
    if (!flib_isdir(from)){
        eprintfn("Not a valid directory: '%s'!", from);
        return 1;
    }
    if (!flib_isdir(to)){
        eprintfn("Not a valid directory: '%s'!", to);
        return 1;
    }
    char dest[FILENAME_MAX] = {0};
    const char *name;
    size_t name_length = 0;
    cwk_path_get_basename(from, &name, &name_length);
    cwk_path_join(to, name, dest, FILENAME_MAX);
    if (!create_dir(dest)) return 1;
    copy_dir_rec(from, dest);
    return 0;
}

int main(void)
{
    char *id = "u32";
    Cson *backups = cson_read(FULL_BACKUPS_JSON);
    if (backups == NULL) return 1;
    Cson *backup = cson_get(backups, key(id));
    if (backup == NULL){
        eprintfn("Could not find backup with id '%s'!", id);
        return 1;
    }
    const char *dest = cson_get_string(cson_get(backup, key("dest"))).value;
    CsonArray *dirs = array(cson_get(backup, key("dirs")));
    for (size_t i=0; i<dirs->size; ++i){
        const char *src = cson_get_string(cson_array_get(dirs, i)).value;
        iprintfn("Copying dirs '%s' -> '%s'", src, dest);
        copy_dir(src, dest);
    }
    DIR *dir = opendir(dest);
    if (dir == NULL){
        eprintfn("Could not open dir '%s'!", dest);
        return 1;
    }
    flib_entry entry;
    while (flib_get_entry(dir, dest, &entry)){
        flib_print_entry(entry);
    }
    return 0;
}
