#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

#ifdef MSX

#define CLOCKS_PER_SEC (60)
#define JIFFY          (0xfc9e)

clock_t platform_clock(void) {
  uint8_t* ptr = (uint8_t*)JIFFY;
  return (clock_t)(ptr[0] | (ptr[1] << 8));
}

uint16_t platform_elapsed_ticks(clock_t start, clock_t end) {
  return (uint16_t)((end - start) & 0xFFFF);
}

float platform_elapsed_sec(clock_t start, clock_t end) {
  return platform_elapsed_ticks(start, end) / (float)CLOCKS_PER_SEC;
}

struct regs {
  char     f;
  char     a;
  unsigned bc;
  unsigned de;
  unsigned hl;
};

void calbio(unsigned int address, struct regs* r) {
#asm
CALBIO_CALSLT	equ	001ch
CALBIO_EXPTBL	equ	0fcc1h

    ld	hl,2
	add	hl,sp
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	hl
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	ld  l,c
	ld	h,b

CALBIO01:
	push	hl
	pop	ix
	push	de
	ex	de,hl
	ld	iy,0
	add	iy,sp
	di
	ld	sp,hl
	pop	af
	pop	bc
	pop	de
	pop	hl
	ei
	ld	sp,iy
	push	iy
	ld	iy,(CALBIO_EXPTBL-1)
	call	CALBIO_CALSLT
	pop	iy
	exx
	pop	hl
	ld	bc,8
	add	hl,bc
	di
	ld	sp,hl
	exx
	push	hl
	push	de
	push	bc
	push	af
	ei
	ld	sp,iy
	pop	af
#endasm
}

unsigned char platform_gttrig(int no) {
  struct regs r;
  r.a = (char)no;
  calbio(0x00d8, &r);
  return (r.a != 0);
}

#else // Non-MSX

#include <time.h>
#if X68K

#include <x68k/iocs.h>

#undef  CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 100

static inline uint32_t trap_ontime(void) {
  uint32_t cs;
  __asm__ volatile(
    "moveq  #0x7F,%%d0 \n\t"   // _ONTIME
    "trap   #15        \n\t"   // IOCS
    "move.l %%d0,%0    \n\t"
    : "=d"(cs)
    :
    : "d0","d1","a0","cc","memory"
  );
  return cs;
}

clock_t clock(void) {
  static uint32_t t0 = 0;
  uint32_t now = trap_ontime();
  if (t0 == 0) {
    t0 = now; // initial time
    return 0;
  }
  uint32_t diff_cs = now - t0;
  return (clock_t)diff_cs;    // CLOCK
}
#endif

clock_t platform_clock(void) {
  return clock();
}

float platform_elapsed_sec(clock_t start, clock_t end) {
  float sec = (float)(end - start) / CLOCKS_PER_SEC;
  return (sec <= 0.0f) ? 1e-6f : sec;
}

uint16_t platform_elapsed_ticks(clock_t start, clock_t end) {
  return (uint16_t)(platform_elapsed_sec(start, end) * 60.0f);
}

static inline unsigned char platform_gttrig(int no) {
#if X68K
  if (no == 0) { // keyboard
    static unsigned char prev = 0;
    unsigned char g6 = _iocs_bitsns(6);
    unsigned char cur = (g6 >> 5) & 1;
    unsigned char edge = (cur && !prev);
    prev = cur;
    return edge;
  } else {       // joystick
    no = no -1;
    return !(_iocs_joyget(no) & 0x20);
  }
#else
  return 0; // dummy trigger for non MSX/X68k
#endif
}

#endif

#endif // PLATFORM_H
