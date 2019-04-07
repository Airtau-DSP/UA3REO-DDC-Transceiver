module nco_shift (
	in,
	out
);

input	[13:0] in;
output [11:0] out;

assign out[11:0]=in[13:2];
	
endmodule
