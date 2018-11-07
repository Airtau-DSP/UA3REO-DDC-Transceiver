module stm32_interface(
clk_in,
I,
Q,
DATA_IN,
DATA_SYNC,
ADC_OTR,

DATA_OUT,
freq_out,
preamp_enable,
rx,
tx,
TX_I,
TX_Q,
audio_clk_en,
stage_debug
);

input clk_in;
input signed [15:0] I;
input signed [15:0] Q;
input [3:0] DATA_IN;
input DATA_SYNC;
input ADC_OTR;

output reg [3:0] DATA_OUT;
output reg unsigned [21:0] freq_out=620407;
output reg preamp_enable=0;
output reg rx=1;
output reg tx=0;
output reg audio_clk_en=0;
output reg signed [15:0] TX_I=0;
output reg signed [15:0] TX_Q=0;
output reg [15:0] stage_debug=0;

reg signed [15:0] k=1;
reg signed [15:0] I_HOLD;
reg signed [15:0] Q_HOLD;

always @ (posedge clk_in)
begin
	//начало передачи
	if (DATA_SYNC==1)
	begin
		if(DATA_IN[3:0]=='d1) //GET PARAMS
		begin
			k=100;
		end
		if(DATA_IN[3:0]=='d2) //SEND PARAMS
		begin
			k=200;
		end
		if(DATA_IN[3:0]=='d3) //TX IQ
		begin
			k=300;
		end
		if(DATA_IN[3:0]=='d4) //RX IQ
		begin
			k=400;
		end
		if(DATA_IN[3:0]=='d5) //AUDIO PLL ON
		begin
			audio_clk_en=1;
			k=999;
		end
		if(DATA_IN[3:0]=='d6) //AUDIO PLL OFF
		begin
			audio_clk_en=0;
			k=999;
		end
		if(DATA_IN[3:0]=='d10) //TEST BUS
		begin
			k=500;
		end
	end
	else if (k==100) //GET PARAMS
	begin
		preamp_enable=DATA_IN[2:2];
		if(DATA_IN[3:3]==1)
		begin
			tx=1;
			rx=0;
		end
		else
		begin
			tx=0;
			rx=1;
		end
		k=101;
	end
	else if (k==101)
	begin
		freq_out[21:20]=DATA_IN[1:0];
		k=102;
	end
	else if (k==102)
	begin
		freq_out[19:16]=DATA_IN[3:0];
		k=103;
	end
	else if (k==103)
	begin
		freq_out[15:12]=DATA_IN[3:0];
		k=104;
	end
	else if (k==104)
	begin
		freq_out[11:8]=DATA_IN[3:0];
		k=105;
	end
	else if (k==105)
	begin
		freq_out[7:4]=DATA_IN[3:0];
		k=106;
	end
	else if (k==106)
	begin
		freq_out[3:0]=DATA_IN[3:0];
		k=999;
	end
	else if (k==200) //SEND PARAMS
	begin
		DATA_OUT[3:3]=0;
		DATA_OUT[2:2]=0;
		DATA_OUT[1:1]=0;
		DATA_OUT[0:0]=ADC_OTR;
		k=999;
	end
	else if (k==300) //TX IQ
	begin
		I_HOLD='d0;
		Q_HOLD='d0;
		Q_HOLD[15:12]=DATA_IN[3:0];
		k=301;
	end
	else if (k==301)
	begin
		Q_HOLD[11:8]=DATA_IN[3:0];
		k=302;
	end
	else if (k==302)
	begin
		Q_HOLD[7:4]=DATA_IN[3:0];
		k=303;
	end
	else if (k==303)
	begin
		Q_HOLD[3:0]=DATA_IN[3:0];
		k=304;
	end
	else if (k==304)
	begin
		I_HOLD[15:12]=DATA_IN[3:0];
		k=305;
	end
	else if (k==305)
	begin
		I_HOLD[11:8]=DATA_IN[3:0];
		k=306;
	end
	else if (k==306)
	begin
		I_HOLD[7:4]=DATA_IN[3:0];
		k=307;
	end
	else if (k==307)
	begin
		I_HOLD[3:0]=DATA_IN[3:0];
		TX_I[15:0]=I_HOLD[15:0];
		TX_Q[15:0]=Q_HOLD[15:0];
		k=999;
	end
	else if (k==400) //RX IQ
	begin
		I_HOLD=I; //+'d32767;
		Q_HOLD=Q; //+'d32767;
		DATA_OUT[3:0]=Q_HOLD[15:12];
		k=401;
	end
	else if (k==401)
	begin
		DATA_OUT[3:0]=Q_HOLD[11:8];
		k=402;
	end
	else if (k==402)
	begin
		DATA_OUT[3:0]=Q_HOLD[7:4];
		k=403;
	end
	else if (k==403)
	begin
		DATA_OUT[3:0]=Q_HOLD[3:0];
		k=404;
	end
	else if (k==404)
	begin
		DATA_OUT[3:0]=I_HOLD[15:12];
		k=405;
	end
	else if (k==405)
	begin
		DATA_OUT[3:0]=I_HOLD[11:8];
		k=406;
	end
	else if (k==406)
	begin
		DATA_OUT[3:0]=I_HOLD[7:4];
		k=407;
	end
	else if (k==407)
	begin
		DATA_OUT[3:0]=I_HOLD[3:0];
		k=999;
	end
	else if (k==500) //TEST BUS
	begin
		DATA_OUT[3:0]=DATA_IN[3:0];
		k=501;
	end
	else if (k==501)
	begin
		DATA_OUT[3:0]=DATA_IN[3:0];
		k=502;
	end
	else if (k==502)
	begin
		DATA_OUT[3:0]=DATA_IN[3:0];
		k=503;
	end
	else if (k==503)
	begin
		DATA_OUT[3:0]=DATA_IN[3:0];
		k=999;
	end
	stage_debug=k;
end


endmodule
