#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

void print_usage(const char *program_name)
{
    printf("Usage: %s <target>\n\n", program_name);

    printf("Targets:\n");
    printf("  - cli        Build the cli\n");
    printf("  - gui        Build the gui\n");
    printf("  - lib        Build the core functionality lib\n");
    printf("  - all        Build the all the above\n");
    printf("  - test       Build the current test\n\n");
    
    printf("Other:\n");
    printf("  - help       Print this dialog\n\n");
}

void append_head(Nob_Cmd *cmd)
{
    nob_cmd_append(cmd, "gcc", "-std=gnu23");
    nob_cmd_append(cmd, "-Wall", "-Wextra", "-Werror", "-Wno-unused-value", "-Wno-stringop-overflow"); // definetly not cheating here..    
    nob_cmd_append(cmd, "-I", "./include", "-I.");
    //nob_cmd_append(cmd, "-D", "CEBEQ_DEBUG"); // remove this in production build
}

bool build_lib(Nob_Cmd *cmd)
{
    nob_log(NOB_INFO, "Building core library..");
    
    // Build core.o
    append_head(cmd);
    nob_cmd_append(cmd, "src/backup.c", "src/merge.c", "src/cwalk.c", "src/cson.c", "src/flib.c", "src/cebeq.c", "src/threading.c", "src/message_queue.c");
#ifdef _WIN32
    nob_cmd_append(cmd, "-D", "CEBEQ_EXPORT", "-D", "CEBEQ_MSGQ", "-shared", "-o", "build/core.dll");
#else
    nob_cmd_append(cmd, "-fPIC", "-shared", "-o", "build/libcore.so");
#endif // _WIN32
    return nob_cmd_run_sync_and_reset(cmd);
}   

bool build_cli(Nob_Cmd *cmd)
{
    nob_log(NOB_INFO, "Building cli..");
    append_head(cmd);
    nob_cmd_append(cmd, "-o", "build/cli");
    nob_cmd_append(cmd, "src/cli.c", "-Lbuild", "-lcore");
#ifndef _WIN32
    nob_cmd_append(cmd, "-Wl,-rpath,build");
#endif // _WIN32
    return nob_cmd_run_sync_and_reset(cmd);
}

bool build_gui(Nob_Cmd *cmd)
{
    nob_log(NOB_INFO, "Building gui..");
    const char *raylib_path = "lib/libraylib.a";
    struct stat attr;
    if (stat("lib/libraylib.a", &attr) == -1){
        nob_log(NOB_ERROR, "Raylib static library not found at at %s.", raylib_path);
        nob_log(NOB_ERROR, "Please provide a built raylib lib in that location to procede.");
        return false;
    }
    append_head(cmd);
    nob_cmd_append(cmd, "-o", "build/gui");
    nob_cmd_append(cmd, "src/gui.c", "-Lbuild", "-lcore", "-Llib", "-lraylib", "-lgdi32", "-lwinmm");
    nob_cmd_append(cmd, "-Wno-unused-function"); // caused by nob.h
    nob_cmd_append(cmd, "-D", "NOB_NO_MINIRENT");
#ifndef _WIN32
    nob_cmd_append(cmd, "-Wl,-rpath,build");
#endif // _WIN32
    return nob_cmd_run_sync_and_reset(cmd);
}

bool build_test(Nob_Cmd *cmd)
{
    nob_log(NOB_INFO, "Building test..");
    append_head(cmd);
    nob_cmd_append(cmd, "-o", "build/test");
    nob_cmd_append(cmd, "testing/thread_test.c", "-Lbuild", "-lcore");
#ifndef _WIN32
    nob_cmd_append(cmd, "-Wl,-rpath,build");
#endif // _WIN32
    return nob_cmd_run_sync_and_reset(cmd);
}

bool build_all(Nob_Cmd *cmd)
{
    return build_lib(cmd) && build_cli(cmd) && build_gui(cmd);
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    if (!nob_mkdir_if_not_exists("build")){
        nob_log(NOB_ERROR, "Could not create build directory!");
        return 1;
    }
    Nob_Cmd cmd = {0};
    const char *program_name = argv[0];
    if (argc > 1){
        const char *target = argv[1];
        if (strcmp(target, "cli") ==  0) {
            if (!build_cli(&cmd)) return 1;
        }
        else if (strcmp(target, "gui") == 0) {
            if (!build_gui(&cmd)) return 1;
        }
        else if (strcmp(target, "lib") == 0) {
            if (!build_lib(&cmd)) return 1;
        }
        else if (strcmp(target, "all") == 0) {
            if (!build_all(&cmd)) return 1;
        }
        else if (strcmp(target, "test") == 0) {
            if (!build_test(&cmd)) return 1;
        }
        else if (strcmp(target, "help") == 0) print_usage(program_name);
        else{
            fprintf(stderr, "[ERROR] Unknown target: '%s'!\n", target);
            return 1;
        }
    } else{
        build_all(&cmd);
    }
    return 0;
}
