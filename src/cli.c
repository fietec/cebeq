#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <cebeq.h>
#include <cwalk.h>
#include <cson.h>

typedef enum{
    Cmd_None, Cmd_Backup, Cmd_Merge
} Cmd;

static char *version = "";

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
    printf("  merge               Merge existing backups\n\n");
    
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
    printf("  -l, --list          List all available branches\n");
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

int print_branches(Cson *branches)
{
    Cson *branch_names = cson_map_keys(branches);
    if (!cson_is_array(branch_names)){
        fprintf(stderr, "[ERROR] Could not extract branch_names!\n");
        return 1;
    }
    size_t len = cson_len(branch_names);
    printf("Currently, there are %u branches:\n", len);
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
    printf("%s - version %s\n\b", program_name, version);
}

int run_backup(const char *args[])
{
    return make_backup(args[0], args[1], args[2]);
}

int main(int argc, char **argv)
{
    (void) cson_current_arena;
    cwk_path_set_style(CWK_STYLE_UNIX);
    
    char cwd_path[FILENAME_MAX] = {0};
    if (!get_exe_path(cwd_path, sizeof(cwd_path))) return 1;
    if (!get_parent_dir(cwd_path, cwd_path, sizeof(cwd_path))) return 1;
    
    char info_path[FILENAME_MAX] = {0};
    cwk_path_join(cwd_path, "../data/backups.json", info_path, sizeof(info_path));
    
    Cson *info = cson_read(info_path);
    if (info == NULL){
        eprintfn("Could not find internal info file!");
        return 1;
    }
    version = cson_get_string(info, key("version")).value;
    if (version == NULL){
        eprintfn("Invalid info file state!");
        return 1;
    }
    
    const char *program_name = shift_args(argc, argv);
    Cmd current_command = Cmd_None;
    
    const char *command_options[3] = {0};
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
                else if (strcmp(arg, "--list") == 0 || strcmp(arg, "-l") == 0){
                    Cson *branches = cson_get(info, key("backups"));
                    if (!cson_is_map(branches)){
                        fprintf(stderr, "[ERROR] Invalid info file state!\n");
                        return 1;
                    }
                    return print_branches(branches);
                }
                else{
                    if (command_option_count >= 3){
                        fprintf(stderr, "[ERROR] Unknown argument: '%s'!\n\n", arg);
                        print_backup_usage(program_name);
                        return 1;
                    }
                    command_options[command_option_count++] = arg;
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
                    command_options[command_option_count++] = arg;
                }
            } break;
            default: UNREACHABLE("Invalid Cmd type\n");
        }
    }
    switch (current_command){
        case Cmd_Backup: {
            if (command_option_count < 2){
                fprintf(stderr, "[ERROR] Too few arguments provided!\n\n");
                print_backup_usage(program_name);
                return 1;
            }
            return run_backup(command_options);
        }break;
        case Cmd_Merge:{
            if (command_option_count < 2){
                fprintf(stderr, "[ERROR] Too few arguments provided!\n\n");
                print_merge_usage(program_name);
                return 1;
            }
            printf("Arguments: ");
            for (size_t i=0; i<command_option_count; ++i){
                printf("'%s' ", command_options[i]);
            }
            printf("\nTODO: implement 'merge' call\n");
        }break;
        default: UNREACHABLE("Invalid Cmd type\n\n");
    }
    
    return 0;
}