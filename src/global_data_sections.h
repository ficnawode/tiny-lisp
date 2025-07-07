#ifndef GLOBAL_DATA_SECTIONS_H
#define GLOBAL_DATA_SECTIONS_H

#include <stdio.h>

struct GlobalDataSections
{
  FILE *func_file;
  FILE *text_file;
  FILE *data_file;
  FILE *rodata_file;
  FILE *bss_file;
  char base_filename[256];
};

struct GlobalDataSections *gds_create (const char *base_filename);

void gds_close_and_finalize (struct GlobalDataSections *gds_ctx);

#endif