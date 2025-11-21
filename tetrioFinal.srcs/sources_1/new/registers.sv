`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 11/20/2025 02:41:30 PM
// Design Name: 
// Module Name: registers
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////

module registers(
    input  logic [5:0]  gridaddr,
    output logic [31:0] piece
);

    logic [31:0] regs [0:49];

    localparam logic [31:0] EVEN_VAL = 32'h0123_4567;
    localparam logic [31:0] ODD_VAL  = 32'h89A0_1234;

    always_comb begin
        // assign all 50 registers
        for (int i = 0; i < 50; i++) begin
            if (i % 2 == 0)
                regs[i] = EVEN_VAL;   // even index
            else
                regs[i] = ODD_VAL;    // odd index
        end

        // output selected register
        piece = regs[gridaddr];
    end

endmodule
