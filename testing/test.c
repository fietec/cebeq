#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>

void print_usage(const char *name)
{
    printf("%s - How to use:\n", name);
    printf("  %s [flags] <files...>\n", name);
    printf("  Flags:\n");
    printf("    --help (-h): print this dialog\n");
}

char * shift_args(int *argc, char ***argv)
{
    assert(*argc > 0 && "Exceeded argv boundaries!");
    char *result = **argv;
    *argc -= 1;
    *argv += 1;
    return result;
}

uint64_t f_size(const char *filename)
{
    struct stat attr;
    if (stat(filename, &attr) == -1){
        fprintf(stderr, "[ERROR] Could not access '%s'!\n", filename);
        return 0;
    }
    return attr.st_size;
}

int main(int argc, char **argv)
{
    const char *program_name = shift_args(&argc, &argv);
    if (argc == 0){
        fprintf(stderr, "[ERROR] No input files provided!\n");
        print_usage(program_name);
        return 1;
    }
    const char *files[argc];
    size_t size = 0;
    while (argc > 0){
        const char *arg = shift_args(&argc, &argv);
        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0){
            print_usage(program_name);
            return 0;
        }
        files[size++] = arg;
    }
    for (size_t i=0; i<size; ++i){
        const char *filename = files[i];
        uint64_t size = f_size(filename);
        printf("File '%s' has %llu bytes.\n", filename, size);
    }
    return 0;
}