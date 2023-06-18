#ifndef DISP_H
#define DISP_H

#include <stdint.h>
#include <stddef.h>

void init_window(void);
void uninit_window(void);
void get_window_xy(uint32_t *x, uint32_t *y);
void display_frame(uint8_t *frame, size_t n, size_t line_width);

#endif
