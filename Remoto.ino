
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


//Compila apenas se MASTER não estiver definido no arquivo principal
//21/10/2019 comentado os Serial.print usar só em debug

#ifndef BASE

/***********Endereco I2C do MMA8451***************/
#define Addr 0x1D // MMA8451Q I2C address is 0x1D(28)
 
//Variaveis para armazenar valores dos sensores
const float grav = 9.80665; // m/s2

float StartTime ;
float CurrentTime;
float ElapsedTime;
double FreqReal;// Frequencia de amostragem FS

/********************** Inicio do programa 1 loop *******************************/
void setup(){
  Serial.begin(115200);
  pinMode(LED_INTERNO, OUTPUT);
  pinMode(34,  INPUT);// bateria
  setupDisplay();//Chama a configuração inicial do display
/********************** Abre comunicação com o Wire**************************/ 
  Wire.begin();

/**********************grava configuração no acelerometro**************************/
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
  // Serial.println("");
  //Serial.print("WHO AM I=  0x");
  //Serial.println(Registro, HEX);  // deve aparecer 0x1A

  Wire.beginTransmission(Addr);     // inicia comunicação com endereço do MMA8451
  Wire.write(0x2A);                // envia o registro com o qual se deseja trabalhar
  Wire.endTransmission(false);     // termina transmissão mas continua com I2C aberto (envia STOP e START)
  Wire.requestFrom(Addr, 1);       // configura para receber 1 byte do registro escolhido acima
  Registro = Wire.read();
 // Serial.print("CTRL_REG1= 0x");
 // Serial.println(Registro, HEX);  // deve aparecer 0x25
         
  /****************Chama a configuração inicial do LoRa**************/
  setupLoRa();
 
  // fica esperando a base dizer que pode enviar
  display.clear();
  display.drawString(0, 0, "SENSOR REMOTO");
  display.drawString(0, 12, "Enviando dados");
  display.drawString(0, 24, "do acelerometro");
  display.drawString(0, 36, "Para a Base");
  display.display();
  delay(2500);
}

/**********************programa principal loop infinito *******************************/
void loop()
{
  StartTime = millis();// guarda o inicio do ciclo
  /*****************Faz a leitura dos dados***************/
  uint8_t Status;
  uint8_t data[6];
  Wire.beginTransmission(Addr);     // inicia comunicação com endereço do MMA8451
  Wire.write(0x00);                      // envia o registro com o qual se deseja trabalhar
  Wire.endTransmission(false);          // termina transmissão mas continua com I2C aberto (envia STOP e START)
  Wire.requestFrom(Addr, 1);        // configura para receber 1 byte do registro escolhido acima
  Status = Wire.read();
  Status = bitRead(Status, 3);
  if (Status == 1) {
    Wire.beginTransmission(Addr);     // inicia comunicação com endereço do MMA8451
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
  /******************************************************
    Convert the data to 14-bits, não mexer na rotina
    é complicado de resolver, o dado lido é em complemento
    de 2.
  *******************************************************/
  uint16_t Rx = data[1]; double x_g; double x_v;
  uint16_t Ry = data[3]; double y_g; double y_v;
  uint16_t Rz = data[5]; double z_g; double z_v;

  Rx <<= 8; Rx = Rx | data[2]; Rx >>= 2;
  x_g = Rx / 4096.0; x_v = x_g*grav;
  if (data[1] > 0x7F)
  {
    Rx = (~Rx) + 1; Rx &= 0x3FFF;
    x_g = -1 * Rx;
    x_g = x_g / 4096.0; x_v = x_g*grav;
  }
  Ry <<= 8; Ry = Ry | data[4]; Ry >>= 2;
  y_g = Ry / 4096.0; y_v = y_g*grav;
  if (data[3] > 0x7F)
  {
    Ry = (~Ry) + 1; Ry &= 0x3FFF;
    y_g = -1 * Ry;
    y_g = y_g / 4096.0; y_v = y_g*grav;
  }
  Rz <<= 8; Rz = Rz | data[6]; Rz >>= 2;
  z_g = Rz / 4096.0; z_v = z_g*grav;
  if (data[5] > 0x7F)
  {
    Rz = (~Rz) + 1; Rz &= 0x3FFF;
    z_g = -1 * Rz;
    z_g = z_g / 4096.0; z_v = z_g*grav;
  }

  /* Output data to serial monitor
  Serial.print("Acceleration in X-Axis : ");
  Serial.println(x_v);
  Serial.print("Acceleration in Y-Axis : ");
  Serial.println(y_v);
  Serial.print("Acceleration in Z-Axis : ");
  Serial.println(z_v);*/

/***************************************************/
    int16_t Bat = map (analogRead(34),0,4095,0,100);
    SensorXYZ leitura;
    leitura.X = x_v;
    leitura.Y = y_v; 
    leitura.Z = z_v;
    leitura.Bat = Bat;  
    leitura.Freq = FreqReal; 
 /**********manda transmitir via LORA***********************/   
      TxLoRa (leitura);
      digitalWrite(LED_INTERNO, LOW);
      if (millis() < 15000){showSentData(leitura);}
      else {display.clear();display.display();}// depois de 10 seg apaga o display
      /*para economizar energia*/
   
    delay(10);// controla a frequencia de amostragem dos acelerometros - xxHz
    // um valor menos que 50 causou instabilidade e perda de leitura.
    CurrentTime = millis();
     ElapsedTime = CurrentTime - StartTime;// calcula quanto tempo ficou executando
     FreqReal= 1/(ElapsedTime/1000); // converte o delta tempo em frequencia
     
}

/****************Inicio das subrotinas********/
/****************Ativa comunicação LoRa*******/
void TxLoRa(SensorXYZ leitura)
{ 
    digitalWrite(LED_INTERNO, HIGH);
  //Inicializa a comunicação
  //Cria o pacote para envio
     LoRa.beginPacket();
     LoRa.print(Tx);
     LoRa.write((uint8_t*)&leitura, sizeof(leitura));
      //Finaliza e envia o pacote
     LoRa.endPacket();
 }

//Função onde se faz a leitura dos dados

/****************apresenta a leitura no display*******/
void showSentData(SensorXYZ leitura)
{
  //Mostra no display
  display.clear();
  display.drawString(0, 0, "Transmitindo p/ BASE:");
  display.drawString(0, 12,"X= " + String(leitura.X, 4) + " m/s^2");
  display.drawString(0, 24,"Y= " + String(leitura.Y, 4) + " m/s^2");
  display.drawString(0, 36,"Z= " + String(leitura.Z, 4) + " m/s^2");
  //display.drawString(0, 48,"BATERIA= " + String(leitura.Bat) + " %");
  display.drawString(0, 48,"FS= " + String(leitura.Freq) + " Hz");
  display.display();
}


#endif
