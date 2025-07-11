#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

void print_usage(const char *program_name)
{
    printf("Usage: %s <targets..>\n\n", program_name);

    printf("Targets:\n");
    printf("  - cli        Build the cli\n");
    printf("  - gui        Build the gui\n");
    printf("  - lib        Build the core functionality lib\n");
    printf("  - all        Build all the above\n\n");
    
    printf("Other:\n");
    printf("  - help       Print this dialog\n\n");
}

void append_head(Nob_Cmd *cmd)
{
    nob_cmd_append(cmd, "gcc", "-std=gnu2x");
    nob_cmd_append(cmd, "-Wall", "-Wextra", "-Werror", "-Wno-unused-value", "-Wno-stringop-overflow", "-Wno-format-truncation"); // definetly not cheating here..    
    nob_cmd_append(cmd, "-I", "./include", "-I.");
    //nob_cmd_append(cmd, "-D", "CEBEQ_DEBUG"); // remove this in production build
}

bool build_lib(Nob_Cmd *cmd)
{
    nob_log(NOB_INFO, "Building core library..");
    
    // Build core.o
    append_head(cmd);
    nob_cmd_append(cmd, "src/backup.c", "src/merge.c", "src/cwalk.c", "src/cson.c", "src/flib.c", "src/cebeq.c", "src/threading.c", "src/message_queue.c");
    nob_cmd_append(cmd, "-D", "CEBEQ_EXPORT", "-D", "CEBEQ_MSGQ");
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
    nob_cmd_append(cmd, "-Wl,-rpath,$ORIGIN");
#endif // _WIN32
    return nob_cmd_run_sync_and_reset(cmd);
}

bool build_gui(Nob_Cmd *cmd)
{
    nob_log(NOB_INFO, "Building gui..");
    append_head(cmd);
    nob_cmd_append(cmd, "-o", "build/gui");
    nob_cmd_append(cmd, "src/gui.c", "-Lbuild", "-lcore", "-Llib", "-lraylib");
    nob_cmd_append(cmd, "-Wno-unused-function"); // caused by nob.h
    nob_cmd_append(cmd, "-D", "NOB_NO_MINIRENT");
#ifdef _WIN32
    nob_cmd_append(cmd, "-lgdi32", "-lwinmm");
#else
    nob_cmd_append(cmd, "-lGL", "-lm", "-lpthread", "-ldl", "-lrt", "-lX11");
    nob_cmd_append(cmd, "-Wl,-rpath,$ORIGIN");
#endif // _WIN32
    if (!nob_cmd_run_sync_and_reset(cmd)){
        nob_log(NOB_ERROR, "Failed to build gui! Make sure that you have raylib in your path or in ./lib!");
        return false;
    }
    return true;
}

bool build_all(Nob_Cmd *cmd)
{
    return build_lib(cmd) && build_cli(cmd) && build_gui(cmd);
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Nob_Cmd cmd = {0};
    const char *program_name = nob_shift_args(&argc, &argv);
    if (argc == 0){
        fprintf(stderr, "[ERROR] No target provided!\n");
        print_usage(program_name);
        return 1;
    }
    if (!nob_mkdir_if_not_exists("build")){
        nob_log(NOB_ERROR, "Could not create build directory!");
        return 1;
    }
    while (argc > 0){
        const char *target = nob_shift_args(&argc, &argv);
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
        else if (strcmp(target, "help") == 0){
            print_usage(program_name);
            return 0;
        }
        else{
            fprintf(stderr, "[ERROR] Unknown target: '%s'!\n", target);
            return 1;
        }
    }
    return 0;
}
