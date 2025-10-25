#include "hg_utils.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <file> <symbol>\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file: %s\n", argv[1]);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    usize file_size = (usize)ftell(file);
    rewind(file);

    for (usize i = 0; i < (usize)strlen(argv[2]); i++) {
        switch (argv[2][i]) {
            case '.':
            case '/':
            case '\\':
                argv[2][i] = '_';
                break;
        }
    }

    printf("const unsigned long %s_size = %ld;\n", argv[2], file_size);
    printf("const unsigned char %s[] = {", argv[2]);

    i32 line_count = 0;

    byte b;
    while (fread(&b, 1, 1, file) > 0) {
        if (line_count % 16 == 0)
            printf("\n\t");
        else
            printf(" ");
        ++line_count;

        printf("0x%02x,", b);
    }

    printf("\n};\n");

    return 0;
}

