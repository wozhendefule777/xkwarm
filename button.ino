// ESP8266 按键发送板 
//   ① GPIO5  -> '1'
//   ② GPIO4  -> '2'
//   ③ GPIO13 -> '3'
//   ④ GPIO12 -> '4'   (切换任务 1->2->3->5->1)
//   ⑤ GPIO14 -> 'T'
// 接线:ESP8266 TX(GPIO1) -> UNO RX(D0),两板必须共地(GND)
const int btnPin[5]={5,4,13,12,14};        // ①②③④⑤
const char btnChar[5]={'1','2','3','4','T'};

//   任务一: ①开/关电脑串口pcOn
//   任务二: ①②③=搬A/B/C, ⑤=存示教点(9点:夹取->斜后方->释放)
//   任务三: ①循环搬A/B/C, ②录制/停止, ③播放, ⑤回初始位
//   任务五: ①取消, ②暂停/继续, ③存示教点(15点顺序), ⑤演示画图
//   任意任务: ④=切换任务

bool pressed(int pin){
  if(digitalRead(pin)==LOW){       // 按下
    delay(20);                     // 消抖
    if(digitalRead(pin)==LOW){
      while(digitalRead(pin)==LOW); // 等松手,按住只发一次
      return true;
    }
  }
  return false;
}

void setup(){
  Serial.begin(9600);              
  for(int i=0;i<5;i++) pinMode(btnPin[i],INPUT_PULLUP);
}

void loop(){
  for(int i=0;i<5;i++){
    if(pressed(btnPin[i])){
      Serial.write(btnChar[i]);    // 发单个字符,不带换行
    }
  }
}