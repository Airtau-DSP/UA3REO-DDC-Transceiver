module rx_mixer_shift (
	in,
	out
);

input	[23:0]  in;
output	[22:0]  out;

assign out[22:0]=in[22:0];
	
endmodule
