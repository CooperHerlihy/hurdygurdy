#include <cstdint>
#include <cstdio>

/**
 * A small script which takes a file and turns its data into a C file which can
 * be included and compiled into source
 *
 * The first argument is the file to read and embed
 * The second argument is the name of the symbol to write in the C output
 * The third argument (optional) is the output file path; if omitted, stdout is used
 */
int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <file> <symbol name> [output file]\n", argv[0]);
        return -1;
    }

    if (argc >= 4 && freopen(argv[3], "wb", stdout) == nullptr) {
        fprintf(stderr, "Failed to open output file: %s\n", argv[3]);
        return -1;
    }

    size_t nameLen = 0;
    for (; argv[2][nameLen] != '\0'; ++nameLen);
    for (size_t i = 0; i < nameLen; i++) {
        switch (argv[2][i]) {
            case '.':
            case '/':
            case '\\':
                argv[2][i] = '_';
                break;
        }
    }

    FILE *file = fopen(argv[1], "rb");
    if (file == nullptr) {
        fprintf(stderr, "Failed to open file: %s\n", argv[1]);
        return -1;
    }

    printf("const unsigned char %s[] = {", argv[2]);

    uint8_t byte;
    uint32_t lineCount = 0;
    while (fread(&byte, 1, 1, file) > 0) {
        if (lineCount % 16 == 0)
            printf("\n\t");
        else
            printf(" ");
        ++lineCount;

        printf("0x%02x,", byte);
    }

    printf("\n};\n");

    return 0;
}
