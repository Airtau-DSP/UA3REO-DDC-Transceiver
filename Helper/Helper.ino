#include <SoftwareWire.h>

#define PCF8574_ADDRESS 0xA7
#define PCF8574_WRITE_COMM 0x40 //40 4E

SoftwareWire PCF8574_1 (10, 11); //SDA SCL
SoftwareWire PCF8574_2 (12, 13); //SDA SCL

//control bits
bool PCF8574_1_bit_0=false; //AMP_POWER
bool PCF8574_1_bit_1=false; //20
bool PCF8574_1_bit_2=false; //17
bool PCF8574_1_bit_3=false; //NC
bool PCF8574_1_bit_4=false; //15
bool PCF8574_1_bit_5=false; //12
bool PCF8574_1_bit_6=false; //10
bool PCF8574_1_bit_7=false; //PREAMP
bool PCF8574_2_bit_0=false; //NC
bool PCF8574_2_bit_1=false; //160
bool PCF8574_2_bit_2=false; //80
bool PCF8574_2_bit_3=false; //NC
bool PCF8574_2_bit_4=false; //40
bool PCF8574_2_bit_5=false; //30
bool PCF8574_2_bit_6=false; //NO_DPF
bool PCF8574_2_bit_7=false; //ATT

#define AMP_POWER PCF8574_1_bit_0
#define DPF_160 PCF8574_2_bit_1
#define DPF_80 PCF8574_2_bit_2
#define DPF_40 PCF8574_2_bit_4
#define DPF_30 PCF8574_2_bit_5
#define DPF_20 PCF8574_1_bit_1
#define DPF_17 PCF8574_1_bit_2
#define DPF_15 PCF8574_1_bit_4
#define DPF_12 PCF8574_1_bit_5
#define DPF_10 PCF8574_1_bit_6
#define DPF_NONE PCF8574_2_bit_6
#define ATT PCF8574_2_bit_7
#define PREAMP PCF8574_1_bit_7

#define AMP 5

String serial_readline;
const int bSize = 64; //serial buffer size from desktop
char Buffer[bSize]; //serial buffer from desktop

void setup() {
  Serial.begin(115200);
  pinMode(AMP, OUTPUT);
  digitalWrite(AMP,1);
  PCF8574_1.begin();
  PCF8574_2.begin();
  DPF_NONE=true;
  writeToPCF();
}

void loop() {
  if (Serial.available() > 0) {
    memset(Buffer, 0, bSize);
    Serial.readBytesUntil('\n', Buffer, bSize);
    serial_readline = String(Buffer);
    Serial.print("New command: ");
    Serial.println(serial_readline);
    if(serial_readline.startsWith("DPF"))
    {
      DPF_160=false;
      DPF_80=false;
      DPF_40=false;
      DPF_30=false;
      DPF_20=false;
      DPF_17=false;
      DPF_15=false;
      DPF_12=false;
      DPF_10=false;
      DPF_NONE=false;
    }
    if(serial_readline.startsWith("DPF_160")) DPF_160=true;
    if(serial_readline.startsWith("DPF_80")) DPF_80=true;
    if(serial_readline.startsWith("DPF_40")) DPF_40=true;
    if(serial_readline.startsWith("DPF_30")) DPF_30=true;
    if(serial_readline.startsWith("DPF_20")) DPF_20=true;
    if(serial_readline.startsWith("DPF_17")) DPF_17=true;
    if(serial_readline.startsWith("DPF_15")) DPF_15=true;
    if(serial_readline.startsWith("DPF_12")) DPF_12=true;
    if(serial_readline.startsWith("DPF_10")) DPF_10=true;
    if(serial_readline.startsWith("DPF_NONE")) DPF_NONE=true;
    if(serial_readline.startsWith("ATT_ON")) ATT=true;
    if(serial_readline.startsWith("ATT_OFF")) ATT=false;
    if(serial_readline.startsWith("PREAMP_ON")) PREAMP=true;
    if(serial_readline.startsWith("PREAMP_OFF")) PREAMP=false;
    if(serial_readline.startsWith("AMP_POWER_ON")) AMP_POWER=true;
    if(serial_readline.startsWith("AMP_POWER_OFF")) AMP_POWER=false;
    if(serial_readline.startsWith("AMP_ON")) digitalWrite(AMP,0);
    if(serial_readline.startsWith("AMP_OFF")) digitalWrite(AMP,1);
    Serial.flush();
    writeToPCF();
  }
}

void writeToPCF()
{
  PCF8574_1.beginTransmission(PCF8574_ADDRESS);
  PCF8574_1.write(byte(PCF8574_WRITE_COMM));
  char tmp=0;
  bitWrite(tmp,0,PCF8574_1_bit_0);
  bitWrite(tmp,1,PCF8574_1_bit_1);
  bitWrite(tmp,2,PCF8574_1_bit_2);
  bitWrite(tmp,3,PCF8574_1_bit_3);
  bitWrite(tmp,4,PCF8574_1_bit_4);
  bitWrite(tmp,5,PCF8574_1_bit_5);
  bitWrite(tmp,6,PCF8574_1_bit_6);
  bitWrite(tmp,7,PCF8574_1_bit_7);
  PCF8574_1.write(~tmp);
  PCF8574_1.endTransmission();
  PCF8574_2.beginTransmission(PCF8574_ADDRESS);
  PCF8574_2.write(byte(PCF8574_WRITE_COMM));
  tmp=0;
  bitWrite(tmp,0,PCF8574_2_bit_0);
  bitWrite(tmp,1,PCF8574_2_bit_1);
  bitWrite(tmp,2,PCF8574_2_bit_2);
  bitWrite(tmp,3,PCF8574_2_bit_3);
  bitWrite(tmp,4,PCF8574_2_bit_4);
  bitWrite(tmp,5,PCF8574_2_bit_5);
  bitWrite(tmp,6,PCF8574_2_bit_6);
  bitWrite(tmp,7,PCF8574_2_bit_7);
  PCF8574_2.write(~tmp);
  PCF8574_2.endTransmission();
}
