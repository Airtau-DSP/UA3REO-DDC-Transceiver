#define AMP_POWER 14 //A0
#define AMP 16 //A2
#define DPF_160 3
#define DPF_80 4
#define DPF_40 5
#define DPF_30 6
#define DPF_20 7
#define DPF_17 8
#define DPF_15 9
#define DPF_12 10
#define DPF_10 11
#define DPF_NONE 2
#define ATT 12
#define PREAMP 13

String serial_readline;
const int bSize = 64; //serial buffer size from desktop
char Buffer[bSize]; //serial buffer from desktop

void setup() {
  Serial.begin(115200);
  pinMode(AMP_POWER, OUTPUT);
  pinMode(AMP, OUTPUT);
  pinMode(DPF_160, OUTPUT);
  pinMode(DPF_80, OUTPUT);
  pinMode(DPF_40, OUTPUT);
  pinMode(DPF_30, OUTPUT);
  pinMode(DPF_20, OUTPUT);
  pinMode(DPF_17, OUTPUT);
  pinMode(DPF_15, OUTPUT);
  pinMode(DPF_12, OUTPUT);
  pinMode(DPF_10, OUTPUT);
  pinMode(DPF_NONE, OUTPUT);
  pinMode(ATT, OUTPUT);
  pinMode(PREAMP, OUTPUT);
  
  digitalWrite(AMP_POWER,1);
  digitalWrite(AMP,1);
  digitalWrite(DPF_160,1);
  digitalWrite(DPF_80,1);
  digitalWrite(DPF_40,1);
  digitalWrite(DPF_30,1);
  digitalWrite(DPF_20,1);
  digitalWrite(DPF_17,1);
  digitalWrite(DPF_15,1);
  digitalWrite(DPF_12,1);
  digitalWrite(DPF_10,1);
  digitalWrite(DPF_NONE,0);
  digitalWrite(ATT,1);
  digitalWrite(PREAMP,1);
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
      digitalWrite(DPF_160,1);
      digitalWrite(DPF_80,1);
      digitalWrite(DPF_40,1);
      digitalWrite(DPF_30,1);
      digitalWrite(DPF_20,1);
      digitalWrite(DPF_17,1);
      digitalWrite(DPF_15,1);
      digitalWrite(DPF_12,1);
      digitalWrite(DPF_10,1);
      digitalWrite(DPF_NONE,1);
    }
    if(serial_readline.startsWith("DPF_160")) digitalWrite(DPF_160,0);
    if(serial_readline.startsWith("DPF_80")) digitalWrite(DPF_80,0);
    if(serial_readline.startsWith("DPF_40")) digitalWrite(DPF_40,0);
    if(serial_readline.startsWith("DPF_30")) digitalWrite(DPF_30,0);
    if(serial_readline.startsWith("DPF_20")) digitalWrite(DPF_20,0);
    if(serial_readline.startsWith("DPF_17")) digitalWrite(DPF_17,0);
    if(serial_readline.startsWith("DPF_15")) digitalWrite(DPF_15,0);
    if(serial_readline.startsWith("DPF_12")) digitalWrite(DPF_12,0);
    if(serial_readline.startsWith("DPF_10")) digitalWrite(DPF_10,0);
    if(serial_readline.startsWith("DPF_NONE")) digitalWrite(DPF_NONE,0);
    if(serial_readline.startsWith("ATT_ON")) digitalWrite(ATT,0);
    if(serial_readline.startsWith("ATT_OFF")) digitalWrite(ATT,1);
    if(serial_readline.startsWith("PREAMP_ON")) digitalWrite(PREAMP,0);
    if(serial_readline.startsWith("PREAMP_OFF")) digitalWrite(PREAMP,1);
    if(serial_readline.startsWith("AMP_POWER_ON")) digitalWrite(AMP_POWER,0);
    if(serial_readline.startsWith("AMP_POWER_OFF")) digitalWrite(AMP_POWER,1);
    if(serial_readline.startsWith("AMP_ON")) digitalWrite(AMP,0);
    if(serial_readline.startsWith("AMP_OFF")) digitalWrite(AMP,1);
    Serial.flush();
  }
}

