const int BTN1_PIN=A0;    
const int BTN2_PIN=A1;    
const int BTN3_PIN=A2;    
const int BTN4_PIN=A3;   

void setup() {
  Serial.begin(9600);
  pinMode(BTN1_PIN,INPUT_PULLUP);
  pinMode(BTN2_PIN,INPUT_PULLUP);
  pinMode(BTN3_PIN,INPUT_PULLUP);
  pinMode(BTN4_PIN,INPUT_PULLUP);
}

void loop() {
  if(pressed(BTN1_PIN)) 
  Serial.print('1');
  if(pressed(BTN2_PIN)) 
  Serial.print('2');
  if(pressed(BTN3_PIN)) 
  Serial.print('3');
  if(pressed(BTN4_PIN)) 
  Serial.print('4');
}

bool pressed(int pin){
  if(digitalRead(pin)==LOW)
  {
    delay(20);
    if(digitalRead(pin)==LOW)
    {
      while(digitalRead(pin)==LOW);   
      return true;
    }
  }
  return false;
}
