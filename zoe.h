#ifndef _ZOE_H_
#define _ZOE_H_

#include "miniz.h"

#define ZOE_OK          (0)
#define ZOE_NO_FILE     (-1)
#define ZOE_BAD_FILE    (-2)
#define ZOE_NO_EOCD     (-3)
#define ZOE_NO_CD       (-4)
#define ZOE_MINIZ_ERROR (-5)
#define ZOE_UNKNOWN     (-6)
#define ZOE_CANT_FIND   (-7)

typedef struct {
    FILE *exe;
    mz_zip_archive zip;
    int r;
} zoe_t;

typedef struct {
    FILE *mem;
    char is_mz_file;
    int size;
    unsigned char *buf;
} zoe_file_t;

#define ZOE_FILE(z) (z->mem)
#define ZOE_FILE_SIZE(z) (z->size)

char *zoe_self(char *a0);
char *zoe_error(int r);
zoe_t *zoe_open(char *zipname);
void zoe_close(zoe_t *zoe);
int zoe_files(zoe_t *zoe);
int zoe_find(zoe_t *zoe, char *name);
int zoe_info_at(zoe_t *zoe, int i);
int zoe_size_at(zoe_t *zoe, int i);
char *zoe_name_at(zoe_t *zoe, int i);
zoe_file_t *zoe_file_new(void);
zoe_file_t *zoe_fopen_at(zoe_t *zoe, int i);
zoe_file_t *zoe_fopen(zoe_t *zoe, char *name);
void zoe_fclose(zoe_file_t *zfile);

#endif
