module waterfall_rom (
	input logic clock,
	input logic [18:0] address,
	output logic [2:0] q
);

logic [2:0] memory [0:307199] /* synthesis ram_init_file = "./waterfall/waterfall.COE" */;

always_ff @ (posedge clock) begin
	q <= memory[address];
end

endmodule
