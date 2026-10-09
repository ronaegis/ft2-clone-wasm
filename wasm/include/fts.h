#pragma once

#include <stdint.h>

typedef struct _FTS FTS;
typedef struct _FTSENT FTSENT;

struct _FTS
{
    int dummy;
};

struct _FTSENT
{
    unsigned short fts_info;
    char *fts_accpath;
};

#define FTS_NOCHDIR  0x0001
#define FTS_PHYSICAL 0x0002
#define FTS_XDEV     0x0004

#define FTS_NS      1
#define FTS_DNR     2
#define FTS_ERR     3
#define FTS_D       4
#define FTS_DC      5
#define FTS_DOT     6
#define FTS_NSOK    7
#define FTS_DP      8
#define FTS_F       9
#define FTS_SL      10
#define FTS_SLNONE  11
#define FTS_DEFAULT 12

FTS *fts_open(char * const *path_argv, int options, int (*compar)(const FTSENT **, const FTSENT **));
FTSENT *fts_read(FTS *ftsp);
int fts_close(FTS *ftsp);

