#include <stdbool.h>
#include <string.h>

#define NOB_NO_MINIRENT
#define NOB_STRIP_PREFIX
#include <nob.h>
#undef ERROR

#include <cebeq.h>
#include <cson.h>
#include <cwalk.h>
#include <flib.h>



int merge_rec(const char *src, const char *dest, Cson *files, Cson *dirs)
{
    DIR *dir = opendir(src);
    if (dir == NULL){
        eprintf("Invalid src directory: '%s'!", src);
        return 1;
    }
    if (!flib_isdir(dest)){
        eprintf("Invalid dest directory: '%s'!", dest);
        closedir(dir);
        return 1;
    }
    
    int result = 0;
    
    char info_path[FILENAME_MAX] = {0};
    cwk_path_join(src, INFO_FILE, info_path, FILENAME_MAX);
    Cson *info = cson_read(info_path);
    if (info == NULL){
        eprintf("Could not find backup info file '%s'!", info_path);
        return_defer(1);
    }
    
    if (files == NULL){
        files = cson_map_get(info, cson_str("files"));
        if (files == NULL){
            eprintf("Could not find a 'files' entry in backup info file '%s'!", info_path);
            return_defer(1);
        }
    }
    if (dirs == NULL){
        dirs = cson_map_get(info, cson_str("dirs"));
        if (dirs == NULL){
            eprintf("Could not find a 'dirs' entry in backup info file '%s'!", info_path);
            return_defer(1);
        }
    }
    
    const char *parent = cson_get_cstring(info, key("parent"));
    
    flib_entry entry;
    char item_dest_path[FILENAME_MAX] = {0};
    while (flib_get_entry(dir, src, &entry)){
        if (entry.type == FLIB_FILE && strcmp(entry.name, INFO_FILE) != 0){
            Cson *info_item = cson_map_get(files, cson_str(entry.name));
            if (info_item == NULL) continue;
            int64_t mod_time = cson_get_int(info_item);
            if (mod_time > 0){
                cwk_path_join(dest, entry.name, item_dest_path, FILENAME_MAX);
                (void) copy_file(entry.path, item_dest_path);
            }
            cson_map_remove(files, cson_str(entry.name));
        }
        else if (entry.type == FLIB_DIR){
            Cson *info_item = cson_map_get(dirs, cson_str(entry.name));
            if (info_item == NULL){
                eprintf("Found unregistered directory '%s'!", entry.path);
                return_defer(1);
            }
        }
    }
    
    if (parent == NULL){
        bool found = false;
        Cson *file_keys = cson_map_keys(files);
        for (size_t i=0; i<cson_len(files); ++i){
            eprintf("Could not find any version of '%s'!", cson_get_cstring(file_keys, index(i)));    
            found = true;
        }
        if (found) return_defer(1);
    } else{
        return_defer(merge_rec(parent, dest, files, dirs));
    }
    
  defer:
    closedir(dir);
    return result;
}

int merge_root(const char *src, const char *dest)
{
    DIR *dir = opendir(src);
    if (dir == NULL){
        eprintf("Invalid src directory: '%s'!", src);
        return 1;
    }
    if (!flib_isdir(dest)){
        eprintf("Invalid dest directory: '%s'!", dest);
        closedir(dir);
        return 1;
    }
    
    int result = 0;
    
    CsonArena arena = {0};
    CsonArena *prev_arena = cson_current_arena;
    cson_swap_arena(&arena);
    
    char info_path[FILENAME_MAX] = {0};
    cwk_path_join(src, INFO_FILE, info_path, FILENAME_MAX);
    if (!flib_isfile(info_path)){
        eprintf("Could not find backup info file '%s'!", info_path);
        return_defer(1);
    }
    Cson *info = cson_read(info_path);
    if (info == NULL){
        eprintf("Could not read backup info file '%s'!", info_path);
        return_defer(1);
    }
    
    Cson *files = cson_map_get(info, cson_str("files"));
    if (files == NULL){
        eprintf("Could not find a 'files' entry in backup info file '%s'!", info_path);
        return_defer(1);
    }
    Cson *dirs = cson_map_get(info, cson_str("dirs"));
    if (dirs == NULL){
        eprintf("Could not find a 'dirs' entry in backup info file '%s'!", info_path);
        return_defer(1);
    }
    
    const char *parent = cson_get_cstring(info, key("parent"));
    
    char item_dest_path[FILENAME_MAX] = {0};
    flib_entry entry;
    while (flib_get_entry(dir, src, &entry)){
        cwk_path_join(dest, entry.name, item_dest_path, FILENAME_MAX);
        switch (entry.type){
            case FLIB_FILE:{
                if (strcmp(entry.name, INFO_FILE) == 0) continue;
                Cson *info_item = cson_map_get(files, cson_str(entry.name));
                if (info_item == NULL){
                    eprintf("Found unregistered file '%s'!", entry.path);
                    return_defer(1);
                }
                (void)copy_file(entry.path, item_dest_path);
                (void)cson_map_remove(files, cson_str(entry.name));
            } break;
            case FLIB_DIR: {
                Cson *info_item = cson_map_get(dirs, cson_str(entry.name));
                if (info_item == NULL){
                    eprintf("Found unregistered dir '%s'!", entry.path);
                    return_defer(1);
                }
                if (!flib_isdir(item_dest_path) && !flib_create_dir(item_dest_path)) continue;
                if (merge_root(entry.path, item_dest_path) == 1){
                    eprintf("Failed to merge '%s'!", entry.path);
                }
            } break;
            default: continue;
        }
    }
    if (parent != NULL){
        merge_rec(parent, dest, files, dirs);
    }
        
  defer:
    cson_swap_and_free_arena(prev_arena);
    closedir(dir);
    return result;
}

int merge(const char *src, const char *dest)
{
    DIR *dir = opendir(src);
    if (dir == NULL){
        eprintf("Could not find backup: '%s'!", src);
        return 1;
    }
    if (!flib_isdir(dest)){
        eprintf("Could not find dest dir: '%s'!", dest);
        closedir(dir);
        return 1;
    }
    
    int result = 0;
    char item_dest_path[FILENAME_MAX] = {0};
    flib_entry entry;
    while (flib_get_entry(dir, src, &entry)){
        if (entry.type == FLIB_DIR){
            cwk_path_join(dest, entry.name, item_dest_path, FILENAME_MAX);
            if (!flib_create_dir(item_dest_path)) return_defer(1);
            iprintf("Merging '%s'..", entry.name);
            if (merge_root(entry.path, item_dest_path) == 1){
                eprintf("Failed to merge '%s'!", entry.name);
                flib_delete_dir(item_dest_path);
                return_defer(1);
            }
            iprintf("Successfully merged backups into '%s'!", item_dest_path);
        }
    }
  defer:
    closedir(dir);
    return result;
}

void* tmerge(void *pargs)
{
    thread_args_t *args = (thread_args_t*) pargs;
    (void) merge(args->args[0], args->args[1]);
    worker_done = 1;
    return NULL;
}
