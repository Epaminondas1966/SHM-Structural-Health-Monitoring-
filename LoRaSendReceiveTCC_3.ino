/*MIT License

Copyright (c) 2019 Epaminondas Antonine

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


/*Data da ultima compilação 08/10/2019
/*Programa para medição de aceleração para estruturas metalicas ou concreto.
   desenvolvido para o TCC da Mauá em 2019.
   Utiliza o acelerometro MMA8451 placa da Adafruit, este acelerometro tem um nivel
   de ruido baixo se comparado com os concorrentes, não foi utilizado a biblioteca da Adafruit
   para que fosse obtido o maximo controle sobre o acelerometro.
*/

/*The onboard OLED display is SSD1306 driver and I2C interface. In order to make the
  OLED correctly operation, you should output a high-low-high(1-0-1) signal by soft-
  ware to OLED's reset pin, the low-level signal at least 5ms.

  OLED pins to ESP32 GPIOs via this connecthin:
  OLED_SDA -- GPIO4
  OLED_SCL -- GPIO15
  OLED_RST -- GPIO16 */

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <SSD1306Wire.h>

//Deixe esta linha descomentada para compilar o Master
//Comente ou remova para compilar o Slave
//#define BASE

#define SCK 5 // GPIO5 - SX1278's SCK
#define MISO 19 // GPIO19 - SX1278's MISO
#define MOSI 27 // GPIO27 - SX1278's MOSI
#define SS 18 // GPIO18 - SX1278's CS
#define RST 14 // GPIO14 - SX1278's RESET
#define DI00 26 // GPIO26 - SX1278's IRQ (interrupt request)
#define BAND 915E6 //Frequência do radio - exemplo : 433E6, 868E6, 915E6

#define PIN_SDA 4
#define PIN_SCL 15
#define LED_INTERNO 25

//Constante para informar ao Remoto que queremos os dados
const String Rx = "Ba";
//Constante que o Remoto envia junto com os dados para a Base
const String Tx = "Rm";

String  rssi ;
String  pacote ;

//Estrutura com os dados do sensor
 typedef struct {
  double X;
  double Y;
  double Z;
  int16_t Bat;
  double Freq;
  }SensorXYZ;

//Variável para controlar o display
SSD1306Wire display(0x3c, PIN_SDA, PIN_SCL);

void setupDisplay(){
  //O estado do GPIO16 é utilizado para controlar o display OLED
  pinMode(16, OUTPUT);
  //Reseta as configurações do display OLED
  digitalWrite(16, LOW);
  delay(50);
  digitalWrite(16, HIGH);

  //Configurações do display
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
}

//Configurações iniciais do LoRa
void setupLoRa(){ 
  //Inicializa a comunicação
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI00);

  //Inicializa o LoRa
  if (!LoRa.begin(BAND)){
    //Se não conseguiu inicializar, mostra uma mensagem no display
    display.clear();
    display.drawString(0, 0, "Erro ao inicializar o LoRa!");
    display.display();
    while (1);
  }

  //Ativa o crc
  LoRa.enableCrc();
  //Ativa o recebimento de pacotes
  LoRa.receive();
}
