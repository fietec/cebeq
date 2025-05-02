#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Nob_Cmd cmd = {0};
    if (!nob_mkdir_if_not_exists("build")){
        nob_log(NOB_ERROR, "Could not create build directory!");
        return 1;
    }
    nob_cmd_append(&cmd, "gcc", "-I", "./include");
    nob_cmd_append(&cmd, "-Wall", "-Wextra", "-Werror", "-Wno-unused-value"); // definetly not cheating here..
    nob_cmd_append(&cmd, "-o", "backup");
    nob_cmd_append(&cmd, "src/cebeq.c", "src/backup.c", "src/merge.c", "src/cwalk.c", "src/cson.c", "src/flib.c");
    if (!nob_cmd_run_sync(cmd)) return 1;
    return 0;
}