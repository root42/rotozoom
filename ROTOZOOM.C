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
int16_t SIN512[512];
int16_t SINBIG[512];
int16_t SINPOS[512];
byte pal[768];
struct image *img = NULL;

#define SETPIX(x,y,c) *(framebuf + (dword)SCREEN_WIDTH * (y) + (x)) = c
#define GETPIX(x,y) *(framebuf + (dword)SCREEN_WIDTH * (y) + (x))
#define GETIMG(x,y) *(img->data + (dword)img->width * ((y) % img->height) + ((x) % img->width))

#define FAST

#ifndef FAST
#define USE_FLOAT
#else
#define MODE_Y
#endif

void init_sin()
{
  word i;
  float v;
  for( i = 0; i < 512; ++i ) {
    v = sin( 2.0 * M_PI * i / 511.0 );
    SIN512[ i ] = (int16_t)(255.0 * v);
    SINBIG[ i ] = (int16_t)(65535.0 * v);
    SINPOS[ i ] = (int16_t)(255.0 * 4 * (v + 1.0));
  }
}

void draw_roto(word x, word y, word w, word h, dword t)
{
  byte col;
  word i,j;
  int16_t u,v;
#ifdef USE_FLOAT
  const float angle = M_PI * ( t / 180.0 );
  const float c = cos( angle );
  const float s = sin( angle );
  const float k = 4 * (s + 1);
  const float tx = 64*s;
  const float ty = 64*c;
  float js,jc;
  for( j = y; j < y + h; ++j ) {
    js = j*s;
    jc = j*c;
    for( i = x; i < x + w; ++i ) {
      u = ((int16_t)((i * c - js) * k + tx)) % (int16_t)img->width;
      v = ((int16_t)((i * s + jc) * k + ty)) % (int16_t)img->height;
      col = GETIMG(u,v);
      SETPIX(i,j,col);
    }
  }
#else
  const int16_t c = SIN512[(t + 128) % 512];
  const int16_t s = SIN512[t % 512];
  const int16_t c2 = SINBIG[(t + 128) % 512];
  const int16_t s2 = SINBIG[t % 512];
  const int16_t s3 = SINPOS[t % 512];
  int16_t js,jc;
  int16_t icjs, isjc;
  byte far *pix;

  for( j = y, pix = framebuf + y * w + x;
       j < y + h;
       ++j, pix += SCREEN_WIDTH - w)
  {
    js = j*s;
    jc = j*c;
    for( i = x; i < x + w; ++i, ++pix ) {
      icjs = i * c - js;
      isjc = i * s + jc;
      u = (((icjs >> 8) * s3 + s2) >> 8) % (int16_t)img->width;
      v = (((isjc >> 8) * s3 + c2) >> 8) % (int16_t)img->height;
      *pix = GETIMG(u,v);
    }
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
    draw_roto(80,50,160,100,t);
    wait_for_retrace();
    memcpy(VGA,framebuf,SCREEN_WIDTH*SCREEN_HEIGHT);
#endif
    t++;
  }
  set_text_mode();
  return 0;
}
