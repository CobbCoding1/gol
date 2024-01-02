#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#include <termios.h>
#include <pthread.h>

#define WIDTH 50 
#define HEIGHT 25 

#define SPEED 500 

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

typedef struct {
    char *arg;
    int value;
} Options;

typedef void(*fun_ptr);

typedef struct {
    char *arg;
    fun_ptr ptr;
} Type;

#define TYPE_SIZE 5
#define OPTION_SIZE 3
typedef struct {
    cur *automaton;
    int random;
    Options options[OPTION_SIZE];
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
    //raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    //raw.c_oflag &= ~(OPOST);
    //raw.c_cflag |= (CS8);
    //raw.c_iflag |= IUTF8;
    // ICANON | IEXTEN
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    //raw.c_cc[VMIN] = 0;
    //raw.c_cc[VTIME] = 1;
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        die("tcsetattr");
    }
}

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

int print_grid(Automatons *automaton) {
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

typedef struct {
    void *input1;
    void *input2;
} Inputs;

void *render(void *input) {
    Automatons *automaton = (Automatons*)input;
    printf("\e[1;1H\e[2J");
    print_grid(automaton);
    gen_next(automaton->automaton);
    return (void*)1;
}


void *play(void *input, int time_to_wait) {
    Inputs *inputs = (Inputs*)input;
    Automatons *automaton = (Automatons*)inputs->input1;
    if(mode == PLAY) {
        size_t counter = time_to_wait;
	for(size_t i = counter; i > 0; i--) {
            render(automaton);
            usleep(1000 * 50);
        }
        mode = NORMAL;
    }
    return (void*)0;
}

void *handle_input(void *input) {
    Inputs *inputs = (Inputs*)input;
    Automatons *automaton = (Automatons*)inputs->input1;
    char *c = (char*)inputs->input2;
    init_grid(0);
    if(automaton->options[0].value) init_glider(5);
    if(automaton->options[1].value) init_oscillator(5);
    if(automaton->options[2].value) init_diode(5);
    render(automaton);
    while(read(STDIN_FILENO, c, 1) == 1) {
        switch(*c) {
            case 'q':
                if(mode == OPTION) {
                    mode = NORMAL;
                    render(automaton);
                    break;
                }
                return input;
                break;
            case 'j':
                automaton->type_index -= 1;
                automaton->type_index %= TYPE_SIZE;
                automaton->automaton = type[automaton->type_index].ptr;
                render(automaton);
                break;
            case 'k':
                automaton->type_index += 1;
                automaton->type_index %= TYPE_SIZE;
                automaton->automaton = type[automaton->type_index].ptr;
                render(automaton);
                break;
            case 'n':
                render(automaton);
                break;
            case 'r':
                init_grid(1);
                render(automaton);
                break;
            case 'g':
                if(mode == OPTION) {
                    init_glider(5);
                    mode = NORMAL;
                }
                render(automaton);
                break;
            case 'd':
                if(mode == OPTION) {
                    init_diode(5);
                    mode = NORMAL;
                }
                render(automaton);
                break;
            case 'o':
                if(mode == OPTION) {
                    init_oscillator(5);
                    mode = NORMAL;
                } else {
                    mode = OPTION;
                }
                render(automaton);
                break;
            case 'p': {
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
                play(input, atoi(num));
            } break;
            default:
                break;
        }
    }
    return input;
}

int main(int argc, char **argv) {
    // (void) supresses unused variable warning
    (void)argc;

    srand(time(NULL));
    Automatons *automaton = malloc(sizeof(Automatons));
    Options options[OPTION_SIZE] = {
        {"glider", 0},
        {"oscillator", 0},
        {"diode", 0},
    };
    automaton->automaton = type[0].ptr;

    memcpy(automaton->options, options, sizeof(Options) * OPTION_SIZE);

    char *program = argv[0];
    (void)program;

    begin_raw();

    char c;
    Inputs *inputs = malloc(sizeof(Inputs));
    inputs->input1 = automaton;
    inputs->input2 = &c;

    handle_input(inputs);

    return 0;
}
