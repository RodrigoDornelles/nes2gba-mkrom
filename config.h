#ifndef _CONFIG_H
#define _CONFIG_H
#include "list.h"

#define PPU_HACK       1
#define NOCPU_HACK (1<<1)
#define PAL_TIMING (1<<2)
#define SF_ADDRESS (1<<5)

typedef struct header_ {
        char name[32];
        unsigned int filesize;
        unsigned int flags;
        unsigned int spritefollow;
        unsigned int reserved;
} header;

typedef struct rominfo_ {
        char *filename;
        list *hacks;
        header hdr;
} rominfo;

typedef struct hack_ {
        unsigned int offset;
        unsigned int value;
} hack;

typedef struct phack_ {
        char * filename;
        char * strdata;
        unsigned int offset;
} phack;


typedef struct config_ {
        list *roms;
        char *pnes_rom;
        char *output_rom;
        char *splash_file;
        list *phacks;
        rominfo cur;
} config;

extern config conf;
void load_config(char *file);

#endif                          /* _CONFIG_H */
