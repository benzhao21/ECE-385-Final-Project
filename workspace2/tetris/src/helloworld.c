
/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xparameters.h"
#include "xuartlite.h"
#include "xgpio.h"
#include <xtmrctr.h>
#include <stdbool.h>

#define UART_DEVICE_ID XPAR_UARTLITE_0_DEVICE_ID
#define PLAYER_1_CODE_GPIO_ID XPAR_PLAYER1KEYCODE_DEVICE_ID
#define PLAYER_2_CODE_GPIO_ID XPAR_PLAYER2KEYCODE_DEVICE_ID

#define BOARD_P1 ((volatile uint32_t*)0x44A10000)
#define BOARD_P2 ((volatile uint32_t*)0x44A10064)

#include <stdint.h>

#define COLOR_I 4
#define COLOR_O 6
#define COLOR_T 5
#define COLOR_S 1
#define COLOR_Z 7
#define COLOR_J 2
#define COLOR_L 3
#define COLOR_GARB 8

#define KEY_LEFT   0x25
#define KEY_RIGHT  0x27
#define KEY_ROTATE_CW 0x26
#define KEY_ROTATE_CCW 0x5A
#define KEY_ENTER  0xD
#define KEY_SOFTDROP   0x28
#define KEY_HARDDROP   0x20
#define KEY_HOLD 0x43
#define KEY_NO_HOLD_MOD 0x51
#define KEY_FAST_GRAV_MOD 0x57
#define KEY_MESSY_GARBAGE_MOD 0x45
#define KEY_NO_GARBAGE_MOD 0x52
#define KEY_SINGLE_PLAYER_MOD 0x54


#define MAX_PIECES 1000
#define EMPTY_HOLD 255


#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20

#define LOCK_DELAY_TICKS 50000000


typedef struct {
    bool no_hold;
    bool fast_grav;
    bool messy_garbage;
    bool no_garbage;
    bool single_player;
} GameMods;

GameMods mods = {
    .no_hold = false,
    .fast_grav = false,
    .messy_garbage = false,
    .no_garbage = false,
    .single_player = false,
};


// TETROMINOES[piece][rot][row][col]
uint8_t TETROMINOES[7][4][4][4] = {
    // I piece
    {
        {   // rot 0
            {4,4,4,4},
            {0,0,0,0},
            {0,0,0,0},
            {0,0,0,0}
        },
        {   // rot 1
            {0,0,4,0},
            {0,0,4,0},
            {0,0,4,0},
            {0,0,4,0}
        },
        {   // rot 2
            {4,4,4,4},
            {0,0,0,0},
            {0,0,0,0},
            {0,0,0,0}
        },
        {   // rot 3
            {0,4,0,0},
            {0,4,0,0},
            {0,4,0,0},
            {0,4,0,0}
        }
    },

    // O piece
    {
        {
            {6,6,0,0},
            {6,6,0,0},
            {0,0,0,0},
            {0,0,0,0}
        },
        {   // same for all rotations
            {6,6,0,0},
            {6,6,0,0},
            {0,0,0,0},
            {0,0,0,0}
        },
        {
            {6,6,0,0},
            {6,6,0,0},
            {0,0,0,0},
            {0,0,0,0}
        },
        {
            {6,6,0,0},
            {6,6,0,0},
            {0,0,0,0},
            {0,0,0,0}
        }
    },

    // T piece
    {
        {   // rot 0
            {0,5,0,0},
            {5,5,5,0},
            {0,0,0,0},
            {0,0,0,0}
        },
        {   // rot 1
            {0,5,0,0},
            {0,5,5,0},
            {0,5,0,0},
            {0,0,0,0}
        },
        {   // rot 2
            {0,0,0,0},
            {5,5,5,0},
            {0,5,0,0},
            {0,0,0,0}
        },
        {   // rot 3
            {0,5,0,0},
            {5,5,0,0},
            {0,5,0,0},
            {0,0,0,0}
        }
    },

    // S piece
    {
        {
            {0,1,1,0},
            {1,1,0,0},
            {0,0,0,0},
            {0,0,0,0}
        },
        {
            {1,0,0,0},
            {1,1,0,0},
            {0,1,0,0},
            {0,0,0,0}
        },
        {
            {0,1,1,0},
            {1,1,0,0},
            {0,0,0,0},
            {0,0,0,0}
        },
        {
            {1,0,0,0},
            {1,1,0,0},
            {0,1,0,0},
            {0,0,0,0}
        }
    },

    // Z piece
    {
        {
            {7,7,0,0},
            {0,7,7,0},
            {0,0,0,0},
            {0,0,0,0}
        },
        {
            {0,0,7,0},
            {0,7,7,0},
            {0,7,0,0},
            {0,0,0,0}
        },
        {
            {7,7,0,0},
            {0,7,7,0},
            {0,0,0,0},
            {0,0,0,0}
        },
        {
            {0,0,7,0},
            {0,7,7,0},
            {0,7,0,0},
            {0,0,0,0}
        }
    },

    // J piece
    {
        {
            {2,0,0,0},
            {2,2,2,0},
            {0,0,0,0},
            {0,0,0,0}
        },
        {
            {0,2,2,0},
            {0,2,0,0},
            {0,2,0,0},
            {0,0,0,0}
        },
        {
            {0,0,0,0},
            {2,2,2,0},
            {0,0,2,0},
            {0,0,0,0}
        },
        {
            {0,2,0,0},
            {0,2,0,0},
            {2,2,0,0},
            {0,0,0,0}
        }
    },

    // L piece
    {
        {
            {0,0,3,0},
            {3,3,3,0},
            {0,0,0,0},
            {0,0,0,0}
        },
        {
            {0,3,0,0},
            {0,3,0,0},
            {0,3,3,0},
            {0,0,0,0}
        },
        {
            {0,0,0,0},
            {3,3,3,0},
            {3,0,0,0},
            {0,0,0,0}
        },
        {
            {3,3,0,0},
            {0,3,0,0},
            {0,3,0,0},
            {0,0,0,0}
        }
    }
};

const char E4[5][5] = {
    "####",
    "#   ",
    "### ",
    "#   ",
    "####"
};

const char C4[5][5] = {
    " ###",
    "#   ",
    "#   ",
    "#   ",
    " ###"
};

const char NUM3[5][5] = {
    "### ",
    "  # ",
    "### ",
    "  # ",
    "### "
};

const char NUM8[5][5] = {
    " ## ",
    "#  #",
    " ## ",
    "#  #",
    " ## "
};

const char NUM5[5][5] = {
    "####",
    "#   ",
    "### ",
    "   #",
    "####"
};


uint8_t p1_left_prev = 0;
uint8_t p1_right_prev = 0;
uint8_t p1_down_prev = 0;


uint8_t p2_left_prev = 0;
uint8_t p2_right_prev = 0;
uint8_t p2_down_prev = 0;




typedef struct{
	uint8_t grid[10][20];
	volatile uint32_t* addr;
	uint8_t piece; // 0, 1, 2, ... same order as tetrominoes
	int16_t x; // signed now
	int16_t y; // signed now
	uint8_t rot; // 0 ,1 ,2,3 clockwise rotations
	uint8_t lines;
	uint32_t linestot;
	uint16_t next_piece_index;
	uint32_t score;
	volatile uint32_t* holdaddr;

	 uint8_t hold_piece;         // 0-6, 255 for empty
	 bool can_hold;              // true if player can hold
	 uint8_t next_pieces[5];

	 bool lock_delay_active;
	 uint32_t lock_delay_start;
} Player;

uint8_t piece_queue[MAX_PIECES];

// Initialize once at game start

XUartLite Uart;
XGpio P1KeycodeGpio;
XGpio P2KeycodeGpio;
XTmrCtr Usb_timer;


void draw_letter_4x5(Player *p, int x0, int y0, const char pattern[5][5], uint8_t color) {
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 4; x++) {
            if (pattern[y][x] == '#') {
                int gx = x0 + x;
                int gy = y0 + y;
                if (gx >= 0 && gx < 10 && gy >= 0 && gy < 20)
                    p->grid[gx][gy] = color;
            }
        }
    }
}


void draw_ECE385(Player *p) {
    memset(p->grid, 0, sizeof(p->grid));
    uint8_t C = COLOR_I;

    // Top row: E and C
    draw_letter_4x5(p, 0, 2, E4, 3);
    draw_letter_4x5(p, 6, 2, NUM3, C);

    // Middle row: 3 and 8
    draw_letter_4x5(p, 0, 8, C4, 3);
    draw_letter_4x5(p, 5, 8, NUM8, C);

    // Bottom row: centered 5
    draw_letter_4x5(p, 0, 14, E4, 3);
    draw_letter_4x5(p, 5, 14, NUM5, C);
}



// -------------------------------------------
// Non-blocking UART read attempt
// returns 1 if a byte was read, 0 otherwise
// -------------------------------------------
int try_recv_byte(u8 *dst) {
    return XUartLite_Recv(&Uart, dst, 1);
}

// -------------------------------------------
// Process one input packet (player, key, state)
// -------------------------------------------
void process_input_event(u8 player, u8 key, u8 state) {
    static u8 last_key = 0;
    static u8 last_state = 0;
    static int have_last = 0;

    // ignore duplicate repeat events
    if (have_last && key == last_key && state == last_state)
        return;

    last_key = key;
    last_state = state;
    have_last = 1;

    if (player == 1) {
        XGpio_DiscreteWrite(&P1KeycodeGpio, 1, key);
    } else if (player == 2) {
        XGpio_DiscreteWrite(&P2KeycodeGpio, 1, key);
    }
}


// returns true if collision, false if no collision
bool check_collision(Player *p, int x, int y) {
    uint8_t (*shape)[4] = TETROMINOES[p->piece][p->rot];

    for(int i = 0; i < 4; i++) {       // column in tetromino
        for(int j = 0; j < 4; j++) {   // row in tetromino
            if(shape[j][i] == 0) continue; // empty cell, skip

            int board_x = x + i;
            int board_y = y + j;

            // Check boundaries
            if(board_x < 0 || board_x >= BOARD_WIDTH) return true;
            if(board_y < 0 || board_y >= BOARD_HEIGHT) return true;

            // Check collision with existing blocks
            if(p->grid[board_x][board_y] != 0) return true;
        }
    }

    return false; // no collision
}

void writeboard_raw(Player* p) {
    uint32_t temp = 0;
    int idx = 0;

    for (int j = 0; j < 20; j++) {
        for (int i = 0; i < 10; i++) {
            uint8_t cell = p->grid[i][j];

            // pack into 32-bit temp
            if (idx % 8 == 0)
                temp = 0;

            temp |= (cell & 0xF) << (4 * (7 - (idx % 8)));

            if (idx % 8 == 7) {
                *(p->addr + (idx / 8)) = temp;
            }

            idx++;
        }
    }
}


void writeboard(Player* p) {
    uint32_t temp;
    uint8_t idx;
    uint8_t piece_cell;

    // Calculate drop position (ghost piece)
    int drop_y = p->y;
    while (!check_collision(p, p->x, drop_y + 1)) {
        drop_y++;
    }

    for(int j = 0; j < 20; j++) {
        for(int i = 0; i < 10; i++) {
            idx = i + j * 10;

            if(idx % 8 == 0) temp = 0;

            // Base grid cell
            uint8_t cell = p->grid[i][j];

            // Ghost piece overlay (only if actual piece not present)
            int rel_x = i - p->x;
            int rel_y = j - drop_y;
            if(rel_x >= 0 && rel_x < 4 && rel_y >= 0 && rel_y < 4) {
                uint8_t ghost_cell = TETROMINOES[p->piece][p->rot][rel_y][rel_x];
                if(ghost_cell != 0 && cell == 0) {
                    cell = 9;  // ghost color
                }
            }

            // Actual piece overlay (draw on top)
            rel_x = i - p->x;
            rel_y = j - p->y;
            if(rel_x >= 0 && rel_x < 4 && rel_y >= 0 && rel_y < 4) {
                piece_cell = TETROMINOES[p->piece][p->rot][rel_y][rel_x];
                if(piece_cell != 0) {
                    cell = piece_cell;  // overwrite ghost/grid
                }
            }

            // Shift into temp
            temp += cell << (4 * (7 - (idx % 8)));

            if(idx % 8 == 7) {
                *(p->addr + idx / 8) = temp;
            }
        }
    }
}


void lock_piece(Player* p, int x) {
    uint8_t (*shape)[4] = TETROMINOES[p->piece][p->rot]; // current piece, no rotation

    for(int i = 0; i < 4; i++) {       // column in tetromino
        for(int j = 0; j < 4; j++) {   // row in tetromino
            if(shape[j][i] == 0) continue;

            int board_x = x + i;
            int board_y = p->y + j;

            // Make sure we are inside board
            if(board_x >= 0 && board_x < 10 && board_y >= 0 && board_y < 20) {
                p->grid[board_x][board_y] = shape[j][i];
            }
        }
    }
}


// Simple LCG generator, 32-bit state
static uint32_t rng_state = 1; // seed

uint8_t simple_rand(int k) {
    rng_state = rng_state * 1664525 + 1013904223; // LCG
    return (rng_state >> 16) % k; // return 0..6
}

void init_piece_queue() {
    for (int i = 0; i < MAX_PIECES; i++) {
        piece_queue[i] = simple_rand(7);  // 0..6 for tetromino types
    }
}
//returns true if game over
bool spawn_new_piece(Player* p) {
	p->piece = p->next_pieces[0];  // take from shared queue
    p->y = 0;                       // top of board
    p->x = (BOARD_WIDTH / 2) - 2;   // center horizontally
    p->rot = 0;
    p->can_hold = true;           // reset hold ability for new piece

	// shift next_pieces left and fill last from queue
	for (int i = 0; i < 4; i++) {
		p->next_pieces[i] = p->next_pieces[i + 1];
	}
	p->next_pieces[4] = piece_queue[p->next_piece_index];
	p->next_piece_index = (p->next_piece_index + 1) % MAX_PIECES;

    if (check_collision(p, p->x, p->y)) {
        // handle game over
    	return true;
    }
    return false;
}


// Returns true if piece was locked (hit the bottom or another piece)
bool apply_gravity(Player* p, int x, uint32_t current_tick) {
    if(!check_collision(p, x, p->y + 1)) {
        // Can move down - cancel any active lock delay
        p->y++;
        p->lock_delay_active = false;
        return false;
    } else {
        // Collision detected - start or continue lock delay
        if (!p->lock_delay_active) {
            // Start the lock delay timer
            p->lock_delay_active = true;
            p->lock_delay_start = current_tick;
            return false;  // Don't lock yet
        } else {
            // Check if lock delay has expired
            if ((uint32_t)(current_tick - p->lock_delay_start) >= LOCK_DELAY_TICKS) {
                // Lock delay expired - lock the piece
                lock_piece(p, x);
                p->lock_delay_active = false;
                bool gameover = spawn_new_piece(p);
                return gameover;
            }
            return false;  // Still in lock delay
        }
    }
}

void handle_left_right(Player *p, int key, int state, uint8_t *prev_left, uint8_t *prev_right)
{
    if (state == 1) {
        // LEFT
        if (key == KEY_LEFT) {
            if (*prev_left == 0) {
                if (!check_collision(p, p->x - 1, p->y)) {
                    p->x--;
                    // Reset lock delay if piece can now move down
                    if (!check_collision(p, p->x, p->y + 1)) {
                        p->lock_delay_active = false;
                    }
                }
            }
            *prev_left = 1;
            *prev_right = 0;
            return;
        }

        // RIGHT
        if (key == KEY_RIGHT) {
            if (*prev_right == 0) {
                if (!check_collision(p, p->x + 1, p->y)) {
                    p->x++;
                    // Reset lock delay if piece can now move down
                    if (!check_collision(p, p->x, p->y + 1)) {
                        p->lock_delay_active = false;
                    }
                }
            }
            *prev_right = 1;
            *prev_left = 0;
            return;
        }
    }

    *prev_left = 0;
    *prev_right = 0;
}


// stable gravity table
static const uint32_t gravity_table[] = {
    50000000, 45000000, 40000000, 35000000, 30000000,
    25000000, 20000000, 15000000, 10000000, 5000000
};

// Returns number of cleared lines
void clear_lines(Player *p) {
    // Scan from bottom to top
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        bool full = true;
        // Check if row y is full
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (p->grid[x][y] == 0) {
                full = false;
                break;
            }
        }
        if (full) {
            // Shift everything above down by 1
            for (int yy = y; yy > 0; yy--) {
                for (int x = 0; x < BOARD_WIDTH; x++) {
                    p->grid[x][yy] = p->grid[x][yy - 1];
                }
            }
            // Top row becomes empty
            for (int x = 0; x < BOARD_WIDTH; x++) {
                p->grid[x][0] = 0;
            }
            p->lines++;
            y++;  // re-check the same y index because rows shifted down
        }
    }
    p->linestot += p->lines;
    return;
}

void handle_rotate(Player *p, u8 key, u8 state) {
    if (state != 1) return;   // rotate only on key-down

    uint8_t old_rot = p->rot;

    if (key == KEY_ROTATE_CW) {
        // clockwise: +1 mod 4
        p->rot = (p->rot + 1) & 3;
    }
    else if (key == KEY_ROTATE_CCW) {
        // counterclockwise: -1 mod 4
        p->rot = (p->rot + 3) & 3;   // same as (rot - 1 + 4) % 4
    }
    else {
        return; // not a rotation key
    }

    // Check legality
    if (check_collision(p, p->x, p->y)) {
        p->rot = old_rot; // revert
    }
}

void handle_softdrop(Player *p, u8 key, u8 state, u8* prev_down)
{
    if (key == KEY_SOFTDROP && state == 1) {

        // Rising edge: just pressed now
        if (*prev_down == 0) {
            if (!check_collision(p, p->x, p->y + 1)) {
                p->y++;        // move 1 cell only once per key press
            }
        }

        *prev_down = 1;   // remember it's held
    }
    else {
        *prev_down = 0;   // key released
    }
}


// rising-edge harddrop
void handle_harddrop_edge(Player *p, u8 key, u8 state, u8 prev_state) {
    if (key != KEY_HARDDROP) return;
    if (!(prev_state == 0 && state == 1)) return; // only on 0 -> 1

    // move down until collision
    while (!check_collision(p, p->x, p->y + 1)) {
        p->y++;
    }
    lock_piece(p, p->x);
    clear_lines(p);
    spawn_new_piece(p);
}

void handle_rotate_edge(Player *p, u8 key, u8 state, u8 prev_state) {
    if (!(prev_state == 0 && state == 1)) return;
    uint8_t old_rot = p->rot;
    if (key == KEY_ROTATE_CW) {
        p->rot = (p->rot + 1) & 3;
    } else if (key == KEY_ROTATE_CCW) {
        p->rot = (p->rot + 3) & 3;
    } else {
        return;
    }
    if (check_collision(p, p->x, p->y)) {
        p->rot = old_rot;
    } else {
        // Successful rotation - reset lock delay if piece can now move down
        if (!check_collision(p, p->x, p->y + 1)) {
            p->lock_delay_active = false;
        }
    }
}
void apply_garbage(Player *p, uint8_t amount) {
    int hole = simple_rand(10);
    while (amount--) {
        // shift board up
        if(mods.messy_garbage){
            hole = simple_rand(10);
        }
        for (int y = 0; y < 19; y++) {
            for (int x = 0; x < 10; x++) {
                p->grid[x][y] = p->grid[x][y + 1];
            }
        }

        // make bottom row garbage
        for (int x = 0; x < 10; x++) {
            p->grid[x][19] = (x == hole) ? 0 : COLOR_GARB;  // 8 = garbage color
        }

        // FIX: Move piece up by 1 for each garbage line added
        // This happens inside the loop, so it moves up once per line
        p->y--;

        // FIX: Check if piece is now off the top of the board (game over condition)
        // or still colliding after moving up
        if (p->y < 0) {
            p->y = 0;  // clamp to top
            // If it's still colliding at y=0, the game should end on next gravity tick
        }
    }
}

void handle_hold(Player* p, u8 key, u8 state, u8 prev_state) {
    if(mods.no_hold){
        return;
    }
    if (key != KEY_HOLD) return;
    if (!(prev_state == 0 && state == 1)) return; // rising edge
    if (!p->can_hold){ //I want to make this so that the piece gets grayed out when you can no longer use hold
    	return;                     // only once per piece
    }

    uint8_t temp = p->piece;
    if (p->hold_piece == EMPTY_HOLD) {
        // no held piece yet
        p->hold_piece = temp;
        spawn_new_piece(p);
    } else {
        // swap current and held piece
        p->piece = p->hold_piece;
        p->hold_piece = temp;
        p->y = 0;
        p->x = (BOARD_WIDTH / 2) - 2;
        p->rot = 0;

        // FIX: Don't modify next_pieces array - it's already correct!
        // The next_pieces array is maintained by spawn_new_piece()
        // and should not be touched here
    }
    p->can_hold = false; // cannot hold again until next piece
}

uint32_t line_score(uint8_t lines) {
    switch(lines) {
        case 1: return 100;
        case 2: return 300;
        case 3: return 500;
        case 4: return 800;
        default: return 0;
    }
}

uint32_t pack8Nibbles(uint8_t nibbles[8]) {
    uint32_t result = 0;
    for (int i = 0; i < 8; i++) {
        result = (result << 4) | (nibbles[i] & 0xF); // mask to ensure 4 bits
    }
    return result;
}



uint8_t garbage_from_lines(uint8_t lines) {
    switch (lines) {
        case 2: return 1;
        case 3: return 2;
        case 4: return 4;
        default: return 0;
    }
}

#define HOLDNEXT ((volatile uint32_t*) 0x44A00000)

int main() {
    init_platform();

	XTmrCtr_Initialize(&Usb_timer, XPAR_TIMER_USB_AXI_DEVICE_ID);
	XTmrCtr_SetOptions(&Usb_timer, 0, 0x00000004UL);
	XTmrCtr_Start(&Usb_timer, 0);


    XUartLite_Initialize(&Uart, UART_DEVICE_ID);

    XGpio_Initialize(&P1KeycodeGpio, PLAYER_1_CODE_GPIO_ID);
    XGpio_SetDataDirection(&P1KeycodeGpio, 1, 0);

    XGpio_Initialize(&P2KeycodeGpio, PLAYER_2_CODE_GPIO_ID);
    XGpio_SetDataDirection(&P2KeycodeGpio, 1, 0);


    // UART packet assembly state
    u8 packet[3];
    int bytes_needed = 3;
    uint32_t base_gravity_ticks = 50000000;
    uint32_t gravity_ticks = 50000000;
    uint32_t lr_ticks = 75000; // 50 ms;
    uint32_t tick1;
    uint32_t tick2;
    uint8_t nib1[8];
    uint8_t nib2[8];
    uint32_t b1;
    uint32_t b2;
    int p1_ready = 0;
    int p2_ready = 0;

    while(1) {
    	u8 b;
		if (try_recv_byte(&b)) {
			packet[3 - bytes_needed] = b;
			bytes_needed--;

			if (bytes_needed == 0) {
				// full packet received
				process_input_event(packet[0], packet[1], packet[2]);
				bytes_needed = 3;
			}
		}
	 if (packet[2] == 1 && packet[1] == KEY_ENTER) {
			 if (packet[0] == 1) {
				 p1_ready = 1;
				 tick1 = XTmrCtr_GetValue(&Usb_timer, 0);
			 }
			 if (packet[0] == 2) {
				 p2_ready = 1;
				 tick2 = XTmrCtr_GetValue(&Usb_timer, 0);
			 }
		 }
     //GAME MODS
    if (packet[2] == 1 && packet[1] == KEY_FAST_GRAV_MOD) (mods.fast_grav = true);
    if (packet[2] == 1 && packet[1] == KEY_NO_HOLD_MOD) (mods.no_hold = true);
    if (packet[2] == 1 && packet[1] == KEY_MESSY_GARBAGE_MOD) (mods.messy_garbage = true);
    if (packet[2] == 1 && packet[1] == KEY_NO_GARBAGE_MOD) (mods.no_garbage = true);
    if (packet[2] == 1 && packet[1] == KEY_SINGLE_PLAYER_MOD) (mods.single_player = true);




	 if (p1_ready && p2_ready) {
		 rng_state = tick1 + tick2;
		 break;
	 }
    }
    init_piece_queue();

    Player P1 = {
        .addr = BOARD_P1,
        .piece = piece_queue[0],
        .y = 0,
        .x = 3,
        .rot = 0,
        .lines = 0,
        .next_piece_index = 1,
		.hold_piece = EMPTY_HOLD,
		.can_hold = true,
		.score = 0,
		.holdaddr = HOLDNEXT,
		.linestot = 0,
		.lock_delay_active = false,
		.lock_delay_start = 0
    };

    Player P2 = {
        .addr = BOARD_P2,
        .piece = piece_queue[0],
        .y = 0,
        .x = 3,
        .rot = 0,
        .lines = 0,
        .next_piece_index = 1,
		.hold_piece = EMPTY_HOLD,
		.can_hold = true,
		.score = 0,
		.holdaddr = (HOLDNEXT+6),
		.linestot = 0,
		.lock_delay_active = false,
		.lock_delay_start = 0
    };
    *(HOLDNEXT) = 0x01234561;
    *(HOLDNEXT+1) = 0x12340000;
    for (int i = 0; i < 5; i++) {
            P1.next_pieces[i] = piece_queue[P1.next_piece_index + i];
            P2.next_pieces[i] = piece_queue[P2.next_piece_index + i];

        }
//    Player P1, P2;
//    P1.addr = BOARD_P1;
//   P2.addr = BOARD_P2;
//   P1.piece = piece_queue[0];
//   P1.y = 0;
//   P1.x = 3;
//   P1.rot = 0;
//   P1.lines = 0;
//   P1.next_piece_index = 1;
//   P2.piece = piece_queue[0];
//   P2.y = 0;
//   P2.x = 3;
//   P2.rot = 0;
//   P2.lines = 0;
//   P2.next_piece_index = 1;
//
//   // init boards
   memset(P1.grid, 0, sizeof(P1.grid));
   memset(P2.grid, 0, sizeof(P2.grid));
   writeboard(&P1);

   if(!mods.single_player){
	   writeboard(&P2);
   } else {
	   draw_ECE385(&P2);
	   writeboard_raw(&P2);
   }


    uint32_t last_tick1 = XTmrCtr_GetValue(&Usb_timer, 0);
    uint32_t last_tick2 = XTmrCtr_GetValue(&Usb_timer, 0);

    // Maintain per-player key + state
    u8 key1 = 0, key2 = 0;
    u8 p1Down = 0, p2Down = 0;
    u8 prev_p1Down = 0, prev_p2Down = 0;

    while (1) {

        //-----------------------------
        // NON-BLOCKING UART INPUT
        //-----------------------------
        u8 b;
        if (try_recv_byte(&b)) {
            packet[3 - bytes_needed] = b;
            bytes_needed--;

            if (bytes_needed == 0) {
                // full packet received
                process_input_event(packet[0], packet[1], packet[2]);

                if (packet[0] == 1) {
                    key1      = packet[1];
                    p1Down    = packet[2];
                }
                else if (packet[0] == 2) {
                    key2      = packet[1];
                    p2Down    = packet[2];
                }

                bytes_needed = 3;
            }
        }

        //-----------------------------
        // LEFT/RIGHT, ROTATION, DROPS
        //-----------------------------
        uint32_t now = XTmrCtr_GetValue(&Usb_timer, 0);

        if ((uint32_t)(now - last_tick1) >= lr_ticks) {

            // movement
        	handle_left_right(&P1, key1, p1Down, &p1_left_prev, &p1_right_prev);
            if(!mods.single_player)
            	handle_left_right(&P2, key2, p2Down, &p2_left_prev, &p2_right_prev);

            // soft drop (held)
            handle_softdrop(&P1, key1, p1Down, &p1_down_prev);
            if(!mods.single_player)
            handle_softdrop(&P2, key2, p2Down, &p2_down_prev);

            // rotate (edge only)
            handle_rotate_edge(&P1, key1, p1Down, prev_p1Down);
            if(!mods.single_player)
            handle_rotate_edge(&P2, key2, p2Down, prev_p2Down);

            // hard drop (edge only)
            handle_harddrop_edge(&P1, key1, p1Down, prev_p1Down);
            if(!mods.single_player)
            handle_harddrop_edge(&P2, key2, p2Down, prev_p2Down);

            handle_hold(&P1, key1, p1Down, prev_p1Down);
            if(!mods.single_player) handle_hold(&P2, key2, p2Down, prev_p2Down);

            prev_p1Down = p1Down;
            if(!mods.single_player) prev_p2Down = p2Down;

            last_tick1 += lr_ticks;
        }

        //-----------------------------
        // GRAVITY TICK
        //-----------------------------
        if ((uint32_t)(now - last_tick2) >= gravity_ticks) {
        	bool g2 = false;
            bool g1 = apply_gravity(&P1, P1.x, now);
            if(!mods.single_player) g2 = apply_gravity(&P2, P2.x, now);

            clear_lines(&P1);
            clear_lines(&P2);

            uint8_t gsend1 = garbage_from_lines(P1.lines);
            uint8_t gsend2 = garbage_from_lines(P2.lines);

            if(!mods.no_garbage && !mods.single_player){
                if (gsend1 > 0) apply_garbage(&P2, gsend1);
                if (gsend2 > 0) apply_garbage(&P1, gsend2);
            }


            P1.score += line_score(P1.lines);
            if(!mods.single_player)
            P2.score += line_score(P2.lines);

            // Update gravity speed based on cumulative lines
            uint8_t total_lines = P1.linestot + P2.linestot; // or maintain a total_lines_cleared variable
            gravity_ticks = base_gravity_ticks;
            uint8_t level = total_lines / 10;   // advance every 10 lines
            if (level > 9) level = 9;
            gravity_ticks = gravity_table[level];

            if(mods.fast_grav){
                gravity_ticks = gravity_table[level] - 10000000;
            }

            P1.lines = 0;
            P2.lines = 0;
            last_tick2 += gravity_ticks;

            if (g1 || g2) break; // gameover
        }

        //-----------------------------
        // RENDER
        //-----------------------------
        writeboard(&P1);
        if(!mods.single_player)
            writeboard(&P2);

        if (!mods.single_player) {
            // full multiplayer nibble pack
            nib1[0] = P1.hold_piece;
            nib1[1] = P1.next_pieces[0];
            nib1[2] = P1.next_pieces[1];
            nib1[3] = P1.next_pieces[2];
            nib1[4] = P1.next_pieces[3];
            nib1[5] = P1.next_pieces[4];
            nib1[6] = P2.hold_piece;
            nib1[7] = P2.next_pieces[0];

            nib2[0] = P2.next_pieces[1];
            nib2[1] = P2.next_pieces[2];
            nib2[2] = P2.next_pieces[3];
            nib2[3] = P2.next_pieces[4];
        } else {
            // single player: fill P2 area with blanks or zeros
            nib1[0] = P1.hold_piece;
            nib1[1] = P1.next_pieces[0];
            nib1[2] = P1.next_pieces[1];
            nib1[3] = P1.next_pieces[2];
            nib1[4] = P1.next_pieces[3];
            nib1[5] = P1.next_pieces[4];
            nib1[6] = 0;  // P2 disabled
            nib1[7] = 0;

            nib2[0] = 0;
            nib2[1] = 0;
            nib2[2] = 0;
            nib2[3] = 0;
        }


        b1 = pack8Nibbles(nib1);
        b2 = pack8Nibbles(nib2);
        *(HOLDNEXT) = b1;
        *(HOLDNEXT + 1) = b2;

    }


    cleanup_platform();
    return 0;
}
