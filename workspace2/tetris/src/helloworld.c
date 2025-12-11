/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

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
#include <stdbool.h>

#define UART_DEVICE_ID XPAR_UARTLITE_0_DEVICE_ID
#define PLAYER_1_CODE_GPIO_ID XPAR_PLAYER1KEYCODE_DEVICE_ID
#define PLAYER_2_CODE_GPIO_ID XPAR_PLAYER2KEYCODE_DEVICE_ID

XUartLite Uart;
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

    if (player == 1) {
        XGpio_DiscreteWrite(&P1KeycodeGpio, 1, key);
    } else if (player == 2) {
        XGpio_DiscreteWrite(&P2KeycodeGpio, 1, key);
    }
}

int main() {
    init_platform();

    XUartLite_Initialize(&Uart, UART_DEVICE_ID);

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

    }

    cleanup_platform();
    return 0;
}
