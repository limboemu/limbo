// Copyright 2012 Google Inc. All Rights Reserved.
//
// This code is licensed under the same terms as WebM:
//  Software License Agreement:  http://www.webmproject.org/license/software/
//  Additional IP Rights Grant:  http://www.webmproject.org/license/additional/
// -----------------------------------------------------------------------------
//
// JPEG decode.

#ifndef WEBP_EXAMPLES_JPEGDEC_H_
#define WEBP_EXAMPLES_JPEGDEC_H_

#include <stdio.h>
#include "webp/types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

struct Metadata;
struct WebPPicture;

// Reads a JPEG from 'in_file', returning the decoded output in 'pic'.
// The output is RGB.
// Returns true on success.
int ReadJPEG(FILE* in_file, struct WebPPicture* const pic,
             struct Metadata* const metadata);

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

#endif  // WEBP_EXAMPLES_JPEGDEC_H_
