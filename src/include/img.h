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


int rgb_to_grey(image_t *src, image_t *dst);
int resize_image(image_t* src, image_t* dst);
void decompress_jpeg(
    uint8_t *compressed_image,
    unsigned int jpeg_size,
    image_t *dst
);

void set_thread_n(int n);
int get_thread_n(void);

#endif
