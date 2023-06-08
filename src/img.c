/* image processing routines */

#include <pthread.h>
#include "include/img.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
