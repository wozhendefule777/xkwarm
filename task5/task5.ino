#include <Servo.h>

Servo servobase;
Servo servoleft;
Servo servoright;
Servo servoclaw;

const int SERVO_BASE_PIN=9;
const int SERVO_LEFT_PIN=8;
const int SERVO_RIGHT_PIN=7;
const int SERVO_CLAW_PIN=6;

const float L1=100.0;      
const float L2=100.0;      

float pendown=30.0;       
const float penup=60.0;    

float k0=1.0, off0=90.0;   
float k1=1.0, off1=90.0;   
float k2=1.0, off2=90.0;   

const int penhold=60;      

const int homeX=0;         
const int homeY=0;
const int homeZ=0;

float curX=150;            
float curY=0;

float lineX1=120, lineY1=-40;  
float lineX2=160, lineY2= 40; 

float v_x[3]={120,170,120};    
float v_y[3]={ 30, 30,-30};    

float tx[5]={0,0,0,0,0};        
float ty[5]={0,0,0,0,0};        
int tcount=0;                   
bool teachflag=false;  
bool smoothflag=false;            

void setup(){
  Serial.begin(9600);
  servobase.attach(SERVO_BASE_PIN);
  servoleft.attach(SERVO_LEFT_PIN);
  servoright.attach(SERVO_RIGHT_PIN);
  servoclaw.attach(SERVO_CLAW_PIN);
  servobase.write(homeX);
  servoleft.write(homeY);
  servoright.write(homeZ);
  servoclaw.write(penhold);
  delay(500);
  Serial.println("READY");
}

void loop(){
  if(Serial.available()>0){
    char cmd=Serial.read();
    switch(cmd){
      case 'D': drawLine();    break; 
      case 'V': drawV();       break;   
      case 'R': recordPoint(); break;   
      case 'X': teachflag=false;       
                Serial.println("TEACH OFF"); break;
      case 'H': moveAll(homeX,homeY,homeZ);
                Serial.println("HOME"); break;
      case '+': pendown=pendown+2;      
                travelTo(curX,curY,pendown);
                Serial.println(pendown); break;
      case '-': pendown=pendown-2;      
                travelTo(curX,curY,pendown);
                Serial.println(pendown); break;
      case 'P': printPos(); break;
      case 'M': smoothflag=!smoothflag;
                Serial.println(smoothflag?"CURVE MODE":"LINE MODE");
                break;
    }
  }
  if(teachflag) joyTeach();    
}

bool drawTask(float x1,float y1,float x2,float y2){
  float len=sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
  int n=(int)(len/2.0);
  if(n<10) n=10;
  if(n>120) n=120;
  for(int i=1;i<=n;i++){
    if(!checkControl())
      return false;
    float x=x1+(x2-x1)*i/n;
    float y=y1+(y2-y1)*i/n;
    int s0,s1,s2;
    if(!ik(x,y,pendown,s0,s1,s2))
      continue;
    curX=x; curY=y;
    servobase.write(s0);
    servoleft.write(s1);
    servoright.write(s2);
    delay(40);
  }
  return true;
}

void drawLine(){
  Serial.println("DRAW LINE");
  travelTo(lineX1,lineY1,penup);
  travelTo(lineX1,lineY1,pendown);
  if(!drawTask(lineX1,lineY1,lineX2,lineY2)){
    abortDraw();
    return;
  }
  moveAll(homeX,homeY,homeZ);
  Serial.println("DONE");
}

void drawV(){
  Serial.println("DRAW V");
  travelTo(v_x[0],v_y[0],penup);
  travelTo(v_x[0],v_y[0],pendown);
  for(int i=0;i<2;i++){
    if(!drawTask(v_x[i],v_y[i],v_x[i+1],v_y[i+1])){
      abortDraw();
      return;
    }
  }
  moveAll(homeX,homeY,homeZ);
  Serial.println("DONE");
}

void recordPoint(){
  if(!teachflag){                        
    teachflag=true;
    tcount=0;
    travelTo(curX,curY,pendown+8);       
    Serial.println("TEACH ON");
    return;
  }
  tx[tcount]=curX;                       
  ty[tcount]=curY;
  tcount++;
  Serial.print("POINT ");Serial.print(tcount);
  Serial.print(" X");Serial.print(curX);
  Serial.print(" Y");Serial.println(curY);
  if(tcount==5){
    Serial.println("5 POINTS OK");
    delay(1500);                    
    if(smoothflag) drawSmooth();    
    else           drawTeach();    
  }           
}

float cr(float t,float p0,float p1,float p2,float p3){
  return 0.5*(2*p1 + (-p0+p2)*t + (2*p0-5*p1+4*p2-p3)*t*t
             + (-p0+3*p1-3*p2+p3)*t*t*t);
}

bool drawSmooth(){
  for(int seg=0;seg<4;seg++){              
    int i0=seg-1; if(i0<0) i0=0;          
    int i1=seg;
    int i2=seg+1;
    int i3=seg+2; if(i3>4) i3=4;           
    for(int j=1;j<=20;j++){                
      if(!checkControl())
        return false;
      float t=j/20.0;
      float x=cr(t,tx[i0],tx[i1],tx[i2],tx[i3]);
      float y=cr(t,ty[i0],ty[i1],ty[i2],ty[i3]);
      int s0,s1,s2;
      if(!ik(x,y,pendown,s0,s1,s2))
        continue;
      curX=x; curY=y;
      servobase.write(s0);
      servoleft.write(s1);
      servoright.write(s2);
      delay(40);
    }
  }
  return true;
}

void drawsmoothTask(){
  Serial.println("DRAW CURVE");
  moveAll(homeX,homeY,homeZ);              
  travelTo(tx[0],ty[0],penup);
  travelTo(tx[0],ty[0],pendown);
  if(!drawSmooth()){
    abortDraw();
    return;
  }
  travelTo(tx[4],ty[4],penup);           
  moveAll(homeX,homeY,homeZ);
  Serial.println("DONE");
}

void joyTeach(){
  int jx=analogRead(A0);
  int jy=analogRead(A1);
  float nx=curX, ny=curY;
  if(jx>800) nx=nx+2;
  if(jx<200) nx=nx-2;
  if(jy>800) ny=ny+2;
  if(jy<200) ny=ny-2;
  if(nx==curX&&ny==curY) return;         
  int s0,s1,s2;
  if(ik(nx,ny,pendown+8,s0,s1,s2)){      
    curX=nx; curY=ny;
    servobase.write(s0);
    servoleft.write(s1);
    servoright.write(s2);
  }
  delay(40);
}

void drawTeach(){
  if(tcount<5){Serial.println("NEED 5 POINTS");return;}
  teachflag=false;
  moveAll(homeX,homeY,homeZ);            
  travelTo(tx[0],ty[0],penup);
  travelTo(tx[0],ty[0],pendown);
  for(int i=0;i<4;i++){
    if(!drawTask(tx[i],ty[i],tx[i+1],ty[i+1])){
      abortDraw();
      return;
    }
  }
  travelTo(tx[4],ty[4],penup);
  Serial.println("DONE");
}

void abortDraw(){
  travelTo(curX,curY,penup);
  moveAll(homeX,homeY,homeZ);
  Serial.println("ABORTED");
}

void travelTo(float x,float y,float z){
  int s0,s1,s2;
  if(!ik(x,y,z,s0,s1,s2)){
    Serial.println("OUT OF REACH");
    return;
  }
  curX=x;
  curY=y;
  moveAll(s0,s1,s2);
}

bool checkControl(){
  if(Serial.available()>0){
    char a=Serial.read();
    if(a=='X'){
      Serial.println("CANCEL");
      return false;
    }
    if(a=='T'){
      Serial.println("STOP");
      while(true){
        if(Serial.available()>0){
          char b=Serial.read();
          if(b=='G'){
            Serial.println("GO");
            return true;
          }
          if(b=='X'){
            Serial.println("CANCEL");
            return false;
          }
        }
      }
    }
  }
  return true;
}

bool ik(float x,float y,float z,int &s0,int &s1,int &s2){
  float r=sqrt(x*x+y*y);
  float c=(x*x+y*y+z*z-L1*L1-L2*L2)/(2*L1*L2);
  if(c>1.0||c<-1.0) return false;
  float th2=acos(c);
  float th1=atan2(z,r)-atan2(L2*sin(th2),L1+L2*cos(th2));
  float th0=atan2(y,x);
  s0=(int)(k0*th0*180/PI+off0);
  s1=(int)(k1*th1*180/PI+off1);
  s2=(int)(k2*(th1+th2)*180/PI+off2);   // parallel arm: absolute angle
  return true;
}

void printPos(){
  Serial.print(servobase.read());
  Serial.print(',');
  Serial.print(servoleft.read());
  Serial.print(',');
  Serial.println(servoright.read());
}

void moveAll(int targetX,int targetY,int targetZ){
  int fromX=servobase.read();
  int fromY=servoleft.read();
  int fromZ=servoright.read();
  int disX=abs(fromX-targetX);
  int disY=abs(fromY-targetY);
  int disZ=abs(fromZ-targetZ);
  int maxDis=max(disX,max(disY,disZ));
  if(maxDis==0) return;
  float stepX=(float)(targetX-fromX)/maxDis;
  float stepY=(float)(targetY-fromY)/maxDis;
  float stepZ=(float)(targetZ-fromZ)/maxDis;
  float accX=fromX;
  float accY=fromY;
  float accZ=fromZ;
  for(int i=1;i<=maxDis;i++){
    accX+=stepX;
    accY+=stepY;
    accZ+=stepZ;
    servobase.write(int(accX));
    servoleft.write(int(accY));
    servoright.write(int(accZ));
    delay(30);
  }
  servobase.write(targetX);
  servoleft.write(targetY);
  servoright.write(targetZ);
}