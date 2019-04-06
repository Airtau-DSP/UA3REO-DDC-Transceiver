module DAC_corrector(
clk_in,
DATA_IN,
DATA_OUT
);

input clk_in;
input signed [31:0] DATA_IN;
output reg [13:0] DATA_OUT;

reg signed [40:0] tmp1=0;
reg signed [13:0] tmp2=0;

always @ (posedge clk_in)
begin
	tmp1=DATA_IN*2;
	tmp2={DATA_IN[31],tmp1[31:19]};
	DATA_OUT=tmp2+'d8191;
end


endmodule
