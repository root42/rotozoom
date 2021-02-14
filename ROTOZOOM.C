#include <stdio.h>
#include <stdlib.h>
#include <alloc.h>
#include <conio.h>
#include <dos.h>
#include <math.h>
#include <mem.h>

#include "gif.h"
#include "types.h"
#include "vga.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

byte far *framebuf;
int16_t SIN256[256];
int16_t SIN512[512];
int16_t SINZOOM[256];
byte pal[768];
struct image *img = NULL;

#define SETPIX(x,y,c) *(framebuf + (dword)SCREEN_WIDTH * (y) + (x)) = c
#define GETPIX(x,y) *(framebuf + (dword)SCREEN_WIDTH * (y) + (x))
#define GETIMG(x,y) *(img->data + (dword)img->width * ((y) % img->height) + ((x) % img->width))

#define NO_FLOAT
#define NO_MODE_Y

void init_sin()
{
  word i;
  float v;
  for( i = 0; i < 256; ++i ) {
    v = sin( 2.0 * M_PI * i / 255.0 );
    SIN256[ i ] = (int16_t)(127.0 * v);
    SINZOOM[ i ] = (int16_t)(127.0 * 2 * (v + 1.2));
  }
  for( i = 0; i < 512; ++i ) {
    v = sin( 2.0 * M_PI * i / 511.0 );
    SIN512[ i ] = (int16_t)(63.0 * v);
  }
}

void draw_roto(
  const word x,
  const word y,
  const word w,
  const word h,
  const dword t)
{
  word i,j;
  int16_t u,v;
  byte col;
  byte far *pix;
#ifdef USE_FLOAT
  const float angle = M_PI * ( t / 180.0 );
  const float c = cos( angle );
  const float s = sin( angle );
  const float z = (s + 1.5);
  const float tx = 64 * (s + 1);
  const float ty = 64 * (c + 1);
  float js, jc;
  float icjs, isjc;

  js = (y+ty)*s;
  jc = (y+ty)*c;
  for( j = 0, pix = framebuf + y * w + x;
       j < h;
       ++j, pix += SCREEN_WIDTH - w
  ) {
    icjs = (x+tx)*c-js;
    isjc = (x+tx)*s+jc;
    for( i = 0; i < w; ++i ) {
      u = ((int16_t)(icjs * z)) % (int16_t)img->width;
      v = ((int16_t)(isjc * z)) % (int16_t)img->height;
      col = GETIMG(u,v);
      *pix++ = col;
#ifndef MODE_Y
      *pix++ = col;
      ++i;
#endif
      icjs += c;
      isjc += s;
    }
    js += s;
    jc += c;
  }
#else
  const int16_t c = SIN256[(t+64) % 256];
  const int16_t s = SIN256[t % 256];
  const int16_t z = SINZOOM[t % 256];
  const int16_t tx = SIN512[t % 512];
  const int16_t ty = SIN512[(t + 128) % 512];
  int16_t js, jc;
  int16_t icjs, isjc;

  js = (y+ty)*s;
  jc = (y+ty)*c;
  for( j = 0, pix = framebuf + y * w + x;
       j < h;
       ++j, pix += SCREEN_WIDTH - w )
  {
    icjs = (x+tx)*c-js;
    isjc = (x+tx)*s+jc;
    for( i = 0; i < w; ++i ) {
      u = (((icjs >> 7) * z) >> 7) & 0x7f;
      v = (((isjc >> 7) * z) >> 7) & 0x7f;
      col = GETIMG(u,v);
      *pix++ = col;
#ifndef MODE_Y
      *pix++ = col;
      ++i;
#endif
      icjs += c;
      isjc += s;
    }
    js += s;
    jc += c;
  }
#endif
}

int main()
{
  byte do_quit = 0;
  int i;
  dword t = 36;

  init_sin();

  framebuf = malloc(SCREEN_WIDTH*SCREEN_HEIGHT);
  if(framebuf == NULL) {
    printf("not enough memory\n");
    return 1;
  }
  img = load_gif("roto.gif", 0);
  if(img == NULL ) {
    printf("couldn't load image\n");
    return 1;
  }

#ifdef MODE_Y
  set_mode_y();
#else
  set_graphics_mode();
#endif
  set_palette(&img->palette[0][0]);
  memset(framebuf, 0x00, SCREEN_WIDTH*SCREEN_HEIGHT);
  while( !do_quit ) {
    if(kbhit()){
      getch();
      do_quit = 1;
    }
#ifdef MODE_Y
    draw_roto(0,0,80,50,t);
    wait_for_retrace();
    blit4(framebuf,0,0,80,100);
#else
    draw_roto(0,0,320,100,t);
    wait_for_retrace();
    for( i = 0; i < 100; i++ ) {
      memcpy(VGA + 2*i      *SCREEN_WIDTH, framebuf + i * SCREEN_WIDTH, SCREEN_WIDTH);
      memcpy(VGA + (2*i + 1)*SCREEN_WIDTH, framebuf + i * SCREEN_WIDTH, SCREEN_WIDTH);
    }
#endif
    t++;
  }
  set_text_mode();
  return 0;
}
