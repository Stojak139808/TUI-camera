#include<opencv4/opencv2/opencv.hpp>
#include<iostream>
#include "include/disp.h"
#include <stddef.h>
#include <string.h>
#include <opencv2/core/utils/logger.hpp>

#include <unistd.h>
#include <ncurses.h>

using namespace std;
using namespace cv;

static uint32_t max_x = 0;
static uint32_t max_y = 0;

int main() {
    uint8_t *frame;
    size_t frame_size;
    Mat captured_frame;
    Mat resized_frame;
    VideoCapture cap(0);
    int c = 0;

    utils::logging::setLogLevel(utils::logging::LOG_LEVEL_SILENT);

    init_window();

    get_window_xy(&max_x, &max_y);
    frame_size = (max_x + 1)*(max_y + 1);

    if (!cap.isOpened()){
        cout << "No video stream detected\n";
        system("pause");
        return -1;
    }

    for(;;){
        /* grab a frame */
        cap.read(captured_frame);
        if (captured_frame.empty()){
            break;
        }

        /* Prepare the frame */
        resize(captured_frame, resized_frame, Size(max_x, max_y));
        cvtColor(resized_frame, resized_frame, COLOR_RGB2GRAY);
        flip(resized_frame, resized_frame, 1);

        /* show the frame */
        frame = (uint8_t*)resized_frame.data;
        //imshow("camera", resized_frame);
        display_frame(frame, frame_size);

        /* exit when ESC was pressed */
        c = wgetch(stdscr);
        if (c == 27){
            break;
        }
    }

    cap.release();
    uninit_window();
    putchar('\n');
    return 0;
}
