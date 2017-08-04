
//#include <curses.h>//graph lib
//#include <unistd.h>//usleep
//#include <pthread.h>// mul process
#include "stdlib.h"        // standard library
#include "time.h"
#include "stdio.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_err.h"
#include "driver/i2c.h"

#define HEAD_LEFT     1
#define HEAD_DOWN     2
#define HEAD_UP       3
#define HEAD_RIGHT    4
#define MODE_AUTO   (1)

extern void srand (unsigned int seed);
extern int random (void);

typedef struct
{
    int x, y;
} curPoint;

typedef struct
{
    curPoint body[200];
    int len, arrowHead, score, dead;
} MainSnack;

MainSnack Ms;

#define MAX_COLS     (8)
#define MAX_LINES    (12)
#define HEAD_DUTY    (0xff/10)
#define SNAKE_DUTY   (0xff/10)
#define FOOD_DUTY    (0xff/10)

void initGame();//Initial the variable for the game
extern void fill_pixel(uint8_t x, uint8_t y, uint8_t duty);

void disp_pos(curPoint* point)
{
    printf("x: %d; y: %d\n", point->x, point->y);
}

bool is_dead()
{
    if (Ms.body[1].x < 0 || Ms.body[1].x == MAX_COLS || Ms.body[1].y < 0 || Ms.body[1].y == MAX_LINES) {
        printf("x: %d; y: %d; max x: %d; max y: %d\n", Ms.body[1].x, Ms.body[1].y, MAX_COLS, MAX_LINES);
        return true;
    }

    for (int i = 1; i <= Ms.len; i++) {
        for (int j = i + 1; j <= Ms.len; j++) {
            if (Ms.body[i].x == Ms.body[j].x && Ms.body[i].y == Ms.body[j].y) {
                return true;
            }
        }
    }
    return false;

}

void create_food(curPoint* food)
{
    int found = 0;
    extern uint64_t system_get_rtc_time(void);
    srand(system_get_rtc_time());
    while (1) {
        found = 0;
        int loop_num = rand() % 10;
        while (loop_num--) {
            rand();
        }
        food->y = rand() % (MAX_LINES - 1) + 1;
        food->x = rand() % (MAX_COLS - 1) + 1;
        for (int i = 1; i <= Ms.len; i++) {
            if (food->x == Ms.body[i].x && food->y == Ms.body[i].y) {
                found = 1;
            }
        }
        if (found == 0) {
            return;
        }
    }
}


void snake_auto_run()
{
    if(Ms.body[1].x == 4 && Ms.body[1].y == 0) {
        Ms.arrowHead = HEAD_LEFT;
    } else if(Ms.body[1].x == 0 && Ms.body[1].y % 2 == 0) {
        Ms.arrowHead = HEAD_DOWN;
    } else if(Ms.body[1].x == 0 && Ms.body[1].y % 2 == 1) {
        Ms.arrowHead = HEAD_RIGHT;
    } else if(Ms.body[1].x == 6 && Ms.body[1].y % 2 == 1 && Ms.body[1].y < 11) {
        Ms.arrowHead = HEAD_DOWN;
    } else if(Ms.body[1].x == 6 && Ms.body[1].y % 2 == 0) {
        Ms.arrowHead = HEAD_LEFT;
    } else if(Ms.body[1].x == 7 && Ms.body[1].y == 11) {
        Ms.arrowHead = HEAD_UP;
    } else if(Ms.body[1].x == 7 && Ms.body[1].y == 0) {
        Ms.arrowHead = HEAD_LEFT;
    }
}

void GameProcess(void *arg)
{
    int mode = (int) arg;
    int cnt=0, i, j;
    curPoint food;
    food.y = -1;
    food.x = -1;

    while(gpio_get_level(0) == 1) {
        curPoint tmp;
        if (food.y == -1) {
            create_food(&food);
            printf("food: ");
            disp_pos(&food);
            fill_pixel(food.x, food.y, FOOD_DUTY);
        }
        tmp = Ms.body[Ms.len];
        fill_pixel(Ms.body[Ms.len].x, Ms.body[Ms.len].y, 0x0);
        //move forward by one step
        for (i = Ms.len - 1; i >= 1; i--) {
            Ms.body[i + 1] = Ms.body[i];
        }
        switch (Ms.arrowHead) {
            case HEAD_RIGHT:
                Ms.body[1].x++;
                break;
            case HEAD_LEFT:
                Ms.body[1].x--;
                break;
            case HEAD_UP:
                Ms.body[1].y--;
                break;
            case HEAD_DOWN:
                Ms.body[1].y++;
                break;
        }
        if(mode == MODE_AUTO) {
            snake_auto_run();
        }
        //check food
        if (Ms.body[1].x == food.x && Ms.body[1].y == food.y) {
            food.y = -1;
            Ms.len++;
            Ms.body[Ms.len].x = -1;
            Ms.body[Ms.len].y = -1;
            if(Ms.len == MAX_COLS * MAX_LINES) {
                printf("DONE!!!\n");
                vTaskDelay(1000/portTICK_PERIOD_MS);
                return;
            }
        }
        //check dead
        if (is_dead()) {
            Ms.dead = 1;
            printf("\n=======\n\nDEAD!!!!\n");
            vTaskDelay(500 / portTICK_PERIOD_MS);
            return;
        }
        //move and draw
        for (i = 1; i <= Ms.len; i++) {
            if (1 == i) {
                fill_pixel(Ms.body[i].x, Ms.body[i].y, HEAD_DUTY);
            } else {
                //fill_pixel(Ms.body[i].x, Ms.body[i].y, SNAKE_DUTY);
            }
        }


        if (mode == MODE_AUTO) {
            vTaskDelay(20 / portTICK_PERIOD_MS);
        } else {
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
}

void snake_set_dir_up()
{
    printf("up\n");
    if (Ms.arrowHead != HEAD_DOWN) {
        Ms.arrowHead = HEAD_UP;
    }
}

void snake_set_dir_down()
{
    printf("down\n");
    if (Ms.arrowHead != HEAD_UP) {
        Ms.arrowHead = HEAD_DOWN;
    }
}

void snake_set_dir_left()
{
    printf("left\n");
    if (Ms.arrowHead != HEAD_RIGHT) {
        Ms.arrowHead = HEAD_LEFT;
    }
}

void snake_set_dir_right()
{
    printf("right\n");
    if (Ms.arrowHead != HEAD_LEFT) {
        Ms.arrowHead = HEAD_RIGHT;
    }
}

#include "touchpad.h"
void touch_control_task(void* arg)
{
    touch_pad_t pads[] = { TOUCH_PAD_NUM6, TOUCH_PAD_NUM7, TOUCH_PAD_NUM8, TOUCH_PAD_NUM9 };
    touchpad_slide_handle_t slide = touchpad_slide_create(4, pads);
    int vol_prev = 0;
    uint8_t reg, reg_val;

    while (1) {
        int pos = touchpad_slide_position(slide);
        if (pos == 255) {

        } else {
            printf("position: %d\n", pos);
            if (pos >= 0 && pos < 7) {
                snake_set_dir_up();
            } else if (pos >= 7 && pos <= 15) {
                snake_set_dir_down();
            } else if (pos > 15 && pos <= 25) {
                snake_set_dir_right();
            } else if (pos > 25 && pos <= 30) {
                snake_set_dir_left();
            }
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void initGame()
{
    curPoint tmp;
    tmp.x = MAX_COLS / 2;
    tmp.y = MAX_LINES / 2;
    Ms.len = 2;
    for (int i = 1; i <= Ms.len; i++) {
        Ms.body[i].x = tmp.x - (i - 1);
        Ms.body[i].y = tmp.y;
    }
    Ms.arrowHead = HEAD_UP;
    Ms.score = 0;
    Ms.dead = 0;
    //xTaskCreate(GameProcess, "GameProcess", 2048, NULL, 10, NULL);
    //xTaskCreate(touch_control_task, "touch_control_task", 2048, NULL, 10, NULL);
    GameProcess(NULL);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    tmp.x = MAX_COLS / 2;
    tmp.y = MAX_LINES / 2;
    Ms.len = 2;
    for (int i = 1; i <= Ms.len; i++) {
        Ms.body[i].x = tmp.x - (i - 1);
        Ms.body[i].y = tmp.y;
    }
    Ms.arrowHead = HEAD_UP;
    Ms.score = 0;
    Ms.dead = 0;
    GameProcess((void*) 1);
}
