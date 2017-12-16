// Copyright 2012 Google Inc. All Rights Reserved.
//
// This code is licensed under the same terms as WebM:
//  Software License Agreement:  http://www.webmproject.org/license/software/
//  Additional IP Rights Grant:  http://www.webmproject.org/license/additional/
// -----------------------------------------------------------------------------
//
//  Metadata types and functions.
//

#ifndef WEBP_EXAMPLES_METADATA_H_
#define WEBP_EXAMPLES_METADATA_H_

#include "webp/types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct MetadataPayload {
  uint8_t* bytes;
  size_t size;
} MetadataPayload;

typedef struct Metadata {
  MetadataPayload exif;
  MetadataPayload iccp;
  MetadataPayload xmp;
} Metadata;

#define METADATA_OFFSET(x) offsetof(Metadata, x)

void MetadataInit(Metadata* const metadata);
void MetadataPayloadDelete(MetadataPayload* const payload);
void MetadataFree(Metadata* const metadata);

// Stores 'metadata' to 'payload->bytes', returns false on allocation error.
int MetadataCopy(const char* metadata, size_t metadata_len,
                 MetadataPayload* const payload);

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

#endif  // WEBP_EXAMPLES_METADATA_H_
