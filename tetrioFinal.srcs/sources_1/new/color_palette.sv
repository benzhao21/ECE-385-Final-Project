`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 11/20/2025 03:33:55 PM
// Design Name: 
// Module Name: color_palette
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
module color_palette(
    input  logic [3:0] piece,          // 0-9 used
    output logic [3:0] red,
    output logic [3:0] green,
    output logic [3:0] blue
);

    // 12-bit RGB = {R[3:0], G[3:0], B[3:0]}
    localparam logic [11:0] PALETTE [0:15] = '{
        // 0-9 defined, 10-15 default black
        12'hF00,   // 0: Red
        12'h0F0,   // 1: Green
        12'h008,   // 2: Dark Blue
        12'hF70,   // 3: Orange
        12'h6CF,   // 4: Sky Blue
        12'h90C,   // 5: Purple
        12'hFF0,   // 6: Yellow
        12'h000,   // 7: Black
        12'h888,   // 8: Gray
        12'hFFF,   // 9: White
        12'h000,   // 10: unused
        12'h000,   // 11: unused
        12'h000,   // 12: unused
        12'h000,   // 13: unused
        12'h000,   // 14: unused
        12'h000    // 15: unused
    };

    logic [11:0] rgb;

    always_comb begin
        rgb = PALETTE[piece];        // 12-bit color output

        red   = rgb[11:8];
        green = rgb[7:4];
        blue  = rgb[3:0];
    end

endmodule
