module nco_shift (
	in,
	out
);

input	[15:0] in;
output [11:0] out;

assign out[11:0]=in[15:4];
	
endmodule
