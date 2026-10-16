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
int clawangle=90;        // 爪子当前角度,初始=张开
bool clawIsOpen=true;    // 爪子状态:开/关
int baseStep=15;
int speedDelay=50;
int openAngle=30;
int closeAngle=150;

int armStep=15;          // 每推一下走5度
int joyTrig=400;        // 触发阈值:偏离中位超过400才算"推了"
bool latch1=false;      // 1号摇杆锁:推过就锁,回中才解锁
bool latch2=false;      // 2号摇杆锁
float baseangle=93;    
float leftangle=98;
float rightangle=89;       
 
int cenX=512,cenY1=512,cenY2=512;

bool clawmode=false;                
unsigned long t1=0,t2=0;            //按钮按下时刻 
bool prevB1=false,prevB2=false;    //区分单机和双击
const unsigned long comboWin=300;  //双击间隔时间窗口  

void setup() {
  Serial.begin(9600);
  pinMode(JOY1_BTN_PIN,INPUT_PULLUP);
  pinMode(JOY2_BTN_PIN,INPUT_PULLUP);
  servobase.write(93);      
  servoleft.write(98);
  servoright.write(89);
  servobase.attach(SERVO_BASE_PIN);
  servoleft.attach(SERVO_LEFT_PIN);
  servoright.attach(SERVO_RIGHT_PIN);
  servoclaw.attach(SERVO_CLAW_PIN);
  delay(speedDelay);
  cenX=analogRead(JOY1_X_PIN);   //读取摇杆中位
  cenY1=analogRead(JOY1_Y_PIN);
  cenY2=analogRead(JOY2_Y_PIN);
}

void loop() {
  if(Serial.available()>0)
  {
      char firstChar=Serial.peek();
        if(firstChar=='x'||firstChar=='y'||firstChar=='z')
    {
       String cmd=Serial.readStringUntil('\n');
       cmd.trim();
       controlServos(cmd);
    }
    else
    {
      char cmd=Serial.read();
      switch(cmd){
        case'O':servoclaw.write(openAngle);
        break;
        case'S':servoclaw.write(closeAngle);
        break;
        case'H':speedDelay=max(5,(int)(speedDelay*0.7));  
        break;
        case'L':speedDelay=min(200,(int)(speedDelay*1.4));
        break;
      }
    }
  }
  joyDrive();
} 

void joyDrive(){
  checkBaseBtn();   //按钮调节底座/爪子
  int Vy1=analogRead(JOY1_Y_PIN);
  int Vy2=analogRead(JOY2_Y_PIN);
  //1号摇杆Y → 左臂
  if(!latch1){
    if(cenY1-Vy1>joyTrig){           //向上推
      leftangle=constrain(leftangle+armStep,0,180);
      servoleft.write((int)leftangle);
      Serial.print("LEFT "); Serial.println((int)leftangle);
      latch1=true;                   //锁住,回中前不再触发
    }
    else if(Vy1-cenY1>joyTrig){      //向下压
      leftangle=constrain(leftangle-armStep,0,180);
      servoleft.write((int)leftangle);
      Serial.print("LEFT "); Serial.println((int)leftangle);
      latch1=true;
    }
  }
  else if(abs(Vy1-cenY1)<100) latch1=false;   //回中解锁,才能推下一次
  //2号摇杆Y → 右臂,同理
  if(!latch2){
    if(cenY2-Vy2>joyTrig){
      rightangle=constrain(rightangle+armStep,0,180);
      servoright.write((int)rightangle);
      Serial.print("RIGHT "); Serial.println((int)rightangle);
      latch2=true;
    }
    else if(Vy2-cenY2>joyTrig){
      rightangle=constrain(rightangle-armStep,0,180);
      servoright.write((int)rightangle);
      Serial.print("RIGHT "); Serial.println((int)rightangle);
      latch2=true;
    }
  }
  else if(abs(Vy2-cenY2)<100) latch2=false;
  delay(20);
}
void baseLeft(){
  baseangle=constrain(baseangle-baseStep,0,180);
  servobase.write(constrain((int)baseangle,0,180));
  Serial.print("BASE "); Serial.println((int)baseangle);
}

void baseRight(){
  baseangle=constrain(baseangle+baseStep,0,180);
  servobase.write(constrain((int)baseangle,0,180));
  Serial.print("BASE "); Serial.println((int)baseangle);
}

void checkBaseBtn(){
  bool b1=digitalRead(JOY1_BTN_PIN)==LOW;
  bool b2=digitalRead(JOY2_BTN_PIN)==LOW;
  if(b1&&!prevB1&&millis()-t1>50) t1=millis(); // 1号刚按下(带消抖)
  if(b2&&!prevB2&&millis()-t2>50) t2=millis();  // 2号刚按下
  prevB1=b1; prevB2=b2;    //存入上一次按钮状态 

  // 两个钮都按了且间隔在窗口内 → 组合键:切换模式
  if(t1&&t2&&abs((long)(t1-t2))<=comboWin){
    t1=0; t2=0;     //时间归零
    clawmode=!clawmode;
    Serial.println(clawmode?"CLAW MODE":"BASE MODE");
    return;
  }
  // 等了300ms另一个没来 → 算单按
    if(t1&&millis()-t1>comboWin){
    t1=0;
    if(clawmode){
      clawIsOpen=!clawIsOpen;                          // 状态翻转
      clawangle=clawIsOpen?openAngle:closeAngle;
      servoclaw.write(clawangle);
      Serial.println(clawIsOpen?"CLAW OPEN":"CLAW CLOSE");
    }
    else baseLeft();
  }
  if(t2&&millis()-t2>comboWin){
    t2=0;
    if(!clawmode) baseRight();   // 爪子模式下2号什么都不做
  }
}

  void controlServos(String cmd){
   int line1=cmd.indexOf(',');
   int line2=cmd.indexOf(',',line1+1);
   String partX=cmd.substring(0,line1);
   String partY=cmd.substring(line1+1,line2);
   String partZ=cmd.substring(line2+1);
   int targetX=partX.substring(1).toInt();
   int targetY=partY.substring(1).toInt();
   int targetZ=partZ.substring(1).toInt();
   moveServosTogether(targetX,targetY,targetZ);
   int usX=map(constrain(targetX,0,180),0,180,544,2400); //角度转脉宽
   int usY=map(constrain(targetY,0,180),0,180,544,2400);
   int usZ=map(constrain(targetZ,0,180),0,180,544,2400);
   Serial.print("X:"); 
   Serial.print(usX/200.0); Serial.println("%");
   Serial.print("Y:"); 
   Serial.print(usY/200.0); Serial.println("%");
   Serial.print("Z:"); 
   Serial.print(usZ/200.0); Serial.println("%");
  }
  
  void moveServosTogether(int targetX,int targetY,int targetZ){
    int curX=servobase.read();
    int curY=servoleft.read();
    int curZ=servoright.read();
    int disX=abs(curX-targetX);
    int disY=abs(curY-targetY);
    int disZ=abs(curZ-targetZ);
    int maxDis=max(disX,max(disY,disZ));
    if(maxDis==0) return;
    float stepX=(float)(targetX-curX)/maxDis;
    float stepY=(float)(targetY-curY)/maxDis;
    float stepZ=(float)(targetZ-curZ)/maxDis;
    float accX=curX;
    float accY=curY;
    float accZ=curZ;
    for(int i=1;i<=maxDis;i++)
    {
      accX+=stepX;
      accY+=stepY;
      accZ+=stepZ;
      servobase.write(int(accX));
      servoleft.write(int(accY));
      servoright.write(int(accZ));
      delay(speedDelay);
    }
    servobase.write(targetX);
    servoleft.write(targetY);
    servoright.write(targetZ);
}
