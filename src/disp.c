#include "include/disp.h"
#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>

struct screen{
    WINDOW* window;
    uint32_t max_x;
    uint32_t max_y;
};

struct screen main_window;

static const char palette[] = {' ', '.', ':', '-', '=', '+', '*', '#', '%', '@'};
//static char palette[] = {' ', '.', '_', '+', '&', '#'};
//static char palette[] = {'$', '@', 'B', '%', '8', '&', 'W', 'M', '#', '*', 'o',
//    'a', 'h', 'k', 'b', 'd', 'p', 'q', 'w', 'm', 'Z', 'O', '0', 'Q', 'L', 'C',
//    'J', 'U', 'Y', 'X', 'z', 'c', 'v', 'u', 'n', 'x', 'r', 'j', 'f', 't', '/',
//    '\\', '|', '(', ')', '1', '{', '}', '[', ']', '?', '-', '_', '+', '~', '<',
//    '>', 'i', '!', 'l', 'I', ';', ':', ',', '\"', '^', '`', '\'', '.', ' '};

static const uint8_t intervals = sizeof(palette)/sizeof(char) - 1;

void init_window(){
    /* should return stdscr */
    main_window.window = initscr();
    getmaxyx(
        main_window.window,
        main_window.max_y,
        main_window.max_x
    );

    if (!main_window.max_x || !main_window.max_y) {
        fprintf(stderr, "At least one terminal dimension is equal to 0\n");
        exit(EXIT_FAILURE);
    }

    raw();
    //noecho();
    nodelay(main_window.window, 1);
}

void uninit_window(){
    endwin();
}

void get_window_xy(uint32_t *x, uint32_t *y){
    *x = main_window.max_x;
    *y = main_window.max_y;
}

void display_frame(uint8_t *frame, size_t n, size_t line_width){
    clear();
    for (int y = 0; y < n; y += line_width) {
        for (int x = y + line_width - 1; x >= y; x--){
            addch(palette[((frame[x])*intervals)/0xffu]);
        }
    }
    refresh();
}
