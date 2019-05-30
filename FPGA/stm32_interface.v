module stm32_interface(
clk_in,
SPEC_I,
SPEC_Q,
VOICE_I,
VOICE_Q,
DATA_SYNC,
ADC_OTR,
DAC_OTR,
ADC_IN,
adcclk_in,

DATA_BUS,
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
input signed [15:0] SPEC_I;
input signed [15:0] SPEC_Q;
input signed [15:0] VOICE_I;
input signed [15:0] VOICE_Q;
input DATA_SYNC;
input ADC_OTR;
input DAC_OTR;
input signed [11:0] ADC_IN;
input adcclk_in;

output reg unsigned [21:0] freq_out=620407;
output reg preamp_enable=0;
output reg rx=1;
output reg tx=0;
output reg audio_clk_en=1;
output reg signed [15:0] TX_I=0;
output reg signed [15:0] TX_Q=0;
output reg [15:0] stage_debug=0;

inout [7:0] DATA_BUS;
reg   [7:0] DATA_BUS_OUT;
reg         DATA_BUS_OE; // 1 - out 0 - in
assign DATA_BUS = DATA_BUS_OE ? DATA_BUS_OUT : 8'bZ ;

reg signed [15:0] k=1;
reg signed [15:0] I_HOLD;
reg signed [15:0] Q_HOLD;
reg signed [11:0] ADC_MIN;
reg signed [11:0] ADC_MAX;
reg ADC_MINMAX_RESET;

always @ (posedge clk_in)
begin
	//начало передачи
	if (DATA_SYNC==1)
	begin
		DATA_BUS_OE=0;
		ADC_MINMAX_RESET=0;
		if(DATA_BUS[7:0]=='d1) //GET PARAMS
		begin
			k=100;
		end
		else if(DATA_BUS[7:0]=='d2) //SEND PARAMS
		begin
			k=200;
		end
		else if(DATA_BUS[7:0]=='d3) //TX IQ
		begin
			k=300;
		end
		else if(DATA_BUS[7:0]=='d4) //RX IQ
		begin
			k=400;
		end
		else if(DATA_BUS[7:0]=='d5) //AUDIO PLL ON
		begin
			audio_clk_en=1;
			k=999;
		end
		else if(DATA_BUS[7:0]=='d6) //AUDIO PLL OFF
		begin
			audio_clk_en=0;
			k=999;
		end
		else if(DATA_BUS[7:0]=='d7) //BUS TEST
		begin
			k=700;
		end
	end
	else if (k==100) //GET PARAMS
	begin
		preamp_enable=DATA_BUS[2:2];
		if(DATA_BUS[3:3]==1)
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
		freq_out[21:16]=DATA_BUS[5:0];
		k=102;
	end
	else if (k==102)
	begin
		freq_out[15:8]=DATA_BUS[7:0];
		k=103;
	end
	else if (k==103)
	begin
		freq_out[7:0]=DATA_BUS[7:0];
		k=999;
	end
	else if (k==200) //SEND PARAMS
	begin
		DATA_BUS_OE=1;
		DATA_BUS_OUT[1:1]=DAC_OTR;
		DATA_BUS_OUT[0:0]=ADC_OTR;
		k=201;
	end
	else if (k==201)
	begin
		DATA_BUS_OUT[7:4]=ADC_MIN[11:8];
		DATA_BUS_OUT[3:0]=ADC_MAX[11:8];
		k=202;
	end
	else if (k==202) //SEND PARAMS
	begin
		DATA_BUS_OUT[7:0]=ADC_MIN[7:0];
		k=203;
	end
	else if (k==203)
	begin
		DATA_BUS_OUT[7:0]=ADC_MAX[7:0];
		ADC_MINMAX_RESET=1;
		k=999;
	end
	else if (k==300) //TX IQ
	begin
		Q_HOLD[15:8]=DATA_BUS[7:0];
		k=301;
	end
	else if (k==301)
	begin
		Q_HOLD[7:0]=DATA_BUS[7:0];
		k=302;
	end
	else if (k==302)
	begin
		I_HOLD[15:8]=DATA_BUS[7:0];
		k=303;
	end
	else if (k==303)
	begin
		I_HOLD[7:0]=DATA_BUS[7:0];
		TX_I[15:0]=I_HOLD[15:0];
		TX_Q[15:0]=Q_HOLD[15:0];
		k=999;
	end
	else if (k==400) //RX IQ SPECTRUM
	begin
		DATA_BUS_OE=1;
		I_HOLD=SPEC_I;
		Q_HOLD=SPEC_Q;
		DATA_BUS_OUT[7:0]=Q_HOLD[15:8];
		k=401;
	end
	else if (k==401)
	begin
		DATA_BUS_OUT[7:0]=Q_HOLD[7:0];
		k=402;
	end
	else if (k==402)
	begin
		DATA_BUS_OUT[7:0]=I_HOLD[15:8];
		k=403;
	end
	else if (k==403)
	begin
		DATA_BUS_OUT[7:0]=I_HOLD[7:0];
		k=404;
	end
	else if (k==404) //RX IQ VOICE
	begin
		I_HOLD=VOICE_I;
		Q_HOLD=VOICE_Q;
		DATA_BUS_OUT[7:0]=Q_HOLD[15:8];
		k=405;
	end
	else if (k==405)
	begin
		DATA_BUS_OUT[7:0]=Q_HOLD[7:0];
		k=406;
	end
	else if (k==406)
	begin
		DATA_BUS_OUT[7:0]=I_HOLD[15:8];
		k=407;
	end
	else if (k==407)
	begin
		DATA_BUS_OUT[7:0]=I_HOLD[7:0];
		k=999;
	end
	else if (k==700)
	begin
		Q_HOLD[7:0]=DATA_BUS[7:0];
		DATA_BUS_OE=1;
		DATA_BUS_OUT[7:0]=Q_HOLD[7:0];
		k=999;
	end
	stage_debug=k;
end

always @ (posedge adcclk_in)
begin
	if(ADC_MINMAX_RESET==1)
	begin
		ADC_MIN=2000;
		ADC_MAX=-2000;
	end
	if(ADC_MAX<ADC_IN)
	begin
		ADC_MAX=ADC_IN;
	end
	if(ADC_MIN>ADC_IN)
	begin
		ADC_MIN=ADC_IN;
	end
end

endmodule
