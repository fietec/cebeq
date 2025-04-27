#ifndef _SYNCIT_H
#define _SYNCIT_H

#include <stdio.h>
#include <inttypes.h>

#define BACKUPS_JSON "D:/cdemeer/Programming/C/syncit/data/backups.json"
#define FULL_BACKUPS_JSON "D:/cdemeer/Programming/C/syncit/data/full_backups.json"

#define INFO_FILE ".syncit"

#define s_bool(s) ((s)>0? "true": "false")

#define ansi_rgb(r, g, b) ("\e[38;2;" #r ";" #g ";" #b "m") 
#define ansi_end ("\e[0m") 

#define return_defer(v) do{value = (v); goto defer;}while(0)

#define iprintfn(msg, ...) (fprintf(stdout, "%s[INFO] %s:%d " msg "\n%s", ansi_rgb(0, 196, 196), __FILE__, __LINE__, ##__VA_ARGS__, ansi_end))
#define eprintfn(msg, ...) (fprintf(stderr, "%s[ERROR] %s:%d in %s: " msg "\n%s", ansi_rgb(196, 0, 0), __FILE__, __LINE__, __func__, ##__VA_ARGS__, ansi_end))

#endif // _SYNCIT_H