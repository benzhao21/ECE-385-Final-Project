#include <stdio.h>
#include "platform.h"
#include "xparameters.h"
#include "xuartlite.h"
#include "xgpio.h"

#define UART_DEVICE_ID XPAR_UARTLITE_0_DEVICE_ID //make sure to change baud rate to match in device manager
#define PLAYER_GPIO_ID XPAR_AXI_GPIO_0_DEVICE_ID //not needed
#define STATE_GPIO_ID XPAR_AXI_GPIO_1_DEVICE_ID //not needed
#define PLAYER_1_CODE_GPIO_ID XPAR_PLAYER1KEYCODE_DEVICE_ID //change to gpio on board
#define PLAYER_2_CODE_GPIO_ID XPAR_PLAYER2KEYCODE_DEVICE_ID  //change to gpio on board

XUartLite Uart;
XGpio PlayerGpio;
XGpio StateGpio;
XGpio P1KeycodeGpio;
XGpio P2KeycodeGpio;

static void recv_byte(u8 *dst) { //idk if we can use this for our case tbh because we dont have time to sit around and wait, maybe we can add a timeout?
    while (XUartLite_Recv(&Uart, dst, 1) == 0) {
        // wait for data
    }
}

int main() {
    init_platform();

    u8 player, key, state;
    u8 last_key = 0;
    u8 last_state = 0;
    int have_last = 0;

    XUartLite_Initialize(&Uart, UART_DEVICE_ID);

    XGpio_Initialize(&PlayerGpio, PLAYER_GPIO_ID);
    XGpio_SetDataDirection(&PlayerGpio, 1, 0x0);

    XGpio_Initialize(&StateGpio, STATE_GPIO_ID);
    XGpio_SetDataDirection(&StateGpio, 1, 0x0);

    XGpio_Initialize(&P1KeycodeGpio, PLAYER_1_CODE_GPIO_ID);
    XGpio_SetDataDirection(&P1KeycodeGpio, 1, 0x0000);

    XGpio_Initialize(&P2KeycodeGpio, PLAYER_2_CODE_GPIO_ID);
    XGpio_SetDataDirection(&P2KeycodeGpio, 1, 0x0000);


    while (1) {

        // Block until all 3 bytes arrive
        recv_byte(&player);
        recv_byte(&key);
        recv_byte(&state);

        // De-duplicate repeated events
        if (have_last && key == last_key && state == last_state) {
            continue;
        }

        last_key = key;
        last_state = state;
        have_last = 1;


        XGpio_DiscreteWrite(&PlayerGpio, 1, player & 0x01);

        XGpio_DiscreteWrite(&StateGpio, 1, state & 0x01);

        if(player == 1){
        	XGpio_DiscreteWrite(&P1KeycodeGpio, 1, key);
        } else if (player == 2){
        	XGpio_DiscreteWrite(&P2KeycodeGpio, 1, key);
        }



        u8 packet[3] = { player, key, state };

        // Wait until previous send is done
        while (XUartLite_IsSending(&Uart)); //don't need to send data back except for debugging
        XUartLite_Send(&Uart, packet, 3);
    }

    cleanup_platform();
    return 0;
}
