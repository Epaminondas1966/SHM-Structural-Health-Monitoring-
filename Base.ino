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


//Compila apenas se MASTER estiver definido no arquivo principal
// inserido comunicação UDP com o ESP-AP
//Retirado as rotinas de Blynk
//alterações em 17/10/2019
//alterações em 21/10/2019 inserido RTC e retirado rotinas desnecessarias

#ifdef BASE

/********************** Configurações do WiFi *******************************/
#include <WiFi.h>
#include <WiFiUdp.h>
WiFiUDP udp;//Cria um objeto da classe UDP.

const char* ssid     = "ESP32_TCC";
const char* password = "";
const String Nome = "LoRa1"; // nome do unidade sensora LoRa

unsigned long Contador = 1;
String Data_Comp;
/********************** Configurações do RTC *******************************/
#include "RTClib.h"
#define DS3231_I2C_ADDR             0x68

char Tempo [15];// array de concatenação de informações.
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
RTC_DS3231 rtc;

void setup(){

  Serial.begin(115200);
  pinMode(LED_INTERNO, OUTPUT);
  //Chama a configuração inicial do display
  setupDisplay();

  /********************** verificação do RTC *******************************/
 if (! rtc.begin()) {
    //Serial.println("Couldn't find RTC");
       while (1);
  }

     //rtc.adjust(DateTime(2019, 10, 21, 21, 52, 0));

      
  //Chama a configuração inicial do LoRa
  setupLoRa();
  //WIFIScan(1);Desabilitado pois se conecta apenas com ESP32_TCC
  WIFISetUp();
  
  display.clear();
  display.drawString(0, 0, "BASE");
  delay (1000);
  display.drawString(0, 12, "Recebendo dados");
  display.drawString(0, 24, "do acelerometro");
  display.drawString(0, 36, "do ponto remoto");
  display.display();
  delay(2500);
}

void loop(){

  //Verificamos se há pacotes para receber
  receive();
  digitalWrite(LED_INTERNO, LOW);
    //Envia o codigo para informar ao Remoto que queremos receber os dados
    //send();// temporariamente desabilitado
    
  }
/**************a rotina abaixo não esta sendo chamada******
void send(){
  //Inicializa o pacote
  LoRa.beginPacket();
  //Envia o que está contido em "Rx"
  LoRa.print(Rx);
  //Finaliza e envia o pacote
  LoRa.endPacket();
}*/
/****************Verifica se LORA tem dados***********************/
void receive(){
  //Tentamos ler o pacote
  digitalWrite(LED_INTERNO, HIGH);
  int packetSize = LoRa.parsePacket();
  pacote = String(packetSize,DEC); //transforma o tamanho do pacote em String para imprimirmos
  //Verificamos se o pacote tem o tamanho mínimo de caracteres que esperamos
  if (packetSize > Tx.length()){
    String recebido = "";
    //Armazena os dados do pacote em uma string
    for(int i=0; i<Tx.length(); i++)
    {
      recebido += (char) LoRa.read();
    }

    //Se o cabeçalho é o que esperamos fazemos a leitura do resto da informação
    if(recebido.equals(Tx)){
      SensorXYZ leitura;
      LoRa.readBytes((uint8_t*)&leitura, sizeof(leitura));
      rssi = String(LoRa.packetRssi(), DEC); //configura a String de Intensidade de Sinal (RSSI) 
      
      //Mostramos os dados no display
      showData(leitura);
      
        char x_text[30];
        char y_text[30];
        char z_text[30];
        char Bat_text[30];
        char Freq_text[30];
            
        dtostrf(leitura.X, 10, 10, x_text);
        dtostrf(leitura.Y, 10, 10, y_text);
        dtostrf(leitura.Z, 10, 10, z_text);
        dtostrf(leitura.Bat, 3, 0, Bat_text);
        dtostrf(leitura.Freq, 2, 0, Freq_text);
         DateTime now = rtc.now();
        sprintf(Tempo,"%2d/%02d;%02d:%02d:%02d", now.day(),now.month(),now.hour(),now.minute(), now.second());
        char text[94];
        snprintf(text, 94, "%s;%s;%s;%s", x_text, y_text, z_text,Bat_text,Freq_text );
        
/*****************************Transmite os dados para o ESP32 master*************/

    Data_Comp = Nome +";"+Tempo +";"+String (Contador)+";"+ text+";"+rssi;// string que será gravada no cartão SD
    //Serial.println(Data_Comp);
        if (WiFi.status() == WL_CONNECTED)//Só ira enviar dados se estiver conectado.
        {
      udp.beginPacket("192.168.4.1", 555);//Inicializa o pacote de transmissao ao IP e PORTA.
      udp.println(Data_Comp);//Adiciona-se o valor ao pacote.
      udp.endPacket();//Finaliza o pacote e envia.
 
         }
   else//Caso nao esteja com uma conexao estabelicida ao host
   {
       delay(25);
       showInfo1();
       ESP.restart();
   }
        
    }
    else{    showInfo();}
  }
  delay(2);// deve ser mais rapido do que o LoRa emissor para não perder dados.
  Contador = ++Contador;
}

void showData(SensorXYZ leitura)
{
  display.clear();
  display.drawString(0, 0, "X= " + String(leitura.X, 4) + " m/s^2");
  display.drawString(0, 12,"Y= " + String(leitura.Y, 4) + " m/s^2");
  display.drawString(0, 24,"Z= " + String(leitura.Z, 4) + " m/s^2");
  display.drawString(0, 36, "RSSI= " + rssi + "dB");
  //display.drawString(61 ,36, "Pack "+ pacote + " bytes");
  //display.drawString(0 ,48, "BATERIA= "+ String(leitura.Bat) + " %");
  display.drawString(0 ,48, "FS= "+ String(leitura.Freq) + " Hz");
  display.display();
 
}

void showInfo()
{
  //Mostra no display
  display.clear();
  display.drawString(0, 0, "Dados desconhecidos");
  display.display();
}

void showInfo1()
{
  //Mostra no display
  display.clear();
  display.drawString(0, 0, "Sem Conexao no WiFi");
  display.display();
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
  display.drawString(0, 10, "WIFI Setup done");
  display.drawString(0, 20, (String)WiFi.SSID());
  display.drawString(0, 40, "IP: ");
  display.drawString(20, 40, IP);
  display.display();
  delay(2000);
}

/* temporariamente desabilitada*****
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
        display.drawString(90,(i+1)*9, " (");
        display.drawString(98,(i+1)*9, (String)(WiFi.RSSI(i)));
        display.drawString(114,(i+1)*9, ")");
        //            display.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
        delay(500);
      }
    }
    display.display();
    delay(800);
    display.clear();
  }
}*/


    
#endif
