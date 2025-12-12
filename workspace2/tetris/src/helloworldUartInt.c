
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


#define MAX_PIECES 1000
#define EMPTY_HOLD 255


#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20

// ----------------------
// UART INTERRUPT FIFO
// ----------------------
#define UART_FIFO_SIZE 256
volatile u8 uart_fifo[UART_FIFO_SIZE];
volatile int uart_head = 0;
volatile int uart_tail = 0;

static inline int uart_fifo_empty() {
    return uart_head == uart_tail;
}

static inline void uart_fifo_push(u8 b) {
    int next = (uart_head + 1) & (UART_FIFO_SIZE - 1);
    if (next != uart_tail) {   // drop when full (should not happen)
        uart_fifo[uart_head] = b;
        uart_head = next;
    }
}

static inline int uart_fifo_pop(u8 *b) {
    if (uart_fifo_empty()) return 0;
    *b = uart_fifo[uart_tail];
    uart_tail = (uart_tail + 1) & (UART_FIFO_SIZE - 1);
    return 1;
}



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


typedef struct{
	uint8_t grid[10][20];
	volatile uint32_t* addr;
	uint8_t piece; // 0, 1, 2, ... same order as tetrominoes
	int16_t x; // signed now
	int16_t y; // signed now
	uint8_t rot; // 0 ,1 ,2,3 clockwise rotations
	uint8_t lines;
	uint16_t next_piece_index;
	uint32_t score;
	volatile uint32_t* holdaddr;

	 uint8_t hold_piece;         // 0-6, 255 for empty
	 bool can_hold;              // true if player can hold
	 uint8_t next_pieces[5];
} Player;

uint8_t piece_queue[MAX_PIECES];

// Initialize once at game start

XUartLite Uart;
XGpio P1KeycodeGpio;
XGpio P2KeycodeGpio;
XTmrCtr Usb_timer;

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
    uint8_t (*shape)[4] = TETROMINOES[p->piece][p->rot]; // current piece, 0� orientation

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
bool apply_gravity(Player* p, int x) {
    if(!check_collision(p, x, p->y + 1)) {
        // Move down
        p->y++;
        return false; // piece not locked yet
    } else {
        // Collision detected lock piece
        lock_piece(p, x);
        bool gameover = spawn_new_piece(p);
        return gameover;
    }
}

void handle_left_right(Player* p, u8 key, u8 state) {
    if (state != 1) return; // move only on key-down

    if (key == KEY_LEFT) {
        if (!check_collision(p, p->x - 1, p->y))
            p->x--;
    }
    else if (key == KEY_RIGHT) {
        if (!check_collision(p, p->x + 1, p->y))
            p->x++;
    }
}

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

void handle_softdrop(Player *p, u8 key, u8 state) {
    if (state != 1) return; // only act on key-down

    if (key != KEY_SOFTDROP) return;

    // Try moving piece down by 1
    if (!check_collision(p, p->x, p->y + 1)) {
        p->y++;
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

// optional: rising-edge rotate (if you want single rotation per press)
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
    }
}
void apply_garbage(Player *p, uint8_t amount) {
    int hole = simple_rand(10);
    while (amount--) {
        // shift board up
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
    if (key != KEY_HOLD) return;
    if (!(prev_state == 0 && state == 1)) return; // rising edge
    if (!p->can_hold) return;                     // only once per piece

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

void UartHandler(void *CallBackRef, unsigned int ByteCount)
{
    XUartLite *Inst = (XUartLite*)CallBackRef;
    u8 buf[16];

    int n = XUartLite_Recv(Inst, buf, sizeof(buf));
    for (int i = 0; i < n; i++) {
        uart_fifo_push(buf[i]);
    }
}


#define HOLDNEXT ((volatile uint32_t*) 0x44A00000)

int main() {
    init_platform();

	XTmrCtr_Initialize(&Usb_timer, XPAR_TIMER_USB_AXI_DEVICE_ID);
	XTmrCtr_SetOptions(&Usb_timer, 0, 0x00000004UL);
	XTmrCtr_Start(&Usb_timer, 0);


    XUartLite_Initialize(&Uart, UART_DEVICE_ID);
    XUartLite_SetRecvHandler(&Uart, UartHandler, &Uart);
    XUartLite_EnableInterrupt(&Uart);

    XGpio_Initialize(&P1KeycodeGpio, PLAYER_1_CODE_GPIO_ID);
    XGpio_SetDataDirection(&P1KeycodeGpio, 1, 0);

    XGpio_Initialize(&P2KeycodeGpio, PLAYER_2_CODE_GPIO_ID);
    XGpio_SetDataDirection(&P2KeycodeGpio, 1, 0);

    microblaze_enable_interrupts();


    // UART packet assembly state
    u8 packet[3];
    int bytes_needed = 3;
    uint32_t base_gravity_ticks = 50000000;
    uint32_t gravity_ticks = 50000000;
    uint32_t lr_ticks = 10000000; // 50 ms;
    uint32_t tick1;
    uint32_t tick2;
    uint8_t nib1[8];
    uint8_t nib2[8];
    uint32_t b1;
    uint32_t b2;
    int p1_ready = 0;
    int p2_ready = 0;

    // Maintain per-player key + state
    u8 key1 = 0, key2 = 0;
    u8 p1Down = 0, p2Down = 0;
    u8 prev_p1Down = 0, prev_p2Down = 0;

    while(1) {
    	while (uart_fifo_pop(&b)) {
            packet[3 - bytes_needed] = b;
            bytes_needed--;

            if (bytes_needed == 0) {
                process_input_event(packet[0], packet[1], packet[2]);
                if (packet[0] == 1) { key1 = packet[1]; p1Down = packet[2]; }
                if (packet[0] == 2) { key2 = packet[1]; p2Down = packet[2]; }
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
		.holdaddr = HOLDNEXT
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
		.holdaddr = (HOLDNEXT+6)
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
   writeboard(&P2);

    uint32_t last_tick1 = XTmrCtr_GetValue(&Usb_timer, 0);
    uint32_t last_tick2 = XTmrCtr_GetValue(&Usb_timer, 0);

    while (1) {

        //-----------------------------
        // NON-BLOCKING UART INPUT
        //-----------------------------
        while (uart_fifo_pop(&b)) {
            packet[3 - bytes_needed] = b;
            bytes_needed--;

            if (bytes_needed == 0) {
                process_input_event(packet[0], packet[1], packet[2]);
                if (packet[0] == 1) { key1 = packet[1]; p1Down = packet[2]; }
                if (packet[0] == 2) { key2 = packet[1]; p2Down = packet[2]; }
                bytes_needed = 3;
            }
        }

        //-----------------------------
        // LEFT/RIGHT, ROTATION, DROPS
        //-----------------------------
        uint32_t now = XTmrCtr_GetValue(&Usb_timer, 0);

        if ((uint32_t)(now - last_tick1) >= lr_ticks) {

            // movement
            handle_left_right(&P1, key1, p1Down);
            handle_left_right(&P2, key2, p2Down);

            // soft drop (held)
            handle_softdrop(&P1, key1, p1Down);
            handle_softdrop(&P2, key2, p2Down);

            // rotate (edge only)
            handle_rotate_edge(&P1, key1, p1Down, prev_p1Down);
            handle_rotate_edge(&P2, key2, p2Down, prev_p2Down);

            // hard drop (edge only)
            handle_harddrop_edge(&P1, key1, p1Down, prev_p1Down);
            handle_harddrop_edge(&P2, key2, p2Down, prev_p2Down);

            handle_hold(&P1, key1, p1Down, prev_p1Down);
            handle_hold(&P2, key2, p2Down, prev_p2Down);

            prev_p1Down = p1Down;
             prev_p2Down = p2Down;

            last_tick1 += lr_ticks;
        }

        //-----------------------------
        // GRAVITY TICK
        //-----------------------------
        if ((uint32_t)(now - last_tick2) >= gravity_ticks) {

            bool g1 = apply_gravity(&P1, P1.x);
            bool g2 = apply_gravity(&P2, P2.x);

            clear_lines(&P1);
            clear_lines(&P2);

            uint8_t gsend1 = garbage_from_lines(P1.lines);
            uint8_t gsend2 = garbage_from_lines(P2.lines);
            if (gsend1 > 0) apply_garbage(&P2, gsend1);
            if (gsend2 > 0) apply_garbage(&P1, gsend2);

            P1.score += line_score(P1.lines);
            P2.score += line_score(P2.lines);

            // Update gravity speed based on cumulative lines
            uint8_t total_lines = P1.lines + P2.lines; // or maintain a total_lines_cleared variable
            gravity_ticks = base_gravity_ticks;
            uint8_t speed_level = total_lines / 10;
            for(int i=0; i<speed_level; i++) gravity_ticks = gravity_ticks * 9 / 10;
            if(gravity_ticks < 5000000) gravity_ticks = 5000000;

            P1.lines = 0;
            P2.lines = 0;
            last_tick2 += gravity_ticks;

            if (g1 || g2) break; // gameover
        }

        //-----------------------------
        // RENDER
        //-----------------------------
        writeboard(&P1);
        writeboard(&P2);

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

        b1 = pack8Nibbles(nib1);
        b2 = pack8Nibbles(nib2);
        *(HOLDNEXT) = b1;
        *(HOLDNEXT + 1) = b2;

    }


    cleanup_platform();
    return 0;
}
