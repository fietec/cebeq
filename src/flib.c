#include <flib.h>

bool flib_read(const char *path, flib_cont *fc)
{
    if (fc == NULL) return false;
    FILE *file = fopen(path, "r");
    if (file == NULL){
        flib_error("Could not open file '%s'!", path);
        return false;
    }
    fsize_t size = flib_size(path);
    char *content = (char*) flib_calloc(size+1, sizeof(char));
    assert(content != NULL && "Out of memory!");
    if (fread(content, sizeof(char), size, file) <= 0){
        flib_error("Could not read file '%s'!", path);
        fclose(file);
        return false;
    }
    fclose(file);
    fc->buffer = content;
    fc->size = size;
    return true;
}

bool create_dir(const char *path)
{
    if (mkdir(path) == -1){
        eprintfn("Could not create directory '%s': %s!", path, strerror(errno));
        return false;
    }
    return true;
}

int flib_delete_dir(const char *path)
{
    DIR *dir = opendir(path);
    if (dir == NULL){
        flib_error("Could not find dir '%s'!", path);
        return 1;
    }
    flib_entry entry;
    while (flib_get_entry(dir, path, &entry)){
        if (entry.type == FLIB_DIR){
            flib_delete_dir(entry.path);
        } else{
            unlink(entry.path);
        }
    }
    closedir(dir);
    return rmdir(path);
}

int copy_file(const char *from, const char *to)
{
  #ifdef _WIN32
    if (CopyFile(from, to, false) == 0){
        eprintfn("Failed to copy '%s' -> '%s'", from, to);
        return 1;
    }
    return 0;
  #endif // _WIN32
    int fd_to, fd_from;
    char buffer[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0){
        eprintfn("Could not read file '%s'!", from);
        return 1;
    }

    fd_to = open(to, O_WRONLY | O_CREAT, 0666);
    if (fd_to < 0){
        eprintfn("Could not write file '%s'!", to);
        return 1;
    }
    while (nread = read(fd_from, buffer, sizeof(buffer)), nread > 0){
        char *out_ptr = buffer;
        ssize_t nwritten;
        do {
            nwritten = write(fd_to, out_ptr, nread);
            if (nwritten >= 0){
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR){
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0){
        if (close(fd_to) < 0){
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);
        return 0;
    }
  out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0) close(fd_to);

    errno = saved_errno;
    eprintfn("Failed to copy '%s' -> '%s': %s!", from, to, strerror(errno));
    return 1;
}

int copy_dir_rec(const char *src, const char *dest)
{
    if (!flib_isdir(dest)){
        eprintfn("Not a valid directory: '%s'!", dest);
        return 1;
    }
    DIR *dir = opendir(src);
    if (dir == NULL){
        eprintfn("Not a valid directory: '%s'!", src);
        return 1;
    }
    struct dirent *entry;
    char item_src_path[FILENAME_MAX] = {0};
    char item_dest_path[FILENAME_MAX] = {0};
    while ((entry = readdir(dir)) != NULL){
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        cwk_path_join(src, entry->d_name, item_src_path, FILENAME_MAX);
        cwk_path_join(dest, entry->d_name, item_dest_path, FILENAME_MAX);
        switch (entry->d_type){
            case DT_REG:{
                copy_file(item_src_path, item_dest_path);
            }break;
            case DT_DIR:{
                if (!create_dir(item_dest_path)) continue;
                copy_dir_rec(item_src_path, item_dest_path);
            }break;
            default: break;
        }
    }
    closedir(dir);
    return 0;
}

int copy_dir_rec_ignore(const char *src, const char *dest, const char **ignore, size_t ignore_count)
{
    if (!flib_isdir(dest)){
        eprintfn("Not a valid directory: '%s'!", dest);
        return 1;
    }
    DIR *dir = opendir(src);
    if (dir == NULL){
        eprintfn("Not a valid directory: '%s'!", src);
        return 1;
    }
    struct dirent *entry;
    char item_src_path[FILENAME_MAX] = {0};
    char item_dest_path[FILENAME_MAX] = {0};
    while ((entry = readdir(dir)) != NULL){
        for (size_t i=0; i<ignore_count; ++i){
            if (strcmp(entry->d_name, ignore[i]) == 0) continue;
        }
        cwk_path_join(src, entry->d_name, item_src_path, FILENAME_MAX);
        cwk_path_join(dest, entry->d_name, item_dest_path, FILENAME_MAX);
        switch (entry->d_type){
            case DT_REG:{
                copy_file(item_src_path, item_dest_path);
            }break;
            case DT_DIR:{
                if (!create_dir(item_dest_path)) continue;
                copy_dir_rec(item_src_path, item_dest_path);
            }break;
            default: break;
        }
    }
    closedir(dir);
    return 0;
}

fsize_t flib_size(const char *path)
{
    struct stat attr;
    if (stat(path, &attr) == -1){
        return 0;
    }
    return (fsize_t) attr.st_size;
}

bool flib_exists(const char *path)
{
    struct stat attr;
    return stat(path, &attr) == 0;
}

bool flib_isfile(const char *path)
{
    struct stat attr;
    if (stat(path, &attr) == -1) return false;
    return S_ISREG(attr.st_mode);
}

bool flib_isdir(const char *path)
{
    struct stat attr;
    if (stat(path, &attr) == -1) return false;
    return S_ISDIR(attr.st_mode);
}

bool flib_get_entry(DIR *dir, const char *dir_path, flib_entry *entry)
{
    if (dir == NULL || dir_path == NULL || entry == NULL) return false;
    struct dirent *d_entry = readdir(dir);
    if (d_entry == NULL) return false;
    strncpy(entry->name, d_entry->d_name, FILENAME_MAX);
    if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0) return flib_get_entry(dir, dir_path, entry);
    (void)cwk_path_join(dir_path, d_entry->d_name, entry->path, sizeof(entry->path));
    switch (d_entry->d_type){
        case DT_DIR:{
            entry->type = FLIB_DIR;
        } break;
        case DT_REG:{
            entry->type = FLIB_FILE;
            struct stat attr;
            if (stat(entry->path, &attr) == -1){
                eprintfn("Could not access '%s': %s\n", entry->path, strerror(errno));
                return false;
            }
            entry->size = attr.st_size;
            entry->mod_time = attr.st_mtime;
        } break;
        default:{
            entry->type = FLIB_UNSP;
        }
    }
    return true;
}

fsize_t flib_dir_size(DIR *dir, const char *dir_path)
{
    if (dir == NULL || dir_path == NULL) return FLIB_SIZE_ERROR;
    fsize_t size = 0;
    struct dirent *d_entry;
    char path[FILENAME_MAX] = {0};
    while ((d_entry = readdir(dir)) != NULL){
        if (d_entry->d_type == DT_REG){
            (void)cwk_path_join(dir_path, d_entry->d_name, path, sizeof(path));
            struct stat attr;
            if (stat(path, &attr) == -1){
                eprintfn("Could not access '%s': %s!", path, strerror(errno));
                continue;
            }
            size += attr.st_size;
        }
    }
    return size;
}

fsize_t flib_dir_size_rec(DIR *dir, const char *dir_path)
{
    if (dir == NULL || dir_path == NULL) return FLIB_SIZE_ERROR;
    fsize_t size = 0;
    struct dirent *d_entry;
    char path[FILENAME_MAX] = {0};
    while ((d_entry = readdir(dir)) != NULL){
        (void)cwk_path_join(dir_path, d_entry->d_name, path, sizeof(path));
        switch (d_entry->d_type){
            case DT_REG: {
                struct stat attr;
                if (stat(path, &attr) == -1){
                    eprintfn("Could not access file '%s': %s!", path, strerror(errno));
                    continue;
                }
                size += attr.st_size;
            } break;
            case DT_DIR: {
                DIR *sub_dir = opendir(path);
                if (sub_dir == NULL){
                    eprintfn("Could not find dir '%s': %s!", path, strerror(errno));
                    continue;
                }
                size += flib_dir_size_rec(sub_dir, path);
                closedir(sub_dir);
            } break;
        }
    }
    return size;
}

void flib_print_entry(flib_entry entry)
{
    switch(entry.type){
        case FLIB_FILE:{
            printf("File: '%s' '%s', size: %"PRIu64, entry.path, entry.name, entry.size);
            struct tm *time_info;
            char time_string[20];
            time_info = localtime(&entry.mod_time);
            strftime(time_string, sizeof(time_string), "%d.%m.%Y %H:%M:%S", time_info);
            printf(", last_mod: %s\n", time_string);
        } break;
        case FLIB_DIR:{
            printf("Dir: '%s' ('%s')\n", entry.path, entry.name);
        } break;
        default:{
            printf("Unsupported: '%s' ('%s')\n", entry.path, entry.name);
        }    
    }
}