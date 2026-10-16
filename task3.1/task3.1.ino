#include <Servo.h>

Servo servobase;
Servo servoleft;
Servo servoright;
Servo servoclaw;

const int SERVO_BASE_PIN=9;
const int SERVO_LEFT_PIN=8;
const int SERVO_RIGHT_PIN=7;
const int SERVO_CLAW_PIN=6;

const int JOY1_X_PIN=A0;
const int JOY1_Y_PIN=A1;
const int JOY2_Y_PIN=A3;
const int JOY1_BTN_PIN=5;
const int JOY2_BTN_PIN=4;
int basestep=10;

int openangle=0;

float baseangle=93;    
float leftangle=98;
float rightangle=89;  

const int homeX=93;
const int homeY=98;
const int homeZ=89;

int apickdown[3]={ 0, 170, 170};
int aplacedown[3] ={ 120, 90,90};
int aclose=90;

int bpickdown[3]={ 90, 60, 90};
int bplacedown[3]={ 45, 40,110};
int bclose=15;

int cpickdown[3]={140, 60, 90};
int cplacedown[3]={ 90, 30,118};
int cclose=8;

int cycleflag=0;

int cenX=512,cenY1=512,cenY2=512;

const int maxrec=200;
const int samplems=100;
byte recBuf[maxrec][3];       
int nowrec=0;
bool recording=false;
unsigned long lastsample=0;

void setup() {
  Serial.begin(9600);
  pinMode(JOY1_BTN_PIN,INPUT_PULLUP);
  pinMode(JOY2_BTN_PIN,INPUT_PULLUP);
  servobase.write(homeX);
  servoleft.write(homeY);
  servoright.write(homeZ);
  servoclaw.write(openangle);
  servobase.attach(SERVO_BASE_PIN);
  servoleft.attach(SERVO_LEFT_PIN);
  servoright.attach(SERVO_RIGHT_PIN);
  servoclaw.attach(SERVO_CLAW_PIN);
  delay(100);
  cenX=analogRead(JOY1_X_PIN);
  cenY1=analogRead(JOY1_Y_PIN);
  cenY2=analogRead(JOY2_Y_PIN);
  Serial.println("READY");
}

void loop() {
  if(Serial.available()>0)
  {
    char cmd=Serial.read();
    switch(cmd){
      case'1':doCycle();                            
      break;
      case'2':joyRecord();                       
      break;
      case'3':playRecord();                        
      break;
      case'4':moveServosTogether(homeX,homeY,homeZ); 
      Serial.println("HOME");
      break;
    }
  }

  if(recording)
  {
    joyDrive();
    if(millis()-lastsample>=samplems)
    {
      lastsample=millis();
      if(nowrec<maxrec)
      {
        recBuf[nowrec][0]=servobase.read();
        recBuf[nowrec][1]=servoleft.read();
        recBuf[nowrec][2]=servoright.read();
        nowrec++;
      }
    }
  }
}

void doCycle(){
  if(cycleflag==0)      
  runTask(apickdown,aplacedown,aclose);
  else if(cycleflag==1) 
  runTask(bpickdown,bplacedown,bclose);
  else                 
  runTask(cpickdown,cplacedown,cclose);
  cycleflag=(cycleflag+1)%3;
}

void joyRecord(){
  if(!recording)
  {
    recording=true;
    nowrec=0;
    lastsample=millis();
    Serial.println("REC START");
  }
  else
  {
    recording=false;
    Serial.print("REC STOP ");
    Serial.println(nowrec);
  }
}

void playRecord(){
  if(nowrec<2)
  {
    Serial.println("NO DATA");
    return;
  }
  Serial.println("PLAY");
  for(int i=0;i<nowrec;i++)
  {
    servobase.write(recBuf[i][0]);
    servoleft.write(recBuf[i][1]);
    servoright.write(recBuf[i][2]);
    delay(samplems);
  }
  Serial.println("DONE");
}

void joyDrive(){
  checkBaseBtn();   //按钮调节底座
  int Vy1=analogRead(JOY1_Y_PIN);
  int Vy2=analogRead(JOY2_Y_PIN);
  if(abs(Vy1-cenY1)>80||abs(Vy2-cenY2)>80)  //防止抖动
  {//循环叠加角度
  leftangle=leftangle+(Vy1-512)/200.0;  
  rightangle=rightangle+(Vy2-512)/200.0;
  leftangle=constrain(leftangle,0,180);  //控制角度在0-180
  rightangle=constrain(rightangle,0,180);
  servoleft.write((int)leftangle);
  servoright.write((int)rightangle);
  delay(20);
  }
}

void checkBaseBtn(){
  if(digitalRead(JOY1_BTN_PIN)==LOW){        // 1号钮:左转一格
    delay(30);
    if(digitalRead(JOY1_BTN_PIN)==LOW){     //消抖
      baseangle=constrain(baseangle+basestep,0,180);
      servobase.write((int)baseangle);
      Serial.print("BASE "); Serial.println((int)baseangle);
      while(digitalRead(JOY1_BTN_PIN)==LOW); // 等松手,按一下只走一格
    }
  }
  if(digitalRead(JOY2_BTN_PIN)==LOW){        // 2号钮:右转一格
    delay(30);
    if(digitalRead(JOY2_BTN_PIN)==LOW){
      baseangle=constrain(baseangle-basestep,0,180);
      servobase.write((int)baseangle);
      Serial.print("BASE "); Serial.println((int)baseangle);
      while(digitalRead(JOY2_BTN_PIN)==LOW);
    }
  }
}

void runTask(int pick[3],int place[3],int close){                   
  moveServosTogether(pick[0],pick[1],pick[2]);
  delay(60);                              
  servoclaw.write(close);
  delay(400);
  moveServosTogether(pick[0],pick[1]+20,pick[2]-20);  
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
    if(curX<targetX) curX++;
    else curX--;
    servobase.write(curX);
    delay(30);
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
    delay(30);
  }
  servoleft.write(targetY);
  servoright.write(targetZ);
}
