#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "config.h"

int fsize(char *file);

typedef struct binary_ {
        unsigned int length;
        char *filename;
        char *data;
} binary;

binary *load_binary(char *file)
{
        char *data;
        FILE *fp;
        int size, len, read, offset=0;
        binary *bin;
        if ((size = fsize(file)) == -1) return NULL;

        data = (char *)calloc(1, size);
        len = size;
        if(!(fp = fopen(file,"r"))) return NULL;

        while (!feof(fp) && len > 0) {
                read = fread(data + offset, 1, len, fp);
                len -= read;
                offset += read;
        }
        
        fclose(fp);

        bin = (binary *)calloc(1, sizeof(binary));
        bin->length = offset;
        bin->filename = strdup(file);
        bin->data = data;
        return bin;
}

void free_binary(binary *bin) {
        free(bin->data);
        free(bin);
}

void apply_hacks(binary *bin, list *hacks)
{
        list_foreach(hacks, hack *, i, c)
                if (i->offset > bin->length) {
                        fprintf(stderr, "Offset greater than file length for %s (%.8X:%.2X > %.8X)\n",
                                        bin->filename, i->offset, i->value, bin->length);
                        exit(3);
                }
                bin->data[i->offset] = (char)(i->value & 0xFF);
        list_foreach_end;
}

void apply_pnes_hacks(binary *bin)
{
        binary *file;

        list_foreach(conf.phacks, phack *, i, c)
                if (i->filename) {
                        if(!(file = load_binary(i->filename))) {
                                fprintf(stderr, "Failed to load patch file %s!\n",
                                                i->filename);
                                exit(2);
                        }
                        if (i->offset + file->length > bin->length) {
                                fprintf(stderr, "Patch %s is too large.\n",
                                                i->filename);
                                exit(3);
                        }
                        /* - 1 for compatibility with Hoe's offsets.ini */
                        memcpy(bin->data + i->offset - 1, file->data, file->length);
                        free_binary(file);
                } else if (i->strdata) {
                        if (i->offset + strlen(i->strdata) > bin->length) {
                                fprintf(stderr, "Value %s is too large.\n",
                                                i->strdata);
                                exit(3);
                        }
                        memcpy(bin->data + i->offset - 1, i->strdata, strlen(i->strdata));
                }
        list_foreach_end;
}

void append_binary(FILE * dest, binary * src)
{
        fwrite(src->data, 1, src->length, dest);
}

void append_file(FILE * dest, FILE * src)
{
        char *buf = (char *) calloc(1024, 1);
        size_t read = 0;

        while (!feof(src)) {
                read = fread(buf, 1, 1024, src);
                fwrite(buf, read, 1, dest);
        }
        free(buf);
}

void report_stats()
{
        double s;
        s = fsize(conf.output_rom);
        if (s < 0)
                exit(42);
        printf("Output size: %lf megabits\n", s * 8 / 1024 / 1024);
}

int main(int argc, char **argv)
{
        FILE *output;
        binary *pnes_rom;
        binary *nes_rom;
        struct stat s;

        rominfo info;

        load_config("config.xml");

        output = fopen(conf.output_rom, "w");
        pnes_rom = load_binary(conf.pnes_rom);
        if (!output || !pnes_rom) {
                perror("fopen");
                exit(2);
        }
        apply_pnes_hacks(pnes_rom);
        append_binary(output, pnes_rom);
        if (conf.splash_file) {
                FILE *splash = fopen(conf.splash_file, "r");
                if (!splash) {
                        perror(conf.splash_file);
                        exit(2);
                }
                append_file(output, splash);
                fclose(splash);
        }

        list_foreach(conf.roms, rominfo *, i, c)
                nes_rom = load_binary(i->filename);
                if (!nes_rom) {
                        perror(i->filename);
                        exit(2);
                }
                apply_hacks(nes_rom, i->hacks);
                fwrite(&i->hdr, sizeof(header), 1, output);
                append_binary(output, nes_rom);
                free_binary(nes_rom);
        list_foreach_end;

        fclose(output);
        free_binary(pnes_rom);

        report_stats();

        return 0;
}
