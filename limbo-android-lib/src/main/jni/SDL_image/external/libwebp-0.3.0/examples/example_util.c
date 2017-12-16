// Copyright 2012 Google Inc. All Rights Reserved.
//
// This code is licensed under the same terms as WebM:
//  Software License Agreement:  http://www.webmproject.org/license/software/
//  Additional IP Rights Grant:  http://www.webmproject.org/license/additional/
// -----------------------------------------------------------------------------
//
//  Utility functions used by the example programs.
//

#include "./example_util.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

// -----------------------------------------------------------------------------
// File I/O

int ExUtilReadFile(const char* const file_name,
                   const uint8_t** data, size_t* data_size) {
  int ok;
  void* file_data;
  size_t file_size;
  FILE* in;

  if (file_name == NULL || data == NULL || data_size == NULL) return 0;
  *data = NULL;
  *data_size = 0;

  in = fopen(file_name, "rb");
  if (in == NULL) {
    fprintf(stderr, "cannot open input file '%s'\n", file_name);
    return 0;
  }
  fseek(in, 0, SEEK_END);
  file_size = ftell(in);
  fseek(in, 0, SEEK_SET);
  file_data = malloc(file_size);
  if (file_data == NULL) return 0;
  ok = (fread(file_data, file_size, 1, in) == 1);
  fclose(in);

  if (!ok) {
    fprintf(stderr, "Could not read %d bytes of data from file %s\n",
            (int)file_size, file_name);
    free(file_data);
    return 0;
  }
  *data = (uint8_t*)file_data;
  *data_size = file_size;
  return 1;
}

int ExUtilWriteFile(const char* const file_name,
                    const uint8_t* data, size_t data_size) {
  int ok;
  FILE* out;

  if (file_name == NULL || data == NULL) {
    return 0;
  }
  out = fopen(file_name, "wb");
  if (out == NULL) {
    fprintf(stderr, "Error! Cannot open output file '%s'\n", file_name);
    return 0;
  }
  ok = (fwrite(data, data_size, 1, out) == 1);
  fclose(out);
  return ok;
}

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif
