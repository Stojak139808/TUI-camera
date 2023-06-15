#ifndef DISP_H
#define DISP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

void init_window(void);
void uninit_window(void);
void get_window_xy(uint32_t *x, uint32_t *y);
void display_frame(uint8_t *frame, size_t n);

#ifdef __cplusplus
}
#endif

#endif
