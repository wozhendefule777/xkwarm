#include <Servo.h>

Servo servobase;
Servo servoleft;
Servo servoright;
Servo servoclaw;

const int SERVO_BASE_PIN=9;
const int SERVO_LEFT_PIN=8;
const int SERVO_RIGHT_PIN=7;
const int SERVO_CLAW_PIN=6;

int openangle=90;            

const int homeX=93;
const int homeY=89;
const int homeZ=98;

int apickdown[3]={123,54,104};  
int aplacedown[3]={63,23,119};    
int aclose=0;                    

int bpickdown[3];
int bplacedown[3];
int bclose=132;

int cpickdown[3];
int cplacedown[3];
int cclose=25;

void setup() {
  Serial.begin(9600);
  servobase.attach(SERVO_BASE_PIN);
  servoleft.attach(SERVO_LEFT_PIN);
  servoright.attach(SERVO_RIGHT_PIN);
  servoclaw.attach(SERVO_CLAW_PIN);
  servobase.write(homeX);
  servoleft.write(homeY);
  servoright.write(homeZ);
  servoclaw.write(openangle);
  Serial.println("READY");
}

void loop() {
  if(Serial.available()>0)
    {
      char cmd=Serial.read();
      switch(cmd){
        case'A':runTask(apickdown,aplacedown,aclose);   
        break;
        case'B':runTask(bpickdown,bplacedown,bclose);
        break;
        case'C':runTask(cpickdown,cplacedown,cclose);
        break; 
      }
    }
}

void runTask(int pick[3],int place[3],int close){                   
  moveServosTogether(pick[0],pick[1],pick[2]);
  delay(60);                              
  servoclaw.write(close);
  delay(400);  
  moveServosTogether(place[0],place[1],place[2]);
  delay(30);        
  servoclaw.write(openangle);  
  delay(60);                     
  moveServosTogether(homeX,homeY,homeZ);                       
  Serial.println("DONE");
}

void moveServosTogether(int targetX,int targetY,int targetZ){
  int curX=servobase.read();
  while(curX!=targetX)
  {
    if(curX<targetX)
    curX++;
    else
    curX--;
    servobase.write(curX);
    delay(15);
  }
  int curY=servoleft.read();
  int curZ=servoright.read();
  int disY=abs(curY-targetY);
  int disZ=abs(curZ-targetZ);
  int maxDis=max(disY,disZ);
  if(maxDis==0) return;
  float stepY=(float)(targetY-curY)/maxDis;
  float stepZ=(float)(targetZ-curZ)/maxDis;
  float accY=curY;
  float accZ=curZ;
  for(int i=1;i<=maxDis;i++)
  {
    accY+=stepY;
    accZ+=stepZ;
    servoleft.write(int(accY));
    servoright.write(int(accZ));
    delay(15);
  }
  servoleft.write(targetY);
  servoright.write(targetZ);
}
