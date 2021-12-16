#include "mailbox.h"
#include "fb.h"

// This prevents the GPU and CPU from caching mailbox messages
#define GPU_NOCACHE 0x40000000

typedef struct {
  unsigned int width;       // width of the display
  unsigned int height;      // height of the display
  unsigned int virtual_width;  // width of the virtual framebuffer
  unsigned int virtual_height; // height of the virtual framebuffer
  unsigned int pitch;       // number of bytes per row
  unsigned int depth;       // number of bits per pixel
  unsigned int x_offset;    // x of the upper left corner of the virtual fb
  unsigned int y_offset;    // y of the upper left corner of the virtual fb
  unsigned int framebuffer; // pointer to the start of the framebuffer
  unsigned int size;        // number of bytes in the framebuffer
} fb_config_t;

// fb is volatile because the GPU will write to it
static volatile fb_config_t fb __attribute__ ((aligned(16)));
static int fbmode;

void fb_init(unsigned int width, unsigned int height, unsigned int depth, unsigned int mode)
{
  fb.width = width;
  fb.virtual_width = width;
  fb.height = height;
  fbmode = mode;

  //double the height if in double buffer mode
  if(mode == FB_SINGLEBUFFER){
    fb.virtual_height = height;
  }
  else{
    fb.virtual_height = 2 * height;
  }
  fb.depth = depth * 8; // convert number of bytes to number of bits
  fb.x_offset = 0;
  fb.y_offset = 0;

  // the manual requires we to set these value to 0
  // the GPU will return new values
  fb.pitch = 0;
  fb.framebuffer = 0;
  fb.size = 0;

  mailbox_write(MAILBOX_FRAMEBUFFER, (unsigned)&fb + GPU_NOCACHE);
  (void) mailbox_read(MAILBOX_FRAMEBUFFER);
}

enum display_screen {
  TOP = 0, BOTTOM=1
};

static int display = TOP;

void fb_swap_buffer(void)
{
  if(fbmode == FB_DOUBLEBUFFER){
    if(display == TOP){
      display = BOTTOM;
      fb.y_offset = fb.height;
    }
    else{
      display = TOP;
      fb.y_offset = 0;
    }

    mailbox_write(MAILBOX_FRAMEBUFFER, (unsigned)&fb + GPU_NOCACHE);
    (void) mailbox_read(MAILBOX_FRAMEBUFFER);
  }
}

unsigned char* fb_get_draw_buffer(void)
{
    if(display == BOTTOM || fbmode == FB_SINGLEBUFFER){
      return (unsigned char * )fb.framebuffer;
    }
    return (unsigned char *)fb.framebuffer + fb.height * fb.pitch ;
}

unsigned int fb_get_width(void)
{
    return fb.width;
}

unsigned int fb_get_height(void)
{
    return fb.height;
}

unsigned int fb_get_depth(void)
{
    return fb.depth;
}

unsigned int fb_get_pitch(void)
{
    return fb.pitch;
}
