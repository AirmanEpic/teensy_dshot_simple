void setup(){
  Serial.begin(9600);
  pinMode(4, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(13, OUTPUT);
  CORE_PIN4_CONFIG = IOMUXC_PAD_DSE(7);
}

int currentCommand = -1;
int lastCommandAt = 0;

// #define NOP __asm__ __volatile__ ("nop\n\t")

void loop(){
  // byte data_size = Serial.available();

  // byte buff[data_size], degree = 1;

  // long recv_data = 0; dub = 1;
  // bool minus = 0;

  // for (byte i=0; i<data_size; i++){
  //  buf[i] = Serial.read();

  //  if (buf[i] >= '0' && buf[i] <= '9') 
  //    buf[i] -= '0';
  //  else{
  //    if (buf[i] == '-')
  //      minus = 1;
  //    else 
  //      degree = 0;
  //  }
  // }

  // if degree == 1
  //  degree = data_size - minus;

  // for (byte i=0; i<degree; i++){
  //  recv_data += buf[(data_size-1) - i] * dub;
  //  dub *= 10;
  // }

  // if (minus)
  //  recv_data *= -1;
  while (Serial.available()){
    String a = Serial.readString();
    Serial.println("Received Signal: "+a);
    handleCommand(a);
  }

  if (currentCommand != -1)
  sendSingleCommand(currentCommand);
//    sendCalibrateCommand();
}

void handleCommand(String command){
  command = command.replace("\n","");
  command = command.replace("\r","");
  int codeCommand = -1;

  if (command == "stop"){
    Serial.println("Received engine stop command");
    codeCommand = 0;
  }

  if (command == "astop"){
    Serial.println("Received engine LOW command");
    codeCommand = 48;
  }

  if (command == "low"){
    Serial.println("Received engine LOW command");
    codeCommand = 50;
  }

  if (command == "lowmed"){
    Serial.println("Received engine LOWmed command");
    codeCommand = 150;
  }

  if (command == "med"){
    Serial.println("Received engine MEDIUM command");
    codeCommand = 1024;
  }

  if (command == "hi"){
    Serial.println("Received engine HIGH command");
    codeCommand = 2040;
  }

  if (command == "max"){
    Serial.println("maxing throttle");
    codeCommand = 2047;
  }

  if (codeCommand == -1){
    return -1;
  }

 currentCommand = codeCommand;
}

void sendCalibrateCommand(){
  digitalWrite(4,HIGH);
  delayNS(3000);
  digitalWrite(4,LOW);
  delayMicroseconds(7);
}

void sendSingleCommand(int code){
  //this actually sends the command
  uint16_t signal = 0b0;
  //this is going for DSHOT600
  int T1H = 1250;
  int T0H = 625;
  int periodTime = 1670;
  if (code == 2040){
    signal = 0b11111111000;
  }

  if (code == 50){
    signal = 0b110010;
  }

  if (code == 150){
    signal = 0b10010110;
  }

  if (code == 1024){
    signal = 0b10000000000;
  }

  if (code == 2047){
    signal = 0b11111111111;
  }

  if (code == 48){
    signal = 0b110000;
  }

  if (code == 0){
    signal = 0b00000000000;
  }
  int startTime = micros();
  digitalWrite(13, HIGH);

  uint16_t packet = signal;

  int csum = 0;
  int csum_data = packet;
  for (int i=0; i<3; i++){
    csum^= csum_data;
    csum_data >>= 4;
  }

  csum &= 0xf;
  packet = (packet<<4) | csum;

  
  bool leading = false;
  for (int i = sizeof(packet) * 8 - 1; i>=0; i--){
    int bit = (packet >> i) & 1;
    leading |= bit;

    digitalWrite(4,HIGH);
    if (leading){
      delayNS(T1H - 17 - 40); // I looked at what I should get for a delay and what I actually got and subtracted (and added in one instance, dunno wtf is up there) the difference between expected and measured values
    }
    else{
      delayNS(T0H - 17 - 40); // I originally subtracted 17 to compensate for the time it takes to flip a bit in the output register, and I haven't bothered integrating it into one number because the compiler will turn it into a bunch of nops anyways
    }
    digitalWrite(4, LOW);

    if (leading){
      delayNS(periodTime - T1H - 17 + 80);
    }
    else{
      delayNS(periodTime - T0H - 17 - 30);
    }

  }
  digitalWrite(4,LOW);
  digitalWrite(13, LOW);

  delayMicroseconds(21*periodTime/1000);
}

/* One nop is 1.66ns (1/cpu frequency, so 1/600000000 here) and one cycle of the for loop is another cycle,
 * so we want to divide the number of ns delay requested by this number to calculate how many nops we need to achieve this.
 * This means that this function has a granularity of 3.33ns.
 * I know that dividing an int by a float is asking for trouble, but it works!
 */

void delayNS(int delay){
  delay /= 3.3333;
  for (int i=0; i<delay; i++){
    asm volatile("nop");
  }
}
