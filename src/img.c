#include <pthread.h>
#include "include/img.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "include/fixed_point.h"
#include <turbojpeg.h>
#include <errno.h>
#include <string.h>

unsigned int n_threads = 1;

/* slice image into n horizontal stripes to re-color */
struct t_rgb_to_grey_info {
    image_t *src;
    image_t *dst;
    int     y_start;
    int     height;
};

static void *t_rgb_to_grey(void *arg){
    /* assume that the sizes match and don't check */
    struct t_rgb_to_grey_info *args = (struct t_rgb_to_grey_info*)arg;
    uint8_t *s;

    for(int y = args->y_start; y < args->y_start + args->height; y++){
        for(int x = 0; x < args->dst->width; x++){
            s = PIXEL_AT(args->src, x, y);
            /* convert by average */
            *(args->dst->image + args->dst->width*y + x) = (s[0]+s[1]+s[2])/3;
        }
    }

    return NULL;
}

int rgb_to_grey(image_t *src, image_t *dst){

    pthread_t *tid;
    struct t_rgb_to_grey_info *targs;
    int px_per_thread = 0;

    if(src->height != dst->height || src->width  != dst->width){
        fprintf(stderr, "rgb_to_grey: sizes don't match\n");
        return 1;
    }

    if(dst->depth != 1){
        fprintf(stderr, "rgb_to_grey: dst has wrong depth\n");
        return 1;
    }

    tid = (pthread_t*)malloc(sizeof(pthread_t) * n_threads);
    targs = (struct t_rgb_to_grey_info*)malloc(
        sizeof(struct t_rgb_to_grey_info) * n_threads);

    px_per_thread = dst->height / n_threads;

    /* create threads */
    for(int i = 0; i < n_threads; i++){
        targs[i].src = src;
        targs[i].dst = dst;
        targs[i].height = px_per_thread;
        targs[i].y_start = px_per_thread * i;
        pthread_create(tid + i, NULL, t_rgb_to_grey, (void*)(targs + i));
    }

    /* join all children */
    for (int i = 0; i < n_threads; i++){
        pthread_join(tid[i], NULL);
    }

    free(tid);
    free(targs);

    return 0;

}

struct t_resize_image_info {
    image_t *src;
    image_t *dst;
    int     y_start;
    int     height;
};

void *t_resize_image(void *arg){
    struct t_resize_image_info *args = (struct t_resize_image_info*)arg;

    int x = 0;
    int y = 0;

    int x_ratio = INT_TO_FIXED(args->src->width) / args->dst->width;
    int y_ratio = INT_TO_FIXED(args->src->height) / args->dst->height;

    int Px = 0;
    int Py = 0;

    int y1 = 0;
    int y2 = 0;
    int x1 = 0;
    int x2 = 0;

    int new_x = 0;
    int new_y = 0;

    for(y = args->y_start; y < args->y_start + args->height; y++){
        for(x = 0; x < args->dst->width; x++){
            Px = MULTIPLY_FIXED(INT_TO_FIXED(x), x_ratio);
            Py = MULTIPLY_FIXED(INT_TO_FIXED(y), y_ratio);
            y1 = CEIL(Py);
            y2 = FLOOR(Py);
            x1 = CEIL(Px);
            x2 = FLOOR(Px);

            if(x1 - Px < -1 * (x2 - Px)){
                new_x = FIXED_TO_INT(x1);
            }
            else{
                new_x = FIXED_TO_INT(x2);
            }

            if(y1 - Py < -1 * (y2 - Py)){
                new_y = FIXED_TO_INT(y1);
            }
            else{
                new_y = FIXED_TO_INT(y2);
            }

            *PIXEL_AT(args->dst, x, y) = *PIXEL_AT(args->src, new_x, new_y);
        }
    }

    return NULL;

}

int resize_image(image_t* src, image_t* dst){

    /* use nearest neighbour sampling for speed */
    pthread_t *tid;
    struct t_resize_image_info *targs;
    int px_per_thread = 0;

    tid = (pthread_t*)malloc(sizeof(pthread_t) * n_threads);
    targs = (struct t_resize_image_info*)malloc(
        sizeof(struct t_resize_image_info) * n_threads);

    px_per_thread = dst->height / n_threads;

    /* create threads */
    for(int i = 0; i < n_threads; i++){
        targs[i].src = src;
        targs[i].dst = dst;
        targs[i].height = px_per_thread;
        targs[i].y_start = px_per_thread * i;
        pthread_create(tid + i, NULL, t_resize_image, (void*)(targs + i));
    }
    for(int i = 0; i < n_threads; i++){
        pthread_join(tid[i], NULL);
    }

    free(tid);
    free(targs);

    return 0;

}

void decompress_jpeg(uint8_t *compressed_image, unsigned int jpeg_size,
    image_t *dst){

    int jpegSubsamp, width = 1000, height = 1000;
    int ret = 0;
    uint8_t buffer[1000*1000*3];

    tjhandle jpeg_decompressor = tjInitDecompress();

    if( NULL == jpeg_decompressor ){
        fprintf(
            stderr,
            "jpeg error: failed to create decompressor\n"
        );
        exit(EXIT_FAILURE);
    }

    ret = tjDecompressHeader2(
        jpeg_decompressor,
        compressed_image,
        jpeg_size,
        &width,
        &height,
        &jpegSubsamp
    );

    if( -1 == ret ){
        fprintf(stderr, "jpeg error: %s\n", tjGetErrorStr2(jpeg_decompressor));
        exit(EXIT_FAILURE);
    }

    ret = tjDecompress2(
        jpeg_decompressor,
        compressed_image,
        jpeg_size,
        buffer,
        width,
        0,
        height,
        TJPF_RGB,
        TJFLAG_FASTDCT
    );

    if( -1 == ret ){
        fprintf(stderr, "jpeg error: %s\n", tjGetErrorStr2(jpeg_decompressor));
        exit(EXIT_FAILURE);
    }

    dst->image = (uint8_t*)malloc(width*height*3);
    dst->height = height;
    dst->width = width;
    dst->depth = 3;

    memcpy(dst->image, buffer, dst->height * dst->width * dst->depth);

    tjDestroy(jpeg_decompressor);

}
