#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#include <termios.h>

#define WIDTH 30 
#define HEIGHT 20 

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
    printf("\n automaton: %s\n", type[automaton->type_index].arg);
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

void render(Automatons *automaton) {
    system("clear");
    print_grid(automaton);
    gen_next(automaton->automaton);
}

int main(int argc, char **argv) {
    // (void) supresses unused variable warning
    (void)argc;

    srand(time(NULL));
    Automatons automaton = {0};
    Options options[OPTION_SIZE] = {
        {"glider", 0},
        {"oscillator", 0},
        {"diode", 0},
    };

    memcpy(automaton.options, options, sizeof(Options) * OPTION_SIZE);

    char *program = *argv + 0;
    char *automaton_input = *(++argv);
    if(automaton_input == NULL) {
        usage(program);
    }

    for(size_t i = 0; i < TYPE_SIZE; i++) {
        if(strcmp(type[i].arg, automaton_input) == 0) {
            automaton.type_index = i;
            automaton.automaton = type[i].ptr; 
        }
    }
    if(automaton.automaton == NULL) {
        usage(program);
    }

    while(*(++argv) != NULL) {
        char *flag = *(argv);
        if(strcmp(flag, "-r") == 0) {
            automaton.random = 1;
        } 
        if(strcmp(flag, "-o") == 0) {
            if(*(argv + 1) == NULL) {
                usage(program);
            }
            // (void) supresses the unused calculation warning
            (void)*(++argv);
            for(size_t i = 0; i < OPTION_SIZE && *(argv) != NULL; i++) {
                char *option = *(argv);
                printf("%s\n", option);
                if(strcmp(option, automaton.options[i].arg) == 0) {
                    automaton.options[i].value = 1;
                    (void)*(++argv);
                }
            }
        }
        if(strcmp(flag, "-h") == 0) {
            usage(program);
        }
    }

    begin_raw();

    char c;

    init_grid(automaton.random);
    if(automaton.options[0].value) init_glider(5);
    if(automaton.options[1].value) init_oscillator(5);
    if(automaton.options[2].value) init_diode(5);
    int option_mode = 0;
    render(&automaton);
    while(read(STDIN_FILENO, &c, 1) == 1) {
        switch(c) {
            case 'q':
                return 0;
                break;
            case 'j':
                automaton.type_index -= 1;
                automaton.type_index %= TYPE_SIZE;
                automaton.automaton = type[automaton.type_index].ptr;
                render(&automaton);
                break;
            case 'k':
                automaton.type_index += 1;
                automaton.type_index %= TYPE_SIZE;
                automaton.automaton = type[automaton.type_index].ptr;
                render(&automaton);
                break;
            case 'n':
                render(&automaton);
                break;
            case 'r':
                init_grid(1);
                render(&automaton);
                break;
            case 'g':
                if(option_mode) {
                    init_glider(5);
                    option_mode = 0;
                }
                render(&automaton);
                break;
            case 'd':
                if(option_mode) {
                    init_glider(5);
                    option_mode = 0;
                }
                render(&automaton);
                break;
            case 'o':
                if(option_mode) {
                    init_glider(5);
                    option_mode = 0;
                    render(&automaton);
                } else {
                    option_mode = 1; 
                }
                break;
            default:
                break;
        }
    }
}
