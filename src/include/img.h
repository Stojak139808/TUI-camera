#ifndef IMG_H
#define IMG_H

#include <stdint.h>

typedef struct{
    uint8_t *image;
    int width;
    int height;
    int depth;
}image_t;

/* macro for calculating the image array address at the given place */
#define PIXEL_AT(img, x, y)  (img->image + \
                                img->width * img->depth * y +\
                                img->depth * x)

/* global variable with number of threads to be used */
extern unsigned int n_threads;

int rgb_to_grey(image_t *src, image_t *dst);

#endif