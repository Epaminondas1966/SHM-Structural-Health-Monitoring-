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


//Programa de recepção dos acelerometros que estão remotos, trabalho de TCC
//Maua 2019.
//atualizado em 13/10/2019 - acrescentado servidor de hora NTP
//atualizado em 14/10/2019 - acrescentado estrutura para decodificar as strings recebidas
//atualizado em 17/10/2019 - acrescentado comunicação serial para passar dados para o ESP32-Blynk
//atualizado em 22/10/2019 - Mudança de biblioteca do drive Oled



#include "SSD1306Spi.h"
#include <WiFi.h>//Biblioteca do WiFi.
#include <WiFiUdp.h>//Biblioteca do UDP.
#include <SD.h>
#include "FS.h"

/********************** Configurações do Display OLED SPI *******************************/
SSD1306Spi display(27, 4, 35);  // RES, DC, CS

WiFiUDP udp;//Cria um objeto da classe UDP.

String req;//String que armazena os dados recebidos pela rede.
char* req1;

String Nome;
String Dia;
String Hora;
String Index;
String X;
String Y;
String Z;
String Bat;
String Temp;


/********************** Configurações da comunicação Serial com Mestre IoT*******************************/
/*
   There are three serial ports on the ESP known as U0UXD, U1UXD and U2UXD.

   U0UXD is used to communicate with the ESP32 for programming and during reset/boot.
   U1UXD is unused and can be used for your projects. Some boards use this port for SPI Flash access though
   U2UXD is unused and can be used for your projects.

*/

#define RXD2 16
#define TXD2 17

int Pino_CS = 5;// CS do cartão SD.

void setup()
{
  pinMode(35, OUTPUT);//Habilita saida 35 como controle CS
  digitalWrite(35, LOW);//Desliga a saida.

  Serial.begin(115200);//Habilita a comunicaçao serial para a string recebida ser lida no Serial monitor.
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  delay(1000);// tempo para estabelecer a alimentação no display.
  setup_display ();

  pinMode(2, OUTPUT);//Habilita o LED onboard como saida.
  digitalWrite(2, LOW);//Desliga o LED.

  WiFi.mode(WIFI_AP);//Define o ESP32 como Acess Point
  WiFi.softAP("ESP32_TCC", "");//Cria um WiFi de nome ESP32_TCC e sem senha.
  //endereço IP do Acess Point criado automaticamente (HOST = 192.168.4.1)


  display.clear();
  display.drawString(0, 0, "Access Point...");
  display.drawString(0, 10, "AP:192.168.4.1");
  display.drawString(0, 20, "Rede ESP32_TCC");
  display.display();

  delay(2000);//Aguarda 2 segundos para completar a criaçao do wifi.
  udp.begin(555);//Inicializa a recepçao de dados UDP na porta 555


  /********************** ativação do cartão SD**************************/
  display.clear();
  display.drawString(0, 0, "Iniciando SD Card...");
  display.display();
  
  //Inicia o cartao SD
  if (!SD.begin(Pino_CS)) {
    display.drawString(0, 10, "Erro no SD Card");
    display.display();
    while (1);
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    display.drawString(0, 10, "Erro no SD Card");
    display.display();
    while (1);
  }
  delay(1000);
    display.drawString(0, 10, "SD Card, OK");
    display.display();
    delay(2000);
}

void loop()
{
  listen();//Sub-rotina para verificar a existencia de pacotes UDP.
  delay(0);
  display.clear();
  display.drawString(0, 0, Nome);
  display.display();
}

/********Sub-rotina que verifica se há pacotes UDP's para serem lidos*/

void listen()
{
  if (udp.parsePacket() > 0)//Se houver pacotes para serem lidos
  {
    req = "";//Reseta a string para receber uma nova informaçao
    while (udp.available() > 0)//Enquanto houver dados para serem lidos
    {
      char z = udp.read();//Adiciona o byte lido em uma char
      req += z;//Adiciona o char à string
    }


    //Após todos os dados serem lidos, a String estara pronta.

    Serial2.println(req); //envia para o ESP-Blynk.
    digitalWrite(2, LOW);//Liga o Led
    delay(10);//-
    digitalWrite(2, HIGH);//Pisca o LED rapidamente apos receber a string.

    splitString ();
  }

}

//Exemplo de mensagem: Remota1;x;y;z;Bat;Temp
//"desmembra" mensagem atribuindo os valores às variáveis
void splitString()
{
  //inicia variáveis com vazio
  Nome = Dia = Hora = Index = X = Y = Z = Bat = Temp =  "";

  int i = 0;


  //percorre string até o próximo delimitador atribuindo valor de Nome
  while (i < req.length() && req.charAt(i) != ';')
  {
    Nome += req.charAt(i);
    i++;
  }
  if (Nome == "Remota1") {

    req.toCharArray(req1, req.length());
    if (!SD.exists("/Sensor1.txt")) {
      writeFile(SD, "/Sensor1.txt", 0);
    }
    if (SD.exists("/Sensor1.txt")) {
      appendFile(SD, "/Sensor1.txt", req1);
    }
  }

  if (Nome == "Remota2") {

    req.toCharArray(req1, req.length());
    if (!SD.exists("/Sensor2.txt")) {
      writeFile(SD, "/Sensor2.txt", 0);
    }
    if (SD.exists("/Sensor2.txt")) {
      appendFile(SD, "/Sensor2.txt", req1);
    }
  }

  if (Nome == "LoRa1") {

    req.toCharArray(req1, req.length());
    if (!SD.exists("/LoRa1.txt")) {
      writeFile(SD, "/LoRa1.txt", 0);
    }
    if (SD.exists("/LoRa1.txt")) {
      appendFile(SD, "/LoRa1.txt", req1);
    }
  }
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    return;
  }
  if (file.println(message)) {
    file.close();
  }

}

void appendFile(fs::FS &fs, const char * path, const char * message) {
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    return;
  }
  if (file.println(req)) {
    file.close();
  }

}

/********************Setup do display***************************/
void setup_display ()
{
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  delay(1000);
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Iniciando....");
  display.drawString(0, 10, "Modulo Mestre");
  display.drawString(0, 20, "Receptor");
  display.display();
  delay(2000);
}
