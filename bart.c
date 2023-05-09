#include <stdio.h>

#include "zoe.h"

int main(int argc, char *argv[]) {
    char *zipname = zoe_self(argv[0]);
    char *which = "DO-NOT-DELETE";
    printf("argv[0]={%s} / zipname={%s}\n", argv[0], zipname);
    zoe_t *zip;
    if (argc > 1) which = argv[1];
    zip = zoe_open(zipname);
    printf("# zip->r = %d {%s}\n", zip->r, zoe_error(zip->r));
    int files = zoe_files(zip);
    printf("# files = %d\n", files);
    for (int i=0; i<files; i++) {
        zoe_info_at(zip, i);
    }
    int r = zoe_find(zip, which);
    if (r >= 0) {
        printf("# reading from %s in attached archive\n", which);
        zoe_file_t *file = zoe_fopen_at(zip, r);
        if (file) {
            printf("(size=%d)\n", ZOE_FILE_SIZE(file));
            char line[1024];
            int i = 0;
            char mode = 0; // text
            // scan first n-bytes of file and decide if we
            // should dump line-by-line or as hexdump
            while (1) {
                char *s = fgets(line, sizeof(line), ZOE_FILE(file));
                if (s == NULL) break;
                line[strcspn(line, "\r\n")] = '\0';
                printf("[%d] {%s}\n", i, line);
                i++;
            }
            printf("# read %d lines\n", i);
            zoe_fclose(file);
        }
    }
    zoe_close(zip);
    return 0;
}
