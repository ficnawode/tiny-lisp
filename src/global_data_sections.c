#include "global_data_sections.h"
#include <errno.h> // For perror and errno
#include <stdbool.h>
#include <stdio.h> // For ftell, ferror, etc.
#include <stdlib.h>
#include <string.h>

static void
make_temp_filename (char *buffer, size_t buf_size, const char *base,
                    const char *section)
{
  snprintf (buffer, buf_size, "%s.%s.tmp.s", base, section);
}

static int
copy_file_content (FILE *dest, FILE *src)
{
  char buffer[4096];
  size_t bytes_read;
  rewind (src);

  while ((bytes_read = fread (buffer, 1, sizeof (buffer), src)) > 0)
    {
      if (fwrite (buffer, 1, bytes_read, dest) != bytes_read)
        {
          perror ("fwrite failed in copy_file_content");
          return -1;
        }
    }

  if (ferror (src))
    {
      perror ("fread failed in copy_file_content");
      return -1;
    }
  return 0;
}

static void
append_and_cleanup_section (FILE *final_asm_file, const char *base_filename,
                            const char *section_name, FILE *section_file,
                            bool write_section_header)
{
  if (!section_file)
    {
      return;
    }

  fflush (section_file);
  long file_size = ftell (section_file);

  if (file_size > 0)
    {
      if (write_section_header)
        {
          fprintf (final_asm_file, "\nsection .%s\n", section_name);
        }
      if (copy_file_content (final_asm_file, section_file) != 0)
        {
          fprintf (stderr, "Warning: Failed to copy content for section .%s\n",
                   section_name);
        }
    }
  else if (file_size == -1L)
    {
      perror ("Warning: ftell failed on temporary file");
    }

  fclose (section_file);

  char temp_name[256];
  make_temp_filename (temp_name, sizeof (temp_name), base_filename,
                      section_name);
  remove (temp_name);
}

static void
gds_destroy (struct GlobalDataSections *gds)
{
  if (!gds)
    {
      return;
    }

  const char *sections[] = { "func", "text", "data", "rodata", "bss" };
  FILE *files[] = { gds->func_file, gds->text_file, gds->data_file,
                    gds->rodata_file, gds->bss_file };
  char temp_name[256];

  for (size_t i = 0; i < sizeof (sections) / sizeof (sections[0]); ++i)
    {
      if (files[i])
        {
          fclose (files[i]);
          make_temp_filename (temp_name, sizeof (temp_name),
                              gds->base_filename, sections[i]);
          remove (temp_name);
        }
    }
  free (gds);
}

struct GlobalDataSections *
gds_create (const char *base_filename)
{
  struct GlobalDataSections *gds
      = calloc (1, sizeof (struct GlobalDataSections));
  if (!gds)
    {
      perror ("calloc failed for GlobalDataSections");
      return NULL;
    }

  strncpy (gds->base_filename, base_filename, sizeof (gds->base_filename) - 1);
  gds->base_filename[sizeof (gds->base_filename) - 1] = '\0';

  const char *sections[] = { "func", "text", "data", "rodata", "bss" };
  FILE **file_ptrs[] = { &gds->func_file, &gds->text_file, &gds->data_file,
                         &gds->rodata_file, &gds->bss_file };
  char temp_name_buf[256];

  for (size_t i = 0; i < sizeof (sections) / sizeof (sections[0]); ++i)
    {
      make_temp_filename (temp_name_buf, sizeof (temp_name_buf), base_filename,
                          sections[i]);
      *file_ptrs[i] = fopen (temp_name_buf, "w+");

      if (!*file_ptrs[i])
        {
          perror ("Failed to create temporary section file");
          fprintf (stderr, " -> Section: %s, File: %s\n", sections[i],
                   temp_name_buf);
          gds_destroy (gds);
          return NULL;
        }
    }

  return gds;
}

void
gds_close_and_finalize (struct GlobalDataSections *gds_ctx)
{
  if (!gds_ctx)
    {
      return;
    }

  char final_filename[256 + 3];
  snprintf (final_filename, sizeof (final_filename), "%s.s",
            gds_ctx->base_filename);

  FILE *final_asm_file = fopen (final_filename, "w");
  if (!final_asm_file)
    {
      perror ("Failed to open final assembly file");
      gds_destroy (gds_ctx);
      return;
    }
  append_and_cleanup_section (final_asm_file, gds_ctx->base_filename, "text",
                              gds_ctx->func_file, true);

  append_and_cleanup_section (final_asm_file, gds_ctx->base_filename, "",
                              gds_ctx->text_file, false);
  append_and_cleanup_section (final_asm_file, gds_ctx->base_filename, "rodata",
                              gds_ctx->rodata_file, true);
  append_and_cleanup_section (final_asm_file, gds_ctx->base_filename, "data",
                              gds_ctx->data_file, true);
  append_and_cleanup_section (final_asm_file, gds_ctx->base_filename, "bss",
                              gds_ctx->bss_file, true);

  fclose (final_asm_file);
  free (gds_ctx);
}
