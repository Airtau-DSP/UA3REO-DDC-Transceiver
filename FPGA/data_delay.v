//фильтр скользящего среднего

module data_delay(
clk_in,
data_in,
data_out
);

parameter bus_length = 16;
parameter delay_length = 32;

input clk_in;
input [bus_length-1:0] data_in;
output [bus_length-1:0] data_out;

reg [bus_length-1: 0] fifo_buf [delay_length-1: 0];

integer k;

assign data_out[bus_length-1: 0] = fifo_buf[delay_length-1];

always @ (posedge clk_in)
begin
	for (k = 1; k <= delay_length-1; k = k+1)
	begin 
		fifo_buf[k] <= fifo_buf[k-1]; 
		fifo_buf[0] <= data_in; 
	end

end

endmodule

