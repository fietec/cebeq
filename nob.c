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
}

void append_head(Nob_Cmd *cmd)
{
    nob_cmd_append(cmd, "gcc", "-I", "./include");
    nob_cmd_append(cmd, "-Wall", "-Wextra", "-Werror", "-Wno-unused-value"); // definetly not cheating here..    
}

void build_lib(Nob_Cmd *cmd)
{
    nob_log(NOB_INFO, "Building core library..");
    
    // Build core.o
    append_head(cmd);
    nob_cmd_append(cmd, "src/backup.c", "src/merge.c", "src/cwalk.c", "src/cson.c", "src/flib.c");
#ifdef _WIN32
    nob_cmd_append(cmd, "-shared", "-o", "build/core.dll");
#else
    nob_cmd_append(cmd, "-fPIC", "-shared", "-o", "build/libcore.so");
#endif // _WIN32
    nob_cmd_run_sync_and_reset(cmd);
}   

void build_cli(Nob_Cmd *cmd)
{
    build_lib(cmd);
    nob_log(NOB_INFO, "Building cli..");
    append_head(cmd);
    nob_cmd_append(cmd, "-o", "build/cli");
    nob_cmd_append(cmd, "src/cli.c", "-Lbuild", "-lcore");
#ifndef _WIN32
    nob_cmd_append(cmd, "-Wl,rpath=build");
#endif // _WIN32
    nob_cmd_run_sync_and_reset(cmd);
}

void build_gui(Nob_Cmd *cmd)
{
    nob_log(NOB_INFO, "Building gui..");
    append_head(cmd);
    nob_cmd_append(cmd, "-o", "build/gui");
    nob_cmd_append(cmd, "src/gui.c", "-Lbuild", "-lcore");
#ifndef _WIN32
    nob_cmd_append(cmd, "-Wl,rpath=build");
#endif // _WIN32
    nob_cmd_run_sync_and_reset(cmd);
}

void build_test(Nob_Cmd *cmd)
{
    nob_log(NOB_INFO, "Building test..");
    append_head(cmd);
    nob_cmd_append(cmd, "-o", "build/test");
    nob_cmd_append(cmd, "src/cebeq.c", "src/backup.c", "src/merge.c", "src/cwalk.c", "src/cson.c", "src/flib.c");
    nob_cmd_run_sync_and_reset(cmd);
}

void build_all(Nob_Cmd *cmd)
{
    // TODO: add gui target
    build_cli(cmd);
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
        if (strcmp(target, "cli") ==  0) build_cli(&cmd);
        else if (strcmp(target, "gui") == 0) build_gui(&cmd);
        else if (strcmp(target, "lib") == 0) build_lib(&cmd);
        else if (strcmp(target, "all") == 0) build_all(&cmd);
        else if (strcmp(target, "test") == 0) build_test(&cmd);
        else if (strcmp(target, "--help") == 0) print_usage(program_name);
        else{
            fprintf(stderr, "[ERROR] Unknown target: '%s'!\n", target);
            return 1;
        }
    } else{
        build_all(&cmd);
    }
    return 0;
}