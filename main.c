#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define WIDTH 40 
#define HEIGHT 20 

#define SPEED 50 

#define BACKGROUND '-'
#define CELL '#'
#define ALMOST_DEAD '*'
#define CONDUCTOR_CELL '^'


typedef enum {
    DEAD,
    ALIVE,
    DYING,
    CONDUCTOR,
} State;

typedef State cur[9];

typedef enum {
    GOL,
    SEEDS,
    BRAIN,
    DAYNIGHT,
    WIREWORLD,
} Automaton;

typedef struct {
    State state;
} Cell;

State gol[2][9] = {
    {DEAD, DEAD, DEAD, ALIVE, DEAD, DEAD, DEAD, DEAD, DEAD},
    {DEAD, DEAD, ALIVE, ALIVE, DEAD, DEAD, DEAD, DEAD, DEAD},
};

State seeds[2][9] = {
    {DEAD, DEAD, ALIVE, DEAD, DEAD, DEAD, DEAD, DEAD, DEAD},
    {DEAD, DEAD, DEAD, DEAD, DEAD, DEAD, DEAD, DEAD, DEAD},
};

State brain[3][9] = {
    {DEAD, DEAD, ALIVE, DEAD, DEAD, DEAD, DEAD, DEAD, DEAD},
    {DYING, DYING, DYING, DYING, DYING, DYING, DYING, DYING, DYING},
    {DEAD, DEAD, DEAD, DEAD, DEAD, DEAD, DEAD, DEAD, DEAD},
};

State daynight[3][9] = {
    {DEAD, DEAD, DEAD, ALIVE, DEAD, DEAD, ALIVE, ALIVE, ALIVE},
    {DEAD, DEAD, DEAD, ALIVE, ALIVE, DEAD, ALIVE, ALIVE, ALIVE},
};

State wireworld[4][9] = {
    {DEAD, DEAD, DEAD, ALIVE, DEAD, DEAD, ALIVE, ALIVE, ALIVE},
    {DYING, DYING, DYING, DYING, DYING, DYING, DYING, DYING, DYING},
    {CONDUCTOR, CONDUCTOR, CONDUCTOR, CONDUCTOR, CONDUCTOR, CONDUCTOR, CONDUCTOR, CONDUCTOR, CONDUCTOR},
    {CONDUCTOR, ALIVE, ALIVE, CONDUCTOR, CONDUCTOR, CONDUCTOR, CONDUCTOR, CONDUCTOR, CONDUCTOR},
};

Cell grid[HEIGHT][WIDTH] = {0};

void init_grid(int random) {
    for(size_t i = 0; i < HEIGHT; i++) {
        for(size_t j = 0; j < WIDTH; j++) {
            if(rand() % 2 == 0 && random == 1) {
                grid[i][j].state = ALIVE;
                continue;
            }
            grid[i][j].state = DEAD;
        }
    }
}

void gen_next(State automaton[][9]) {
    Cell new_grid[HEIGHT][WIDTH] = {0};
    memcpy(new_grid, grid, sizeof(Cell) * HEIGHT * WIDTH);
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
            new_grid[i][j].state = automaton[grid[i][j].state][alive_count];
        }
    }
    memcpy(grid, new_grid, sizeof(Cell) * HEIGHT * WIDTH);
}

int print_grid() {
    int alive_count = 0;
    for(size_t i = 0; i < HEIGHT; i++) {
        for(size_t j = 0; j < WIDTH; j++) {
            switch(grid[i][j].state) {
                case ALIVE:
                    alive_count++;
                    printf("%c", CELL);
                    break;
                case DEAD:
                    printf("%c", BACKGROUND);
                    break;
                case DYING:
                    printf("%c", ALMOST_DEAD);
                    break;
                case CONDUCTOR:
                    alive_count++;
                    printf("%c", CONDUCTOR_CELL);
                    break;
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

void init_oscillator(size_t offset) {
    grid[offset+0][5].state = ALIVE;
    grid[offset+0][6].state = ALIVE;
    grid[offset+1][5].state = ALIVE;
    grid[offset+1][6].state = ALIVE;
    grid[offset+1][4].state = DYING;
    grid[offset+0][7].state = DYING;
    grid[offset-1][5].state = DYING;
    grid[offset+2][6].state = DYING;
}

void init_diode(size_t offset) {
    grid[offset+1][0].state = CONDUCTOR;
    grid[offset+1][1].state = DYING;
    grid[offset+1][2].state = ALIVE;
    grid[offset+0][3].state = CONDUCTOR;
    grid[offset+2][3].state = CONDUCTOR;
    grid[offset+2][4].state = CONDUCTOR;
    grid[offset+0][4].state = CONDUCTOR;
    grid[offset+1][5].state = CONDUCTOR;
    grid[offset+1][6].state = CONDUCTOR;
    for(size_t i = 7; i < WIDTH; i++) {
        grid[offset+1][i].state = CONDUCTOR;
    }
}

void usage(char *program) {
    fprintf(stderr, "usage: %s <gol | seeds | bbrain | daynight | wireworld> -r -o <glider & oscillator & diode> \n", program);
    exit(1);
}

int main(int argc, char **argv) {
    (void)argc;
    char *program = *argv + 0;
    int glider = 0;
    int oscillator = 0;
    int diode = 0;
    int random = 0;
    char *automaton_input = *(++argv);
    int automaton = GOL;
    if(automaton_input == NULL) {
        usage(program);
    }
    if(strcmp(automaton_input, "gol") == 0) {
        automaton = GOL;
    } else if(strcmp(automaton_input, "seeds") == 0) {
        automaton = SEEDS;
    } else if(strcmp(automaton_input, "bbrain") == 0) {
        automaton = BRAIN;
    } else if(strcmp(automaton_input, "daynight") == 0) {
        automaton = DAYNIGHT;
    } else if(strcmp(automaton_input, "wireworld") == 0) {
        automaton = WIREWORLD;
    } else {
        usage(program);
    }
    while(*(++argv) != NULL) {
        char *flag = *(argv);
        if(strcmp(flag, "-r") == 0) {
            random = 1;
        } 
        if(strcmp(flag, "-o") == 0) {
            if(*(argv + 1) == NULL) {
                usage(program);
            }
            while(*(++argv) != NULL) {
                char *option = *(argv);
                if(!glider) glider = strcmp(option, "glider") == 0;
                if(!oscillator) oscillator = strcmp(option, "oscillator") == 0;
                if(!diode) diode = strcmp(option, "diode") == 0;
            }
        }
        if(strcmp(flag, "-h") == 0) {
            usage(program);
        }
    }
    srand(time(NULL));
    system("clear");
    init_grid(random);
    if(glider) init_glider(5);
    if(oscillator) init_oscillator(5);
    if(diode) init_diode(5);
    cur *current_state = gol;
    switch(automaton) {
        case GOL:
            current_state = gol;
            break;
        case SEEDS:
            current_state = seeds;
            break;
        case BRAIN:
            current_state = brain;
            break;
        case DAYNIGHT:
            current_state = daynight;
            break;
        case WIREWORLD:
            current_state = wireworld;
            break;
    }

    while(print_grid() != 0) {
        usleep(SPEED * 1000);
        gen_next(current_state);
        system("clear");
    }
}
