#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <unistd.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

const char *lib_files[] = {
    "backup", "merge", "cwalk", "cson", "flib", "cebeq", "threading", "message_queue"
};

#define lib_file_count sizeof(lib_files)/sizeof(*lib_files)

char src_files[lib_file_count][32] = {0};
char obj_files[lib_file_count][32] = {0};

void print_usage(const char *program_name)
{
    printf("Usage: %s [options] <targets..>\n\n", program_name);

    printf("Targets:\n");
    printf("  lib        Build the cebeq functionality library\n");
    printf("  cli        Build the CLI application (requires 'lib' to be built first)\n");
    printf("  gui        Build the GUI application (requires 'lib' to be built first)\n");
    printf("  all        Build lib, cli, and gui in the correct order\n");
    printf("  clean      Remove all build and bin artifacts\n\n");

    printf("Options:\n");
    printf("  --static   Build statically linked versions of following targets\n");
    printf("  -h, --help Show this help message\n\n");
}

void append_head(Nob_Cmd *cmd)
{
    nob_cmd_append(cmd, "gcc", "-std=gnu2x");
    nob_cmd_append(cmd, "-Wall", "-Wextra", "-Werror", "-Wno-unused-value", "-Wno-stringop-overflow", "-Wno-format-truncation");
    nob_cmd_append(cmd, "-I", "./include", "-I.");
}

void create_lib_files()
{
    for (size_t i=0; i<lib_file_count; ++i){
        sprintf(src_files[i], "src/%s.c", lib_files[i]);
        sprintf(obj_files[i], "build/%s.o", lib_files[i]);
    }
}

bool build_lib(Nob_Cmd *cmd, bool compile_static)
{
    nob_log(NOB_INFO, "Building cebeq library..");
    create_lib_files();
    if (compile_static){
        // create object files asynchronously
        Nob_Procs procs = {0};
        for (size_t i=0; i<lib_file_count; ++i){
            append_head(cmd);
            nob_cmd_append(cmd, "-c", src_files[i], "-o", obj_files[i], "-D", "CEBEQ_MSGQ");
            nob_da_append(&procs, nob_cmd_run_async_and_reset(cmd));
        }
        if (!nob_procs_wait(procs)) return false;
        // create static library
        nob_cmd_append(cmd, "ar", "rcs", "build/libcebeq.a");
        for (size_t i=0; i<lib_file_count; ++i){
            nob_cmd_append(cmd, obj_files[i]);
        }
        return nob_cmd_run_sync_and_reset(cmd);
    } else {
        append_head(cmd);
        for (size_t i=0; i<lib_file_count; ++i){
            nob_cmd_append(cmd, src_files[i]);
        }
        nob_cmd_append(cmd, "-DCEBEQ_SHARED", "-DCEBEQ_EXPORTS", "-DCEBEQ_MSGQ");

    #ifdef _WIN32
        nob_cmd_append(cmd, "-shared", "-o", "bin/cebeq.dll");
    #else
        nob_cmd_append(cmd, "-fPIC", "-shared", "-o", "bin/libcebeq.so");
    #endif
        return nob_cmd_run_sync_and_reset(cmd);
    }
}

bool build_cli(Nob_Cmd *cmd, bool compile_static)
{
    nob_log(NOB_INFO, "Building cli..");
    append_head(cmd);
    nob_cmd_append(cmd, "-o", "bin/cbq", "src/cli.c");

    if (compile_static) {
        nob_cmd_append(cmd, "-static");
        nob_cmd_append(cmd, "build/libcebeq.a");
    } else {
        nob_cmd_append(cmd, "-Lbin", "-lcebeq");
    }

#ifndef _WIN32
    if (!compile_static)
        nob_cmd_append(cmd, "-Wl,-rpath,$ORIGIN");
#endif
    return nob_cmd_run_sync_and_reset(cmd);
}

bool build_gui(Nob_Cmd *cmd, bool compile_static)
{
    nob_log(NOB_INFO, "Building gui..");
    append_head(cmd);
    nob_cmd_append(cmd, "-o", "bin/cbqgui", "src/gui.c");

    if (compile_static) {
        nob_cmd_append(cmd, "build/libcebeq.a");
    } else {
        nob_cmd_append(cmd, "-Lbin", "-lcebeq");
        nob_cmd_append(cmd, "-Wl,-rpath,$ORIGIN");
    }

    nob_cmd_append(cmd, "-Llib", "-lraylib");
    nob_cmd_append(cmd, "-Wno-unused-function");
    nob_cmd_append(cmd, "-D", "NOB_NO_MINIRENT");

#ifdef _WIN32
    nob_cmd_append(cmd, "-lgdi32", "-lwinmm");
#else
    nob_cmd_append(cmd, "-lGL", "-lm", "-lpthread", "-ldl", "-lrt", "-lX11");
#endif

    if (!nob_cmd_run_sync_and_reset(cmd)) {
        nob_log(NOB_ERROR, "Failed to build gui! Make sure that you have raylib in your path or in ./lib!");
        return false;
    }

    return true;
}

bool build_all(Nob_Cmd *cmd, bool compile_static)
{
    return build_lib(cmd, compile_static) && build_cli(cmd, compile_static) && build_gui(cmd, compile_static);
}

void clear_dir(const char *dir_path)
{
    DIR *dir = opendir(dir_path);
    if (dir == NULL){
        printf("No 'build' directory found. Nothing to clean.\n");
        return;
    }
    char entry_path[FILENAME_MAX];
    struct dirent *entry;
    while ((entry = readdir(dir))){
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        snprintf(entry_path, sizeof(entry_path), "%s/%s", dir_path, entry->d_name);
        struct stat attr;
        if (stat(entry_path, &attr) == -1){
            fprintf(stderr, "[ERROR] Could not stat '%s'!\n", entry_path);
            continue;
        }
        if (S_ISREG(attr.st_mode)){
            nob_delete_file(entry_path);
        }
    }
    closedir(dir);
}

void cleanup(void)
{
    clear_dir("./build");
    clear_dir("./bin");
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
    if (!nob_mkdir_if_not_exists("bin")){
        nob_log(NOB_ERROR, "Could not create bin directory!");
        return 1;
    }
    bool compile_static = false;
    while (argc > 0){
        const char *target = nob_shift_args(&argc, &argv);
        if (strcmp(target, "cli") ==  0) {
            if (!build_cli(&cmd, compile_static)) return 1;
        }
        else if (strcmp(target, "gui") == 0) {
            if (!build_gui(&cmd, compile_static)) return 1;
        }
        else if (strcmp(target, "lib") == 0) {
            if (!build_lib(&cmd, compile_static)) return 1;
        }
        else if (strcmp(target, "all") == 0) {
            if (!build_all(&cmd, compile_static)) return 1;
        }
        else if (strcmp(target, "clean") == 0){
            cleanup();
        }
        else if (strcmp(target, "--help") == 0){
            print_usage(program_name);
            return 0;
        }
        else if (strcmp(target, "--static") == 0){
            compile_static = true;
        }
        else{
            fprintf(stderr, "[ERROR] Unknown target: '%s'!\n", target);
            return 1;
        }
    }
    return 0;
}
