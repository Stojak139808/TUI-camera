/* 
 * based on V4L2 example from the linux kernel documentation
 * https://www.kernel.org/doc/html/v4.9/media/uapi/v4l/capture.c.html
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <getopt.h>
#include <ncurses.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include "include/disp.h"
#include "include/img.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct buffer {
    void   *start;
    size_t  length;
};

static char             *dev_name;
static int              fd = -1;
struct buffer           *buffers;
static unsigned int     n_buffers;

static image_t          decompressed_image;
static image_t          gray_buffer;
static image_t          resized_buffer;

static int xioctl(int fh, int request, void *arg);
static void errno_exit(const char *s);

static void init_image_processing(){

    int camera_y, camera_x; /* camera dimensions */
    unsigned int terminal_y, terminal_x; /* camera dimensions */
    struct v4l2_format fmt;

    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt)){
        errno_exit("VIDIOC_G_FMT");
    }

    camera_y = fmt.fmt.pix.height;
    camera_x = fmt.fmt.pix.width;

    gray_buffer.height = camera_y;
    gray_buffer.width = camera_x;
    gray_buffer.depth = 1;
    gray_buffer.image = (uint8_t*)malloc(camera_y * camera_x);

    get_window_xy(&terminal_x, &terminal_y);

    resized_buffer.height = terminal_y;
    resized_buffer.width = terminal_x;
    resized_buffer.depth = 1;
    resized_buffer.image = (uint8_t*)malloc(terminal_y * terminal_x);

}

static void uninit_image_processing(){
    free(gray_buffer.image);
    free(resized_buffer.image);
}

static void errno_exit(const char *s){
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg){

    int r;

    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

static void process_image(void *p, int size){

    decompress_jpeg(p, size, &decompressed_image);

    rgb_to_grey(&decompressed_image, &gray_buffer);
    resize_image(&gray_buffer, &resized_buffer);

    display_frame(
        resized_buffer.image,
        resized_buffer.width * resized_buffer.height,
        resized_buffer.width
    );

    free(decompressed_image.image);
}

static int read_frame(void){

    struct v4l2_buffer buf;

    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
        case EAGAIN:
            return 0;
        case EIO:
            /* Could ignore EIO, see spec. */
            /* fall through */
        default:
            errno_exit("VIDIOC_DQBUF");
        }
    }

    assert(buf.index < n_buffers);

    process_image(buffers[buf.index].start, buf.bytesused);

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)){
        errno_exit("VIDIOC_QBUF");
    }

    return 1;
}

static void mainloop(void){

    fd_set fds;
    struct timeval tv;
    int r;

    for (;;) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        /* Timeout. */
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        r = select(fd + 1, &fds, NULL, NULL, &tv);

        if (-1 == r) {
            if (EINTR == errno)
                continue;
            errno_exit("select");
        }

        if (0 == r) {
            fprintf(stderr, "select timeout\n");
            exit(EXIT_FAILURE);
        }

        read_frame();

        switch (wgetch(stdscr)){
        case 27: /* esc */
            break;
        default:
            /* do nothing, repeat loop */
            continue;
        }
        break;
    }
}

static void stop_capturing(void){

    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type)){
        errno_exit("VIDIOC_STREAMOFF");
    }
}

static void start_capturing(void){

    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
        errno_exit("VIDIOC_QBUF");
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type)){
        errno_exit("VIDIOC_STREAMON");
    }
}

static void uninit_device(void){

    unsigned int i;

    for (i = 0; i < n_buffers; ++i){
        if (-1 == munmap(buffers[i].start, buffers[i].length)){
            errno_exit("munmap");
        }
    }
    free(buffers);
}

static void init_mmap(void){

    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support "
            "memory mappingn", dev_name);
            exit(EXIT_FAILURE);
        } 
        else {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) {
        fprintf(
            stderr, 
            "Insufficient buffer memory on %s\n",
            dev_name
        );
        exit(EXIT_FAILURE);
    }

    buffers = calloc(req.count, sizeof(*buffers));

    if (!buffers) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = n_buffers;

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf)){
            errno_exit("VIDIOC_QUERYBUF");
        }

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start =
        mmap(NULL /* start anywhere */,
              buf.length,
            PROT_READ | PROT_WRITE /* required */,
            MAP_SHARED /* recommended */,
            fd, buf.m.offset\
        );

        if (MAP_FAILED == buffers[n_buffers].start)
            errno_exit("mmap");
        }
}

static void init_device(void){

    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\n",
                 dev_name);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is no video capture device\n",
             dev_name);
        exit(EXIT_FAILURE);
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "%s does not support streaming i/o\n",
             dev_name);
        exit(EXIT_FAILURE);
    }

    /* Select video input, video standard and tune here. */

    CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
            case EINVAL:
                /* Cropping not supported. */
                break;
            default:
                /* Errors ignored. */
                break;
            }
        }
    } else {
        /* Errors ignored. */
    }

    /* set RGB24 format */
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt)){
        errno_exit("VIDIOC_G_FMT");
    }

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min){
        fmt.fmt.pix.bytesperline = min;
    }

    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min){
        fmt.fmt.pix.sizeimage = min;
    }

    init_mmap();
}

static void close_device(void){

    if (-1 == close(fd))
        errno_exit("close");

    fd = -1;
}

static void open_device(void){

    struct stat st;

    if (-1 == stat(dev_name, &st)) {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n",
             dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!S_ISCHR(st.st_mode)) {
        fprintf(stderr, "%s is no devicen", dev_name);
        exit(EXIT_FAILURE);
    }

    fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (-1 == fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n",
             dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static void usage(FILE *fp, int argc, char **argv){

    fprintf(fp,
        "Usage: %s [options]\n\n"
        "Options:\n"
        "-d | --device name    Video device name [%s]\n"
        "-j | --threads n      number of threads to use for image processing\n"
        "-h | --help           Print this message\n"
        "",
        argv[0],
        dev_name
    );
}

static const char short_options[] = "d:j:h";

static const struct option long_options[] = {
    { "device",     required_argument, NULL, 'd' },
    { "threads",    required_argument, NULL, 'j' },
    { "help",       no_argument,       NULL, 'h' },
    { 0,            0,                 0,     0  }
};

int main(int argc, char **argv){

    dev_name = "/dev/video0";
    int i = 0;

    for (;;) {
        int idx;
        int c;

        c = getopt_long(argc, argv,
                short_options, long_options, &idx);

        if (-1 == c)
            break;

        switch (c) {
        case 0: /* getopt_long() flag */
            break;

        case 'd':
            dev_name = optarg;
            break;

        case 'j':
            i = 0;
            while(optarg[i]){
                if(!isdigit(optarg[i])){
                    usage(stdout, argc, argv);
                    exit(EXIT_SUCCESS);
                }
                i++;
            }
            set_thread_n(atoi(optarg));
            break;

        case 'h':
            usage(stdout, argc, argv);
            exit(EXIT_SUCCESS);

        default:
            usage(stderr, argc, argv);
            exit(EXIT_FAILURE);
        }
    }

    struct call_functions{ void (*f)(void) } call_queue[] = {
        open_device,
        init_device,
        start_capturing,
        init_window,
        init_image_processing,
        mainloop,
        uninit_window,
        uninit_image_processing,
        stop_capturing,
        uninit_device,
        close_device
    };

    for(int i = 0; i < sizeof(call_queue)/sizeof(struct call_functions); i++){
        call_queue[i].f();
    }

    fprintf(stderr, "\n");

    return 0;

}
