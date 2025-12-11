
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

#define KEY_LEFT   0x25
#define KEY_RIGHT  0x27
#define KEY_ROTATE_CW 0x26
#define KEY_ROTATE_CCW 0x5A
#define KEY_ENTER  0xD
#define KEY_SOFTDROP   0x28
#define KEY_HARDDROP   0x20



#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20

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
} Player;

XUartLite Uart;
XGpio P1KeycodeGpio;
XGpio P2KeycodeGpio;
XTmrCtr Usb_timer;


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

void writeboard(Player* p) {
    uint32_t temp;
    uint8_t idx;
    uint8_t piece_cell;

    for(int j = 0; j < 20; j++) {
        for(int i = 0; i < 10; i++) {
            idx = i + j * 10;

            if(idx % 8 == 0) temp = 0;

            // Check if the current piece occupies this cell
            piece_cell = 0; // default
            int rel_x = i - p->x; // relative to piece top-left
            int rel_y = j - p->y;

            if(rel_x >= 0 && rel_x < 4 && rel_y >= 0 && rel_y < 4) {
                piece_cell = TETROMINOES[p->piece][p->rot][rel_y][rel_x];
            }

            // Overlay piece on grid using OR
            temp += (p->grid[i][j] | piece_cell) << (4 * (7 - (idx % 8)));

            if(idx % 8 == 7) {
                *(p->addr + idx / 8) = temp;
            }
        }
    }
}

// returns true if collision, false if no collision
bool check_collision(Player *p, int x, int y) {
    uint8_t (*shape)[4] = TETROMINOES[p->piece][p->rot]; // current piece, 0° orientation

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

uint8_t simple_rand7() {
    rng_state = rng_state * 1664525 + 1013904223; // LCG
    return (rng_state >> 16) % 7; // return 0..6
}

//returns true if game over
bool spawn_new_piece(Player* p) {
    p->piece = simple_rand7();      // random tetromino
    p->y = 0;                       // top of board
    p->x = (BOARD_WIDTH / 2) - 2;   // center horizontally
    p->rot = 0;
    // Optional: check for collision/game over
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
int clear_lines(Player *p) {
    int lines_cleared = 0;
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
            lines_cleared++;
            y++;  // re-check the same y index because rows shifted down
        }
    }

    return lines_cleared;
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
//
//
//
//void handle_harddrop(Player *p, u8 key, u8 state) {
//    if (state != 1) return; // act only on key-down
//    if (key != KEY_HARDDROP) return;
//    while (!check_collision(p, p->x, p->y + 1)) {
//            p->y++;
//        }
//        // Lock piece where it stopped
//        lock_piece(p, p->x);
//        // Clear completed lines
//        clear_lines(p);
//        // Spawn the next piece
//        spawn_new_piece(p);
//}

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
    u8 key1, key2;
    u8 p1Down, p2Down;
    u8 prev_p1Down = 0;
    u8 prev_p2Down = 0;
    Player P1, P2;

    uint32_t gravity_ticks = 50000000; // 500 ms at 100 MHz
    uint32_t lr_ticks = 10000000; // 50 ms;
    uint32_t tick1;
    uint32_t tick2;
    uint8_t lines1;
    uint8_t lines2;
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
	 if (p1_ready && p2_ready) {
		 rng_state = tick1 + tick2;
		 break;
	 }
    }

    P1.addr = BOARD_P1;
   P2.addr = BOARD_P2;
   P1.piece = simple_rand7();
   P1.y = 0;
   P1.x = 3;
   P1.rot = 0;
   P2.piece = simple_rand7();
   P2.y = 0;
   P2.x = 3;
   P2.rot = 0;

   // init boards
   memset(P1.grid, 0, sizeof(P1.grid));
   memset(P2.grid, 0, sizeof(P2.grid));
   writeboard(&P1);
   writeboard(&P2);

    uint32_t last_tick1 = XTmrCtr_GetValue(&Usb_timer, 0);
    uint32_t last_tick2 = XTmrCtr_GetValue(&Usb_timer, 0);
    bool gameover1;
    bool gameover2;
    bool gameover;
    while (1) {

        // ---- NON-BLOCKING UART RECEIVE ----
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
        if(packet[0] == 1) {
        	key1 = packet[1];
        	p1Down = packet[2];
        	p2Down = 0;

        }
        if(packet[0] == 2 ) {
        	key2 = packet[1];
        	p1Down = 0;
        	p2Down = packet[2];
        }

        uint32_t now = XTmrCtr_GetValue(&Usb_timer, 0);
        if ((uint32_t)(now - last_tick1) >= lr_ticks) {
        	 handle_left_right(&P1,key1, p1Down);
        	 handle_left_right(&P2,key2, p2Down);
        	    handle_rotate_edge(&P1, key1, p1Down, prev_p1Down);
        	    handle_rotate_edge(&P2, key2, p2Down, prev_p2Down);
        	 handle_softdrop(&P1,key1,p1Down);
			handle_softdrop(&P2,key2,p2Down);
		    handle_harddrop_edge(&P1, key1, p1Down, prev_p1Down);
		    handle_harddrop_edge(&P2, key2, p2Down, prev_p2Down);
        	 last_tick1 += lr_ticks;
        }
        if ((uint32_t)(now - last_tick2) >= gravity_ticks) {
              gameover1 = apply_gravity(&P1, P1.x); // move piece down
              gameover2 = apply_gravity(&P2, P2.x); // move piece down
              lines1 = clear_lines(&P1);
              lines2 = clear_lines(&P2);
              last_tick2 += gravity_ticks; // keep in sync
          }

        // ---- GAME LOGIC ALWAYS RUNNING ----
        writeboard(&P1);
        writeboard(&P2);
        gameover = gameover1 || gameover2;
        if(gameover) {
        	break;
        }
    }


    cleanup_platform();
    return 0;
}
