# serial_test package

## パッケージの説明

mbedのマイコンとROSをつなぐためのライブラリ  

rosserial と役割は同じだが、違いは  
・rosserial 特有の書き方をしなくて良い  
・通信速度が早い  
が挙げられる。  

Float, Int, Char 型の３種類の配列を送受信できる  

## ROS 側

### 起動

ノード名：**serial_test_node**  
launch ファイル：**serial_test.launch**  
パラメータ：  

- **port**  
  ポート名  
  デフォルト：`/dev/ttyACM0`  
- **baudrate**  
  シリアル通信のボーレート(マイコン側と合わせる)  
  デフォルト：`B115200`  
- **looprate**  
  通信読み取りの周期[Hz]  
  デフォルト：`200`  

### トピック・パブリッシュ

- **/Serial_pub_float**  
   メッセージ型：std_msgs::Float32MultiArray  
- **/Serial_pub_int**  
   メッセージ型：std_msgs::Int32MultiArray  
- **/Serial_pub_string**  
   メッセージ型：std_msgs::String  

### トピック・サブスクライブ

- **/Serial_sub_float**  
   メッセージ型：std_msgs::Float32MultiArray  
- **/Serial_sub_int**  
   メッセージ型：std_msgs::Int32MultiArray  
- **/Serial_sub_string**  
   メッセージ型：std_msgs::String  

※パブリッシュ/サブスクライブは最大**200Hz**まで  

### 仕様

配列のサイズはなんでもよい  
文字列の出力 (`/Serial_pub_string`) は文字列の最後に `\n` がつく  

<br>

## mbed 側

### ソースコードの場所

platformio\\nucleo\\src の`mbedserial.cpp`と`mbedserial.h`  
※`example.cpp` はサンプルコード  

### 使い方

- 宣言  

```c++
// ヘッダファイルをインクルード
#include "mbedserial.h"
// 普通のシリアル通信の宣言
// ボーレートも明記するとよい
Serial pc(USBTX, USBRX, 115200);
// 今回作ったクラスの宣言
//引数は上で宣言したmbed::Serialクラスオブジェクト
Mbedserial Ms(pc);
```

- 送信(メンバ関数)  
  - Float 型
    `float_write(float *array, int arraysize);`  
    引数 : ( Float 型の配列またはポインタ, 配列のサイズ )  
  - Int 型  
    `int_write(int *array, int arraysize);`  
    引数 : ( Int 型の配列またはポインタ, 配列のサイズ )  
  - Char 型  
    `char_write(char *array, int arraysize);`  
    引数 : ( Char 型の配列またはポインタ, 配列のサイズ )  

- 受信(メンバ変数)  
  - Float 型  
    `float* getfloat`  
    受信したデータ(Float 型の配列/ポインタ)  
    `int Ms.floatarraysize`  
    受信した配列の長さ(Int)  
  - Int 型  
    `int* getint`  
     受信したデータ(Int 型の配列/ポインタ)  
     `int Mbedserial::intarraysize`  
     受信した配列の長さ(Int)  
  - Char 型  
    `char* getchar`  
     受信したデータ(Char 型の配列/ポインタ)  
     `int chararraysize`  
     受信した配列の長さ(Int)  

- 受信コールバック関数(メンバ関数)  
  PC から受信した時に呼び出される関数の設定  
  引数は void 型・引数無しの関数  

  - Float 型  
    `float_attach(void (*pfunc)())`  
  - Int 型  
    `int_attach(void (*pfunc)())`  
  - Char 型  
    `char_attach(void (*pfunc)())`  

  **注意!**  
  コールバック関数内には `wait` 関数を入れないこと  

### サンプルコード

- 1秒おきに整数配列を送信する処理  

<details>
<summary>write.cpp</summary>

```c++
#include "mbedserial.h"

Serial pc(USBTX, USBRX, 115200);
Mbedserial Ms(pc);

int main()
{
  int data[2] = {0, 0};
  while (1)
  {
    Ms.int_write(data, 2); // 送信

    data[0]++;
    data[1]--;

    wait(1);
  }
}
```
</details>

- 浮動小数点配列を受信して足し合わせる処理  

<details>
<summary>read.cpp</summary>

```c++
#include "mbedserial.h"

Serial pc(USBTX, USBRX, 115200);
Mbedserial Ms(pc);

int main()
{
  while (1)
  {
    float sum = 0;
    int size = Ms.floatarraysize; // 配列サイズを取得
    
    for(int i = 0; i < size; i++){
      sum += Ms.getfloat[i]; // データを取得して加算
    }

    wait(0.1);
  }
}
```
</details>

- Int 型のデータを受信するとLEDが点灯/消灯する処理  

<details>
<summary>callback.cpp</summary>

```c++
#include "mbedserial.h"

Serial pc(USBTX, USBRX, 115200);
Mbedserial Ms(pc);

DigitalOut myled(LED1);

// Int 型のデータを受け取ると呼び出される関数
void CallBack_int(){
  myled = !myled;
}

int main()
{
  // 受信コールバック関数の設定
  Ms.int_attach(CallBack_int);

  while (1)
  {
    wait(0.1);
  }
}
```
</details>

- 受信した値をそのまま返す処理  

<details>
<summary>example.cpp</summary>

```c++
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
```
</details>