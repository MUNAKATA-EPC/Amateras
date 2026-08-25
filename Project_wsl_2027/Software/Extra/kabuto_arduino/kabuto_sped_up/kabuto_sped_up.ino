#include <RPlib.h>                    //  ロボプロシールドを使うためのライブラリ(設定ファイル)
#include <Servo.h>                    //  サーボモーターを使うためのライブラリ(設定ファイル)
#include <PS2X_lib.h>                 //  コントローラーを使うためのライブラリ(設定ファイル)
#include <MsTimer2.h>                 //  タイマー割り込みを使うためのライブラリ(設定ファイル)

#define   H_TARGET_POSITION_1    10   //  ツノを上げた時の目標位置
#define   H_TARGET_POSITION_2   142   //  ツノを下げた時の目標位置
#define   H_BASE_POSITION        90   //  ツノの原点位置

#define   STRIDE_ANGLE        75      //  脚を前後に動かすときのサーボモーターを回す移動量
#define   LIFT_ANGLE          60      //  脚を上げるときのサーボモーターを回す移動量
#define   MOVE_RATE           15      //  現在値の更新をするときの1回あたりの進み量

#define   L_BASE_POSITION     90      //  左脚の原点位置
#define   R_BASE_POSITION     90      //  右脚の原点位置
#define   M_BASE_POSITION     90      //  中脚の原点位置

#define   DELAY_TIME          100     //  初期化の時にサーボモーターを1つずつ原点移動させるときの待ち時間(ミリ秒)
#define   SERVO_PERIOD         20     //  サーボモーターの現在位置を更新する時間間隔(ミリ秒)
#define   GAMEPAD_PERIOD      100     //  コントローラーのボタンの状態を更新する時間間隔(ミリ秒)


#define   MODE_STOP             0     //  歩行モード停止
#define   MODE_FORWARD          1     //  歩行モード前進
#define   MODE_BACKWARD         2     //  歩行モード後退
#define   MODE_TURN_LEFT        3     //  歩行モード左旋回
#define   MODE_TURN_RIGHT       4     //  歩行モード右旋回

#define   STEP_1                0     //  動作ステップ1
#define   STEP_2                1     //  動作ステップ2
#define   STEP_3                2     //  動作ステップ3
#define   STEP_4                3     //  動作ステップ4
#define   STEP_5                4     //  動作ステップ5
#define   STEP_6                5     //  動作ステップ6

int mode_flag;                        //  歩行モードの管理用
int step_flag;                        //  動作ステップの管理用

Servo servoL;                         //  サーボモーターを使うときのオマジナイ(個別に名づけて用意する)
Servo servoR;                         //  L(Left)・R(Right)・M(Middle)・H(Head)の意味
Servo servoM;
Servo servoH;

PS2X ps2x;                            //  コントローラーに名前をつける

int targetL;                          //  servoLの目標(target)位置を管理
int targetR;                          //  servoRの目標(target)位置を管理
int targetM;                          //  servoMの目標(target)位置を管理
int targetH;                          //  servoHの目標(target)位置を管理

int currentL;                         //  servoLの現在(current)位置を管理
int currentR;                         //  servoRの現在(current)位置を管理
int currentM;                         //  servoMの現在(current)位置を管理
int currentH;                         //  servoHの現在(current)位置を管理


void setup(){
  mode_flag = MODE_STOP;                           //  歩行モード停止(コントローラーのボタンの状態の取得前が不定だと困るため)
  step_flag = STEP_1;                              //  ステップは１からはじめる

  targetL  = L_BASE_POSITION;                      //  目標位置は仮にL_BASE_POSITIONと設定
  targetR  = R_BASE_POSITION;                      //  目標位置は仮にR_BASE_POSITIONと設定
  targetM  = M_BASE_POSITION;                      //  目標位置は仮にM_BASE_POSITIONと設定
  targetH  = H_BASE_POSITION;                      //  目標位置は仮にH_BASE_POSITIONと設定

  currentL = L_BASE_POSITION;                      //  現在位置は仮にL_BASE_POSITIONと設定
  currentR = R_BASE_POSITION;                      //  現在位置は仮にR_BASE_POSITIONと設定
  currentM = M_BASE_POSITION;                      //  現在位置は仮にM_BASE_POSITIONと設定
  currentH = H_BASE_POSITION;                      //  現在位置は仮にH_BASE_POSITIONと設定

  ps2x.config_gamepad(13, 11, 10, 12);             //  コントローラーの初期化
  MsTimer2::set(GAMEPAD_PERIOD, gamepad_update);   //  タイマー割り込み機能を利用(GAMEPAD_PERIODミリ秒ごとにgamepad_update()を実行するよう設定)

  pinMode(D2, INPUT_PULLUP);                       //  D2にタッチセンサーを接続
  pinMode(D3, INPUT_PULLUP);                       //  D3にタッチセンサーを接続

  servoL.attach(S0);                               //  servoLと名づけたモーターをS0に接続
  servoR.attach(S1);                               //  servoRと名づけたモーターをS1に接続
  servoM.attach(S2);                               //  servoMと名づけたモーターをS2に接続
  servoH.attach(S3);                               //  servoHと名づけたモーターをS3に接続

  servoL.write(currentL);                          //  servoLのサーボモーターの初期位置を設定
  delay(DELAY_TIME);                               //  DELAY_TIMEミリ秒待つ(同時に多くのモーターを動かすと電源が不安定になるため)
  servoR.write(currentR);                          //  servoRのサーボモーターの初期位置を設定
  delay(DELAY_TIME);
  servoM.write(currentM);                          //  servoMのサーボモーターの初期位置を設定
  delay(DELAY_TIME);
  servoH.write(currentH);                          //  servoHのサーボモーターの初期位置を設定
  delay(DELAY_TIME);

  MsTimer2::start();                               //  タイマー割り込みスタート
}

void loop(){
  target_update();                      //  目標位置を管理
  servo_update();                       //  現在位置を管理

  servoL.write(currentL);               //  現在位置をservoLに反映
  servoR.write(currentR);               //  現在位置をservoRに反映
  servoM.write(currentM);               //  現在位置をservoMに反映
  servoH.write(currentH);               //  現在位置をservoHに反映


  if ( ( currentL == targetL ) && ( currentR == targetR ) && ( currentM == targetM ) ){
    //  もし全ての脚の目標位置と現在位置が一致したら
    if ((mode_flag == MODE_STOP) && (step_flag == STEP_1 || step_flag == STEP_4)){
      ;
    }
    else{
      step_flag++;                      //  動作ステップを進める
    }

    if (step_flag > STEP_6){            //  動作ステップを最初に戻す
      step_flag = STEP_1;
    }
  }

  delay(SERVO_PERIOD);                  //  SERVO_PERIODミリ秒待つ
}

//
//  歩行モードと動作ステップによって各脚の目標位置を更新
//
void target_update(){
  if (step_flag == STEP_1){
    //  もし動作ステップが1だったら
    targetM  = M_BASE_POSITION;         //  中脚の目標位置を原点に設定
  }
  else if (step_flag == STEP_2){
    //  もし動作ステップが2だったら
    targetM = M_BASE_POSITION - LIFT_ANGLE;              //  中脚の目標位置を左前後脚遊脚に設定
  }
  else if (step_flag == STEP_3){
    //  もし動作ステップが3だったら
    if (mode_flag == MODE_FORWARD){
      //  もし歩行モードが前進だったら
      targetL = L_BASE_POSITION + STRIDE_ANGLE;          //  左脚の目標位置を前に設定
      targetR = R_BASE_POSITION + STRIDE_ANGLE;          //  右脚の目標位置を後ろに設定
    }
    else if (mode_flag == MODE_BACKWARD){
      //  もし歩行モードが後退だったら
      targetL = L_BASE_POSITION - STRIDE_ANGLE;          //  左脚の目標位置を後ろに設定
      targetR = R_BASE_POSITION - STRIDE_ANGLE;          //  右脚の目標位置を前に設定
    }
    else if (mode_flag == MODE_TURN_LEFT){
      //  もし歩行モードが左旋回だったら
      targetL = L_BASE_POSITION - STRIDE_ANGLE;          //  左脚の目標位置を後ろに設定
      targetR = R_BASE_POSITION + STRIDE_ANGLE;          //  右脚の目標位置を後ろに設定
    }
    else if (mode_flag == MODE_TURN_RIGHT){
      //  もし歩行モードが右旋回だったら
      targetL = L_BASE_POSITION + STRIDE_ANGLE;          //  左脚の目標位置を前に設定
      targetR = R_BASE_POSITION - STRIDE_ANGLE;          //  右脚の目標位置を前に設定
    }
    else {
      //  そうでなかった(歩行モードが停止だった)ら
      targetL = L_BASE_POSITION;                         //  左脚の目標位置を原点に設定
      targetR = R_BASE_POSITION;                         //  右脚の目標位置を原点に設定
    }
  }
  else if (step_flag == STEP_4){
    //  もし動作ステップが4だったら
    targetM  = M_BASE_POSITION;                          //  中脚の目標位置を原点に設定
  }
  else if (step_flag == STEP_5){
    //  もし動作ステップが5だったら
    targetM = M_BASE_POSITION + LIFT_ANGLE;              //  中脚の目標位置を右前後脚遊脚に設定
  }
  else if (step_flag == STEP_6){
    //  もし動作ステップが6だったら
    if (mode_flag == MODE_FORWARD){
      //  もし歩行モードが前進だったら
      targetL = L_BASE_POSITION - STRIDE_ANGLE;          //  左脚の目標位置を後ろに設定
      targetR = R_BASE_POSITION - STRIDE_ANGLE;          //  右脚の目標位置を前に設定
    }
    else if (mode_flag == MODE_BACKWARD){
      //  もし歩行モードが後退だったら
      targetL = L_BASE_POSITION + STRIDE_ANGLE;          //  左脚の目標位置を前に設定
      targetR = R_BASE_POSITION + STRIDE_ANGLE;          //  右脚の目標位置を後ろに設定
    }
    else if (mode_flag == MODE_TURN_LEFT){
      //  もし歩行モードが左旋回だったら
      targetL = L_BASE_POSITION + STRIDE_ANGLE;          //  左脚の目標位置を前に設定
      targetR = R_BASE_POSITION - STRIDE_ANGLE;          //  右脚の目標位置を前に設定
    }
    else if (mode_flag == MODE_TURN_RIGHT){
      //  もし歩行モードが右旋回だったら
      targetL = L_BASE_POSITION - STRIDE_ANGLE;          //  左脚の目標位置を後ろに設定
      targetR = R_BASE_POSITION + STRIDE_ANGLE;          //  右脚の目標位置を後ろに設定
    }
    else {
      //  そうでなかった(歩行モードが停止だった)ら
      targetL = L_BASE_POSITION;                         //  左脚の目標位置を原点に設定
      targetR = R_BASE_POSITION;                         //  右脚の目標位置を原点に設定
    }
  }
  else {
    //  動作ステップが1～6でないということはプログラム上発生しない
  }
}

//
//  サーボモーターの目標位置に現在位置を近づけて移動
//
void servo_update(){
  if (currentL < targetL){            //  もしservoLの現在位置が目標位置より小さかったら
    currentL += MOVE_RATE;          //  現在値を+MOVE_RATE
    if (currentL > targetL){
      currentL = targetL;
    }
  }
  else if (currentL > targetL){       //  もしservoLの現在位置が目標位置より大きかったら
    currentL -= MOVE_RATE;            //  現在値を-MOVE_RATE
    if (currentL < targetL){
      currentL = targetL;
    }
  }
  if (currentR < targetR){            //  もしservoRの現在位置が目標位置より小さかったら
    currentR += MOVE_RATE;            //  現在値を+MOVE_RATE
    if (currentR > targetR){
      currentR = targetR;
    }
  }
  else if (currentR > targetR){       //  もしservoRの現在位置が目標位置より大きかったら
    currentR -= MOVE_RATE;            //  現在値を-MOVE_RATE
    if (currentR < targetR){
      currentR = targetR;
    }
  }
  if (currentM < targetM){            //  もしservoMの現在位置が目標位置より小さかったら
    currentM += MOVE_RATE;            //  現在値を+MOVE_RATE
    if (currentM > targetM){
      currentM = targetM;
    }
  }
  else if (currentM > targetM){       //  もしservoMの現在位置が目標位置より大きかったら
    currentM -= MOVE_RATE;            //  現在値を-MOVE_RATE
    if (currentM < targetM){
      currentM = targetM;
    }
  }
  if (currentH < targetH){            //  もしservoHの現在位置が目標位置より小さかったら
    currentH += MOVE_RATE;            //  現在値を+MOVE_RATE
    if (currentH > targetH){
      currentH = targetH;
    }
  }
  else if (currentH > targetH){       //  もしservoHの現在位置が目標位置より大きかったら
    currentH -= MOVE_RATE;            //  現在値を-MOVE_RATE
    if (currentH < targetH){
      currentH = targetH;
    }
  }
}

//
//  コントローラーのボタンの状態を取得し歩行モードを更新
//
void  gamepad_update(){
  ps2x.read_gamepad();                       //  コントローラーから値を読み取る

  if (ps2x.Button(PSB_PAD_UP)    == HIGH){   //  もし十字ボタンの上がおされたら
    mode_flag = MODE_FORWARD;                //  歩行モードを前進にする
  }
  else if (ps2x.Button(PSB_PAD_DOWN)  == HIGH){        //  もし十字ボタンの下がおされたら
    mode_flag = MODE_BACKWARD;                         //  歩行モードを後退にする
  }
  else if (ps2x.Button(PSB_PAD_LEFT)  == HIGH){        //  もし十字ボタンの左がおされたら
    mode_flag = MODE_TURN_LEFT;                        //  歩行モードを左旋回にする
  }
  else if (ps2x.Button(PSB_PAD_RIGHT)  == HIGH){       //  もし十字ボタンの左がおされたら
    mode_flag = MODE_TURN_RIGHT;                       //  歩行モードを右旋回にする
  }
  else {                                   //  そうでなかった（十字ボタンがどれもおされていなかった）ら
    mode_flag = MODE_STOP;                 //  歩行モード停止
  }

  // ツノの目標値の設定
  /*
  if (ps2x.Button(PSB_L2) == HIGH){        //  もしL2ボタンがおされたら
   targetH = H_TARGET_POSITION_1;         //  ツノの目標位置を上に設定
   }
   else if (ps2x.Button(PSB_R2) == HIGH){   //  もしR2ボタンがおされたら
   targetH = H_TARGET_POSITION_2;         //  ツノの目標位置を下に設定
   }
   else {                                   //  そうでなかった（L2もR2もおされていなかった）ら
   targetH = H_BASE_POSITION;             //  目標位置を原点に設定
   }
   */
  if(mode_flag == MODE_FORWARD){
    if(step_flag == STEP_1 || step_flag == STEP_4){
      targetH = H_TARGET_POSITION_2;         //  ツノの目標位置を下に設定
    }
    else
    {
      targetH = H_BASE_POSITION;             //  目標位置を原点に設定
    }
  }
  else{
    targetH = H_BASE_POSITION;             //  目標位置を原点に設定
  }
}










