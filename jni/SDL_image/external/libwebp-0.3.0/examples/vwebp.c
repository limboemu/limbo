// Copyright 2011 Google Inc. All Rights Reserved.
//
// This code is licensed under the same terms as WebM:
//  Software License Agreement:  http://www.webmproject.org/license/software/
//  Additional IP Rights Grant:  http://www.webmproject.org/license/additional/
// -----------------------------------------------------------------------------
//
//  Simple OpenGL-based WebP file viewer.
//
// Author: Skal (pascal.massimino@gmail.com)
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(HAVE_GLUT_GLUT_H)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#ifdef FREEGLUT
#include <GL/freeglut.h>
#endif
#endif

#ifdef WEBP_HAVE_QCMS
#include <qcms.h>
#endif

#include "webp/decode.h"
#include "webp/demux.h"

#include "./example_util.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

static void Help(void);

// Unfortunate global variables. Gathered into a struct for comfort.
static struct {
  int has_animation;
  int has_color_profile;
  int done;
  int decoding_error;
  int print_info;
  int use_color_profile;

  int canvas_width, canvas_height;
  int loop_count;
  uint32_t bg_color;

  const char* file_name;
  WebPData data;
  WebPDecoderConfig* config;
  const WebPDecBuffer* pic;
  WebPDemuxer* dmux;
  WebPIterator frameiter;
  struct {
    int width, height;
    int x_offset, y_offset;
    enum WebPMuxAnimDispose dispose_method;
  } prev_frame;
  WebPChunkIterator iccp;
} kParams;

static void ClearPreviousPic(void) {
  WebPFreeDecBuffer((WebPDecBuffer*)kParams.pic);
  kParams.pic = NULL;
}

static void ClearParams(void) {
  ClearPreviousPic();
  WebPDataClear(&kParams.data);
  WebPDemuxReleaseIterator(&kParams.frameiter);
  WebPDemuxReleaseChunkIterator(&kParams.iccp);
  WebPDemuxDelete(kParams.dmux);
  kParams.dmux = NULL;
}

// -----------------------------------------------------------------------------
// Color profile handling
static int ApplyColorProfile(const WebPData* const profile,
                             WebPDecBuffer* const rgba) {
#ifdef WEBP_HAVE_QCMS
  int i, ok = 0;
  uint8_t* line;
  uint8_t major_revision;
  qcms_profile* input_profile = NULL;
  qcms_profile* output_profile = NULL;
  qcms_transform* transform = NULL;
  const qcms_data_type input_type = QCMS_DATA_RGBA_8;
  const qcms_data_type output_type = QCMS_DATA_RGBA_8;
  const qcms_intent intent = QCMS_INTENT_DEFAULT;

  if (profile == NULL || rgba == NULL) return 0;
  if (profile->bytes == NULL || profile->size < 10) return 1;
  major_revision = profile->bytes[8];

  qcms_enable_iccv4();
  input_profile = qcms_profile_from_memory(profile->bytes, profile->size);
  // qcms_profile_is_bogus() is broken with ICCv4.
  if (input_profile == NULL ||
      (major_revision < 4 && qcms_profile_is_bogus(input_profile))) {
    fprintf(stderr, "Color profile is bogus!\n");
    goto Error;
  }

  output_profile = qcms_profile_sRGB();
  if (output_profile == NULL) {
    fprintf(stderr, "Error creating output color profile!\n");
    goto Error;
  }

  qcms_profile_precache_output_transform(output_profile);
  transform = qcms_transform_create(input_profile, input_type,
                                    output_profile, output_type,
                                    intent);
  if (transform == NULL) {
    fprintf(stderr, "Error creating color transform!\n");
    goto Error;
  }

  line = rgba->u.RGBA.rgba;
  for (i = 0; i < rgba->height; ++i, line += rgba->u.RGBA.stride) {
    qcms_transform_data(transform, line, line, rgba->width);
  }
  ok = 1;

 Error:
  if (input_profile != NULL) qcms_profile_release(input_profile);
  if (output_profile != NULL) qcms_profile_release(output_profile);
  if (transform != NULL) qcms_transform_release(transform);
  return ok;
#else
  (void)profile;
  (void)rgba;
  return 1;
#endif  // WEBP_HAVE_QCMS
}

//------------------------------------------------------------------------------
// File decoding

static int Decode(void) {   // Fills kParams.frameiter
  const WebPIterator* const iter = &kParams.frameiter;
  WebPDecoderConfig* const config = kParams.config;
  WebPDecBuffer* const output_buffer = &config->output;
  int ok = 0;

  ClearPreviousPic();
  output_buffer->colorspace = MODE_RGBA;
  ok = (WebPDecode(iter->fragment.bytes, iter->fragment.size,
                   config) == VP8_STATUS_OK);
  if (!ok) {
    fprintf(stderr, "Decoding of frame #%d failed!\n", iter->frame_num);
  } else {
    kParams.pic = output_buffer;
    if (kParams.use_color_profile) {
      ok = ApplyColorProfile(&kParams.iccp.chunk, output_buffer);
      if (!ok) {
        fprintf(stderr, "Applying color profile to frame #%d failed!\n",
                iter->frame_num);
      }
    }
  }
  return ok;
}

static void decode_callback(int what) {
  if (what == 0 && !kParams.done) {
    int duration = 0;
    if (kParams.dmux != NULL) {
      WebPIterator* const iter = &kParams.frameiter;
      if (!WebPDemuxNextFrame(iter)) {
        WebPDemuxReleaseIterator(iter);
        if (WebPDemuxGetFrame(kParams.dmux, 1, iter)) {
          --kParams.loop_count;
          kParams.done = (kParams.loop_count == 0);
        } else {
          kParams.decoding_error = 1;
          kParams.done = 1;
          return;
        }
      }
      duration = iter->duration;
    }
    if (!Decode()) {
      kParams.decoding_error = 1;
      kParams.done = 1;
    } else {
      glutPostRedisplay();
      glutTimerFunc(duration, decode_callback, what);
    }
  }
}

//------------------------------------------------------------------------------
// Callbacks

static void HandleKey(unsigned char key, int pos_x, int pos_y) {
  (void)pos_x;
  (void)pos_y;
  if (key == 'q' || key == 'Q' || key == 27 /* Esc */) {
#ifdef FREEGLUT
    glutLeaveMainLoop();
#else
    ClearParams();
    exit(0);
#endif
  } else if (key == 'c') {
    if (kParams.has_color_profile && !kParams.decoding_error) {
      kParams.use_color_profile = 1 - kParams.use_color_profile;

      if (kParams.has_animation) {
        // Restart the completed animation to pickup the color profile change.
        if (kParams.done && kParams.loop_count == 0) {
          kParams.loop_count =
              (int)WebPDemuxGetI(kParams.dmux, WEBP_FF_LOOP_COUNT) + 1;
          kParams.done = 0;
          // Start the decode loop immediately.
          glutTimerFunc(0, decode_callback, 0);
        }
      } else {
        Decode();
        glutPostRedisplay();
      }
    }
  } else if (key == 'i') {
    kParams.print_info = 1 - kParams.print_info;
    glutPostRedisplay();
  }
}

static void HandleReshape(int width, int height) {
  // TODO(skal): proper handling of resize, esp. for large pictures.
  // + key control of the zoom.
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

static void PrintString(const char* const text) {
  void* const font = GLUT_BITMAP_9_BY_15;
  int i;
  for (i = 0; text[i]; ++i) {
    glutBitmapCharacter(font, text[i]);
  }
}

static float GetColorf(uint32_t color, int shift) {
  return (color >> shift) / 255.f;
}

static void DrawCheckerBoard(void) {
  const int square_size = 8;  // must be a power of 2
  int x, y;
  GLint viewport[4];  // x, y, width, height

  glPushMatrix();

  glGetIntegerv(GL_VIEWPORT, viewport);
  // shift to integer coordinates with (0,0) being top-left.
  glOrtho(0, viewport[2], viewport[3], 0, -1, 1);
  for (y = 0; y < viewport[3]; y += square_size) {
    for (x = 0; x < viewport[2]; x += square_size) {
      const GLubyte color = 128 + 64 * (!((x + y) & square_size));
      glColor3ub(color, color, color);
      glRecti(x, y, x + square_size, y + square_size);
    }
  }
  glPopMatrix();
}

static void HandleDisplay(void) {
  const WebPDecBuffer* const pic = kParams.pic;
  const WebPIterator* const iter = &kParams.frameiter;
  GLfloat xoff, yoff;
  if (pic == NULL) return;
  glPushMatrix();
  glPixelZoom(1, -1);
  xoff = (GLfloat)(2. * iter->x_offset / kParams.canvas_width);
  yoff = (GLfloat)(2. * iter->y_offset / kParams.canvas_height);
  glRasterPos2f(-1.f + xoff, 1.f - yoff);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, pic->u.RGBA.stride / 4);

  if (kParams.prev_frame.dispose_method == WEBP_MUX_DISPOSE_BACKGROUND) {
    // TODO(later): these offsets and those above should factor in window size.
    //              they will be incorrect if the window is resized.
    // glScissor() takes window coordinates (0,0 at bottom left).
    const int window_x = kParams.prev_frame.x_offset;
    const int window_y = kParams.canvas_height -
                         kParams.prev_frame.y_offset -
                         kParams.prev_frame.height;
    glEnable(GL_SCISSOR_TEST);
    // Only updated the requested area, not the whole canvas.
    glScissor(window_x, window_y,
              kParams.prev_frame.width, kParams.prev_frame.height);

    glClear(GL_COLOR_BUFFER_BIT);  // use clear color
    DrawCheckerBoard();

    glDisable(GL_SCISSOR_TEST);
  }
  kParams.prev_frame.width = iter->width;
  kParams.prev_frame.height = iter->height;
  kParams.prev_frame.x_offset = iter->x_offset;
  kParams.prev_frame.y_offset = iter->y_offset;
  kParams.prev_frame.dispose_method = iter->dispose_method;

  glDrawPixels(pic->width, pic->height,
               GL_RGBA, GL_UNSIGNED_BYTE,
               (GLvoid*)pic->u.RGBA.rgba);
  if (kParams.print_info) {
    char tmp[32];

    glColor4f(0.90f, 0.0f, 0.90f, 1.0f);
    glRasterPos2f(-0.95f, 0.90f);
    PrintString(kParams.file_name);

    snprintf(tmp, sizeof(tmp), "Dimension:%d x %d", pic->width, pic->height);
    glColor4f(0.90f, 0.0f, 0.90f, 1.0f);
    glRasterPos2f(-0.95f, 0.80f);
    PrintString(tmp);
    if (iter->x_offset != 0 || iter->y_offset != 0) {
      snprintf(tmp, sizeof(tmp), " (offset:%d,%d)",
               iter->x_offset, iter->y_offset);
      glRasterPos2f(-0.95f, 0.70f);
      PrintString(tmp);
    }
  }
  glPopMatrix();
  glFlush();
}

static void StartDisplay(void) {
  const int width = kParams.canvas_width;
  const int height = kParams.canvas_height;
  glutInitDisplayMode(GLUT_RGBA);
  glutInitWindowSize(width, height);
  glutCreateWindow("WebP viewer");
  glutDisplayFunc(HandleDisplay);
  glutIdleFunc(NULL);
  glutKeyboardFunc(HandleKey);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glClearColor(GetColorf(kParams.bg_color, 0),
               GetColorf(kParams.bg_color, 8),
               GetColorf(kParams.bg_color, 16),
               GetColorf(kParams.bg_color, 24));
  HandleReshape(width, height);
  glClear(GL_COLOR_BUFFER_BIT);
  DrawCheckerBoard();
}

//------------------------------------------------------------------------------
// Main

static void Help(void) {
  printf("Usage: vwebp in_file [options]\n\n"
         "Decodes the WebP image file and visualize it using OpenGL\n"
         "Options are:\n"
         "  -version  .... print version number and exit.\n"
         "  -noicc ....... don't use the icc profile if present.\n"
         "  -nofancy ..... don't use the fancy YUV420 upscaler.\n"
         "  -nofilter .... disable in-loop filtering.\n"
         "  -mt .......... use multi-threading.\n"
         "  -info ........ print info.\n"
         "  -h     ....... this help message.\n"
         "\n"
         "Keyboard shortcuts:\n"
         "  'c' ................ toggle use of color profile.\n"
         "  'i' ................ overlay file information.\n"
         "  'q' / 'Q' / ESC .... quit.\n"
        );
}

int main(int argc, char *argv[]) {
  WebPDecoderConfig config;
  int c;

  if (!WebPInitDecoderConfig(&config)) {
    fprintf(stderr, "Library version mismatch!\n");
    return -1;
  }
  kParams.config = &config;
  kParams.use_color_profile = 1;

  for (c = 1; c < argc; ++c) {
    if (!strcmp(argv[c], "-h") || !strcmp(argv[c], "-help")) {
      Help();
      return 0;
    } else if (!strcmp(argv[c], "-noicc")) {
      kParams.use_color_profile = 0;
    } else if (!strcmp(argv[c], "-nofancy")) {
      config.options.no_fancy_upsampling = 1;
    } else if (!strcmp(argv[c], "-nofilter")) {
      config.options.bypass_filtering = 1;
    } else if (!strcmp(argv[c], "-info")) {
      kParams.print_info = 1;
    } else if (!strcmp(argv[c], "-version")) {
      const int dec_version = WebPGetDecoderVersion();
      const int dmux_version = WebPGetDemuxVersion();
      printf("WebP Decoder version: %d.%d.%d\nWebP Demux version: %d.%d.%d\n",
             (dec_version >> 16) & 0xff, (dec_version >> 8) & 0xff,
             dec_version & 0xff, (dmux_version >> 16) & 0xff,
             (dmux_version >> 8) & 0xff, dmux_version & 0xff);
      return 0;
    } else if (!strcmp(argv[c], "-mt")) {
      config.options.use_threads = 1;
    } else if (argv[c][0] == '-') {
      printf("Unknown option '%s'\n", argv[c]);
      Help();
      return -1;
    } else {
      kParams.file_name = argv[c];
    }
  }

  if (kParams.file_name == NULL) {
    printf("missing input file!!\n");
    Help();
    return 0;
  }

  if (!ExUtilReadFile(kParams.file_name,
                      &kParams.data.bytes, &kParams.data.size)) {
    goto Error;
  }

  kParams.dmux = WebPDemux(&kParams.data);
  if (kParams.dmux == NULL) {
    fprintf(stderr, "Could not create demuxing object!\n");
    goto Error;
  }

  if (WebPDemuxGetI(kParams.dmux, WEBP_FF_FORMAT_FLAGS) & FRAGMENTS_FLAG) {
    fprintf(stderr, "Image fragments are not supported for now!\n");
    goto Error;
  }
  kParams.canvas_width = WebPDemuxGetI(kParams.dmux, WEBP_FF_CANVAS_WIDTH);
  kParams.canvas_height = WebPDemuxGetI(kParams.dmux, WEBP_FF_CANVAS_HEIGHT);
  if (kParams.print_info) {
    printf("Canvas: %d x %d\n", kParams.canvas_width, kParams.canvas_height);
  }

  kParams.prev_frame.width = kParams.canvas_width;
  kParams.prev_frame.height = kParams.canvas_height;
  kParams.prev_frame.x_offset = kParams.prev_frame.y_offset = 0;
  kParams.prev_frame.dispose_method = WEBP_MUX_DISPOSE_BACKGROUND;

  memset(&kParams.iccp, 0, sizeof(kParams.iccp));
  kParams.has_color_profile =
      !!(WebPDemuxGetI(kParams.dmux, WEBP_FF_FORMAT_FLAGS) & ICCP_FLAG);
  if (kParams.has_color_profile) {
#ifdef WEBP_HAVE_QCMS
    if (!WebPDemuxGetChunk(kParams.dmux, "ICCP", 1, &kParams.iccp)) goto Error;
    printf("VP8X: Found color profile\n");
#else
    fprintf(stderr, "Warning: color profile present, but qcms is unavailable!\n"
            "Build libqcms from Mozilla or Chromium and define WEBP_HAVE_QCMS "
            "before building.\n");
#endif
  }

  if (!WebPDemuxGetFrame(kParams.dmux, 1, &kParams.frameiter)) goto Error;

  kParams.has_animation = (kParams.frameiter.num_frames > 1);
  kParams.loop_count = (int)WebPDemuxGetI(kParams.dmux, WEBP_FF_LOOP_COUNT);
  kParams.bg_color = WebPDemuxGetI(kParams.dmux, WEBP_FF_BACKGROUND_COLOR);
  printf("VP8X: Found %d images in file (loop count = %d)\n",
         kParams.frameiter.num_frames, kParams.loop_count);

  // Decode first frame
  if (!Decode()) goto Error;

  // Position iterator to last frame. Next call to HandleDisplay will wrap over.
  // We take this into account by bumping up loop_count.
  WebPDemuxGetFrame(kParams.dmux, 0, &kParams.frameiter);
  if (kParams.loop_count) ++kParams.loop_count;

  // Start display (and timer)
  glutInit(&argc, argv);
#ifdef FREEGLUT
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
#endif
  StartDisplay();

  if (kParams.has_animation) glutTimerFunc(0, decode_callback, 0);
  glutMainLoop();

  // Should only be reached when using FREEGLUT:
  ClearParams();
  return 0;

 Error:
  ClearParams();
  return -1;
}

//------------------------------------------------------------------------------
