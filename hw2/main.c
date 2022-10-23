#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "charset.h"

#define SIZEOFF(ARR) (sizeof(ARR) / sizeof(ARR[0]))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))


int main(int argc, char **argv) {
  if (argc < 4) {
    puts("Convert code-paged text file to utf8.\n"
           "Usage:\n\tcp2utf8 <cp-file-in> <code-page> <out-file>\n"
           "Code pages supported:");
    for (size_t i = 0; i < cp_size; i++) {
      printf("\t%s\n",cp_names[i]);
    }
    return EXIT_FAILURE;
  }
  const char *inp_file_path = argv[1];
  const char *code_page_name = argv[2];
  const char *out_file_path = argv[3];

  // Search for charset

  size_t cp_index = 0;
  while (cp_index < cp_size) {
    if (strncmp(code_page_name, cp_names[cp_index], max_cp_name_len) == 0){
      break;
    }
    cp_index++;
  }

  if (cp_index == cp_size) {
    fprintf(stderr, "Error: code-page is not supported\n Run witout args to see supported code-pages\n");
    return EXIT_FAILURE;
  }

  FILE *fin = fopen(inp_file_path, "rb");
  if (!fin) {
    fprintf(stderr, "%s - io error: %s\n", inp_file_path, strerror(errno));
    return EXIT_FAILURE;
  }

  FILE *fout = fopen(out_file_path, "wb");
  if (!fout) {
    fprintf(stderr, "%s - io error: %s\n", inp_file_path, strerror(errno));
    fclose(fin);
    return EXIT_FAILURE;
  }

  int ch;
  const unsigned int *cp = cp_data[cp_index];
  int io_result = 0;

  while ((ch = fgetc(fin)) != EOF && io_result != EOF) {
    if (ch < 0x80) {
      io_result = fputc(ch, fout);
    } else {
      unsigned int up = cp[ch - 0x80];
      if (up < 0x800) {
        fputc(0xC0 | up >> 6, fout);
      } else if (up < 0x8000) {
        fputc(0xE0 | up >> 12, fout);
        fputc(0x80 | (up >> 6 & 0x3F), fout);
      } else {
        fputc(0xF0 | up >> 18, fout);
        fputc(0x80 | (up >> 12 & 0x3F), fout);
        fputc(0x80 | (up >> 6 & 0x3F), fout);
      }
      io_result = fputc(0x80 | (up & 0x3F), fout);
    }
  }
  if (io_result == EOF) {
    perror("Failed on writing file:");
  }
  fclose(fin);
  fclose(fout);
  return io_result == EOF ? EXIT_FAILURE :EXIT_SUCCESS;
}
