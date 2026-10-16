#include <Servo.h>

Servo servobase;
Servo servoleft;
Servo servoright;
Servo servoclaw;

const int SERVO_BASE_PIN=9;
const int SERVO_LEFT_PIN=8;
const int SERVO_RIGHT_PIN=7;
const int SERVO_CLAW_PIN=6;

const int JOY1_Y_PIN=A1;
const int JOY2_Y_PIN=A3;
const int JOY1_BTN_PIN=5;   // 单按=底座左转,双按组合后=爪子开
const int JOY2_BTN_PIN=4;   // 单按=底座右转,双按组合后=爪子闭

int task=0;         // 0=待机
bool pcOn=false;    // 任务一:按①(a)后才接收电脑串口(x,y,z/O/S/H/L)

// 爪子
int openangle=90;
int closeangle=120;
int aclose=150;        // 三个物体夹取角度
int bclose=150;
int cclose=150;

int baseStep=5;        // 按钮按一下,底座走5度
int clawStep=5;        // 按钮按一下,爪子走5度
int clawangle=90;      // 爪子当前角度,和 openangle 保持一致
bool clawmode=false;   // false=底座模式 true=爪子模式

const int homeX=93;
const int homeY=89;
const int homeZ=89;

// 摇杆点动参数(摇杆功能全程有效)
int armStep=15;        // 每推一下走的度数(固定,想改就改这里)
int joyTrig=400;       // 推动临界值
int cenX=512,cenY1=512,cenY2=512;  // 摇杆中位
bool latchX=false,latch1=false,latch2=false;

// 摇杆按钮状态
bool prevB1=false,prevB2=false;
unsigned long t1=0,t2=0;
const unsigned long comboWin=300;  // 双按时间窗口

// 任务一
int speedDelay=30;

// 任务二点位(现场示教)
int apickdown[3],aplacedown[3];
int bpickdown[3],bplacedown[3];
int cpickdown[3],cplacedown[3];
int aback[3],bback[3],cback[3];   // 夹取点斜后方过渡点(防撞B/C)
bool pickSaved[3]={false,false,false};
bool placeSaved[3]={false,false,false};
bool backSaved[3]={false,false,false};
int saveSeq=0;

// 任务三
int cycleflag=0;
const int maxrec=150;    // 录制容量(15秒)
const int samplems=100;
byte recBuf[maxrec][3];
int nowrec=0;
bool recording=false;
unsigned long lastsample=0;

// 任务五
const float L1=8.0;    // 大臂
const float L2=8.0;    // 小臂
const float X0=1.45;      
float pendown=30.0;    // 落笔高
float penlift=20.0;    // 抬笔高
float penup=60.0;      
float k0=1.0,off0=93.0;      // 偏差角
float k1=1.0,off1=90.0;      // 
float k2=-1.01,off2=180.0;   // 
const int penhold=60;
float curX=150,curY=0;
float lineX1,lineY1,lineX2,lineY2;   // 直线端点坐标
float v_x[3],v_y[3];                 // V字端点坐标
float l_x[5],l_y[5];   // 折线点坐标
float c_x[5],c_y[5];   // 曲线点坐标
// 示教原始数据:摇杆到位后直接存三个舵机角度
byte lineEP[2][3];     // 直线2个端点
byte vEP[3][3];        // V字3个端点
byte lAng[5][3];       // 折线5点
byte cAng[5][3];       // 曲线5点
int lcount=0,ccount=0;
bool straiflag=false;  // 示教模式开关
int tseq=0;            // 示教序号0~14:直线2->V字3->折线5->曲线5
int demoSeq=0;         // 演示序号0~3:直线->V字->折线->曲线

void setup(){
  Serial.begin(9600);
  Serial.setTimeout(100);
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
  delay(500);
  cenY1=analogRead(JOY1_Y_PIN);
  cenY2=analogRead(JOY2_Y_PIN);
  Serial.println("READY, press d to switch task");
}

void loop(){
  if(Serial.available()>0){
    String s=Serial.readStringUntil('\n');
    s.trim();
    if(s.length()>0) handleCmd(s);
  }
  if(task!=0){             // 摇杆+摇杆按钮全程有效
    joyStep();
    checkBtn();
  }
  if(recording&&millis()-lastsample>=samplems){   // 任务三录制采样
    lastsample=millis();
    if(nowrec<maxrec){
      recBuf[nowrec][0]=servobase.read();
      recBuf[nowrec][1]=servoleft.read();
      recBuf[nowrec][2]=servoright.read();
      nowrec++;
    }
  }
}

// 指令总调度(按键板: ①=a ②=b ③=c ④=d ⑤=e)
void handleCmd(String s){
  // 任务一且按过a:接收电脑串口 x,y,z / O / S / H / L
  if(task==1&&pcOn){
    if(s.charAt(0)=='x'&&s.indexOf(',')>0){ controlServos(s); return; }
    if(s.length()==1){
      char c=s.charAt(0);
      if(c=='O'){ servoclaw.write(openangle);  clawangle=openangle;  Serial.println("OPEN");  return; }
      if(c=='S'){ servoclaw.write(closeangle); clawangle=closeangle; Serial.println("CLOSE"); return; }
      if(c=='H'){ speedDelay=max(5,(int)(speedDelay*0.7));  Serial.println(speedDelay); return; }
      if(c=='L'){ speedDelay=min(200,(int)(speedDelay*1.4)); Serial.println(speedDelay); return; }
    }
  }
  if(s.length()!=1) return;   // 其余情况只认按键板单字符
  char cmd=s.charAt(0);

  if(cmd=='d'){ switchTask(); return; }   // ④d全局切任务

  if(task==1){
    if(cmd=='a'){ pcOn=!pcOn; Serial.println(pcOn?"PC ON":"PC OFF"); }
  }
  else if(task==2){
    switch(cmd){
      case 'a': tryRun(0); break;   // 夹A放A
      case 'b': tryRun(1); break;   
      case 'c': tryRun(2); break;   
      case 'e': saveNext(); break;  // ⑤存一个位置
    }
  }
  else if(task==3){
    switch(cmd){
      case 'a': doCycle(); break;
      case 'b': joyRecord(); break;
      case 'c': playRecord(); break;
      case 'e': moveServosTogether(homeX,homeY,homeZ); Serial.println("HOME"); break;  // ⑤回中
    }
  }
  else if(task==5){
    switch(cmd){
      case 'c': recordPoint(); break;   // ③存点
      case 'e': demoNext();  break;     // ⑤演示
    }
  }
}

// ④d:任务循环切换 1->2->3->5->1
void switchTask(){
  if(task==1) task=2;
  else if(task==2) task=3;
  else if(task==3) task=5;
  else task=1;
  pcOn=false;   // 切任务时关闭电脑串口接收
  Serial.print("TASK "); Serial.println(task);
  if(task==1) Serial.println("a=PC serial on/off | joystick always on");
  if(task==2) Serial.println("a/b/c=run A/B/C, e=save point");
  if(task==3) Serial.println("a=cycle b=rec c=play, e=home");
  if(task==5) Serial.println("a=cancel b=pause/go c=teach, e=demo");
}

// 摇杆点动 
void joyStep(){
  int Vy1=analogRead(JOY1_Y_PIN);
  int Vy2=analogRead(JOY2_Y_PIN);
  if(!latch1){                          // 1号Y:上推左臂+,下推-
    if(Vy1<cenY1-joyTrig){
      servoleft.write(constrain(servoleft.read()+armStep,0,180));
      latch1=true;
    }
    else if(Vy1>cenY1+joyTrig){
      servoleft.write(constrain(servoleft.read()-armStep,0,180));
      latch1=true;
    }
  }
  else if(abs(Vy1-cenY1)<100) latch1=false;
  if(!latch2){                          // 2号Y:同理控制右臂
    if(Vy2<cenY2-joyTrig){
      servoright.write(constrain(servoright.read()+armStep,0,180));
      latch2=true;
    }
    else if(Vy2>cenY2+joyTrig){
      servoright.write(constrain(servoright.read()-armStep,0,180));
      latch2=true;
    }
  }
  else if(abs(Vy2-cenY2)<100) latch2=false;
  delay(20);
}

void baseLeft(){
  int a=constrain(servobase.read()+baseStep,0,180);
  servobase.write(a);
  Serial.print("BASE "); Serial.println(a);
}

void baseRight(){
  int a=constrain(servobase.read()-baseStep,0,180);
  servobase.write(a);
  Serial.print("BASE "); Serial.println(a);
}

void clawOpenStep(){
  clawangle=max(clawangle-clawStep,openangle);
  servoclaw.write(clawangle);
  Serial.print("CLAW "); Serial.println(clawangle);
}

void clawCloseStep(){
  clawangle=min(clawangle+clawStep,closeangle);
  servoclaw.write(clawangle);
  Serial.print("CLAW "); Serial.println(clawangle);
}

void checkBtn(){
  bool b1=digitalRead(JOY1_BTN_PIN)==LOW;
  bool b2=digitalRead(JOY2_BTN_PIN)==LOW;
  if(b1&&!prevB1&&millis()-t1>50) t1=millis();
  if(b2&&!prevB2&&millis()-t2>50) t2=millis();
  prevB1=b1;
  prevB2=b2;
  if(t1&&t2&&abs((long)(t1-t2))<=comboWin){   // 双按:切底座/爪子模式
    t1=0;
    t2=0;
    clawmode=!clawmode;
    Serial.println(clawmode?"CLAW MODE":"BASE MODE");
    return;
  }
  if(t1&&millis()-t1>comboWin){
    t1=0;
    if(clawmode) clawOpenStep();
    else baseLeft();
  }
  if(t2&&millis()-t2>comboWin){
    t2=0;
    if(clawmode) clawCloseStep();
    else baseRight();
  }
}

// 任务一:x,y,z指令+占空比输出(仅pcOn时可用) 
void controlServos(String cmd){
  int line1=cmd.indexOf(',');
  int line2=cmd.indexOf(',',line1+1);
  int targetX=cmd.substring(1,line1).toInt();
  int targetY=cmd.substring(line1+2,line2).toInt();
  int targetZ=cmd.substring(line2+2).toInt();
  moveServosTogether(targetX,targetY,targetZ);
  int usX=map(constrain(targetX,0,180),0,180,544,2400);
  int usY=map(constrain(targetY,0,180),0,180,544,2400);
  int usZ=map(constrain(targetZ,0,180),0,180,544,2400);
  Serial.print("X:"); Serial.print(usX/200.0); Serial.println("%");
  Serial.print("Y:"); Serial.print(usY/200.0); Serial.println("%");
  Serial.print("Z:"); Serial.print(usZ/200.0); Serial.println("%");
}

// 任务二:存点与执行 
// ⑤e存点:按顺序 A夹->A放->B夹->B放->C夹->C放
void saveNext(){
  if(saveSeq>8){ Serial.println("ALL SAVED"); return; }
  savePoint(saveSeq/3,saveSeq%3);   // 每物体3点:夹取->斜后方->释放
  saveSeq++;
}

void savePoint(int idx,int role){
  int x=servobase.read();
  int y=servoleft.read();
  int z=servoright.read();
  char name='A'+idx;
  if(role==0){          // 夹取点
    if(idx==0){ apickdown[0]=x; apickdown[1]=y; apickdown[2]=z; }
    if(idx==1){ bpickdown[0]=x; bpickdown[1]=y; bpickdown[2]=z; }
    if(idx==2){ cpickdown[0]=x; cpickdown[1]=y; cpickdown[2]=z; }
    pickSaved[idx]=true;
    Serial.print("PICK ");
  }
  else if(role==1){     // 斜后方过渡点
    if(idx==0){ aback[0]=x; aback[1]=y; aback[2]=z; }
    if(idx==1){ bback[0]=x; bback[1]=y; bback[2]=z; }
    if(idx==2){ cback[0]=x; cback[1]=y; cback[2]=z; }
    backSaved[idx]=true;
    Serial.print("BACK ");
  }
  else{                 // 释放点
    if(idx==0){ aplacedown[0]=x; aplacedown[1]=y; aplacedown[2]=z; }
    if(idx==1){ bplacedown[0]=x; bplacedown[1]=y; bplacedown[2]=z; }
    if(idx==2){ cplacedown[0]=x; cplacedown[1]=y; cplacedown[2]=z; }
    placeSaved[idx]=true;
    Serial.print("PLACE ");
  }
  Serial.print(name); Serial.print(" SAVED: ");
  Serial.print(x); Serial.print(",");
  Serial.print(y); Serial.print(",");
  Serial.println(z);
}

void tryRun(int idx){
  if(!pickSaved[idx]||!backSaved[idx]||!placeSaved[idx]){
    Serial.print("NOT SET: teach ");
    Serial.print((char)('A'+idx));
    Serial.println(" first!");
    return;
  }
  if(idx==0) runTask(apickdown,aback,aplacedown,aclose);
  if(idx==1) runTask(bpickdown,bback,bplacedown,bclose);
  if(idx==2) runTask(cpickdown,cback,cplacedown,cclose);
}
void runTask(int pick[3],int back[3],int place[3],int close){
  moveServosTogether(back[0],back[1],back[2]);        // 先到斜后方(避开B/C)
  delay(300);
  moveServosTogether(pick[0],pick[1],pick[2]);        // 再进夹取点
  delay(300);
  servoclaw.write(close);
  delay(400);
  moveServosTogether(pick[0],pick[1]+20,pick[2]-20);   // 抬起
  moveServosTogether(back[0],back[1],back[2]);        // 原路退到斜后方再平移
  delay(300);
  moveServosTogether(place[0],place[1],place[2]);
  delay(300);
  servoclaw.write(openangle);
  delay(400);
  moveServosTogether(back[0],back[1],back[2]);        // 放回后仍从斜后方撤出
  delay(300);
  moveServosTogether(homeX,homeY,homeZ);
  Serial.println("DONE");
}

// 任务三:循环/录制/播放/回中 
void doCycle(){
  if(!pickSaved[cycleflag]||!backSaved[cycleflag]||!placeSaved[cycleflag]){
    Serial.println("teach points in TASK2 first!");
    return;
  }
  if(cycleflag==0) runTask(apickdown,aback,aplacedown,aclose);
  else if(cycleflag==1) runTask(bpickdown,bback,bplacedown,bclose);
  else runTask(cpickdown,cback,cplacedown,cclose);
  cycleflag=(cycleflag+1)%3;
}

void joyRecord(){
  if(!recording){
    recording=true;
    nowrec=0;
    lastsample=millis();
    Serial.println("REC START");
  }
  else{
    recording=false;
    Serial.print("REC STOP ");
    Serial.println(nowrec);
  }
}

void playRecord(){
  if(nowrec<2){
    Serial.println("NO DATA");
    return;
  }
  Serial.println("PLAY");
  for(int i=0;i<nowrec;i++){
    moveServosTogether(recBuf[i][0],recBuf[i][1],recBuf[i][2]);
  }
  Serial.println("DONE");
}

//  舵机同步转动
void moveServosTogether(int targetX,int targetY,int targetZ){
  int cx=servobase.read();
  int cy=servoleft.read();
  int cz=servoright.read();
  int disX=abs(cx-targetX);
  int disY=abs(cy-targetY);
  int disZ=abs(cz-targetZ);
  int maxDis=max(disX,max(disY,disZ));
  if(maxDis==0) return;
  float stepX=(float)(targetX-cx)/maxDis;
  float stepY=(float)(targetY-cy)/maxDis;
  float stepZ=(float)(targetZ-cz)/maxDis;
  float accX=cx,accY=cy,accZ=cz;
  for(int i=1;i<=maxDis;i++){
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

// 任务五:逆解/正解 
bool ik(float x,float y,float z,float &s0,float &s1,float &s2){
  float r=sqrt(x*x+y*y)-X0;  // 水平距离减去肩膀轴的水平偏移
  float d2=r*r+z*z;
  float d=sqrt(d2);
  if(d>L1+L2||d<fabs(L1-L2)) return false;
  float th0=atan2(y,x);
  float c2=(d2-L1*L1-L2*L2)/(2*L1*L2);
  if(c2>1) c2=1;
  if(c2<-1) c2=-1;
  float th2=acos(c2);
  float th1=atan2(z,r)-atan2(L2*sin(th2),L1+L2*cos(th2));
  s0=k0*th0*180/PI+off0;
  s1=k1*th1*180/PI+off1;
  s2=k2*(th1+th2)*180/PI+off2;
  if(s0<0||s0>180||s1<0||s1>180||s2<0||s2>180) return false;
  return true;
}

// 正解:角度->坐标
void fk(float s0,float s1,float s2,float &x,float &y,float &z){
  float th0=(s0-off0)/k0*PI/180;
  float th1=(s1-off1)/k1*PI/180;
  float th12=(s2-off2)/k2*PI/180;
  float r=L1*cos(th1)+L2*cos(th12)+X0;  // 加X0:肩膀轴偏移回世界坐标
  x=r*cos(th0);
  y=r*sin(th0);
  z=L1*sin(th1)+L2*sin(th12);
}

bool travelTo(float x,float y,float z){
  float s0,s1,s2;
  if(!ik(x,y,z,s0,s1,s2)) return false;
  servobase.write((int)s0);
  servoleft.write((int)s1);
  servoright.write((int)s2);
  curX=x; curY=y;
  delay(400);
  return true;
}

// 任务五:取消 暂停/继续
bool checkControl(){
  if(Serial.available()>0){
    char c=Serial.read();
    if(c=='b'){                  // ②b暂停,再按b继续
      Serial.println("PAUSE");
      while(true){
        if(Serial.available()>0){
          char c2=Serial.read();
          if(c2=='b'){ Serial.println("RESUME"); break; }
          if(c2=='a'){ Serial.println("CANCEL"); return false; }  
        }
      }
    }
    if(c=='a'){ Serial.println("CANCEL"); return false; }   // ①a取消
  }
  return true;
}

// 任务五:画线引擎 
bool drawlineTask(float x1,float y1,float x2,float y2){
  float len=sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
  int n=(int)(len/2);
  if(n<10) n=10;
  if(n>120) n=120;
  for(int i=1;i<=n;i++){
    if(!checkControl()) return false;
    float x=x1+(x2-x1)*i/n;
    float y=y1+(y2-y1)*i/n;
    float s0,s1,s2;
    if(!ik(x,y,pendown,s0,s1,s2)) continue;
    curX=x; curY=y;
    servobase.write((int)s0);
    servoleft.write((int)s1);
    servoright.write((int)s2);
    delay(40);
  }
  return true;
}

void drawLine(){
  float zA,zB;
  fk(lineEP[0][0],lineEP[0][1],lineEP[0][2],lineX1,lineY1,zA);  // 示教角度换算成坐标+高度
  fk(lineEP[1][0],lineEP[1][1],lineEP[1][2],lineX2,lineY2,zB);
  pendown=(zA+zB)/2;          // 落笔高度=两个端点高度
  penup=pendown+penlift;      // 抬笔高度
  Serial.println("LINE START");
  travelTo(curX,curY,penup);
  travelTo(lineX1,lineY1,penup);
  travelTo(lineX1,lineY1,pendown);
  bool ok=drawlineTask(lineX1,lineY1,lineX2,lineY2);
  travelTo(curX,curY,penup);
  moveServosTogether(homeX,homeY,homeZ);
  Serial.println(ok?"DONE":"ABORTED");
}

void drawV(){
  float z,za=0;
  for(int i=0;i<3;i++){
    fk(vEP[i][0],vEP[i][1],vEP[i][2],v_x[i],v_y[i],z);  // 示教角度换算
    za+=z;
  }
  pendown=za/3;
  penup=pendown+penlift;
  Serial.println("V START");
  travelTo(curX,curY,penup);
  travelTo(v_x[0],v_y[0],penup);
  travelTo(v_x[0],v_y[0],pendown);
  bool ok=true;
  for(int i=0;i<2;i++){
    if(!drawlineTask(v_x[i],v_y[i],v_x[i+1],v_y[i+1])){ ok=false; break; }
  }
  travelTo(curX,curY,penup);
  moveServosTogether(homeX,homeY,homeZ);
  Serial.println(ok?"DONE":"ABORTED");
}

// 折线:连接5个折线示教点
void drawStrai(){
  float z,za=0;
  for(int i=0;i<5;i++){
    fk(lAng[i][0],lAng[i][1],lAng[i][2],l_x[i],l_y[i],z);  // 示教角度换算
    za+=z;
  }
  pendown=za/5;
  penup=pendown+penlift;
  Serial.println("POLYLINE START");
  moveServosTogether(homeX,homeY,homeZ);
  delay(500);
  travelTo(l_x[0],l_y[0],penup);
  travelTo(l_x[0],l_y[0],pendown);
  bool ok=true;
  for(int i=0;i<4;i++){
    if(!drawlineTask(l_x[i],l_y[i],l_x[i+1],l_y[i+1])){ ok=false; break; }
  }
  travelTo(curX,curY,penup);
  moveServosTogether(homeX,homeY,homeZ);
  Serial.println(ok?"DONE":"ABORTED");
}

// 曲线 
float cr(float t,float p0,float p1,float p2,float p3){
  return 0.5*(2*p1+(-p0+p2)*t+(2*p0-5*p1+4*p2-p3)*t*t+(-p0+3*p1-3*p2+p3)*t*t*t);
}

bool drawcurveTask(){
  for(int seg=0;seg<4;seg++){
    int i0=seg-1; if(i0<0) i0=0;
    int i1=seg;
    int i2=seg+1;
    int i3=seg+2; if(i3>4) i3=4;
    for(int j=1;j<=20;j++){
      if(!checkControl()) return false;
      float t=j/20.0;
      float x=cr(t,c_x[i0],c_x[i1],c_x[i2],c_x[i3]);
      float y=cr(t,c_y[i0],c_y[i1],c_y[i2],c_y[i3]);
      float s0,s1,s2;
      if(!ik(x,y,pendown,s0,s1,s2)) continue;
      curX=x; curY=y;
      servobase.write((int)s0);
      servoleft.write((int)s1);
      servoright.write((int)s2);
      delay(40);
    }
  }
  return true;
}

// 曲线:平滑穿过5个曲线示教点
void drawSmooth(){
  float z,za=0;
  for(int i=0;i<5;i++){
    fk(cAng[i][0],cAng[i][1],cAng[i][2],c_x[i],c_y[i],z);  // 示教角度换算
    za+=z;
  }
  pendown=za/5;
  penup=pendown+penlift;
  Serial.println("CURVE START");
  moveServosTogether(homeX,homeY,homeZ);
  delay(500);
  travelTo(c_x[0],c_y[0],penup);
  travelTo(c_x[0],c_y[0],pendown);
  bool ok=drawcurveTask();
  travelTo(curX,curY,penup);
  moveServosTogether(homeX,homeY,homeZ);
  Serial.println(ok?"DONE":"ABORTED");
}

//  任务五:15点顺序示教
// 摇杆走到位->按③c->直接存三个舵机角度(仿照任务二),画的时候才换算坐标
// 顺序:直线端点2个 -> V字端点3个 -> 折线5点 -> 曲线5点
void recordPoint(){
  if(!straiflag){
    straiflag=true;
    tseq=0;
    lcount=0;
    ccount=0;
    Serial.println("TEACH ON: line EP1,EP2 -> V1,V2,V3 -> poly x5 -> curve x5");
    return;
  }
  if(tseq>=15){ Serial.println("ALL 15 SAVED, press e to demo"); return; }
  int b=servobase.read();    // 读舵机角
  int l=servoleft.read();
  int r=servoright.read();
  if(tseq<2){                       // 直线2个端点
    lineEP[tseq][0]=b; lineEP[tseq][1]=l; lineEP[tseq][2]=r;
    Serial.print("LINE EP "); Serial.println(tseq+1);
  }
  else if(tseq<5){                  // V字3个端点
    int i=tseq-2;
    vEP[i][0]=b; vEP[i][1]=l; vEP[i][2]=r;
    Serial.print("V EP "); Serial.println(i+1);
  }
  else if(tseq<10){                 // 折线5点
    int i=tseq-5;
    lAng[i][0]=b; lAng[i][1]=l; lAng[i][2]=r; lcount=i+1;
    Serial.print("POLY P"); Serial.println(i+1);
  }
  else{                             // 曲线5点
    int i=tseq-10;
    cAng[i][0]=b; cAng[i][1]=l; cAng[i][2]=r; ccount=i+1;
    Serial.print("CURVE P"); Serial.println(i+1);
  }
  Serial.print(" B="); Serial.print(b);
  Serial.print(" L="); Serial.print(l);
  Serial.print(" R="); Serial.println(r);
  tseq++;
  if(tseq==15) Serial.println("ALL 15 SAVED");
}

// 任务五:⑤e演示,按一次画一个图形 
// 顺序:直线 -> V字 -> 折线 -> 曲线,循环;点没教够就提示并跳过这次
void demoNext(){
  if(demoSeq==0){
    if(tseq<2){ Serial.println("teach 2 LINE EP first"); return; }
    drawLine();
  }
  else if(demoSeq==1){
    if(tseq<5){ Serial.println("teach 3 V EP first"); return; }
    drawV();
  }
  else if(demoSeq==2){
    if(tseq<10){ Serial.println("teach 5 POLY points first"); return; }
    drawStrai();
  }
  else{
    if(tseq<15){ Serial.println("teach 5 CURVE points first"); return; }
    drawSmooth();
  }
  demoSeq=(demoSeq+1)%4;
}