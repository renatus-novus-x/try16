#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdint.h>
#include "platform.h"

void edge_count(unsigned char cur, unsigned char* prev, unsigned int* cnt){
  if (cur && !(*prev)) (*cnt)++;
  *prev = cur;
}

int main(int argc, char* argv[]) {
  platform_init();
  printf("Try16: A retro-style rapid fire counter.\n");
  printf("Strike SPACE key or Trigger-A button to start\n");
  int trigger_source = -1;   /* select trigger source */
  while (trigger_source < 0) {
    if (platform_gttrig(0)) trigger_source = 0;      // SPACE
    else if (platform_gttrig(1)) trigger_source = 1; // Joy Port-1
    else if (platform_gttrig(2)) trigger_source = 2; // Joy Port-2
  }
  const uint32_t total_ticks = platform_seconds_to_ticks(10); /* 10 sec (CLOCKS_PER_SEC)*/
  const uint32_t one_sec     = platform_seconds_to_ticks(1);
  clock_t start = platform_clock();
  clock_t now   = start;
  clock_t last  = start;
  unsigned int   count = 0;
  unsigned char  prev  = 0;
  while (platform_elapsed_ticks(start, now) < total_ticks) { // (MSX:60Hz, X68K:100Hz)
    unsigned char trig = platform_gttrig(trigger_source);
    edge_count(trig, &prev, &count);
    if (platform_elapsed_ticks(last, now) >= one_sec) {      // 1 sec
      last = now;
      uint32_t elapsed_ticks = platform_elapsed_ticks(start, now);
      uint32_t elapsed_sec   = elapsed_ticks / CLOCKS_PER_SEC;
      printf("\r%2u.0 sec %3u shots     ", (unsigned)elapsed_sec, count);
      fflush(stdout);
    }
    platform_wait_next_tick(&now); // MSX:16.7ms / X68K:10ms
  }
  printf("\r10.0 sec %3u shots     \n", count);
  printf("Avg %u.%01u shots/sec\n", count / 10, count % 10);
  printf("Done.\n");
  return 0;
}
