#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <cebeq.h>
#include <cwalk.h>
#include <cson.h>
#include <threading.h>
#include <message_queue.h>

typedef enum{
    Cmd_None, Cmd_Backup, Cmd_Merge, Cmd_Branch
} Cmd;

static char *version = "";
static thread_t worker_thread;

#define shift_args(argc, argv) _shift_args(&argc, &argv)
char* _shift_args(int *argc, char ***argv)
{
    assert(*argc > 0 && "argv: out of bounds\n");
    char *result = **argv;
    *argc -= 1;
    *argv += 1;
    return result;
}

void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS] <command> [COMMAND_OPTIONS]\n\n", program_name);

    printf("Commands:\n");
    printf("  backup              Create a backup\n");
    printf("  merge               Merge existing backups\n");
    printf("  branch              View and modify existing branches\n\n");
    
    printf("Options:\n");
    printf("  -h, --help          Show this help message\n");
    printf("  -v, --version       Show program version\n\n");

    printf("Use '%s <command> --help' to see options for a specific command.\n", program_name);
}

void print_backup_usage(const char *program_name) 
{
    printf("Usage: %s backup <branch_name> <dest> [parent] [OPTIONS]\n\n", program_name);
    
    printf("Args:\n");
    printf("  branch_name         The name of the backup branch\n");
    printf("  dest                The folder to create the backup in\n");
    printf("  parent              The parent backup\n\n");
    
    printf("Options for backup:\n");
    printf("  -h, --help          Show this help message\n");
}

void print_merge_usage(const char *program_name) 
{
    printf("Usage: %s merge <src> <dest> [OPTIONS]\n\n", program_name);
    
    printf("Args:\n");
    printf("  src                 The path of the backup to merge\n");
    printf("  dest                The destination to write the merged files to\n\n");
    
    printf("Options for merge:\n");
    printf("  -h, --help          Show this help message\n");
}

void print_branch_usage(const char *program_name) 
{
    printf("Usage: %s branch <command> [OPTIONS]\n\n", program_name);
    
    printf("Commands:\n");
    printf("  list                  List all available branches\n");
    printf("  new <name> <dirs>...  Create a new branch\n");
    printf("  delete <name>         Delete a branch\n\n");
    
    printf("Options for branch:\n");
    printf("  -h, --help            Show this help message\n");
}

int print_branches(Cson *branches)
{
    Cson *branch_names = cson_map_keys(branches);
    if (!cson_is_array(branch_names)){
        fprintf(stderr, "[ERROR] Could not extract branch_names!\n");
        return 1;
    }
    size_t len = cson_len(branch_names);
    printf("Currently, there are %zu branches:\n", len);
    for (size_t i=0; i<len; ++i){
        CsonStr key = cson_get_string(branch_names, index(i));
        Cson *dirs = cson_get(branches, key(key.value), key("dirs"));
        if (!cson_is_array(dirs)){
            fprintf(stderr, "[ERROR] Could not find dirs for key '%s'!\n", key.value);
            return 1;
        }
        printf("  '%s' : [", key.value);
        for (size_t i=0; i<cson_len(dirs); ++i){
            CsonStr dir = cson_get_string(dirs, index(i));
            printf("%s\"%s\"", i>0? ", ": "", dir.value);
        }
        printf("]\n");
    }
    return 0;
}

void print_version(const char *program_name)
{
    printf("%s - version %s\n", program_name, version);
}

void run(thread_fn fn, thread_args_t args)
{
    msgq_init();
    thread_create(&worker_thread, fn, &args);
    char msg[256];
    while (true){
        bool got_msg = false;
        while (msgq_pop(msg, sizeof(msg))){
            printf("%s\n", msg);
            got_msg = true;
        }
        if (worker_done && !got_msg) break;
    }
    msgq_destroy();
}

int main(int argc, char **argv)
{
    if (!setup()) return 1;
    
    char info_path[FILENAME_MAX] = {0};
    cwk_path_join(program_dir, "data/backups.json", info_path, sizeof(info_path));
    
    Cson *info = cson_read(info_path);
    if (info == NULL){
        eprintf("Could not find internal info file!\n");
        return 1;
    }
    version = cson_get_string(info, key("version")).value;
    if (version == NULL){
        eprintf("Invalid info file state!\n");
        return 1;
    }
    
    const char *program_name = shift_args(argc, argv);
    Cmd current_command = Cmd_None;
    
    thread_args_t command_options;
    size_t command_option_count = 0;

    if (argc < 1){
        fprintf(stderr, "[ERROR] No arguments provided!\n\n");
        print_usage(program_name);
        return 1;
    }
    while (argc > 0){
        const char *arg = shift_args(argc, argv);
        switch (current_command){
            case Cmd_None:{
                if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0){
                    print_usage(program_name);
                    return 0;
                }
                if (strcmp(arg, "--version") == 0 || strcmp(arg, "-v") == 0){
                    print_version(program_name);
                    return 0;
                }
                if (strcmp(arg, "backup") == 0){
                    current_command = Cmd_Backup;
                }
                else if (strcmp(arg, "merge") == 0){
                    current_command = Cmd_Merge;
                }
                else if (strcmp(arg, "branch") == 0){
                    current_command = Cmd_Branch;
                }
                else{
                    fprintf(stderr, "[ERROR] Unknown argument: '%s'!\n\n", arg);
                    print_usage(program_name);
                    return 1;
                }
            } break;
            case Cmd_Backup:{
                if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0){
                    print_backup_usage(program_name);
                    return 0;
                }
                else{
                    if (command_option_count >= 3){
                        fprintf(stderr, "[ERROR] Unknown argument: '%s'!\n\n", arg);
                        print_backup_usage(program_name);
                        return 1;
                    }
                    command_options.args[command_option_count++] = arg;
                }
            } break;
            case Cmd_Merge:{
                if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0){
                    print_merge_usage(program_name);
                    return 0;
                }
                else{
                    if (command_option_count >= 2){
                        fprintf(stderr, "[ERROR] Unknown argument: '%s'!\n\n", arg);
                        print_merge_usage(program_name);
                        return 1;
                    }
                    command_options.args[command_option_count++] = arg;
                }
            } break;
            case Cmd_Branch: {
                if (strcmp(arg, "list") == 0){
                    Cson *branches = cson_get(info, key("backups"));
                    if (!cson_is_map(branches)){
                        fprintf(stderr, "[ERROR] Invalid info file state!\n");
                        return 1;
                    }
                    return print_branches(branches);
                }
                else if (strcmp(arg, "new") == 0){
                    printf("TODO: branch new\n");
                    return 0;
                }
                else if (strcmp(arg, "delete") == 0){
                    printf("TODO: branch delete\n");
                    return 0;
                }
                else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0){
                    print_branch_usage(program_name);
                    return 0;
                }
                else{
                    fprintf(stderr, "[ERROR] Unknown option: '%s'!\n", arg);
                    print_branch_usage(program_name);
                    return 1;
                }
            } break;
            default: {assert(0 && "Invalid Cmd type\n");};
        }
    }
    switch (current_command){
        case Cmd_Backup: {
            if (command_option_count < 2){
                fprintf(stderr, "[ERROR] Too few arguments provided!\n\n");
                print_backup_usage(program_name);
                return 1;
            }
            run(tbackup, command_options);
        }break;
        case Cmd_Merge:{
            if (command_option_count < 2){
                fprintf(stderr, "[ERROR] Too few arguments provided!\n\n");
                print_merge_usage(program_name);
                return 1;
            }
            run(tmerge, command_options);
        }break;
        case Cmd_Branch:{
            print_branch_usage(program_name);
        } break;
        default: {assert(0 && "Invalid Cmd type\n\n");};
    }
    cleanup();
    return 0;
}