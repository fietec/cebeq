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

#define MAX_LONG_PATH 32767
#define MAX_QUEUE 100
#define MAX_MSG_LEN 256

#define PROGRAM_NAME "cebeq"
#define INFO_FILE "." PROGRAM_NAME
#define BACKUPS_JSON "data/info.json"

#define s_bool(s) ((s)>0? "true": "false")

#define ansi_rgb(r, g, b) ("\e[38;2;" #r ";" #g ";" #b "m") 
#define ansi_end ("\e[0m") 

#define arr_len(arr) (sizeof((arr)) / sizeof((arr)[0])) 

#define return_defer(v) do{value = (v); goto defer;}while(0)

#ifdef CEBEQ_MSGQ
    #ifdef CEBEQ_COLOR
        #ifdef CEBEQ_DEBUG
            #define dprintf(msg, ...) do{char msgb[MAX_MSG_LEN]; snprintf(msgb, sizeof(msgb), "%s[DEBUG] %s:%d in %s: " msg "%s", ansi_rgb(196, 196, 0), __FILE__, __LINE__, __func__, ##__VA_ARGS__, ansi_end); msgq_push(msgb);}while(0)
            #define iprintf(msg, ...) do{char msgb[MAX_MSG_LEN]; snprintf(msgb, sizeof(msgb), "%s[INFO] %s:%d in %s: " msg "%s", ansi_rgb(0, 196, 196), __FILE__, __LINE__, __func__, ##__VA_ARGS__, ansi_end); msgq_push(msgb);}while(0)
            #define eprintf(msg, ...) do{char msgb[MAX_MSG_LEN]; snprintf(msgb, sizeof(msgb), "%s[ERROR] %s:%d in %s: " msg "%s", ansi_rgb(196, 0, 0), __FILE__, __LINE__, __func__, ##__VA_ARGS__, ansi_end); msgq_push(msgb);}while(0)
        #else
            #define dprintf(msg, ...) 
            #define iprintf(msg, ...) do{char msgb[MAX_MSG_LEN]; snprintf(msgb, sizeof(msgb), "%s[INFO] " msg "%s", ansi_rgb(0, 196, 196), ##__VA_ARGS__, ansi_end); msgq_push(msgb);}while(0)
            #define eprintf(msg, ...) do{char msgb[MAX_MSG_LEN]; snprintf(msgb, sizeof(msgb), "%s[ERROR] " msg "%s", ansi_rgb(196, 0, 0), ##__VA_ARGS__, ansi_end); msgq_push(msgb);}while(0)
        #endif // CEBEQ_DEBUG
    #else
        #ifdef CEBEQ_DEBUG
            #define dprintf(msg, ...) do{char msgb[MAX_MSG_LEN]; snprintf(msgb, sizeof(msgb), "[DEBUG] %s:%d in %s: " msg "", __FILE__, __LINE__, __func__, ##__VA_ARGS__); msgq_push(msgb);}while(0)
            #define iprintf(msg, ...) do{char msgb[MAX_MSG_LEN]; snprintf(msgb, sizeof(msgb), "[INFO] %s:%d in %s: " msg "", __FILE__, __LINE__, __func__, ##__VA_ARGS__); msgq_push(msgb);}while(0)
            #define eprintf(msg, ...) do{char msgb[MAX_MSG_LEN]; snprintf(msgb, sizeof(msgb), "[ERROR] %s:%d in %s: " msg "", __FILE__, __LINE__, __func__, ##__VA_ARGS__); msgq_push(msgb);}while(0)
        #else
            #define dprintf(msg, ...) 
            #define iprintf(msg, ...) do{char msgb[MAX_MSG_LEN]; snprintf(msgb, sizeof(msgb), "[INFO] " msg "", ##__VA_ARGS__); msgq_push(msgb);}while(0)
            #define eprintf(msg, ...) do{char msgb[MAX_MSG_LEN]; snprintf(msgb, sizeof(msgb), "[ERROR] " msg "", ##__VA_ARGS__); msgq_push(msgb);}while(0)
        #endif // CEBEQ_DEBUG
    #endif // CEBEQ_COLOR
#else
    #ifdef CEBEQ_DEBUG
        #define dprintf(msg, ...) do{char msgb[MAX_MSG_LEN]; snprintf(msgb, sizeof(msgb), "[DEBUG] %s:%d in %s: " msg "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); msgq_push(msgb);}while(0)
        #define iprintf(msg, ...) do{char msgb[MAX_MSG_LEN]; snprintf(msgb, sizeof(msgb), "[INFO] %s:%d in %s: " msg "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); msgq_push(msgb);}while(0)
        #define eprintf(msg, ...) do{char msgb[MAX_MSG_LEN]; snprintf(msgb, sizeof(msgb), "[ERROR] %s:%d in %s: " msg "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); msgq_push(msgb);}while(0)
    #else
        #define dprintf(msg, ...) 
        #define iprintf(msg, ...) do{fprintf(stdout, "[INFO] " msg "\n", ##__VA_ARGS__);}while(0)
        #define eprintf(msg, ...) do{fprintf(stderr, "[ERROR] " msg "\n", ##__VA_ARGS__);}while(0)
    #endif // CEBEQ_DEBUG
#endif // CEBEQ_MSGQ

#define UNREACHABLE(...) (eprintf(__VA_ARGS__), assert(0))

typedef struct{
    const char *args[3];
} thread_args_t;

CBQLIB extern char program_dir[FILENAME_MAX];
CBQLIB extern char exe_dir[FILENAME_MAX];
CBQLIB extern char *long_path_buf;
CBQLIB extern volatile int worker_done;

CBQLIB bool setup(void);
CBQLIB void cleanup(void);

CBQLIB void* tbackup(void *args);
CBQLIB void* tmerge(void *args);
CBQLIB int backup(const char *branch_name, const char *dest, const char *parent);
CBQLIB int merge(const char *src, const char *dest);

CBQLIB bool get_exe_path(char *buffer, size_t buffer_size);
CBQLIB bool get_parent_dir(const char *path, char *buffer, size_t buffer_size);
CBQLIB void escape_string(const char *string, char *buffer, size_t buffer_size);

#endif // _CEBEQ_H