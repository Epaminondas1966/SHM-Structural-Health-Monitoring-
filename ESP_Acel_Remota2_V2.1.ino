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



/*atualizado em 12/10/2019 - gravando no cartão SD
  /*Programa para medição de aceleração para estruturas metalicas ou concreto.
   desenvolvido para o TCC da Mauá em 2019.
   Utiliza o acelerometro MMA8451 placa da Adafruit, este acelerometro tem um nivel
   de ruido baixo se comparado com os concorrentes.
  23/10/2019 - revisado geral e retirado o Serial.print
  09/11 acrescentado calculo de freqeuncia de amostragem    */


#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include <RTClib.h>

#include "FS.h"
#define DS3231_TEMPERATURE_ADDR     0x11
#define DS3231_I2C_ADDR             0x68
WiFiUDP udp;//Cria um objeto da classe UDP.

const float grav = 9.80665; // m/s2
int contador1 = 0;
unsigned long Contador = 1;

/********************** Configurações do RTC *******************************/
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
RTC_DS3231 rtc;
char Tempo [15];// array de concatenação de informações.

/********************** Configurações do WiFi *******************************/
const String Nome = "Remota2"; // nome do unidade sensora

#define Addr 0x1D // MMA8451Q I2C address is 0x1D(28)

int Pino_CS = 5;// CS do cartão SD.

String DataString;
String TimeString;
String Data_Comp;
int carga_Bat;
//double Transdutor;
//double Trans_Ini;
double tempC;

float StartTime ;
float CurrentTime;
float ElapsedTime;
double FreqReal;// Frequencia de amostragem FS
/********************** Inicio do programa 1 loop *******************************/

void setup() {

  pinMode(2, OUTPUT);//Habilita o LED onboard como saida.
  digitalWrite(2, LOW);//Desliga o LED.
  Serial.begin(115200);
  pinMode(34,  INPUT);// entrada da bateria
  pinMode(35,  INPUT);// entrada do transdutor
  //Trans_Ini = analogRead(35) * 0.02442;
  /********************** verificação do RTC *******************************/
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  //rtc.adjust(DateTime(2019, 10, 13, 18, 53, 0));


  /********************** Abre comunicação com o Wire**************************/

  // Initialise I2C communication as MASTER
  Wire.begin();

  /********************** ativação do cartão SD**************************/
  //Inicia o cartao SD
  if (!SD.begin(Pino_CS))  {
    while (1);
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    return;
  }

  /********************** Abre conexão com o WiFi**************************/
  // Conectar ao WiFi
  WiFi.mode(WIFI_STA);//Define o ESP32 como Station.
  WiFi.begin("ESP32_TCC", ""); //WiFi.begin(ssid, password);
  // Aguardando conexão do WiFi
  int c = 0;
  do {
    if (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
    }
    else {
      long rssi = WiFi.RSSI();
      break;
    }
    c++;
  } while (c < 5);// tenta 5 vezes a conexão se não conseguir pula


  // Start I2C Transmission
  Wire.beginTransmission(Addr);
  Wire.write(0x2A);// Select control register
  Wire.write(0x00);// StandBy mode
  Wire.endTransmission(true); // Stop I2C Transmission

  // Start I2C Transmission
  Wire.beginTransmission(Addr);
  Wire.write(0x2A);// Select control register
  Wire.write(0x24);// StandBy mode, LNOISE, ODR=50Hz
  Wire.endTransmission(true); // Stop I2C Transmission

  // Start I2C Transmission
  Wire.beginTransmission(Addr);
  Wire.write(0x0E);// Select control register
  Wire.write(0x00);// Set range to +/- 2g
  Wire.endTransmission(true);// Stop I2C Transmission

  // Start I2C Transmission
  Wire.beginTransmission(Addr);// Select control register
  Wire.write(0x0F);// Seleciona a faixa mais baixa de corte da frequencia
  Wire.write(0x03);// Stop I2C Transmission
  Wire.endTransmission(true);

  // Start I2C Transmission
  Wire.beginTransmission(Addr); // Select control register
  Wire.write(0x2A); // Active mode
  Wire.write(0x25);
  Wire.endTransmission(true);// Stop I2C Transmission

  delay(1500);


  /****************verificação dos dados gravados no MMA8451****************************/

  uint8_t Registro;
  Wire.beginTransmission(Addr);     // inicia comunicação com endereço do MMA8451
  Wire.write(0x0D);                      // envia o registro com o qual se deseja trabalhar
  Wire.endTransmission(false);          // termina transmissão mas continua com I2C aberto (envia STOP e START)
  Wire.requestFrom(Addr, 1);        // configura para receber 1 byte do registro escolhido acima
  Registro = Wire.read();
  Serial.println("");
  Serial.print("WHO AM I=  0x");
  Serial.println(Registro, HEX);  // deve aparecer 0x1A

  Wire.beginTransmission(Addr);     // inicia comunicação com endereço do MPU6050
  Wire.write(0x2A);                // envia o registro com o qual se deseja trabalhar
  Wire.endTransmission(false);     // termina transmissão mas continua com I2C aberto (envia STOP e START)
  Wire.requestFrom(Addr, 1);       // configura para receber 1 byte do registro escolhido acima
  Registro = Wire.read();
  Serial.print("CTRL_REG1= 0x");
  Serial.println(Registro, HEX);  // deve aparecer 0x25

}

/**********************programa principal loop infinito *******************************/
void loop() {
  StartTime = millis();// guarda o inicio do ciclo
  uint8_t Status;
  uint8_t data[6];
  Wire.beginTransmission(Addr);     // inicia comunicação com endereço do MPU6050
  Wire.write(0x00);                      // envia o registro com o qual se deseja trabalhar
  Wire.endTransmission(false);          // termina transmissão mas continua com I2C aberto (envia STOP e START)
  Wire.requestFrom(Addr, 1);        // configura para receber 1 byte do registro escolhido acima
  Status = Wire.read();
  Status = bitRead(Status, 3);
  if (Status == 1) {
    Wire.beginTransmission(Addr);     // inicia comunicação com endereço do MPU6050
    Wire.write(0x01);                      // envia o registro com o qual se deseja trabalhar
    Wire.endTransmission(false);          // termina transmissão mas continua com I2C aberto (envia STOP e START)
    Wire.requestFrom(Addr, 6);    // Request 6 bytes of data

    //xAccl msb, xAccl lsb, yAccl msb, yAccl lsb, zAccl msb, zAccl lsb
    if (Wire.available() == 6)
    {
      data[1] = Wire.read();
      data[2] = Wire.read();
      data[3] = Wire.read();
      data[4] = Wire.read();
      data[5] = Wire.read();
      data[6] = Wire.read();
    }
  }
  Wire.endTransmission(true);
  /* Convert the data to 14-bits, não mexer na rotina
      é complicado de resolver, o dado lido é em complemento
      de 2.
  */
  uint16_t Rx = data[1]; double x_g; double x_v;
  uint16_t Ry = data[3]; double y_g; double y_v;
  uint16_t Rz = data[5]; double z_g; double z_v;

  Rx <<= 8; Rx = Rx | data[2]; Rx >>= 2;
  x_g = Rx / 4096.0; x_v = x_g * grav;
  if (data[1] > 0x7F)
  {
    Rx = (~Rx) + 1; Rx &= 0x3FFF;
    x_g = -1 * Rx;
    x_g = x_g / 4096.0; x_v = x_g * grav;
  }
  Ry <<= 8; Ry = Ry | data[4]; Ry >>= 2;
  y_g = Ry / 4096.0; y_v = y_g * grav;
  if (data[3] > 0x7F)
  {
    Ry = (~Ry) + 1; Ry &= 0x3FFF;
    y_g = -1 * Ry;
    y_g = y_g / 4096.0; y_v = y_g * grav;
  }
  Rz <<= 8; Rz = Rz | data[6]; Rz >>= 2;
  z_g = Rz / 4096.0; z_v = z_g * grav;
  if (data[5] > 0x7F)
  {
    Rz = (~Rz) + 1; Rz &= 0x3FFF;
    z_g = -1 * Rz;
    z_g = z_g / 4096.0; z_v = z_g * grav;
  }

  //Transdutor = analogRead(35) * 0.02442;
  //Transdutor = Transdutor - Trans_Ini;
               DateTime now = rtc.now();
  TimeString = String(now.day()) + "/" + String(now.month()) + ";" + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
  DataString = ";" + String (Contador) + ";" + String(x_v, 5) + ";" + String(y_v, 5) + ";" + String(z_v, 5) + ";" + String(FreqReal, 0) + ";";
  sprintf(Tempo, "%2d/%02d;%02d:%02d:%02d", now.day(), now.month(), now.hour(), now.minute(), now.second());
  Bateria ();
  mostra_Temper();
  Data_Comp = Nome + ";" + Tempo + DataString + String (carga_Bat) + ";" + String(tempC, 2);

  if (!SD.exists("/Sensor2.txt")) {
    writeFile(SD, "/Sensor2.txt", 0);
  }
  if (SD.exists("/Sensor2.txt")) {
    appendFile(SD, "/Sensor2.txt", 0);
  }
  //Serial.println(FreqReal, 0);

  if (WiFi.status() == WL_CONNECTED)//Só ira enviar dados se estiver conectado.
  {
    udp.beginPacket("192.168.4.1", 555);//Inicializa o pacote de transmissao ao IP e PORTA.
    udp.println(Data_Comp);//Adiciona-se o valor ao pacote.
    udp.endPacket();//Finaliza o pacote e envia.

    digitalWrite(2, 1);//-
    delay(10);//controla a frequencia de amostragem dos acelerometros (cuidado ao alterar)
    digitalWrite(2, 0);//Pisca o led rapidamente apos enviar.
  }
  else//Caso nao esteja com uma conexao estabelicida ao host, piscara lentamente.
  {
    digitalWrite(2, 0);
    delay(5);
    digitalWrite(2, 1);
    delay(5);
    //ESP.restart();
  }

  Contador = ++Contador;
     CurrentTime = millis();
     ElapsedTime = CurrentTime - StartTime;// calcula quanto tempo ficou executando
     FreqReal= 1/(ElapsedTime/1000); // converte o delta tempo em frequencia

}


/****************Inicio das subrotinas********/


void mostra_Temper (void) {
  tempC = DS3231_get_treg();
}

float DS3231_get_treg()
{
  double rv;

  uint8_t temp_msb, temp_lsb;
  int8_t nint;

  Wire.beginTransmission(DS3231_I2C_ADDR);
  Wire.write(DS3231_TEMPERATURE_ADDR);
  Wire.endTransmission();

  Wire.requestFrom(DS3231_I2C_ADDR, 2);
  temp_msb = Wire.read();
  temp_lsb = Wire.read() >> 6;

  if ((temp_msb & 0x80) != 0)
    nint = temp_msb | ~((1 << 8) - 1);      // if negative get two's complement
  else
    nint = temp_msb;

  rv = 0.25 * temp_lsb + nint;

  return rv;
}
void Bateria (void) {

  carga_Bat = map (analogRead(34), 2767, 4095, 0, 100);

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
  if (file.print(Tempo)) {
    file.println(DataString);
    file.close();
  }
}
