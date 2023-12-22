#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define WIDTH 100 
#define HEIGHT 50 

#define SPEED 50 

#define BACKGROUND '-'
#define CELL 'E'

typedef enum {
    DEAD,
    ALIVE,
} State;

typedef struct {
    State state;
} Cell;

Cell grid[HEIGHT][WIDTH] = {0};

void init_grid() {
    for(size_t i = 0; i < HEIGHT; i++) {
        for(size_t j = 0; j < WIDTH; j++) {
            if(rand() % 2 == 0) {
                grid[i][j].state = ALIVE;
                continue;
            }
            grid[i][j].state = DEAD;
        }
    }
}

void gen_next() {
    Cell new_grid[HEIGHT][WIDTH] = {0};
    for(size_t i = 0; i < HEIGHT; i++) {
        for(size_t j = 0; j < WIDTH; j++) {
            new_grid[i][j] = grid[i][j];
        }
    }
    for(size_t i = 0; i < HEIGHT; i++) {
        for(size_t j = 0; j < WIDTH; j++) {
            int alive_count = 0;
            for(int k = -1; k <= 1; k++) {
                for(int l = -1; l <= 1; l++) {
                    if(k == 0 && l == 0) continue;
                    int row = (i + k + HEIGHT) % HEIGHT;
                    int col = (j + l + WIDTH) % WIDTH;
                    if(grid[row][col].state == ALIVE) {
                            alive_count++;
                    }
                }
            }
            switch(alive_count) {
                case 0:
                case 1:
                    new_grid[i][j].state = DEAD;
                    break;
                case 2:
                case 3:
                    if(grid[i][j].state == DEAD && alive_count == 3) {
                        new_grid[i][j].state = ALIVE;
                    }
                    break;
                default:
                    new_grid[i][j].state = DEAD;
                    break;
            }
        }
    }
    for(size_t i = 0; i < HEIGHT; i++) {
        for(size_t j = 0; j < WIDTH; j++) {
            grid[i][j] = new_grid[i][j];
        }
    }
}

int print_grid() {
    int alive_count = 0;
    for(size_t i = 0; i < HEIGHT; i++) {
        for(size_t j = 0; j < WIDTH; j++) {
            if(grid[i][j].state == ALIVE) {
                alive_count++;
                printf("%c", CELL);
            } else {
                printf("%c", BACKGROUND);
            }
        }
        printf("\n");
    }
    return alive_count;
}

void init_glider(size_t offset) {
    grid[offset+0][offset+1].state = ALIVE;
    grid[offset+1][offset+2].state = ALIVE;
    grid[offset+2][offset+0].state = ALIVE;
    grid[offset+2][offset+1].state = ALIVE;
    grid[offset+2][offset+2].state = ALIVE;
}

int main() {
    srand(time(NULL));
    init_grid();
    init_glider(0);
    init_glider(4);
    init_glider(8);
    system("clear");
    while(print_grid() != 0) {
        usleep(SPEED * 1000);
        gen_next();
        system("clear");
    }
}
