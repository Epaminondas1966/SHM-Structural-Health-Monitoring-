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
//atualizado em 18/10/2019 - ativado, 


#include "SSD1306Spi.h"

#include <WiFi.h>//Biblioteca do WiFi.
#include <BlynkSimpleEsp32.h>

#include <SPI.h>

// If using Hardware SPI (the default case):

#define OLED_DC    4
#define OLED_CS    35
#define OLED_RESET 27




/********************** codigo projeto Blynk *******************************/
char auth[] = "your code must be put here";

/********************** Configurações do Display OLED SPI *******************************/
SSD1306Spi display(27, 4, 5);  // RES, DC, CS

/********************** Configurações do WiFi *******************************/
//const char* ssid     = ""; // Nome da rede WiFi
//const char* password = ""; // Senha da rede WiFi
const char* ssid     = "your wifi name"; // Nome da rede WiFi
const char* password = "your password"; // Senha da rede WiFi

String req;//String que armazena os dados recebidos pela rede.
char* req1;
  String Nome = "Sem Nome";
  String Dia = "0/0";
  String Hora = "00:00:00";
  String Index = "123";
  String X = "0";
  String Y = "0";
  String Z = "0";
  String Bat = "0";
  String Temp = "0";


/********************** Configurações da comunicação Serial com ESP-Blynk*******************************/  
/*
 * There are three serial ports on the ESP known as U0UXD, U1UXD and U2UXD.
 * 
 * U0UXD is used to communicate with the ESP32 for programming and during reset/boot.
 * U1UXD is unused and can be used for your projects. Some boards use this port for SPI Flash access though
 * U2UXD is unused and can be used for your projects.
 * 
*/

#define RXD2 16
#define TXD2 17
  
void setup()
{
  setup_display ();
  
  /********************** ativação do Blynk (IoT) **************************/ 
   
 
   pinMode(2, OUTPUT);//Habilita o LED onboard como saida.
   digitalWrite(2, LOW);//Desliga o LED.
  
   Serial.begin(115200);//Habilita a comunicaçao serial para a string recebida ser lida no Serial monitor.
   Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
   delay (2000);
 /*****************conecta no WiFi**************/
    WIFIScan(1);
    WIFISetUp(); 
   Blynk.begin(auth, ssid, password);
    mensagem2 ();
     }
 
void loop()
{
   listen();//Sub-rotina para verificar a existencia de pacotes seriais.
   mensagem1 ();
    Blynk.run();
   digitalWrite(2, LOW);//Pisca o LED rapidamente apos receber a string.
        
}
 
void listen()//Sub-rotina que verifica se há pacotes seriais para serem lidos.
{
  if (Serial2.available() > 0)//Se houver pacotes para serem lidos
   {
       req = "";//Reseta a string para receber uma nova informaçao
       while (Serial2.available() > 0)//Enquanto houver dados para serem lidos
       {
           char z = Serial2.read();//Adiciona o byte lido em uma char
           req += z;//Adiciona o char à string
       }
      
       }
          
      digitalWrite(2, HIGH);//Liga o Led
       
       splitString ();
    }
      


//Exemplo de mensagem: Remota1;x;y;z;Bat;Temp
//"desmembra" mensagem atribuindo os valores às variáveis
void splitString()
{
  //inicia variáveis com vazio
  Nome = Dia=Hora=Index=X = Y = Z = Bat = Temp =  "";
 
  int i=0;
  
  
  //percorre string até o próximo delimitador atribuindo valor de Nome
  while(i< req.length() && req.charAt(i)!=';')
  {
    Nome+=req.charAt(i);
    i++;
  }
   
  i++;

//percorre string até o próximo delimitador atribuindo valor de Dia
  while(i< req.length() && req.charAt(i)!=';')
  {
    Dia+=req.charAt(i);
    i++;
  }
   
  i++;

//percorre string até o próximo delimitador atribuindo valor de Hora
  while(i< req.length() && req.charAt(i)!=';')
  {
    Hora+=req.charAt(i);
    i++;
  }
   
  i++;

  //percorre string até o próximo delimitador atribuindo valor de Index
  while(i< req.length() && req.charAt(i)!=';')
  {
    Index+=req.charAt(i);
    i++;
  }
   
  i++;
  
  //percorre string até o símbolo ";" atribuindo valor de X
  while(i< req.length() && req.charAt(i)!=';')
  {
    X+=req.charAt(i);
    i++;
  }
  i++;
  //percorre string até o símbolo ";" atribuindo valor de Y
  while(i< req.length() && req.charAt(i)!=';')
  {
    Y+=req.charAt(i);
    i++;
  }
  i++;
 //percorre string até o símbolo ";" atribuindo valor de Z
  while(i< req.length() && req.charAt(i)!=';')
  {
    Z+=req.charAt(i);
    i++;
  }
  i++;

   //percorre string até o símbolo ";" atribuindo valor de Bat
  while(i< req.length() && req.charAt(i)!=';')
  {
    Bat+=req.charAt(i);
    i++;
  }
  i++;
   //percorre string até o símbolo ";" atribuindo valor de Temp
  while(i< req.length() && req.charAt(i)!=';')
  {
   Temp+=req.charAt(i);
    i++;
  }

 if (Nome == "Remota1"){
      Blynk.virtualWrite(V1, X);
      Blynk.virtualWrite(V2, Y);
      Blynk.virtualWrite(V3, Z);
      Blynk.virtualWrite(V4, Temp,2);
      Blynk.virtualWrite(V5, Bat);
 
     }
     if (Nome == "Remota2"){
      Blynk.virtualWrite(V21, X);
      Blynk.virtualWrite(V22, Y);
      Blynk.virtualWrite(V23, Z);
      Blynk.virtualWrite(V24, Temp,2);
      Blynk.virtualWrite(V25, Bat);
     }
      
     if (Nome == "LoRa1"){
      Blynk.virtualWrite(V31, X);
      Blynk.virtualWrite(V32, Y);
      Blynk.virtualWrite(V33, Z);
      Blynk.virtualWrite(V34, Temp,2);
      Blynk.virtualWrite(V35, Bat);
     }
     
  
}


/********************Setup do display***************************/
void setup_display ()
{
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.clear();
  delay(1000);
    display.drawString(0, 0, "Iniciando....");
    display.drawString(0, 10, "Modulo IoT Blynk");
    display.display(); 
  }
void WIFIScan(unsigned int value)
{
  unsigned int i;
  if(WiFi.status() != WL_CONNECTED)
  {
    WiFi.mode(WIFI_MODE_NULL);
  } 
  for(i=0;i<value;i++)
  {
    display.drawString(0, 20, "Scan start...");
    display.display();

    int n = WiFi.scanNetworks();
    display.drawString(0, 30, "Scan done");
    display.display();
    delay(1500);
    display.clear();

    if (n == 0)
    {
      display.clear();
      display.drawString(0, 0, "no network found");
      display.display();
      //while(1);
    }
    else
    {
      display.drawString(0, 0, (String)n);
      display.drawString(14, 0, "networks found:");
      display.display();
      delay(1000);

      for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
        display.drawString(0, (i+1)*9,(String)(i + 1));
        display.drawString(6, (i+1)*9, ":");
        display.drawString(12,(i+1)*9, (String)(WiFi.SSID(i)));
        delay(500);
        display.display();
      }
    }
    delay(1800);
    display.clear();
  }
}
 void WIFISetUp(void)
{
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.begin(ssid, password);//fill in "Your WiFi SSID","Your Password"
  delay(1000);

  byte count = 0;
  while(WiFi.status() != WL_CONNECTED && count < 10)
  {
    count ++;
    delay(500);
    display.drawString(0, 0, "Connecting...");
    display.display();
  }

  display.clear();
  if(WiFi.status() == WL_CONNECTED)
  {
    display.drawString(0, 0, "Connected...OK.");
    display.display();
//    delay(1500);
  }
  else
  {
    display.clear();
    display.drawString(0, 0, "Connecting...Failed");
    display.display();
    //while(1);
  }
  char IP[16];
  sprintf(IP, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3] );
  display.clear();
  display.drawString(0, 0, "WIFI Setup done");
  display.drawString(0, 10, "Connected to: ");
  display.drawString(70, 10, (String)WiFi.SSID());
  display.drawString(0, 30, "IP: ");
  display.drawString(20, 30, IP);
  display.display();
  delay(3000);
}
void mensagem1 (){
  display.clear();
  display.drawString(0, 0, Nome);
  display.drawString(0, 10, X);
  display.drawString(0, 20, Y);
  display.drawString(0, 30, Z);
  display.drawString(0, 40, Temp);
  display.drawString(0, 50, Bat);
  display.display();
}

void mensagem2 (){
  display.clear();
  display.drawString(0, 0, "Preparado");
  display.drawString(0, 10, "Para enviar");
  display.drawString(0, 20, "Informacoes" );
  display.drawString(0, 30, "Para o Blynk");
  display.display();
  delay(2000);
}
