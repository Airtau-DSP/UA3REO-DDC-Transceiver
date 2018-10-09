module stm32_interface(
clk_in,
I,
Q,
DATA_IN,

DATA_SYNC,
DATA_OUT,
freq_out,
preamp_enable,
rx,
tx,
hilbert
);

input clk_in;
input signed [15:0] I;
input signed [15:0] Q;
input [3:0] DATA_IN;
input DATA_SYNC;

output reg [3:0] DATA_OUT;
output reg unsigned [21:0] freq_out=620407;
output reg preamp_enable=0;
output reg rx=1;
output reg tx=0;
output reg hilbert=0;

integer k=1;
reg unsigned [15:0] I_HOLD;
reg unsigned [15:0] Q_HOLD;

always @ (posedge clk_in)
begin
	//начало передачи
	if (k==1 || DATA_SYNC==1)
	begin
		I_HOLD<=I+'d32767;
		Q_HOLD<=Q+'d32767;
		preamp_enable<=DATA_IN[2:2];
		hilbert<=DATA_IN[1:1];
		if(DATA_IN[3:3]==1)
		begin
			tx<=1;
			rx<=0;
		end
		else
		begin
			tx<=0;
			rx<=1;
		end
		k<=2;
	end
	else if (k==2)
	begin
		freq_out[21:20]<=DATA_IN[1:0];
		DATA_OUT[3:0]=Q_HOLD[15:12];
		k<=3;
	end
	else if (k==3)
	begin
		freq_out[19:16]<=DATA_IN[3:0];
		DATA_OUT[3:0]<=Q_HOLD[11:8];
		k<=4;
	end
	else if (k==4)
	begin
		freq_out[15:12]<=DATA_IN[3:0];
		DATA_OUT[3:0]<=Q_HOLD[7:4];
		k<=5;
	end
	else if (k==5)
	begin
		freq_out[11:8]<=DATA_IN[3:0];
		DATA_OUT[3:0]<=Q_HOLD[3:0];
		k<=6;
	end
	else if (k==6)
	begin
		freq_out[7:4]<=DATA_IN[3:0];
		DATA_OUT[3:0]<=I_HOLD[15:12];
		k<=7;
	end
	else if (k==7)
	begin
		freq_out[3:0]<=DATA_IN[3:0];
		DATA_OUT[3:0]<=I_HOLD[11:8];
		k<=8;
	end
	else if (k==8)
	begin
		DATA_OUT[3:0]<=I_HOLD[7:4];
		k<=9;
	end
	else if (k==9)
	begin
		DATA_OUT[3:0]<=I_HOLD[3:0];
		k<=10;
	end
end


endmodule
