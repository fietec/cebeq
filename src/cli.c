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
    printf("  list [branch]         List all available branches or all backups of specified branch\n");
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
    char msg[MAX_MSG_LEN];
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
    int value = 0;
    if (!setup()) return_defer(1);
    
    char info_path[FILENAME_MAX] = {0};
    cwk_path_join(program_dir, "data/backups.json", info_path, sizeof(info_path));
    
    Cson *info = cson_read(info_path);
    if (info == NULL){
        eprintf("Could not find internal info file!\n");
        return_defer(1);
    }
    version = cson_get_string(info, key("version")).value;
    if (version == NULL){
        eprintf("Invalid info file state!\n");
        return_defer(1);
    }
    
    const char *program_name = shift_args(argc, argv);
    Cmd current_command = Cmd_None;
    
    thread_args_t command_options = {0};
    size_t command_option_count = 0;

    if (argc < 1){
        fprintf(stderr, "[ERROR] No arguments provided!\n\n");
        print_usage(program_name);
        return_defer(1);
    }
    while (argc > 0){
        const char *arg = shift_args(argc, argv);
        switch (current_command){
            case Cmd_None:{
                if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0){
                    print_usage(program_name);
                    return_defer(0);
                }
                if (strcmp(arg, "--version") == 0 || strcmp(arg, "-v") == 0){
                    print_version(program_name);
                    return_defer(0);
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
                    return_defer(1);
                }
            } break;
            case Cmd_Backup:{
                if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0){
                    print_backup_usage(program_name);
                    return_defer(0);
                }
                else{
                    if (command_option_count >= 3){
                        fprintf(stderr, "[ERROR] Unknown argument: '%s'!\n\n", arg);
                        print_backup_usage(program_name);
                        return_defer(1);
                    }
                    command_options.args[command_option_count++] = arg;
                }
            } break;
            case Cmd_Merge:{
                if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0){
                    print_merge_usage(program_name);
                    return_defer(0);
                }
                else{
                    if (command_option_count >= 2){
                        fprintf(stderr, "[ERROR] Unknown argument: '%s'!\n\n", arg);
                        print_merge_usage(program_name);
                        return_defer(1);
                    }
                    command_options.args[command_option_count++] = arg;
                }
            } break;
            case Cmd_Branch: {
                if (strcmp(arg, "list") == 0){
                    Cson *branches = cson_get(info, key("branches"));
                    if (!cson_is_map(branches)){
                        fprintf(stderr, "[ERROR] Invalid info file state!\n");
                        return_defer(1);
                    }
                    if (argc > 0){
                        arg = shift_args(argc, argv);
                        Cson *branch = cson_get(branches, key((char*) arg));
                        if (!cson_is_map(branch)){
                            fprintf(stderr, "[ERROR] Unknown branch '%s'!\n", arg);
                            fprintf(stdout, "Use '%s branch list' to see a list of all branches.\n", program_name);
                            return_defer(1);
                        }
                        Cson *backups = cson_get(branch, key("backups"));
                        if (!cson_is_array(backups) || cson_len(backups) == 0){
                            fprintf(stdout, "Currently, there are no backups for branch '%s'.\n", arg);
                            return_defer(0);
                        }
                        size_t len = cson_len(backups);
                        fprintf(stdout, "Currently, there are %zu backups for branch '%s':\n", len, arg);
                        char backup_info_path[FILENAME_MAX] = {0};
                        for (size_t i=0; i<len; ++i){
                            char *backup = cson_get_cstring(backups, index(i));
                            if (backup != NULL){
                                cwk_path_join(backup, INFO_FILE, backup_info_path, sizeof(backup_info_path));
                                Cson *backup_info = cson_read(backup_info_path);
                                if (!cson_is_map(backup_info)){
                                    fprintf(stdout, " - --invalid backup--\n");
                                } else{
                                    char *time_created = cson_get_cstring(backup_info, key("created"));
                                    if (time_created != NULL){
                                        fprintf(stdout, " - %s, created at %s\n", backup, time_created);
                                    }else{
                                        fprintf(stdout, " - %s\n", backup);
                                    }
                                }
                            }else {
                                fprintf(stdout, " - --null--\n");
                            }
                        }
                        return_defer(0);
                    }
                    return_defer(print_branches(branches));
                }
                else if (strcmp(arg, "new") == 0){
                    if (argc < 2){
                        fprintf(stderr, "[ERROR] Missing arguments!\n");
                        print_branch_usage(program_name);
                        return_defer(1);
                    }
                    Cson *branches = cson_get(info, key("branches"));
                    if (!cson_is_map(branches)){
                        fprintf(stderr, "[ERROR] Invalid info file state!\n");
                        return_defer(1);
                    }
                    const char *name = shift_args(argc, argv);
                    if (cson_map_iskey(branches, cson_str((char*) name))){
                        fprintf(stderr, "[ERROR] Branch already exists!\n");
                        return_defer(1);
                    }
                    Cson *branch = cson_map_new();
                    Cson *dirs = cson_array_new();
                    Cson *id = cson_new_int(0);
                    Cson *backups = cson_array_new();
                    while (argc > 0){
                        const char *dir = shift_args(argc, argv);
                        cson_array_push(dirs, cson_new_cstring((char*) dir));
                    }
                    (void) cson_map_insert(branch, cson_str("dirs"), dirs);
                    (void) cson_map_insert(branch, cson_str("backups"), backups);
                    (void) cson_map_insert(branch, cson_str("last_id"), id);
                    
                    (void) cson_map_insert(branches, cson_str((char*) name), branch);
                    
                    cson_write(info, info_path);
                    fprintf(stdout, "[INFO] Successfully created new branch '%s'!", name);
                    return_defer(0);
                }
                else if (strcmp(arg, "delete") == 0){
                    if (argc < 1){
                        fprintf(stderr, "[ERROR] Missing argument!\n");
                        print_branch_usage(program_name);
                        return_defer(1);
                    }
                    Cson *branches = cson_get(info, key("branches"));
                    if (!cson_is_map(branches)){
                        fprintf(stderr, "[ERROR] Invalid info file state!\n");
                        return_defer(1);
                    }
                    char *name = (char*) shift_args(argc, argv);
                    CsonError error = cson_map_remove(branches, cson_str(name));
                    if (error == CsonError_KeyError){
                        fprintf(stderr, "[ERROR] This branch does not exist: '%s'!\n", name);
                        return_defer(1);
                    }else if (error == CsonError_Success){
                        cson_write(info, info_path);
                        fprintf(stdout, "[INFO] Successfully deleted branch '%s'!\n", name);
                        return_defer(0);
                    }else{
                        fprintf(stderr, "[ERROR] An error occured while deleting branch '%s': %s!\n", name, CsonErrorStrings[error]);
                        return_defer(1);
                    }
                }
                else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0){
                    print_branch_usage(program_name);
                    return_defer(0);
                }
                else{
                    fprintf(stderr, "[ERROR] Unknown option: '%s'!\n", arg);
                    print_branch_usage(program_name);
                    return_defer(1);
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
                return_defer(1);
            }
            run(tbackup, command_options);
        }break;
        case Cmd_Merge:{
            if (command_option_count < 2){
                fprintf(stderr, "[ERROR] Too few arguments provided!\n\n");
                print_merge_usage(program_name);
                return_defer(1);
            }
            run(tmerge, command_options);
        }break;
        case Cmd_Branch:{
            print_branch_usage(program_name);
        } break;
        default: {assert(0 && "Invalid Cmd type\n\n");};
    }
  defer:
    cleanup();
    return value;
}