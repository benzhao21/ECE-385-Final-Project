module waterfall_example (
	input logic vga_clk,
	input logic [9:0] DrawX, DrawY,
	input logic blank,
	output logic [3:0] red, green, blue
);

logic [16:0] rom_address;
logic [3:0] rom_q;

logic [3:0] palette_red, palette_green, palette_blue;

logic negedge_vga_clk;

// read from ROM on negedge, set pixel on posedge
assign negedge_vga_clk = ~vga_clk;

// address into the rom = (x*xDim)/640 + ((y*yDim)/480) * xDim
// this will stretch out the sprite across the entire screen
assign rom_address = ((DrawX * 300) / 640) + (((DrawY * 300) / 480) * 300);

always_ff @ (posedge vga_clk) begin
	red <= 4'h0;
	green <= 4'h0;
	blue <= 4'h0;

	if (blank) begin
	   if(ingrid) begin
	       red <= grid_red;
	       green <= grid_green;
	       blue <= grid_blue;
	   end
	   else begin
            red <= palette_red;
            green <= palette_green;
            blue <= palette_blue;
		end
	end
end

waterfall_rom waterfall_rom (
	.clka   (negedge_vga_clk),
	.addra (rom_address),
	.douta       (rom_q)
);

waterfall_palette waterfall_palette (
	.index (rom_q),
	.red   (palette_red),
	.green (palette_green),
	.blue  (palette_blue)
);

logic ingrid;
logic ingrid1x;
logic ingrid2x;
assign ingrid1x = (80 <= DrawX)  && (DrawX <= 239);
assign ingrid2x = (400 <= DrawX)  && (DrawX <= 559);
assign ingrid = ( ingrid1x || ingrid2x) && ((80 <= DrawY) && (DrawY <= 399));

logic [4:0] row;
logic [3:0] col;
always_comb begin
    row = 0;
    col = 0;
    if(ingrid) begin
        row = ((DrawY - 80) >> 4);
        if(ingrid1x)
            col = ((DrawX - 80) >> 4);
        else if (ingrid2x)
            col = ((DrawX - 400) >> 4);
    end
end

logic [5:0] gridaddr;
logic [7:0] gridnum;
logic [2:0] regidx;
logic [31:0] piece;
logic [3:0] pieceidx;

always_comb begin
    gridnum = col + 10*row;
    gridaddr = (gridnum) >> 3;
    if(ingrid2x)
        gridaddr = gridaddr + 25;
    regidx = gridnum[2:0];
end

always_comb begin
    case(regidx)
        3'h0: pieceidx = piece[31:28];
        3'h1: pieceidx = piece[27:24];
        3'h2: pieceidx = piece[23:20];
        3'h3: pieceidx = piece[19:16];
        3'h4: pieceidx = piece[15:12];
        3'h5: pieceidx = piece[11:8];
        3'h6: pieceidx = piece[7:4];
        3'h7: pieceidx  = piece[3:0];
    endcase
end

registers gridregs(
    .gridaddr(gridaddr),
    .piece(piece)
);

logic[3:0] grid_red, grid_green, grid_blue;
color_palette tetris_palette(
    .piece(pieceidx),
    .red(grid_red),
    .green(grid_green),
    .blue(grid_blue)
);


endmodule
