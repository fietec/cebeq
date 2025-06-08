#ifndef _CEBEQ_H
#define _CEBEQ_H

#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include <stdbool.h>

#ifdef _WIN32
  #ifdef CEBEQ_EXPORT
    #define CBQLIB __declspec(dllexport)
  #else
    #define CBQLIB __declspec(dllimport)
  #endif
#else
  #define CBQLIB __attribute__((visibility("default")))
#endif // _WIN32

#define PROGRAM_NAME "cebeq"
#define INFO_FILE "." PROGRAM_NAME
#define BACKUPS_JSON "data/backups.json"

#define s_bool(s) ((s)>0? "true": "false")

#define ansi_rgb(r, g, b) ("\e[38;2;" #r ";" #g ";" #b "m") 
#define ansi_end ("\e[0m") 

#define arr_len(arr) (sizeof((arr)) / sizeof((arr)[0])) 

#define return_defer(v) do{value = (v); goto defer;}while(0)

#ifdef CEBEQ_DEBUG
    #define dprintfn(msg, ...) (fprintf(stdout, "%s[DEBUG] %s:%d in %s: " msg "\n%s", ansi_rgb(196, 196, 0), __FILE__, __LINE__, __func__, ##__VA_ARGS__, ansi_end))
    #define iprintfn(msg, ...) (fprintf(stdout, "%s[INFO] %s:%d in %s: " msg "\n%s", ansi_rgb(0, 196, 196), __FILE__, __LINE__, __func__, ##__VA_ARGS__, ansi_end))
    #define eprintfn(msg, ...) (fprintf(stderr, "%s[ERROR] %s:%d in %s: " msg "\n%s", ansi_rgb(196, 0, 0), __FILE__, __LINE__, __func__, ##__VA_ARGS__, ansi_end))
#else
    #define dprintfn(msg, ...) 
    #define iprintfn(msg, ...) (fprintf(stdout, "%s[INFO] " msg "\n%s", ansi_rgb(0, 196, 196), ##__VA_ARGS__, ansi_end))
    #define eprintfn(msg, ...) (fprintf(stderr, "%s[ERROR] " msg "\n%s", ansi_rgb(196, 0, 0), ##__VA_ARGS__, ansi_end))
#endif // CEBEQ_DEBUG

#define UNREACHABLE(...) (eprintfn(__VA_ARGS__), assert(0))

CBQLIB extern char program_dir[FILENAME_MAX];
CBQLIB extern char exe_dir[FILENAME_MAX];

CBQLIB bool setup(void);

CBQLIB int make_backup(const char *branch_name, const char *dest, const char *parent);
CBQLIB int merge(const char *src, const char *dest);

CBQLIB bool get_exe_path(char *buffer, size_t buffer_size);
CBQLIB bool get_parent_dir(const char *path, char *buffer, size_t buffer_size);
CBQLIB void norm_path(const char *path, char *buffer, size_t buffer_size); // cwk_path_normalize but with unix-style seperators forced

#endif // _CEBEQ_H