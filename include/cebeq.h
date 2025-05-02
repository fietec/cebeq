#ifndef _SYNCIT_H
#define _SYNCIT_H

#include <stdio.h>
#include <inttypes.h>

#define PROGRAM_NAME "cebeq"

#define BACKUPS_JSON "D:/cdemeer/Programming/C/" PROGRAM_NAME "/data/backups.json"
#define FULL_BACKUPS_JSON "D:/cdemeer/Programming/C/" PROGRAM_NAME "/data/full_backups.json"

#define INFO_FILE "." PROGRAM_NAME

#define s_bool(s) ((s)>0? "true": "false")

#define ansi_rgb(r, g, b) ("\e[38;2;" #r ";" #g ";" #b "m") 
#define ansi_end ("\e[0m") 

#define arr_len(arr) (sizeof((arr)) / sizeof((arr)[0])) 

#define return_defer(v) do{value = (v); goto defer;}while(0)

#define dprintfn(msg, ...) (fprintf(stdout, "%s[INFO] %s:%d " msg "\n%s", ansi_rgb(0, 196, 196), __FILE__, __LINE__, ##__VA_ARGS__, ansi_end))
#define iprintfn(msg, ...) (fprintf(stdout, "%s[INFO] " msg "\n%s", ansi_rgb(0, 196, 196), ##__VA_ARGS__, ansi_end))
#define eprintfn(msg, ...) (fprintf(stderr, "%s[ERROR] %s:%d in %s: " msg "\n%s", ansi_rgb(196, 0, 0), __FILE__, __LINE__, __func__, ##__VA_ARGS__, ansi_end))

int make_backup(const char *branch_name, const char *dest, const char *parent);
int merge(const char *src, const char *dest);

#endif // _SYNCIT_H