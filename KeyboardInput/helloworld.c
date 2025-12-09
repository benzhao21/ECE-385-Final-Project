#include <stdio.h>
#include "platform.h"
#include "xparameters.h"
#include "xuartlite.h"
#include "xgpio.h"

#define UART_DEVICE_ID XPAR_UARTLITE_0_DEVICE_ID
#define PLAYER_GPIO_ID XPAR_AXI_GPIO_0_DEVICE_ID
#define STATE_GPIO_ID XPAR_AXI_GPIO_1_DEVICE_ID
#define PLAYER_1_CODE_GPIO_ID XPAR_PLAYER1KEYCODE_DEVICE_ID
#define PLAYER_2_CODE_GPIO_ID XPAR_PLAYER2KEYCODE_DEVICE_ID

XUartLite Uart;
XGpio PlayerGpio;
XGpio StateGpio;
XGpio P1KeycodeGpio;
XGpio P2KeycodeGpio;

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

    // Write GPIO outputs
    XGpio_DiscreteWrite(&PlayerGpio, 1, player & 0x01);
    XGpio_DiscreteWrite(&StateGpio, 1, state & 0x01);

    if (player == 1) {
        XGpio_DiscreteWrite(&P1KeycodeGpio, 1, key);
    } else if (player == 2) {
        XGpio_DiscreteWrite(&P2KeycodeGpio, 1, key);
    }
}

// -----------------------------------------------------

int main() {
    init_platform();

    XUartLite_Initialize(&Uart, UART_DEVICE_ID);

    XGpio_Initialize(&PlayerGpio, PLAYER_GPIO_ID);
    XGpio_SetDataDirection(&PlayerGpio, 1, 0);

    XGpio_Initialize(&StateGpio, STATE_GPIO_ID);
    XGpio_SetDataDirection(&StateGpio, 1, 0);

    XGpio_Initialize(&P1KeycodeGpio, PLAYER_1_CODE_GPIO_ID);
    XGpio_SetDataDirection(&P1KeycodeGpio, 1, 0);

    XGpio_Initialize(&P2KeycodeGpio, PLAYER_2_CODE_GPIO_ID);
    XGpio_SetDataDirection(&P2KeycodeGpio, 1, 0);

    // UART packet assembly state
    u8 packet[3];
    int bytes_needed = 3;

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

        // ---- GAME LOGIC ALWAYS RUNNING ----
        // update physics, animations, timers, etc
        // update_game_logic();
        // render_frame();
    }

    cleanup_platform();
    return 0;
}
