#ifndef _FLIB_H
#define _FLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#ifdef _WIN32
    #include <windows.h>
#endif // _WIN32

/* Dependencies */
#include <cwalk.h> // https://github.com/likle/cwalk

#ifndef FLIB_SILENT
    #define flib_error(msg, ...) (fprintf(stderr, "[ERROR] "msg"\n", __VA_ARGS__))
#else
    #define flib_error(msg, ...)
#endif // FLIB_SILENT

#ifndef flib_calloc
#define flib_calloc calloc
#endif // flib_calloc

#define ansi_rgb(r, g, b) ("\e[38;2;" #r ";" #g ";" #b "m") 
#define ansi_end ("\e[0m") 
#define eprintfn(msg, ...) (fprintf(stderr, "%s[ERROR] %s:%d in %s: " msg "\n%s", ansi_rgb(196, 0, 0), __FILE__, __LINE__, __func__, ##__VA_ARGS__, ansi_end))

#define FLIB_SIZE_ERROR (fsize_t)-1

typedef uint64_t fsize_t;

typedef struct{
    char *buffer;
    fsize_t size;
} flib_cont;

typedef enum{
    FLIB_FILE,
    FLIB_DIR,
    FLIB_UNSP
} flib_type;

typedef struct{
    char name[FILENAME_MAX];
    char path[FILENAME_MAX];
    flib_type type;
    fsize_t size;
    time_t mod_time;
} flib_entry;

bool flib_read(const char *path, flib_cont *fc);
fsize_t flib_size(const char *path);
bool flib_exists(const char *path);
bool flib_isfile(const char *path);
bool flib_isdir(const char *path);

bool create_dir(const char *path);
int flib_delete_dir(const char *path);
int copy_file(const char *from, const char *to);
int copy_dir_rec(const char *src, const char *dest);
int copy_dir_rec_ignore(const char *src, const char *dest, const char **ignore_names, size_t ignore_count);

bool flib_get_entry(DIR *dir, const char *path, flib_entry *entry);
fsize_t flib_dir_size(DIR *dir, const char *path);
fsize_t flib_dir_size_rec(DIR *dir, const char *path);
void flib_print_entry(flib_entry entry);

#endif // _FLIB_H
