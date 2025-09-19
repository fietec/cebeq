#include <flib.h>

char *long_path_buf = NULL;
#ifdef _WIN32
    LPVOID win_get_last_error(void) 
    { 
        LPVOID lpMsgBuf;
        DWORD dw = GetLastError(); 

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL );
        return lpMsgBuf;
    }
    
    const char *win_long_path(const char *path)
    {
        if (!long_path_buf) {
            long_path_buf = calloc(1, MAX_LONG_PATH);
            assert(long_path_buf != NULL && "Buy more RAM lol");
        }
        if (strncmp(path, "\\\\?\\", 4) == 0) return path;
        snprintf(long_path_buf, MAX_LONG_PATH, "\\\\?\\%s", path);
        return long_path_buf;
    }
#endif // _WIN32

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

bool flib_create_dir(const char *path)
{
  #ifdef _WIN32
    const char *long_path = win_long_path(path);
    if (mkdir(long_path) == -1){
        LPVOID error = win_get_last_error();
        eprintf("Could not create directory '%s': %s!", long_path, (char*) error);
        LocalFree(error);
        return false;
    }
  #else
	if (mkdir(path, 0700) == -1){
        eprintf("Could not create directory '%s': %s!", path, strerror(errno));
        return false;
    }
  #endif // _WIN32
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

int flib_copy_file(const char *from, const char *to)
{
#ifdef _WIN32
    const char *long_path = win_long_path(to);
    if (CopyFile(from, long_path, false) == 0){
        LPVOID error = win_get_last_error();
        eprintf("Failed to copy '%s' -> '%s': %s", from, long_path, (char*) error);
        LocalFree(error);
        return 1;
    }
    return 0;
#else
    int fd_to = -1, fd_from = -1;
    char buffer[4096];
    ssize_t nread;
    int saved_errno;
    struct stat st;

    if (stat(from, &st) < 0) {
        eprintf("Could not stat file '%s': %s\n", from, strerror(errno));
        return 1;
    }

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0){
        eprintf("Could not read file '%s': %s\n", from, strerror(errno));
        return 1;
    }

    fd_to = open(to, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777);
    if (fd_to < 0){
        eprintf("Could not write file '%s': %s\n", to, strerror(errno));
        close(fd_from);
        return 1;
    }

    while ((nread = read(fd_from, buffer, sizeof(buffer))) > 0){
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
        // Copy ownership (ignore errors if not root)
        fchown(fd_to, st.st_uid, st.st_gid);

        // Copy permissions (in case umask interfered)
        fchmod(fd_to, st.st_mode & 0777);

        // Copy timestamps
#if defined(HAVE_FUTIMENS) || (_POSIX_C_SOURCE >= 200809L)
        struct timespec times[2];
        times[0] = st.st_atim;
        times[1] = st.st_mtim;
        futimens(fd_to, times);
#endif

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
    eprintf("Failed to copy '%s' -> '%s': %s!", from, to, strerror(errno));
    return 1;
#endif
}

int flib_copy_dir_rec(const char *src, const char *dest)
{
    if (!flib_isdir(dest)){
        eprintf("Not a valid directory: '%s'!", dest);
        return 1;
    }
    DIR *dir = opendir(src);
    if (dir == NULL){
        eprintf("Not a valid directory: '%s'!", src);
        return 1;
    }
    struct dirent *entry;
    char item_src_path[FILENAME_MAX] = {0};
    char item_dest_path[FILENAME_MAX] = {0};
    struct stat attr;
    while ((entry = readdir(dir)) != NULL){
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        cwk_path_join(src, entry->d_name, item_src_path, FILENAME_MAX);
        cwk_path_join(dest, entry->d_name, item_dest_path, FILENAME_MAX);
        if (stat(item_src_path, &attr) == -1){
            eprintf("Could not access '%s': %s\n", item_src_path, strerror(errno));
            continue;
        }
        if (S_ISREG(attr.st_mode)){
            flib_copy_file(item_src_path, item_dest_path);
        } else if (S_ISDIR(attr.st_mode)){
            if (!flib_create_dir(item_dest_path)) continue;
            flib_copy_dir_rec(item_src_path, item_dest_path);
        }else{
            break;
        }
    }
    closedir(dir);
    return 0;
}

int flib_copy_dir_rec_ignore(const char *src, const char *dest, const char **ignore, size_t ignore_count)
{
    if (!flib_isdir(dest)){
        eprintf("Not a valid directory: '%s'!", dest);
        return 1;
    }
    DIR *dir = opendir(src);
    if (dir == NULL){
        eprintf("Not a valid directory: '%s'!", src);
        return 1;
    }
    
    struct stat attr;
    struct dirent *entry;
    char item_src_path[FILENAME_MAX] = {0};
    char item_dest_path[FILENAME_MAX] = {0};
    while ((entry = readdir(dir)) != NULL){
        for (size_t i=0; i<ignore_count; ++i){
            if (strcmp(entry->d_name, ignore[i]) == 0) continue;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        cwk_path_join(src, entry->d_name, item_src_path, FILENAME_MAX);
        cwk_path_join(dest, entry->d_name, item_dest_path, FILENAME_MAX);
        if (stat(item_src_path, &attr) == -1){
            eprintf("Could not access '%s': %s\n", item_src_path, strerror(errno));
            continue;
        }
        if (S_ISREG(attr.st_mode)){
            flib_copy_file(item_src_path, item_dest_path);
        } else if (S_ISDIR(attr.st_mode)){
            if (!flib_create_dir(item_dest_path)) continue;
            flib_copy_dir_rec(item_src_path, item_dest_path);
        }else{
            break;
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
    
    struct stat attr;
    if (stat(entry->path, &attr) == -1){
        eprintf("Could not access '%s': %s\n", entry->path, strerror(errno));
        return flib_get_entry(dir, dir_path, entry);
    }
   
    if (S_ISREG(attr.st_mode)){
        entry->type = FLIB_FILE;
        entry->size = attr.st_size;
        entry->mod_time = attr.st_mtime;
    }
    else if (S_ISDIR(attr.st_mode)){
        entry->type = FLIB_DIR;
    }
    else{
        entry->type = FLIB_UNSP;
    }
    return true;
}

fsize_t flib_dir_size(DIR *dir, const char *dir_path)
{
    if (dir == NULL || dir_path == NULL) return FLIB_SIZE_ERROR;
    fsize_t size = 0;
    struct dirent *d_entry;
    char path[FILENAME_MAX] = {0};
    struct stat attr;
    while ((d_entry = readdir(dir)) != NULL){
        (void)cwk_path_join(dir_path, d_entry->d_name, path, sizeof(path));
        if (stat(path, &attr) == -1){
            eprintf("Could not access '%s': %s!", path, strerror(errno));
            continue;
        }
        if (S_ISREG(attr.st_mode)){
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
    struct stat attr;
    while ((d_entry = readdir(dir)) != NULL){
        (void)cwk_path_join(dir_path, d_entry->d_name, path, sizeof(path));
        if (stat(path, &attr) == -1){
            eprintf("Could not access file '%s': %s!", path, strerror(errno));
            continue;
        }
        if (S_ISREG(attr.st_mode)){
            size += attr.st_size;
        } else if (S_ISDIR(attr.st_mode)){
            DIR *sub_dir = opendir(path);
            if (sub_dir == NULL){
                eprintf("Could not find dir '%s': %s!", path, strerror(errno));
                continue;
            }
            size += flib_dir_size_rec(sub_dir, path);
            closedir(sub_dir);
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
