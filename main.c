#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#include <termios.h>
#include <pthread.h>

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

typedef enum {
    NORMAL,
    OPTION,
    PLAY,
} Mode;

Mode mode = NORMAL;

typedef State cur[9];

typedef struct {
    State state;
} Cell;

struct termios orig_termios;

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

typedef void(*fun_ptr);

typedef struct {
    char *arg;
    fun_ptr ptr;
} Type;

#define TYPE_SIZE 5
typedef struct {
    cur *automaton;
    int random;
    size_t type_index;
} Automatons;

Type type[TYPE_SIZE] = {
    {"gol", &gol},
    {"seeds", &seeds},
    {"bbrain", &brain},
    {"daynight", &daynight},
    {"wireworld", &wireworld},
};

void die(const char *s) { (void)s; }
void stop_raw() {
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
        die("tcsetattr");
    }
}

void begin_raw() {
    if(tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        die("tcgetattr");
    }
    atexit(stop_raw);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        die("tcsetattr");
    }
}


void init_grid(int random, Cell *grid, size_t height, size_t width) {
    for(size_t i = 0; i < height; i++) {
        for(size_t j = 0; j < width; j++) {
            if(rand() % 2 == 0 && random == 1) {
                grid[i*width+j].state = ALIVE;
                continue;
            }
            grid[i*width+j].state = DEAD;
        }
    }
}

void gen_next(State automaton[][9], Cell *grid, size_t height, size_t width) {
    Cell *new_grid = calloc(height * width, sizeof(Cell));
    memcpy(new_grid, grid, sizeof(Cell) * height * width);
    for(size_t i = 0; i < height; i++) {
        for(size_t j = 0; j < width; j++) {
            int alive_count = 0;
            for(int k = -1; k <= 1; k++) {
                for(int l = -1; l <= 1; l++) {
                    if(k == 0 && l == 0) continue;
                    int row = (i + k + height) % height;
                    int col = (j + l + width) % width;
                    if(grid[row*width+col].state == ALIVE) {
                            alive_count++;
                    }
                }
            }
            new_grid[i*width+j].state = automaton[grid[i*width+j].state][alive_count];
        }
    }
    memcpy(grid, new_grid, sizeof(Cell) * height * width);
}

int print_grid(Automatons *automaton, Cell *grid, size_t height, size_t width) {
    int alive_count = 0;
    for(size_t i = 0; i < height; i++) {
        for(size_t j = 0; j < width; j++) {
            switch(grid[i*width+j].state) {
                case ALIVE:
                    alive_count++;
                    putc(CELL, stdout);
                    break;
                case DEAD:
                    putc(BACKGROUND, stdout);
                    break;
                case DYING:
                    putc(ALMOST_DEAD, stdout);
                    break;
                case CONDUCTOR:
                    alive_count++;
                    putc(CONDUCTOR_CELL, stdout);
                    break;
            }
        }
        putc('\n', stdout);
    }
    printf("\ncontrols: \n");
    printf("render next: n | change automaton: j, k \ninit random: r | option mode: o | play: p\n");
    if(mode == OPTION) {
        printf("options: glider - g, diode - d, oscillator - o\n");
    }
    printf("automaton: %s\n", type[automaton->type_index].arg);

    if(mode == OPTION) {
        printf("\nOPTION MODE\n");
    }
    return alive_count;
}

void init_glider(size_t offset, Cell *grid, size_t height, size_t width) {
    (void)height;
    grid[(offset+0)*width+offset+1].state = ALIVE;
    grid[(offset+1)*width+offset+2].state = ALIVE;
    grid[(offset+2)*width+offset+0].state = ALIVE;
    grid[(offset+2)*width+offset+1].state = ALIVE;
    grid[(offset+2)*width+offset+2].state = ALIVE;
}

void init_oscillator(size_t offset, Cell *grid, size_t height, size_t width) {
    (void)height;
    grid[(offset+0)*width+5].state = ALIVE;
    grid[(offset+0)*width+6].state = ALIVE;
    grid[(offset+1)*width+5].state = ALIVE;
    grid[(offset+1)*width+6].state = ALIVE;
    grid[(offset+1)*width+4].state = DYING;
    grid[(offset+0)*width+7].state = DYING;
    grid[(offset+(-1))*width+5].state = DYING;
    grid[(offset+2)*width+6].state = DYING;
}

void init_diode(size_t offset, Cell *grid, size_t height, size_t width) {
    (void)height;
    grid[(offset+1)*width+0].state = CONDUCTOR;
    grid[(offset+1)*width+1].state = DYING;
    grid[(offset+1)*width+2].state = ALIVE;
    grid[(offset+0)*width+3].state = CONDUCTOR;
    grid[(offset+2)*width+3].state = CONDUCTOR;
    grid[(offset+2)*width+4].state = CONDUCTOR;
    grid[(offset+0)*width+4].state = CONDUCTOR;
    grid[(offset+1)*width+5].state = CONDUCTOR;
    grid[(offset+1)*width+6].state = CONDUCTOR;
    for(size_t i = 7; i < width; i++) {
        grid[(offset+1)*width+i].state = CONDUCTOR;
    }
}

typedef struct {
    void *input1;
    void *input2;
} Inputs;

void render(void *input, Cell *grid, size_t height, size_t width) {
    Automatons *automaton = (Automatons*)input;
    printf("\e[1;1H\e[2J");
    print_grid(automaton, grid, height, width);
    gen_next(automaton->automaton, grid, height, width);
}

void play(void *input, int time_to_wait, Cell *grid, size_t height, size_t width) {
    Inputs *inputs = (Inputs*)input;
    Automatons *automaton = (Automatons*)inputs->input1;
    if(mode == PLAY) {
        size_t counter = time_to_wait;
        for(size_t i = counter; i > 0; i--) {
            render(automaton, grid, height, width);
            usleep(1000 * 50);
        }
        mode = NORMAL;
    }
}

void handle_input(void *input, Cell *grid, size_t height, size_t width) {
    Inputs *inputs = (Inputs*)input;
    Automatons *automaton = (Automatons*)inputs->input1;
    char *c = (char*)inputs->input2;
    init_grid(0, grid, height, width);
    render(automaton, grid, height, width);
    while(read(STDIN_FILENO, c, 1) == 1) {
        switch(*c) {
            case 'q':
                if(mode == OPTION) {
                    mode = NORMAL;
                    render(automaton, grid, height, width);
                    break;
                }
                return;
            case 'j':
                automaton->type_index -= 1;
                automaton->type_index %= TYPE_SIZE;
                automaton->automaton = type[automaton->type_index].ptr;
                render(automaton, grid, height, width);
                break;
            case 'k':
                automaton->type_index += 1;
                automaton->type_index %= TYPE_SIZE;
                automaton->automaton = type[automaton->type_index].ptr;
                render(automaton, grid, height, width);
                break;
            case 'n':
                render(automaton, grid, height, width);
                break;
            case 'r':
                init_grid(1, grid, height, width);
                render(automaton, grid, height, width);
                break;
            case 'g':
                if(mode == OPTION) {
                    init_glider(5, grid, height, width);
                    mode = NORMAL;
                }
                render(automaton, grid, height, width);
                break;
            case 'd':
                if(mode == OPTION) {
                    init_diode(5, grid, height, width);
                    mode = NORMAL;
                }
                render(automaton, grid, height, width);
                break;
            case 'o':
                if(mode == OPTION) {
                    init_oscillator(5, grid, height, width);
                    mode = NORMAL;
                } else {
                    mode = OPTION;
                }
                render(automaton, grid, height, width);
                break;
            case 'p': {
                printf("Enter the time to play in 100s of ms\n");
                char num[8] = {0};
                size_t num_len = 0;
                while(read(STDIN_FILENO, c, 1) == 1 && *c != '\n') {
                    if(!isdigit(*c)) {
                        strncpy(num, "50", 3);
                        break;
                    }
                    num[num_len++] = *c;
                }
                mode = PLAY;
                play(input, atoi(num), grid, height, width);
            } break;
            default:
                break;
        }
    }
}

int main(int argc, char **argv) {
    char *program = argv[0];
    if(argc < 3) {
	fprintf(stderr, "usage: %s <height: 1-25> <width: 1-100>\n", program);
	exit(1);
    }
    size_t height = atoi(argv[1]);
    size_t width = atoi(argv[2]);
    if(height == 0 || width == 0 || height > 25 || width > 100) {
	fprintf(stderr, "please enter a valid width and height\n");
	exit(1);
    }
    Cell *grid = calloc(height*width, sizeof(Cell));

    srand(time(NULL));
    Automatons *automaton = malloc(sizeof(Automatons));
    automaton->automaton = type[0].ptr;

    begin_raw();

    char c;
    Inputs *inputs = malloc(sizeof(Inputs));
    inputs->input1 = automaton;
    inputs->input2 = &c;

    handle_input(inputs, grid, height, width);

    return 0;
}
