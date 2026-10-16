const int BTNS_PIN=A0;    
const int BTNG_PIN=A1;    
const int BTNC_PIN=A2;    
const int BTNR_PIN=A3;    


void setup(){
  Serial.begin(9600);
  pinMode(BTNS_PIN,INPUT_PULLUP);
  pinMode(BTNG_PIN,INPUT_PULLUP);
  pinMode(BTNC_PIN,INPUT_PULLUP);
  pinMode(BTNR_PIN,INPUT_PULLUP);
}

void loop(){
  if(pressed(BTNS_PIN)) Serial.print('S');
  if(pressed(BTNG_PIN)) Serial.print('G');
  if(pressed(BTNC_PIN)) Serial.print('C');
  if(pressed(BTNR_PIN)) Serial.print('R');
}

bool pressed(int pin){
  if(digitalRead(pin)==LOW){
    delay(20);                             
    if(digitalRead(pin)==LOW){
      while(digitalRead(pin)==LOW);         
      return true;
    }
  }
  return false;
}