#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <cebeq.h>
#include <cson.h>
#include <cwalk.h>
#include <flib.h>

bool path_in_backup(const char *path, const char *backup)
{
    if (path == NULL || backup == NULL) return false;
    bool value = true;
    char info_path[FILENAME_MAX] = {0};
    cwk_path_join(backup, INFO_FILE, info_path, FILENAME_MAX);
    
    Cson *info = cson_read(info_path);
    if (info == NULL){
        eprintfn("Could not find backup file '%s'!", info_path);
        return_defer(false);
    }
    Cson *dirs = cson_get(info, key("dirs"));
    if (dirs == NULL){
        eprintfn("Could not find a 'dirs' entry in '%s'!", info_path);
        return_defer(false);
    }
    for (size_t i=0; i<cson_len(dirs); ++i){
        CsonStr dir = cson_get_string(dirs, index(i));
        if (strcmp(path, dir.value) == 0) return_defer(true);
    }
    return_defer(false);
  defer:
    return value;
}

int make_backup_rec(const char *src, const char *dest, const char *prev)
{
    int value = 0;
    DIR *dir = opendir(src);
    if (dir == NULL){
        eprintfn("This is no valid src directory: '%s'!", src);
        return 1;
    }
    if (!flib_isdir(dest)){
        eprintfn("This is no valid dest directory: '%s'!", dest);
        closedir(dir);
        return 1;
    }
    if (prev != NULL && !flib_isdir(prev)){
        eprintfn("This is no valid previous directory: '%s'!", prev);
        closedir(dir);
        return 1;
    }
    
    Cson *prev_files = NULL;
    Cson *prev_dirs = NULL;
    if (prev != NULL){
        char prev_info_path[FILENAME_MAX];
        cwk_path_join(prev, INFO_FILE, prev_info_path, FILENAME_MAX);
        Cson *prev_info = cson_read(prev_info_path);
        if (prev_info == NULL){
            eprintfn("Could not find any '%s' file in '%s'!", INFO_FILE, prev);
            return_defer(1);
        }
        prev_files = cson_get(prev_info, key("files"));
        prev_dirs = cson_get(prev_info, key("dirs"));
        if (prev_files == NULL || prev_dirs == NULL){
            eprintfn("Invalid '%s' file: '%s'!", INFO_FILE, prev_info_path);
            return_defer(1);
        }
    }
    
    Cson *root = cson_map_new();
    Cson *files = cson_map_new();
    Cson *dirs = cson_map_new();
    
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
                if (prev_dirs != NULL){
                    Cson *prev_dir = cson_map_get(prev_dirs, entry_key);
                    if (prev_dir != NULL){
                        if (cson_map_remove(prev_dirs, entry_key) != CsonError_Success) return_defer(1);
                        cson_map_insert(dirs, entry_key, cson_new_int(1));
                        char *p = NULL;
                        if (prev != NULL){
                            cwk_path_join(prev, entry.name, item_prev_path, FILENAME_MAX);
                            p = item_prev_path;
                        }
                        if (!create_dir(item_dest_path)) return_defer(1);
                        if (make_backup_rec(entry.path, item_dest_path, p) == 1) return_defer(1);
                        continue;
                    }
                }
                cson_map_insert(dirs, entry_key, cson_new_int(0));
                if (!create_dir(item_dest_path)) return_defer(1);
                if (make_backup_rec(entry.path, item_dest_path, NULL) == 1) return_defer(1);
                
            } break;
            default : {
                eprintfn("Unsupported file type of '%s'!", entry.path);
            }
        }
    }
    if (prev_files != NULL){
        Cson *deleted_files = cson_map_keys(prev_files);
        for (size_t i=0; i<cson_len(deleted_files); ++i){
            CsonStr del_file = cson_get_string(cson_array_get(deleted_files, i));
            cson_map_insert(files, del_file, cson_new_int(-1));
        }
    }
    if (prev_dirs != NULL){
        Cson *deleted_dirs = cson_map_keys(prev_dirs);
        for (size_t i=0; i<cson_len(deleted_dirs); ++i){
            CsonStr del_dir = cson_get_string(cson_array_get(deleted_dirs, i));
            cson_map_insert(dirs, del_dir, cson_new_int(-1));
        }
    }
    
    cson_map_insert(root, cson_str_new("files"), files);
    cson_map_insert(root, cson_str_new("dirs"), dirs);
    char parent[FILENAME_MAX] = {0};
    if (prev == NULL){
        cson_map_insert(root, cson_str_new("parent"), cson_new_null());
    } else{
        cwk_path_normalize(prev, parent, FILENAME_MAX);
        cson_map_insert(root, cson_str_new("parent"), cson_new_cstring((char*) parent));
    }
    cwk_path_join(dest, INFO_FILE, item_dest_path, FILENAME_MAX);
    if (!cson_write(root, item_dest_path)) return_defer(1);
    
  defer:
    closedir(dir);
    return value;
}

int backup_init(const char *src, const char *dest, const char *parent)
{
    if (parent != NULL){
        if (!path_in_backup(src, parent)){
            eprintfn("'%s' is not included in parent backup '%s'!", src, parent);
            return 1;
        }
    }
    char dest_path[FILENAME_MAX] = {0};
    char parent_path[FILENAME_MAX] = {0};
    const char *name = NULL;
    size_t name_length = 0;
    cwk_path_get_basename(src, &name, &name_length);
    cwk_path_join(dest, name, dest_path, FILENAME_MAX);
    if (!create_dir(dest_path)) return 1;
    if (parent == NULL){
        return make_backup_rec(src, dest_path, NULL);
    }
    cwk_path_join(parent, name, parent_path, FILENAME_MAX);
    return make_backup_rec(src, dest_path, parent_path);
}

int make_backup(const char *branch_name, const char *dest, const char *parent)
{
    if (branch_name == NULL || dest == NULL){
        eprintfn("Invalid arguments: branch_name=%p, dest=%p", branch_name, dest);
        return 1;
    }
    if (!flib_isdir(dest)){
        eprintfn("This is no valid dest directory: '%s'!", dest);
        return 1;
    }
    if (parent != NULL && !flib_isdir(parent)){
        eprintfn("Parent backup no longer exists: '%s'!", parent);
        return 1;
    }
    
    int value = 0;
    
    CsonArena arena = {0};
    CsonArena *prev_arena = cson_current_arena;
    cson_swap_arena(&arena);
    
    char backups_path[FILENAME_MAX];
    cwk_path_join(program_dir, BACKUPS_JSON, backups_path, sizeof(backups_path));
    
    Cson *backups = cson_read(backups_path);
    if (backups == NULL){
        eprintfn("Could not find backups file '%s'!", backups_path);
        return_defer(1);
    }
    Cson *branch = cson_map_get(backups, cson_str((char*) branch_name));
    if (branch == NULL){
        eprintfn("Could not find a branch with name '%s'!", branch_name);
        return_defer(1);
    }
    Cson *last_id = cson_get(branch, key("last_id"));
    int64_t id = cson_get_int(last_id) + 1;
    Cson *dirs = cson_map_get(branch, cson_str("dirs"));
    if (dirs == NULL){
        eprintfn("Branch %s is missing directory entry 'dirs'!", branch_name);
        return_defer(1);
    } 
    
    char dest_name[FILENAME_MAX] = {0};
    char dest_path[FILENAME_MAX] = {0};
    snprintf(dest_name, FILENAME_MAX, "%s_%"PRId64, branch_name, id);
    cwk_path_join(dest, dest_name, dest_path, FILENAME_MAX);
    
    if (!create_dir(dest_path)) return_defer(1);
    
    for (size_t i=0; i<cson_len(dirs); ++i){
        const char *src = cson_get_string(cson_array_get(dirs, i)).value;
        if (!flib_isdir(src)){
            eprintfn("Source directory no longer exists: '%s'!", src);
            return_defer(1);
        }
        if (backup_init(src, dest_path, parent) == 1){
            eprintfn("Failed to create backup! Cleaning up..");
            if (flib_delete_dir(dest_path) == 1){
                eprintfn("Failed to delete backup!");
            }
            return_defer(1);
        }
    }
    last_id->value.integer = id;
    // write backup info file
    Cson *root = cson_map_new();
    cson_map_insert(root, cson_str("dirs"), dirs);
    char parent_norm[FILENAME_MAX] = {0};
    if (parent != NULL){
        cwk_path_normalize(parent, parent_norm, FILENAME_MAX);
        cson_map_insert(root, cson_str("parent"), cson_new_string(cson_str((char*)parent_norm)));
    } else{
        cson_map_insert(root, cson_str("parent"), cson_new_null());
    }
    cwk_path_join(dest_path, INFO_FILE, dest_name, FILENAME_MAX);
    cson_write(root, dest_name);
    
    cson_write(backups, BACKUPS_JSON);
    
    iprintfn("Successfully created backup for branch '%s' at '%s'", branch_name, dest_path);
  defer:
    cson_swap_and_free_arena(prev_arena);
    return value;
}
