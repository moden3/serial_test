// test code

#include "mbedserial.h"

Serial pc(USBTX, USBRX, 115200);
Mbedserial Ms(pc);

void CallBack_float();
void CallBack_int();
void CallBack_char();

int main()
{
  //受信コールバック関数の設定
  Ms.float_attach(CallBack_float);
  Ms.int_attach(CallBack_int);
  Ms.char_attach(CallBack_char);

  while (1)
  {
    wait(0.1);
  }
}

// コールバック関数の定義
void CallBack_float()
{
  // 受信
  int size = Ms.floatarraysize; // 配列サイズを取得
  float *f = Ms.getfloat;       // データを取得
  // 送信
  Ms.float_write(f, size);
}

void CallBack_int()
{
  // 受信
  int size = Ms.intarraysize; // 配列サイズを取得
  int *i = Ms.getint;         // データを取得
  // 送信
  Ms.int_write(i, size);
}

void CallBack_char()
{
  // 受信
  int size = Ms.chararraysize; // 配列サイズを取得
  char *c = Ms.getchar;        // データを取得
  // 送信
  Ms.char_write(c, size);
}