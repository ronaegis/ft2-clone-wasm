#include <stddef.h>

#include "fts.h"

typedef struct
{
    int dummy;
} fts_stub_state_t;

FTS *fts_open(char * const *path_argv, int options, int (*compar)(const FTSENT **, const FTSENT **))
{
    (void)path_argv;
    (void)options;
    (void)compar;
    return NULL;
}

FTSENT *fts_read(FTS *ftsp)
{
    (void)ftsp;
    return NULL;
}

int fts_close(FTS *ftsp)
{
    (void)ftsp;
    return 0;
}
