#include <time.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#ifdef _WIN32
    #include <windows.h>
#endif // _WIN32

#define CSON_PARSE
#define CSON_WRITE
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

int make_backup_rec(const char *src, const char *dest, const char *prev)
{
    int value = 0;
    DIR *dir = opendir(src);
    if (dir == NULL){
        eprintfn("This is no valid directory: '%s'!", src);
        return 1;
    }
    if (!flib_isdir(dest)){
        eprintfn("This is no valid directory: '%s'!", dest);
        closedir(dir);
        return 1;
    }
    if (prev != NULL && !flib_isdir(prev)){
        eprintfn("This is no valid directory: '%s'!", prev);
        closedir(dir);
        return 1;
    }
    CsonArena arena = {0};
    CsonArena *prev_arena = cson_current_arena;
    cson_swap_arena(&arena);
    
    CsonMap *prev_files = NULL;
    CsonMap *prev_dirs = NULL;
    if (prev != NULL){
        char prev_info_path[FILENAME_MAX];
        cwk_path_join(prev, INFO_FILE, prev_info_path, FILENAME_MAX);
        Cson *prev_info = cson_read(prev_info_path);
        if (prev_info == NULL){
            eprintfn("Could not find any '%s' file in '%s'!", INFO_FILE, prev);
            return_defer(1);
        }
        prev_files = cson_get_map(prev_info, key("files"));
        prev_dirs = cson_get_map(prev_info, key("dirs"));
        if (prev_files == NULL || prev_dirs == NULL){
            eprintfn("Invalid '%s' file: '%s'!", INFO_FILE, prev_info_path);
            return_defer(1);
        }
    }
    
    CsonMap *root = cson_map_new();
    CsonMap *files = cson_map_new();
    CsonMap *dirs = cson_map_new();
    
    flib_entry entry;
    char item_dest_path[FILENAME_MAX] = {0};
    char item_prev_path[FILENAME_MAX] = {0};
    while (flib_get_entry(dir, src, &entry)){
        cwk_path_join(dest, entry.name, item_dest_path, FILENAME_MAX);
        CsonStr entry_key = cson_str_new(entry.name);
        switch (entry.type){
            case FLIB_FILE:{
                int64_t mod_time = (int64_t) entry.mod_time;
                cson_map_insert(files, entry_key, cson_new_int(mod_time));
                if (prev_files != NULL){
                    Cson *prev_file = cson_map_get(prev_files, entry_key);
                    if (prev_file != NULL){
                        int64_t t = cson_get_int(prev_file);
                        if (cson_map_remove(prev_files, entry_key) != CsonError_Success) return_defer(1);
                        if (t >= entry.mod_time) continue;
                    }
                }
                copy_file(entry.path, item_dest_path);
            } break;
            case FLIB_DIR:{
                if (prev_files != NULL){
                    Cson *prev_dir = cson_map_get(prev_dirs, entry_key);
                    if (prev_dir != NULL){
                        if (cson_map_remove(prev_dirs, entry_key) != CsonError_Success) return_defer(1);
                        cson_map_insert(dirs, entry_key, cson_new_int(1));
                        goto continue_rec;
                    }
                }
                cson_map_insert(dirs, entry_key, cson_new_int(0));
              continue_rec:;
                char *p = NULL;
                if (prev != NULL){
                    cwk_path_join(prev, entry.name, item_prev_path, FILENAME_MAX);
                    p = item_prev_path;
                }
                if (!create_dir(item_dest_path)) return_defer(1);
                if (make_backup_rec(entry.path, item_dest_path, p) == 1) return_defer(1);
            }
            default : {}
        }
    }
    
    cson_map_insert(root, cson_str_new("files"), cson_new_map(files));
    cson_map_insert(root, cson_str_new("dirs"), cson_new_map(dirs));
    Cson *write_cson = cson_new_map(root);
    cwk_path_join(dest, INFO_FILE, item_dest_path, FILENAME_MAX);
    if (!cson_write(write_cson, item_dest_path)) return_defer(1);
    
  defer:
    cson_swap_and_free_arena(prev_arena);
    closedir(dir);
    return value;
}

int make_backup(const char *src, const char *dest, const char *prev)
{
    if (!flib_isdir(src)){
        eprintfn("This is no valid directory: '%s'!", src);
        return 1;
    }
    if (!flib_isdir(dest)){
        eprintfn("This is no valid directory: '%s'!", dest);
        return 1;
    }
    if (prev != NULL && !flib_isdir(prev)){
        eprintfn("This is no valid directory: '%s'!", prev);
        return 1;
    }
    char dest_path[FILENAME_MAX] = {0};
    const char *name = NULL;
    size_t name_length = 0;
    cwk_path_get_basename(src, &name, &name_length);
    cwk_path_join(dest, name, dest_path, FILENAME_MAX);
    if (!create_dir(dest_path)) return 1;
    return make_backup_rec(src, dest_path, prev);
}

int main(void)
{
    char *id = "69";
    Cson *backups = cson_read(FULL_BACKUPS_JSON);
    if (backups == NULL) return 1;
    Cson *backup = cson_get(backups, key(id));
    if (backup == NULL){
        eprintfn("Could not find backup with id '%s'!", id);
        return 1;
    }
    const char *dest = cson_get_string(cson_get(backup, key("dest"))).value;
    const char *prev = cson_get_string(backup, key("prev")).value;
    CsonArray *dirs = array(cson_get(backup, key("dirs")));
    for (size_t i=0; i<dirs->size; ++i){
        const char *src = cson_get_string(cson_array_get(dirs, i)).value;
        iprintfn("Copying dirs '%s' -> '%s'", src, dest);
        make_backup(src, dest, prev);
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
