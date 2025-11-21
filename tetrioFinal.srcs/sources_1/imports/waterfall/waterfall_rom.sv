module waterfall_rom (
	input logic clock,
	input logic [6:0] address,
	output logic [1:0] q
);

logic [1:0] memory [0:99] /* synthesis ram_init_file = "./waterfall/waterfall.COE" */;

always_ff @ (posedge clock) begin
	q <= memory[address];
end

endmodule
