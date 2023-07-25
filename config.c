#include "list.h"
#include "config.h"
#include <expat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int fsize(char *file)
{
        struct stat s;
        if (stat(file, &s) < 0) {
                return -1;
        }

        return s.st_size;
}

config conf;
int xml_depth = 0;

void free_rom(void *data)
{
        rominfo *rom = (rominfo *) data;
        if (rom->filename)
                free(rom->filename);
        free(rom);
}

void free_phack(void *data)
{
        phack *p = (phack *) data;
        free(p->filename);
        free(p->strdata);
        free(p);
}

void pnes_start(void *data, const char *el, const char **attr)
{
        int i;
        for (i = 0; attr[i]; i += 2) {
                if (!strcmp(attr[i], "source")) {
                        conf.pnes_rom = strdup(attr[i + 1]);
                }
                if (!strcmp(attr[i], "output")) {
                        conf.output_rom = strdup(attr[i + 1]);
                }
        }
}

void pnes_end(void *data, const char *el)
{
        if (!conf.pnes_rom) {
                fprintf(stderr,
                        "Must specify source attribute for pnes element\n");
                exit(1);
        }
        if (!conf.output_rom) {
                fprintf(stderr,
                        "Must specify output attribute for pnes element\n");
                exit(1);
        }
}

unsigned int on_off(int orig, int flag, const char *val)
{
        int new = atoi(val);
        if (new)
                return orig | flag;
        return orig & ~flag;
}

void splash_start(void *data, const char *el, const char **attr)
{
        int i;
        for (i = 0; attr[i]; i += 2) {
                if (!strcmp(attr[i], "source")) {
                        conf.splash_file = strdup(attr[i + 1]);
                        if (fsize(conf.splash_file) < 0) {
                                fprintf(stderr,
                                        "Splash file does not exist on line %d\n",
                                        XML_GetCurrentLineNumber((XML_Parser) data));
                                exit(1);
                        }
                        return;
                }
        }
        fprintf(stderr, "Missing required attribute source on line %d\n",
                XML_GetCurrentLineNumber((XML_Parser) data));
        exit(1);
}

void patch_start(void *data, const char *el, const char **attr)
{
        int i;
        phack *p = (phack *)calloc(1,sizeof(phack));
        
        for (i = 0; attr[i]; i += 2) {
                if (!strcmp(attr[i], "source")) {
                        p->filename = strdup(attr[i + 1]);
                        if (fsize(p->filename) < 0) {
                                fprintf(stderr,
                                        "Patch file does not exist on line %d\n",
                                        XML_GetCurrentLineNumber((XML_Parser) data));
                                exit(1);
                        }
                } else if (!strcmp(attr[i], "offset")) {
                        if (!sscanf(attr[i + 1], "%i", &p->offset)) {
                                fprintf(stderr, "Offset must be numeric on line %d\n",
                                                XML_GetCurrentLineNumber((XML_Parser) data));
                                exit(1);
                        }
                } else if (!strcmp(attr[i], "value")) {
                        p->strdata = strdup(attr[i + 1]);
                }
        }
        if ((!p->filename && !p->strdata) || !p->offset) {
                fprintf(stderr, "Required attribute(s) missing in patch element on line %d\n",
                                XML_GetCurrentLineNumber((XML_Parser) data));
                exit(1);
        }
        if (!conf.phacks) conf.phacks = list_create(free_phack);
        list_append(conf.phacks, p);
}


void cheat_start(void *data, const char *el, const char **attr)
{
        int i;
        hack *h = (hack *)calloc(1, sizeof(hack));
        for (i = 0; attr[i]; i += 2) {
                if (!strcmp(attr[i], "offset")) {
                        if(!sscanf(attr[i + 1], "%i", &h->offset)) {
                                fprintf(stderr, "Offset must be numeric on line %d\n",
                                                XML_GetCurrentLineNumber((XML_Parser) data));
                                exit(1);
                        }
                } else if (!strcmp(attr[i], "value")) {
                        if (!sscanf(attr[i + 1], "%i", &h->value)) {
                                fprintf(stderr, "Value must be numeric on line %d\n",
                                                XML_GetCurrentLineNumber((XML_Parser) data));
                                exit(1);
                        }
                        if (h->value > 255) {
                                fprintf(stderr, "Value must be a single byte on line %d\n",
                                                XML_GetCurrentLineNumber((XML_Parser) data));
                                exit(1);
                        }
                }
        }
        if (!conf.cur.hacks) conf.cur.hacks = list_create(free);
        list_append(conf.cur.hacks, h);
}

void rom_start(void *data, const char *el, const char **attr)
{
        int i;
        memset(&conf.cur, sizeof(rominfo), 0);
        conf.cur.hacks = NULL;
        for (i = 0; attr[i]; i += 2) {
                if (!strcmp(attr[i], "name")) {
                        strncpy(conf.cur.hdr.name, attr[i + 1],
                                sizeof(conf.cur.hdr.name));
                } else if (!strcmp(attr[i], "source")) {
                        conf.cur.filename = strdup(attr[i + 1]);
                } else if (!strcmp(attr[i], "pal")) {
                        conf.cur.hdr.flags =
                            on_off(conf.cur.hdr.flags, PAL_TIMING,
                                   attr[i + 1]);
                } else if (!strcmp(attr[i], "nocpuhack")) {
                        conf.cur.hdr.flags =
                            on_off(conf.cur.hdr.flags, NOCPU_HACK,
                                   attr[i + 1]);
                } else if (!strcmp(attr[i], "ppuhack")) {
                        conf.cur.hdr.flags =
                            on_off(conf.cur.hdr.flags, PPU_HACK,
                                   attr[i + 1]);
                } else if (!strcmp(attr[i], "spritefollow")) {
                        conf.cur.hdr.spritefollow = atoi(attr[i + 1]);
                } else if (!strcmp(attr[i], "spmem")) {
                        conf.cur.hdr.flags =
                            on_off(conf.cur.hdr.flags, SF_ADDRESS,
                                   attr[i + 1]);
                } else {
                        fprintf(stderr,
                                "Unknown attribute in config.xml at line %d: %s\n",
                                XML_GetCurrentLineNumber((XML_Parser)
                                                         data), attr[i]);
                }
        }
}

void rom_end(void *data, const char *el)
{
        if (!conf.cur.filename) {
                fprintf(stderr,
                        "Must specify source attr for rom element\n");
                exit(2);
        }
        if ((conf.cur.hdr.filesize = fsize(conf.cur.filename)) == -1) {
                perror(conf.cur.filename);
                exit(2);
        }
        rominfo *r = calloc(1, sizeof(rominfo));
        memcpy(r, &conf.cur, sizeof(rominfo));
        list_append(conf.roms, r);
}

void start(void *data, const char *el, const char **attr)
{
        if (xml_depth == 0 && strcmp(el, "pnes")) {
                fprintf(stderr, "Top-level element must be pnes\n");
                exit(1);
        }

        if (!strcmp(el, "splash")) {
                splash_start(data, el, attr);
        } else if (!strcmp(el, "pnes")) {
                pnes_start(data, el, attr);
        } else if (!strcmp(el, "rom")) {
                rom_start(data, el, attr);
        } else if (!strcmp(el, "cheat")) {
                cheat_start(data, el, attr);
        } else if (!strcmp(el, "font")) {
                patch_start(data, el, attr);
        } else if (!strcmp(el, "palette")) {
                patch_start(data, el, attr);
        } else if (!strcmp(el, "patch")) {
                patch_start(data, el, attr);
        }
        xml_depth++;
}

void end(void *data, const char *el)
{
        if (!strcmp(el, "rom")) {
                rom_end(data, el);
        } else if (!strcmp(el, "pnes")) {
                pnes_end(data, el);
        }
        xml_depth--;
}

void load_config(char *file)
{
        FILE *fp;
        char *buf = (char *) calloc(1, 1024);
        XML_Parser p = XML_ParserCreate(NULL);

        XML_SetElementHandler(p, start, end);
        XML_SetUserData(p, p);
        fp = fopen(file, "r");
        if (!fp) {
                perror(file);
                exit(2);
        }
        conf.roms = list_create(free_rom);

        while (1) {
                int done, len;
                len = fread(buf, 1, 1024, fp);
                done = feof(fp);
                if (!XML_Parse(p, buf, len, done)) {
                        fprintf(stderr,
                                "Parse error in config at line %d:\n%s\n",
                                XML_GetCurrentLineNumber(p),
                                XML_ErrorString(XML_GetErrorCode(p)));
                        exit(1);
                }

                if (done)
                        break;
        }
}
