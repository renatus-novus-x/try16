#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

#ifdef MSX

#define CLOCKS_PER_SEC (60)
#define JIFFY          (0xfc9e)

void platform_init(){
  uint8_t* ptr = (uint8_t*)JIFFY;
  ptr[0] = 0;
  ptr[1] = 0;
}

clock_t platform_clock() {
  uint8_t* ptr = (uint8_t*)JIFFY;
  return (clock_t)(ptr[0] | (ptr[1] << 8));
}

uint32_t platform_seconds_to_ticks(uint32_t sec){
  return sec * (uint32_t)CLOCKS_PER_SEC;
}

uint16_t platform_elapsed_ticks(clock_t start, clock_t end){
  return (uint16_t)((uint16_t)end - (uint16_t)start); /* 16bit wrap */
}

float platform_elapsed_sec(clock_t start, clock_t end){
  return (float)platform_elapsed_ticks(start, end) / (float)CLOCKS_PER_SEC;
}

static inline void platform_wait_next_tick(clock_t* now){
  clock_t base = *now;
  while (platform_clock() == base) { /* busy-wait 1/60 */ }
  *now = base + 1;
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
  calbio(0x00d8, &r); /* GTTRIG */
  return (r.a != 0);
}

#else  /* ---------------- Non MSX ---------------- */

#include <time.h>

#if X68K

#include <x68k/iocs.h>

#undef  CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 100

static inline uint32_t trap_ontime_cs(void){
  uint32_t cs;
  __asm__ volatile(
    "moveq  #0x7F,%%d0 \n\t"  /* _ONTIME */
    "trap   #15        \n\t"  /* IOCS    */
    "move.l %%d0,%0    \n\t"
    : "=d"(cs)
    :
    : "d0","d1","a0","cc","memory"
  );
  return cs;
}

static inline void platform_init(void){
  // _iocs_vdispst()
}

static inline clock_t platform_clock(void){
  return (clock_t)trap_ontime_cs();
}

static inline uint32_t platform_seconds_to_ticks(uint32_t sec){
  return sec * (uint32_t)CLOCKS_PER_SEC;
}

static inline uint32_t platform_elapsed_ticks(clock_t start, clock_t end){
  return (uint32_t)((uint32_t)end - (uint32_t)start);
}

static inline float platform_elapsed_sec(clock_t start, clock_t end){
  return (float)platform_elapsed_ticks(start, end) / (float)CLOCKS_PER_SEC;
}

static inline void platform_wait_next_tick(clock_t* now){
  clock_t base = *now;
  while ((uint32_t)(trap_ontime_cs() - (uint32_t)base) == 0) { /* busy-wait 1/100 */ }
  *now = base + 1;
}

static inline unsigned char platform_gttrig(int no){
  if (no == 0) { // keyboard
    unsigned char g6 = _iocs_bitsns(6); // SPACE: group6 bit5
    return (g6 >> 5) & 1;
  } else {       // joystick
    no = no -1;
    return !(_iocs_joyget(no) & 0x20); // A-button
  }
}

#else
/* ---------------- ÅistubÅj ---------------- */
#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 60
#endif

static inline void     platform_init(void){}
static inline clock_t  platform_clock(void){ return (clock_t)clock(); }
static inline uint32_t platform_seconds_to_ticks(uint32_t sec){ return sec * (uint32_t)CLOCKS_PER_SEC; }
static inline uint32_t platform_elapsed_ticks(clock_t start, clock_t end){ return (uint32_t)((uint32_t)end - (uint32_t)start); }
static inline float    platform_elapsed_sec(clock_t start, clock_t end){ return (float)platform_elapsed_ticks(start,end) / (float)CLOCKS_PER_SEC; }
static inline void     platform_wait_next_tick(clock_t* now){ clock_t b=*now; while (platform_clock()==b){} *now=b+1; }
static inline unsigned char platform_gttrig(int no){ (void)no; return 0; }

#endif /* X68K */
#endif /* MSX */

#endif /* PLATFORM_H */
