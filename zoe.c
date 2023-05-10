#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if __APPLE__
//
#else
#include <sys/auxv.h>
#endif
#include "miniz.h"

#include "zoe.h"

static int debug = 0;

char *zoe_error(int r) {
    switch (r) {
        case ZOE_OK: return "OK";
        case ZOE_NO_FILE: return "NO_FILE";
        case ZOE_BAD_FILE: return "BAD_FILE";
        case ZOE_NO_EOCD: return "NO_EOCD";
        case ZOE_MINIZ_ERROR: return "MINIZ_ERROR";
        case ZOE_CANT_FIND: return "CANT_FIND";
        case ZOE_UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
    return "UNEXPECTED";
}

// user needs to free the returned non-NULL name after use
char *zoe_self(char *a0) {
    printf("# a0=%s\n", a0);
#if __APPLE__
    char avn[1024];
    uint32_t size = sizeof(avn);
    int r = _NSGetExecutablePath(avn, &size);
    //printf("r=%d size=%d avn=%s\n", r, size, avn);
#else
    char *avn = (char *)getauxval(AT_EXECFN);
#endif
    char name[PATH_MAX];
    if (realpath(avn, name)) {
        return strdup(name);
    }
    return NULL;
}

int zoe_find(zoe_t *zoe, char *name) {
    if (zoe && zoe->r == ZOE_OK && name) {
        int r = mz_zip_reader_locate_file(&zoe->zip, name, NULL, MZ_ZIP_FLAG_IGNORE_PATH);
        if (r >= 0) return r;
    }
    return ZOE_CANT_FIND;
}

zoe_file_t *zoe_file_new(void) {
    zoe_file_t *zfile = malloc(sizeof(zoe_file_t));
    if (zfile) {
        memset(zfile, 0, sizeof(zoe_file_t));
        return zfile;
    }
    return NULL;
}

zoe_file_t *zoe_fopen_at(zoe_t *zoe, int i) {
    if (zoe && zoe->r == ZOE_OK && i >= 0) {
        int size = zoe_size_at(zoe, i);
        zoe_file_t *zfile = zoe_file_new();
        if (zfile) {
            zfile->is_mz_file = 0;
            unsigned char *buf = malloc(size);
            if (buf) {
                int s = mz_zip_reader_extract_to_mem(&zoe->zip, i, buf, size, 0);
                FILE *mem = fmemopen(buf, size, "rb");
                if (mem) {
                    zfile->mem = mem;
                    zfile->is_mz_file = 1;
                    zfile->size = size;
                    zfile->buf = buf;
                    return zfile;
                }
                free(buf);
            }
        }
    }
    return NULL;
}

zoe_file_t *zoe_fopen(zoe_t *zoe, char *name) {
    int i = zoe_find(zoe, name);
    if (i >= 0) {
        return zoe_fopen_at(zoe, i);
    } else {
        FILE *f = fopen(name, "rb");
        if (f) {
            zoe_file_t *zfile = zoe_file_new();
            if (zfile) {
                zfile->mem = f;
                zfile->is_mz_file = 0;
                zfile->size = 0;
                zfile->buf = NULL;
                return zfile;
            }
        }
    }
    return NULL;
}

void zoe_fclose(zoe_file_t *zfile) {
    if (zfile) {
        if (zfile->mem) {
            fclose(zfile->mem);
            zfile->mem = NULL;
        }
        if (zfile->buf) {
            free(zfile->buf);
            zfile->buf = NULL;
        }
    }
}

// user must free returned string
char *zoe_name_at(zoe_t *zoe, int i) {
    if (zoe && zoe->r == ZOE_OK && i >= 0) {
        mz_zip_archive_file_stat file_stat;
        int err = mz_zip_reader_file_stat(&zoe->zip, i, &file_stat);
        return strdup(file_stat.m_filename);
    }
    return NULL;
}

int zoe_size_at(zoe_t *zoe, int i) {
    if (zoe && zoe->r == ZOE_OK && i >= 0) {
        mz_zip_archive_file_stat file_stat;
        int err = mz_zip_reader_file_stat(&zoe->zip, i, &file_stat);
        return file_stat.m_uncomp_size;
    }
    return -1;
}

int zoe_zsize_at(zoe_t *zoe, int i) {
    if (zoe && zoe->r == ZOE_OK && i >= 0) {
        mz_zip_archive_file_stat file_stat;
        int err = mz_zip_reader_file_stat(&zoe->zip, i, &file_stat);
        return file_stat.m_comp_size;
    }
    return -1;
}

int zoe_isdir_at(zoe_t *zoe, int i) {
    if (zoe && zoe->r == ZOE_OK && i >= 0) {
        mz_zip_archive_file_stat file_stat;
        int err = mz_zip_reader_file_stat(&zoe->zip, i, &file_stat);
        return file_stat.m_is_directory;
    }
    return -1;
}

int zoe_info_at(zoe_t *zoe, int i) {
    mz_zip_archive_file_stat file_stat;
    int err = mz_zip_reader_file_stat(&zoe->zip, i, &file_stat);
    printf("[%d] -> %s / uncomp-size=%d / comp-size=%d / is-directory=%d\n",
        i,
        file_stat.m_filename,
        (int)file_stat.m_uncomp_size,
        (int)file_stat.m_comp_size,
        file_stat.m_is_directory
    );
    return -1;
}

int zoe_files(zoe_t *zoe) {
    if (zoe && zoe->r == ZOE_OK) {
        return mz_zip_reader_get_num_files(&zoe->zip);
    }
    return 0;
}

// user must free the memory after
void zoe_close(zoe_t *zoe) {
    if (zoe) {
        if (zoe->exe) {
            fclose(zoe->exe);
            zoe->exe = NULL;
        }
        if (zoe->r == ZOE_OK) {
            mz_zip_reader_end(&zoe->zip);
            zoe->r = ZOE_UNKNOWN;
        }
    }
}

zoe_t *zoe_open(char *zipname) {
    if (debug) printf("miniz.c version: %s\n", MZ_VERSION);
    zoe_t *zoe = malloc(sizeof(zoe_t));
    int err;

    memset(zoe, 0, sizeof(zoe_t));
    zoe->r = ZOE_UNKNOWN;

    if (!zipname) {
        zoe->r = ZOE_NO_FILE;
        return zoe;
    }

    memset(&(zoe->zip), 0, sizeof(mz_zip_archive));

    if (debug) printf("looking at %s\n", zipname);

    zoe->exe = fopen(zipname, "rb");
    if (!zoe->exe) {
        zoe->r = ZOE_BAD_FILE;
        return zoe;
    }

    fseek(zoe->exe, -22, SEEK_END);
    long eocd = ftell(zoe->exe);
    unsigned char eocd_sig[4] = {0x50, 0x4b, 0x05, 0x06};
    for (int i=0; i<4; i++) {
        unsigned char n = fgetc(zoe->exe);
        if (debug) printf("%02x : %02x\n", n, eocd_sig[i]);
        if (n != eocd_sig[i]) {
            if (debug) printf("no EOCD\n");
            fclose(zoe->exe);
            zoe->exe = NULL;
            zoe->r = ZOE_NO_EOCD;
            return zoe;
        }
    }

    fseek(zoe->exe, eocd + 12, SEEK_SET);
    int cs = 0;
    int mul[4] = {1, 256, 65536, 16777216};
    for (int i=0; i<4; i++) {
        unsigned char n = fgetc(zoe->exe);
        if (debug) printf("%02x\n", n);
        cs += (n * mul[i]);
    }
    if (debug) printf("cs = %x\n", cs);
    fseek(zoe->exe, eocd - cs, SEEK_SET);
    long cd = ftell(zoe->exe);
    unsigned char cd_sig[4] = {0x50, 0x4b, 0x01, 0x02};
    for (int i=0; i<4; i++) {
        unsigned char n = fgetc(zoe->exe);
        if (debug) printf("%02x : %02x\n", n, cd_sig[i]);
        if (n != cd_sig[i]) {
            if (debug) printf("no CD\n");
            fclose(zoe->exe);
            zoe->exe = NULL;
            zoe->r = ZOE_NO_CD;
            return zoe;
        }
    }
    fseek(zoe->exe, eocd + 16, SEEK_SET);
    int co = 0;
    for (int i=0; i<4; i++) {
        unsigned char n = fgetc(zoe->exe);
        if (debug) printf("%02x\n", n);
        co += (n * mul[i]);
    }
    if (debug) printf("co = %x\n", co);
    fseek(zoe->exe, cd - co, SEEK_SET);

    // open the appended zip file for reading
    mz_bool status;
    status = mz_zip_reader_init_cfile(&zoe->zip, zoe->exe, 0, 0);

    if (!status) {
        if (debug) fprintf(stderr, "Error: cannot open appended zip file %s\n", zipname);
        fclose(zoe->exe);
        zoe->exe = NULL;
        zoe->r = ZOE_MINIZ_ERROR;
        return zoe;
    }

    zoe->r = ZOE_OK;
    return zoe;

    int num_files = mz_zip_reader_get_num_files(&zoe->zip);
    if (debug) printf("Number of files in zip: %d\n", num_files);
    
    mz_zip_archive_file_stat file_stat;

    // iterate through the files in the zip file
    for (int i = 0; i < num_files; ++i) {
        err = mz_zip_reader_file_stat(&zoe->zip, i, &file_stat);
        if (!err) {
            if (debug) fprintf(stderr, "Error: cannot get file information\n");
            mz_zip_reader_end(&zoe->zip);
            fclose(zoe->exe);
            zoe->exe = NULL;
            //return -4;
            return zoe;
        }
        if (debug) printf("[%d] -> %s (%d)\n",
            i,
            file_stat.m_filename,
            (int)file_stat.m_uncomp_size
        );

    }

    return zoe;

    mz_zip_reader_end(&zoe->zip);
    fclose(zoe->exe);
    zoe->exe = NULL;

    return 0;
}
