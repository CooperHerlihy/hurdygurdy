#include <cstdint>
#include <cstdio>

/**
 * A small script which takes a file and turns its data into a C file which can
 * be included and compiled into source
 *
 * The first argument is the file to read and embed
 * The second argument is the name of the symbol to write in the C output
 *
 * The output is to stdout so can be redirected into a file
 */
int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <file> <symbol name>\n", argv[0]);
        return -1;
    }

    size_t name_len = 0;
    for (; argv[2][name_len] != '\0'; ++name_len);
    for (size_t i = 0; i < name_len; i++) {
        switch (argv[2][i]) {
            case '.':
            case '/':
            case '\\':
                argv[2][i] = '_';
                break;
        }
    }

    FILE *file = nullptr;
    (void)fopen_s(&file, argv[1], "rb");
    if (file == nullptr) {
        fprintf(stderr, "Failed to open file: %s\n", argv[1]);
        return -1;
    }

    printf("const unsigned char %s[] = {", argv[2]);

    uint8_t byte;
    uint32_t line_count = 0;
    while (fread(&byte, 1, 1, file) > 0) {
        if (line_count % 16 == 0)
            printf("\n\t");
        else
            printf(" ");
        ++line_count;

        printf("0x%02x,", byte);
    }

    printf("\n};\n");

    printf("const unsigned long %s_size = %ld;\n", argv[2], ftell(file));

    return 0;
}

