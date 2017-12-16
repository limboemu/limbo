// Copyright 2013 Google Inc. All Rights Reserved.
//
// This code is licensed under the same terms as WebM:
//  Software License Agreement:  http://www.webmproject.org/license/software/
//  Additional IP Rights Grant:  http://www.webmproject.org/license/additional/
// -----------------------------------------------------------------------------
//
// Windows Imaging Component (WIC) decode.

#ifndef WEBP_EXAMPLES_WICDEC_H_
#define WEBP_EXAMPLES_WICDEC_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

struct Metadata;
struct WebPPicture;

// Reads an image from 'filename', returning the decoded output in 'pic'.
// If 'keep_alpha' is true and the image has an alpha channel, the output is
// RGBA otherwise it will be RGB.
// Returns true on success.
int ReadPictureWithWIC(const char* const filename,
                       struct WebPPicture* const pic, int keep_alpha,
                       struct Metadata* const metadata);

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

#endif  // WEBP_EXAMPLES_WICDEC_H_
