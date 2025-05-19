#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <cebeq.h>

typedef enum{
    Cmd_None, Cmd_Backup, Cmd_Merge
} Cmd;

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

    printf("Options:\n");
    printf("  -h, --help          Show this help message\n");
    printf("  -v, --version       Show program version\n\n");

    printf("Commands:\n");
    printf("  backup              Create a backup\n");
    printf("  merge               Merge existing backups\n\n");

    printf("Use '%s <command> --help' to see options for a specific command.\n", program_name);
}

void print_backup_usage(const char *program_name) {
    printf("Usage: %s backup [OPTIONS]\n\n", program_name);

    printf("Options for backup:\n");
    printf("  -h, --help          Show this help message\n");
}

void print_merge_usage(const char *program_name) {
    printf("Usage: %s merge [OPTIONS]\n\n", program_name);

    printf("Options for merge:\n");
    printf("  -h, --help          Show this help message\n");
}

void print_version(const char *program_name)
{
    // TODO: retrieve correct version
    printf("%s - version %s\n", program_name, "0.0.1");
}

int main(int argc, char **argv)
{
    const char *program_name = shift_args(argc, argv);
    Cmd current_command = Cmd_None;
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
                else{
                    fprintf(stderr, "[ERROR] Unknown argument: '%s'!\n\n", arg);
                    print_backup_usage(program_name);
                    return 1;
                }
            } break;
            case Cmd_Merge:{
                if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0){
                    print_merge_usage(program_name);
                    return 0;
                }
                else{
                    fprintf(stderr, "[ERROR] Unknown argument: '%s'!\n\n", arg);
                    print_merge_usage(program_name);
                    return 1;
                }
            } break;
            default: UNREACHABLE("Invalid Cmd type\n");
        }
    }
    if (command_option_count == 0){
        fprintf(stderr, "[ERROR] No arguments provided!\n\n");
        switch (current_command){
            case Cmd_Backup:{
                print_backup_usage(program_name);
            } break;
            case Cmd_Merge:{
                print_merge_usage(program_name);
            } break;
            default: UNREACHABLE("");
        }
        return 1;
    }
    
    return 0;
}