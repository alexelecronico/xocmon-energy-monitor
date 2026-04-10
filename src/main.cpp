//MM
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html
// http://www.steves-internet-guide.com/mqtt-protocol-messages-overview/
// https://docs.platformio.org/en/latest/platforms/espressif32.html#cpu-frequency
// https://docs.platformio.org/en/latest/integration/ide/vscode.html#ide-vscode

//watchdog: https://iotassistant.io/esp32/enable-hardware-watchdog-timer-esp32-arduino-ide/

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <SPI.h>
#include <LoRa.h>
#include <math.h>
//#include <ESP32Ping.h> 
#include <Preferences.h>
Preferences preferences;
#include <HardwareSerial.h>
HardwareSerial atSerial(1);


#include "helpers/OTAClient.h"
OTAClient OTA;
//#include "helpers/wifi.h"
#include "configs.h"
#include "certs.h"
#include <DNSServer.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include <ESP32Ping.h>

//#include "helpers/OTAClient.h"
//OTAClient OTA;
//#include "helpers/wifi.h"
//#include "helpers/updateCurrentTime.h"
//#include "helpers/adc.h"
//#include "helpers/analysis.h"
//#include "helpers/prepare.h"
#include <NTPClient.h> //updatetime
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
unsigned long currentTime;
unsigned long offlinedrift;

unsigned long period;
unsigned long zeit,prezeit;
unsigned long millispre;
const int BACKUPSIZE=200;
unsigned long currenttimearray[BACKUPSIZE];
float payloadbag[BACKUPSIZE][55];

 unsigned long tiempoa;
  unsigned long tiempob;
   unsigned long deltat;


//////////////////////////// TASKS FOR EACH CORE //////////////////////////////////
TaskHandle_t Task1; //core 0
TaskHandle_t Task2; //core 1
//create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0

#define bot    	    36 	  //VP
#define ledred    	13	
#define ledgreen    12	
#define ADCCS1    	33 		   // SPI slave select
#define ADCCS2    	25 		   // SPI slave select
#define MISO        19 // GPIO19 MISO
#define MOSI        27 // GPIO27 MOSI
#define SCK         5   // GPIO5  SCK
#define WDT         32   // GPIO5  SCK
#define SDdetect    39   // VN

int adcvalue;

//cleanup
int vmax1,vmax2,vmax3,vmin1,vmin2,vmin3;
int vmax, vmin;
float V1,V2,V3;
float V;
float Ix[12];
float I;


//addup
byte flagpubcount;
byte current_input;


float V1prom,V2prom,V3prom;
float V1pre,V2pre,V3pre;
float counterstatus;
int intcounterstatus;
float Ixprom[12];
float Px[12];
float Whx[12];
float maxamps[12];

int o;


// #define ADC_VREF    3300     // 3.3V Vref
// #define ADC_CLK     1600000  // SPI clock 1.6MHz

// //lora
// #define CSLORA    	99 		   // SPI LORA slave select //falta asignar


//MCP3208_1 adc(ADC_VREF, ADCCS1);
//flag_onlineMCP3208_2 adc(ADC_VREF, ADCCS2);
//uint16_t raw = adc.read(MCP3208_1::Channel::SINGLE_0);

String ssid,password;

//----------------------------------------------------------------------------------------------
//  ADCCS1:       ADCCS2:
//ch0=  i3        11
//ch1=  i2        5
//ch2=  i1        6
//ch3=  v1        12
//ch4=  v2        10
//ch5=  v3        9
//ch6=  ref2.5    8
//ch7=  i4        7

//TCPClient client;
//UDP UDPClient;

int buttonflag=0;
int backupcounter=0;
int publishnow=1;

unsigned long previousMillis_timerbkp;
// unsigned long backupcurrentTime;
// long timercurrentTime;

unsigned long currentMillis;

//do every
unsigned long previousMillistimerOTA;
unsigned long previousMillis_ping;
unsigned long previousMillis_wificheck;
unsigned long previousMillis_wdt;
unsigned long previousMillistimerReset;
unsigned long previousMillisRestart;
unsigned long restarttolerance=86400000;
int flagofflinedrift=0;
byte flag_wdt;
byte flag_relauchTask2=0;
//float acum;

unsigned long previousMillistimerbkp;
//unsigned long previousMillistimerOTA;

//senddata:
int httpCode =0;
//ping:
int avg_time_ms;

unsigned long timer = 0; //para hacer currentTime backup en eeprom cada hora
unsigned long timerOTA = 0; //para checar si hay actualizacion (cada 60 seg)
unsigned long timerReset = 0; //para checar si hay actualizacion (cada 60 seg)


/////////adc////////

int iread_new[resolution];  
int vread_new[resolution];  
byte pfable_new;

int flag_error=0;

//int iread[12][resolution];     // 12 corrientes
//int vread[3][resolution];      // 3 fases

//byte pfable[12];
long pi[12];
long pv1;
long pv2;
long pv3;
long ppf[12];//contador para addup de cada power factor
int proceedpowerfafctor;
float pf[12];
//byte pfactor[12];
float pfactor_new;

byte flag_proceed_voltage;
byte flag_proceed_current;

float ref25;
unsigned long zeitsync,prezeitsync;
byte syncstatus;
int analogread;
byte channel;

int read(byte ADC, byte chanselector)
{
  //https://forum.arduino.cc/t/interface-adc-mcp3208-w-arduino-mega-using-spi/12920/2
  // primero asigna el chanselector al adc pin del MCP3208 correpondiente, ya sea el 1 o el 2
  // en caso de leer voltajes se usan los valores 21,22,23, si son corrientes se usa 1-12

  
  //  ADCCS1:       ADCCS2:   se mapea el input de corriente a medir con el MCP3208 y su pin.
  // ch1=  3         11
  // ch2=  2         5
  // ch3=  1         6
  // ch4=  v1/21     12
  // ch5=  v2/22     10
  // ch6=  v3/23     9
  // ch7=  ref2.5    8
  // ch8=  4         7
  if (chanselector == 1 || chanselector == 6)
  {
    channel = 3;
  }
  else if (chanselector == 2 || chanselector == 5)
  {
    channel = 2;
  }
  else if (chanselector == 3 || chanselector == 11)
  {
    channel = 1;
  }
  else if (chanselector == 4 || chanselector == 7)
  {
    channel = 8;
  }
  else if (chanselector == 8 || chanselector == 25) //25 only goes with ADC1
  {
    channel = 7;
  }
  else if (chanselector == 9 || chanselector == 23) //23 only goes with ADC1
  {
    channel = 6;
  }
  else if (chanselector == 10 || chanselector == 22)//22 only goes with ADC1
  {
    channel = 5;
  }
  else if (chanselector == 12 || chanselector == 21)//21 only goes with ADC1
  {
    channel = 4;
  }

  adcvalue = 0;
  byte commandbits = B11000000; // command bits - start, mode, chn (3), dont care (3)

  // allow channel selection
  commandbits |= ((channel - 1) << 3);




  if (ADC == 1)
  {
    digitalWrite(ADCCS1, LOW); // Select adc
  }
  else
  {
    digitalWrite(ADCCS2, LOW); // Select adc
  }
  // setup bits to be written
  //--------------------------------START ADC----------------------------------------------------------------------
  for (int i = 7; i >= 3; i--)
  { // hacer lectura del SPI de forma bitbangeada
    digitalWrite(MOSI, commandbits & 1 << i);
    // cycle clock
    digitalWrite(SCK, HIGH);
    digitalWrite(SCK, LOW);
  }
  digitalWrite(SCK, HIGH); // ignores 2 null bits
  digitalWrite(SCK, LOW);
  digitalWrite(SCK, HIGH);
  digitalWrite(SCK, LOW);
  // read bits from adc
  for (int i = 11; i >= 0; i--) //i=11 do not change unless absolutely sure
  {
    adcvalue += digitalRead(MISO) << i;
    // cycle clock
    digitalWrite(SCK, HIGH);
    digitalWrite(SCK, LOW);
  }
  //--------------------------------STOP ADC----------------------------------------------------------------------
  digitalWrite(ADCCS1, HIGH); // turn off device
  digitalWrite(ADCCS2, HIGH); // turn off device
  return adcvalue;

} // end read

void sync(byte fase)
{
  fase = fase+20; //la funcion read() rquiere saber el ADC y el canal, en este caso el ADC siemore es 1 y el canal es 21 o 22 o 23
  // encuentra el cruce por "cero" de negativo a positivo del voltaje de la fase indicada por byte fase
  //OUTPUT: si logra sincronizarse syncstatus=1, si no syncstatus=0
  byte dowhile = 0;
  byte go = 0;
  syncstatus = 0;
  prezeitsync = micros();
  do
  {
    // Serial.println("go1");
    int syncmax=0;
    int syncmin=7000;
    while (go == 0)
    {
      int ramp = 0;
      while (ramp < 200)
      {

        //analogread = analogRead(A0);
        analogread = read(1,fase);
        syncmax = max(syncmax,analogread);
        syncmin = min(syncmin,analogread);
        if (analogread > 2020 && analogread < 2040)
        {
          ramp = 201;
        }
        else
        {
          ramp++;
        }

        zeitsync = micros();
        if ((zeitsync - prezeitsync) > 30000)
        {
          dowhile = 1;
          ramp = 202;
          go = 1;
          syncstatus = 2;
          //Serial.println("sync fail");
        } // syncstatus=2 significa que no pudo sincronizarse
      }

      if (ramp == 201)
      {
        int temporal = read(1,fase);//antes analogRead(A0);
        if (temporal > analogread)
        {
          go = 1;
          dowhile = 1;

          int amplitude = abs(syncmax-syncmin); //que tenga una amplitud minima de +-50 steps de 4096
          if(amplitude > 100){
             syncstatus = 1; // syncstatus=1 significa que si pudo sincronizarse <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
          }
        }
        else
        {
          delay(5);
        }
      } // END IF
    }   // end while ramp
  } while (dowhile == 0);
}//end sync

void readcycle_new(byte x) // current_input puede ser del 1 al 12
{

     Serial.print("read:");
  current_input = x;
     Serial.print(current_input);
  byte attempts = 0;
  byte MCP = 0;
  byte sync_with_voltage = iconfig[current_input];

                                      // Serial.print(" sync_with_voltage:");
                                      // Serial.print(sync_with_voltage);
                                      // Serial.println(" ");

  if (sync_with_voltage > 0)
  {
    if (sync_with_voltage < 4)
    {
      sync(sync_with_voltage); // sincroniza con V1 temp

      pfable_new = 0;
      if (syncstatus == 1)
      {
        pfable_new = 1;
      }
    }

    if (current_input <= 3) // si inputcorriente es 1,2,3,4 lee del MCP3208_1, si no lee del MCP3208_2
    {
      MCP = 1;
    }
    else
    {
      MCP = 2;
    }

 

    attempts = 0;
    if (sync_with_voltage == 4)
    {
      sync_with_voltage = 1; // solo para el caso donde no importa el voltaje sino solo la corriente, ese "1" es solo para que lea algo, no importa que voltaje
    }
    do
    {
                                            //Serial.print("reading..");
      tiempoa = micros();
      for (int a = 0; a < resolution; a++)
      {
        vread_new[a] = read(1, sync_with_voltage + 20);
        iread_new[a] = read(MCP, current_input + 1); //
        // if (a == 1 && vread_new[0] < vread_new[1] && vread_new[0] > 2000 && vread_new[0] < 2100 & iread_new[0]> 1800 && iread_new[0] > 2300) //
        // {
        //   a = 0;
        //   Serial.print("restart 1");
        //   tiempoa = micros();
        //   attempts++;
        // }
      }

      tiempob = micros();
      deltat = tiempob - tiempoa;

      //Serial.print(" DeltaT:");
      //Serial.print(deltat);
      if (deltat > 17000)
      {
        attempts++;
        Serial.print(attempts);
        delay(50);
      }
      if(attempts >= 5)
      {
        flag_error=1; //demasiados attempts
        deltat=16999;
      }
    } while (deltat > 17000);
    // Serial.print(" attempts:");
    // Serial.print(attempts);

//  Serial.print(" iread50:");
//     Serial.print( iread_new[50] );
//     Serial.print(" iread100:");
//     Serial.print( iread_new[100] );
//     Serial.print(" iread200:");
//     Serial.print( iread_new[20] );
  } // end switch
} // end readcycle_new


void cleanup_new()
{
  flag_proceed_voltage = 1;
  flag_proceed_current = 1;
  int counterror1 = 0;
  int counterror2 = 0;

  // Serial.println("pre");
  // for (unsigned int m = 0; m < resolution; m++)
  // {
  //  Serial.print(iread_new[m]);
  //  Serial.print(" ");
  // }
  // Serial.println(" ");
  // Serial.println(" ");

  int z = resolution - 1;
  for (unsigned int m = 0; m < z; m++)
  {
    if (vread_new[m] > 4000 || vread_new[m] < 50)
    {
      counterror1++;
    }

    if (iread_new[m] > 4000 || iread_new[m] < 50)
    {
      counterror2++;
    }

    int a = vread_new[m];
    int b = vread_new[m + 1];
    int c = abs(a - b);
    if (c > 100 && a < 4000 && a > 50)
    {
      vread_new[m + 1] = vread_new[m];
    }


    a = iread_new[m];
    b = iread_new[m + 1];
    c = abs(a - b);
    if (c > 100 && a < 4000 && a > 50)
    {
      iread_new[m + 1] = iread_new[m];
    }
  }



  if (counterror1 > 10)
  {
    Serial.print(" voltage offlimit count: ");
    Serial.print(counterror1);
    flag_proceed_voltage = 0;
  }


  if (counterror2 > 10)
  {
    // Serial.print(" current offlimit count:");
    // Serial.print(counterror2);
    // Serial.println("post");
    // for (unsigned int m = 0; m < resolution; m++)
    // {
    //   Serial.print(iread_new[m]);
    //   Serial.print(" ");
    // }
    // Serial.println(" ");
    // Serial.println(" ");
    flag_proceed_current = 0;
  }
}//end cleanup_new

void voltajes_new() // lee arrays de voltaje y entrega floats V1,V2,V3
  {
    // Serial.print("/vl");
    vmax = 0;
    vmin = 5000;
    V = -2;

    if (flag_proceed_voltage == 1)
    {
      //Serial.print("flag_proceed_voltage=1");
      for (unsigned int m = 0; m < resolution; m++)
      {
        int a = vread_new[m];
        int b = vread_new[m + 1];
        if (a < 4000 && a > 50)
        {
          int jump = abs(a - b);
          if (jump < 300)
          {
            vmax = max(a, vmax);
            vmin = min(a, vmin);
          }
          else
          {
            m = m + 2;
          }
        }
        else
        {
          vread_new[m]=2048;
            Serial.println("xxx:");

        }
      }
       //Serial.print("vmax:"); Serial.print(vmax);
      //Serial.print(" vmin:"); Serial.print(vmin);

      // Pase por cero (2048):
      // rutina para confimrar que Vx @0V pasa por 2048bits del ADC +- no mas de 500.
      // inicialmente se van a descartar valores que salgan de 500 para filtrar ruido,
      // posteriormente se desea hacer un analisis de la curva incluso cuando no pasa por 2048.
      int a = vmax - 2048;
      int b = 2048 - vmin;
      int c = abs(a - b);
      if (c < 100)
      {
        V = (vmax - vmin) * Vcalib; // se obtiene Vrms de cada fase
        // Serial.print(" V simetrico, c=");
        // Serial.print (c);
        // Serial.print(" V: ");
        // Serial.println (V);
      }
      // checa si esta en rangos aceptables
      if (V < 0)
      {
      //Serial.print("__________ V<0:");Serial.print(V);
        V = -1;
      }
      if (V > 400)
      {
        Serial.print("Overvoltage: V=");
        Serial.println(V);
        V = -1;
      }
        Serial.print(" V="); Serial.println(V);
    }
    else
    {
      V = -1;
    }

  } // end voltaje_new

void corrientes_new()
  {
    // Serial.print("/crr");
    // calcula el area de la curva de cada corriente, si iconfig[x]==0 entonces Ix[x]=0.001;
    long iarea = 0;
    int istep = 0;
    int imax = 0;
    int imin = 7000;

    if (flag_proceed_current == 1)
    {
      for (unsigned int m = 0; m < resolution; m++)
      {
        imax = max(iread_new[m], imax);
        imin = min(iread_new[m], imin);
      }
      int a = imax - midpoint;
      int b = midpoint - imin;
      int c = abs(a - b);
      if (c < 100) // symmetry ok?
      {
        // if symmetry is detected proceed with iarea
        // iarea
        iarea = 0;
        for (unsigned int m = 0; m < resolution; m++)
        {
          istep = iread_new[m];
          int istepnext = iread_new[m + 1];
          istep = abs(istep - midpoint);
          istepnext = istepnext - midpoint;
          int jump = abs(istep - istepnext);

          // if(jump>2000){Serial.print(" jump");Serial.print(jump);}

          // if (istep > 2000)//temp
          // {
          //   Serial.print(" ");
          //   Serial.print(istep);
          // }
          if (istep < atenuacion || jump > 1000) // para reducir ruido aunque tambien reduce sensibilidad a corrientes bajas
          {
            istep = 0;
            m++;
          }
          // faflta ver un problema de medicio de corriente que a vveces dice aprox 30A de la nada, puta madre.
          iarea = iarea + istep;
          /// Serial.print(" iarea");Serial.print(iarea);
        }

        I = iarea * icalib[current_input] / 1000000; // calibra cada corriente en en caso de usar otra clamp

        if (I > 1000)
        {
          Serial.print("Overcurrent: I[");
          Serial.print(current_input);
          Serial.print("]=");
          Serial.println(I);
          I = -1;
        }

        // if (current_input == 0) // temp
        // {
        //   Serial.print(" Ix[");
        //   Serial.print(current_input);
        //   Serial.print("]=");
        //   Serial.print(I);
        // }

      } // end if c<100
      else
      {
        I = -1; // error no symmetry detected
        Serial.println("[ERROR] no current symmetry ");
        Serial.print("imax:");
        Serial.print(imax);
        Serial.print(" imin:");
        Serial.println(imin);
      }
    }
    else
    {
      I = -1;
    }


  } // end corrientes_new



void maxcurrent(byte a) //obtiene el valor maximo de potencia de cada periodo
  {
    if (I > maxamps[a])
    {
      maxamps[a] = I;
    }
  }

void powerfactor_new()
  {
    // Serial.print("/pf");
    // para que funcione bien se requiere que los datos no contengan ruido o picos, por lo tanto se tiene que hacer un prefiltrado de picos que sirve de una vez para analisis de voltajes y correintes tambien
    // falta que se haga una confirmacion si Vx=-1, en cuyop caso no calcules power fafctor porque trae algo raro.
    //  notas:
    /*
    hay seis posibles casos:
    1. carga 100% resistiva (pf=1)
    2. carga 100% resistiva inversa (pf=-1, corriente en sentido contrario)
    3. carga capacitiva: corriente despues de voltaje mismo sentido (pf de 0.5 a 1)
    4. carga capacitiva inversa: corriente sentiddo contrario despues de voltaje  (pf de 0.5 a -1)
    5. carga inductivav: corriente antes de voltaje mismo sentido (pf de 0.5 a 1)
    6. carga inductivav inversa: corriente sentiddo contrario antes de voltaje  (pf de 0.5 a -1)
  */

    // 1 realizar analisis de pf solo si iconfig[] es 1 o 2 o 3
    // 2 confirmar que sync() fue exitoso

    // para saber si es carga capacitiva o inductiva:
    // se necesita encontrar el cruce por "cero" del voltaje y de corriente y analizar cual cruza primero.
    // para voltaje:

    byte q = iconfig[current_input];
    int r;
    if(q==1)
    {
      r = V1;
    }
    else if(q==2)
    {
      r = V2;
    }
    else if(q==3)
    {
      r = V3;
    }
    else
    {
      pfable_new = 0;
    }
    int vpointer = 0;
    if (pfable_new == 1 && r >= 1 && I>0.05)
    {
      // Serial.print(" pfable ");
      // Serial.print(i);
      // Serial.print(" ok ");
      proceedpowerfafctor = 0;

      for (int j = 0; j < resolution; j++)
      {
        // Serial.print(vread[0][j]);
        // Serial.print(" ");
        if (vread_new[j] < 2048)
        {
          j++;
        }
        else
        {
          vpointer++;// deberia ser cas siemore el mismo valor al rededor de resolution/2 o sea 520
        }
      }
      //Serial.print(" vpointer: ");
      //Serial.print(vpointer);
      int halfresolution=resolution/2;
      int halfresolution_low= halfresolution-10;
      int halfresolution_high=halfresolution+10;
      if (vpointer > halfresolution_low && vpointer < halfresolution_high)
      {
        proceedpowerfafctor = 1;
       // Serial.print(", in range.");
      }

      if (proceedpowerfafctor == 1)
      {
        int a = 0;
        int b = 0;
        int c = 0;
        int ipointer = vpointer;

        // una vez encontrado el cruce por cero de vvoltaje, checar si la corriente es directa o inversa
        // dado que voltaje cruza por cero hacia "abajo" es decir la segunda mitad del seno, la corriente tambien deberia en caso de ser directa
        a = iread_new[ipointer];
        b = iread_new[ipointer + 4]; // se compara con unas cuantas mediciones despues, en este caso 4
        c = iread_new[ipointer + 8];
        if (a > b && a > c && b > c) // se usan tres puntos para tener redundancia
        {
          //direto
          //sigue obtener si es capacitivo o inductivo:
          if (iread_new[ipointer] > midpoint) // 2048 o midpoint? //directo cpacitivo (a la derecha)
          {
            while (iread_new[ipointer] > midpoint) // falta asignar cual de los tres voltajes (usando iconfig)
            {
              ipointer++;
            }
            // agregar filtros de coherencia
            float x = 3.1416 / 520 * (ipointer - vpointer - 1); // 520 es aprox la mitad de resolution = 1040, n-1 es para ajustar por que la medicion de corriente es intercalada con la de voltaje.
            pfactor_new = cos(x);
            Serial.print(" directo capacitivo:");
            Serial.print(pfactor_new );
            Serial.println("");

            // falta indicar que es directo y capacitivo
          }
          else if (iread_new[ipointer] < midpoint) // directo inductivo (corriente a la izq)
          {
            while (iread_new[ipointer] < midpoint) // falta asignar cual de los tres voltajes (usando iconfig)
            {
              ipointer--;
            }
            // agregar filtros de coherencia
            float x = 3.1416 / 520 * (vpointer - ipointer - 1); // 520 es aprox la mitad de resolution = 1040, n-1 es para ajustar por que la medicion de corriente es intercalada con la de voltaje.
            pfactor_new = cos(x);
            Serial.print(" directo inductivo:");
            Serial.print(pfactor_new );
            Serial.println("");

            // falta indicar que es directo y inductivo
          }
        }
        else if (a < b && a < c && b < c)
        {
          // inverso
          if (iread_new[ipointer] < 2048) // 2048 o midpoint? //inverso cpacitivo (a la derecha)
          {
            while (iread_new[ipointer] < 2048) // falta asignar cual de los tres voltajes (usando iconfig)
            {
              ipointer++;
            }
            // agregar filtros de coherencia
            float x = 3.1416 / 520 * (ipointer - vpointer - 1); // 520 es aprox la mitad de resolution = 1040, n-1 es para ajustar por que la medicion de corriente es intercalada con la de voltaje.
            pfactor_new = cos(x);
            Serial.print(" inverso capacitivo:");
            Serial.print(pfactor_new );
            Serial.println("");

            // falta indicar que es inverso y capacitivo
          }
          else if (iread_new[ipointer] > 2048) // directo inductivo (corriente a la izq)
          {
            while (iread_new[ipointer] > 2048) // falta asignar cual de los tres voltajes (usando iconfig)
            {
              ipointer--;
            }
            // agregar filtros de coherencia
            float x = 3.1416 / 520 * (vpointer - ipointer - 1); // 520 es aprox la mitad de resolution = 1040, n-1 es para ajustar por que la medicion de corriente es intercalada con la de voltaje.
            pfactor_new = cos(x);
            Serial.print(" inverso inductivo:");
            Serial.print(pfactor_new );
            Serial.println("");

            // falta indicar que es inverso y inductivo
          }
        }
        else
        {
          // error
          proceedpowerfafctor = 0;
        }
      } // end proceed
    }
    else
    {
      pfactor_new = 2;//invalid
    }

} // end powerfactor

void analysis_new(byte a)
{
  // Serial.print("analysis:");
  // Serial.print(a);
  current_input = a;
  byte input = iconfig[current_input]; //vain: de 0 a 11

  if (input > 0)  //0 ignore, 1=fase1, 2=fase2, 3=fase3, 4=dont care just read
  {
    //                                                             Serial.println(" ");
    // Serial.print("                                                cleanup_new:");
    cleanup_new();

    //                                                           Serial.println(" "); 
    // Serial.print("                                                voltajes_new:");
    voltajes_new();

    //                                                             Serial.println(" "); 
    // Serial.print("                                                corrientes_new:");
    corrientes_new();

    //                                                             Serial.println(" "); 
    // Serial.print("                                                maxcurrent:");    
    maxcurrent(a);

    //                                                            Serial.println(" "); 
    // Serial.print("                                                powerfactor_new:");
    powerfactor_new();
  }
}

void addup_new(byte j) // va sumando cada lectura a Vxprom y a Ixprom[] y va contando cuantas sumas lleva en "p"
{
  //Serial.print("addup ");
  //voltajes
  if(V>=0) 
  {
    byte vo = iconfig[j];
      //Serial.print(" vo:");Serial.print(vo);
    if (vo == 1)
    {
      V1prom = V1prom + V;
      pv1++;
      //Serial.print(" V1prom:");Serial.print(V1prom);
      
    }
    else if (vo == 2)
    {
      V2prom = V2prom + V;
      pv2++;
       //Serial.print(" V2prom:");Serial.print(V2prom);
    }
    else if (vo == 3)
    {
      V3prom = V3prom + V;
      pv3++;
       //Serial.print(" V3prom:");Serial.print(V3prom);
    }
    else
    {

    }
  }

  

  // corrientes

  if (iconfig[j] > 0)
  {
    if (I >= 0)
    {
      Ixprom[j] = Ixprom[j] + I;
      pi[j]++;
    }
  }

  // powerfactor
  if (iconfig[j] > 0)
  {
    if (pfactor_new < 2)
    {
      //Serial.println("");
      //Serial.print("pf_pre:");
      //Serial.print(pf[j]);
      pfactor_new=abs(pfactor_new);//temp
      pf[j] = pf[j] + pfactor_new;
      ppf[j]++;
      //Serial.print("pfactor_new:");
      //Serial.print(pfactor_new);
      //Serial.print(" ppf");
      //Serial.print(j);
      //Serial.print(":");
      //Serial.println(ppf[j]);
      //Serial.print("pf_post:");
      //Serial.print(pf[j]);
    }
    if (pfactor_new == 2)
    {
      //Serial.print("pfactor_new=2");
    }
  }

} // end addup_new

void average_new()  //cuando ya va a publicar saca el promedio dividiendo la suma entre el conteo de mediciones "p"
{
    Serial.println("averaging_new");
    V1 = V1prom / pv1;
    V2 = V2prom / pv2;
    V3 = V3prom / pv3;
    Serial.print("V1=");
    Serial.print(V1);
    Serial.print(" V2=");
    Serial.print(V2);
    Serial.print(" V3=");
    Serial.println(V3);

    for (int j = 0; j < 12; j++)    //calcula la potencia Px[] dependiendo de cual corriente esta asignada a cual voltaje
    {
        if (pi[j] > 0) { // CAMBIO POR IA. SE LE AGREGA IF PARA ASEGURARSE QUE NO HAYA ERROR SI EL VECTOR DE PI ES CERO. 
            Ix[j] = Ixprom[j] / pi[j];
        } else {
            Ix[j] = 0.0;
        } //HASTA AQUI EL CAMBIO POR IA. 
        Serial.print("average Ix");
        Serial.print(j);
        Serial.print("=");
        Serial.println(Ix[j]);
        if (iconfig[j] == 1)
        {
            Px[j] = Ix[j] * V1;
        }
        else if (iconfig[j] == 2)
        {
            Px[j] = Ix[j] * V2;
        }        
        else if (iconfig[j] == 3)
        {
            Px[j] = Ix[j] * V3;
        }
        else // o sea si iconfig[j] es 0
        {
          Px[j] = 0.0;
        }

        // powerfactor

        Serial.println("average pf:");
        Serial.print("pf[j]");
        Serial.print(pf[j]);
        Serial.print(" ppf[j]");
        Serial.println(ppf[j]);
        if (ppf[j] == 0)
        {
          ppf[j] = 1;
          pf[j] = 2;
        }
        pf[j] = pf[j] / ppf[j];
        Serial.print(" final pf[j]");
        Serial.println(pf[j]);
    }

    pv1=pv2=pv3=0;
    for (int j = 0; j < 12; j++)    
    {
      pi[j] = 0;
      ppf[j] = 0;
    }
}//end average_new


////////end adc/////

void updatecurrentTime()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    // pregunta a internet

    // get internet time
    Serial.println("updatecurrentTime...");
    unsigned long internet_time;

    // try to get internet times x times
    for (int i = 0; i < 10; i++)
    {
      Serial.print("attempt nr: ");
      Serial.print(i);
      timeClient.begin();
      timeClient.setTimeOffset(0);
      timeClient.update();
      internet_time = timeClient.getEpochTime(); // get epoch from internet
      Serial.print(" ,internet time: ");
      Serial.println(internet_time);
      timeClient.end();

      if (internet_time > 1700000000)
      {
        i = 10; // exit
      }
    }

    if (internet_time < 1700000000)
    {
      preferences.begin("my-pref", false);
      currentTime = preferences.getULong("currentTime", 0);
      preferences.end();
    }
    else
    {
      currentTime = internet_time;
    }
  }
  else
  {
    preferences.begin("my-pref", false);
    currentTime = preferences.getULong("currentTime", 0);
    preferences.end();
  }
  // Serial.print("pre currentTime is: ");Serial.println(currentTime);
  // period = millis() - millispre + 500;
  // Serial.print("period is: ");Serial.println(period);
  // currentTime = currentTime + period/1000;
  // millispre=millis();
  // Serial.print("post currentTime is: ");Serial.println(currentTime);

  // if(internet_time < 1640000000)
  // {
  //   if(currentTime < 1640000000)
  //   {
  //     preferences.begin("my-pref", false);
  //     //preferences.putULong("currentTime", internet_time);
  //     //currentTime=internet_time;
  //     long q = preferences.getULong("currentTime", 123);
  //     Serial.print("last saved time is: ");Serial.println(q);
  //     preferences.end();
  //     currentTime=q;
  //   }
  // }
  // else
  // {
  //   long k = abs(currentTime-internet_time);

  //   Serial.print("drift is: ");Serial.println(k);
  //   if(k>20)
  //   {
  //     preferences.begin("my-pref", false);
  //     preferences.putULong("currentTime", internet_time);
  //     currentTime=internet_time;
  //     Serial.print("new saved currenttime is: ");
  //     long q = preferences.getULong("currentTime", 123);
  //     Serial.println(q);
  //     preferences.end();
  //   }
  //   if (flagofflinedrift == 0)
  //   {
  //     offlinedrift = 0;
  //   }
  // }

} // end updatecurrentTime

void updatecurrentTime_old()
{

  //get internet time
  Serial.println("updatecurrentTime...");
  unsigned long internet_time;
  timeClient.begin();
  timeClient.setTimeOffset(0);
  timeClient.update();
  internet_time = timeClient.getEpochTime(); // get epoch from internet
  Serial.print("internet time: ");
  Serial.println(internet_time);
  timeClient.end();

  //get internet time again if unsuccessful
  if (internet_time < 1660000000)
  {
    Serial.print("Reading internet time again: ");
    timeClient.begin();
    timeClient.setTimeOffset(0);
    timeClient.update();
    internet_time = timeClient.getEpochTime(); // get epoch from internet
    Serial.print("internet time: ");
    Serial.println(internet_time);
    timeClient.end();
    if (internet_time < 1660000000)
    {
      offlinedrift=internet_time;
      Serial.print("offlinedrift: ");
      Serial.println(offlinedrift);
      flagofflinedrift=1;
    }
  }

  Serial.print("pre currentTime is: ");Serial.println(currentTime);
  period = millis() - millispre + 500;
  Serial.print("period is: ");Serial.println(period);
  currentTime = currentTime + period/1000;
  millispre=millis();
  Serial.print("post currentTime is: ");Serial.println(currentTime);

  if(internet_time < 1640000000)
  {
    if(currentTime < 1640000000)
    {
      preferences.begin("my-pref", false); 
      //preferences.putULong("currentTime", internet_time);
      //currentTime=internet_time;
      long q = preferences.getULong("currentTime", 123);
      Serial.print("last saved time is: ");Serial.println(q);
      preferences.end();
      currentTime=q;
    }
  }
  else
  {
    unsigned long k = (currentTime-internet_time);

    Serial.print("drift is: ");Serial.println(k);
    if(k>20)
    {
      preferences.begin("my-pref", false); 
      preferences.putULong("currentTime", internet_time);
      currentTime=internet_time;
      Serial.print("new saved currenttime is: ");
      long q = preferences.getULong("currentTime", 123);
      Serial.println(q);
      preferences.end();
    }
    if (flagofflinedrift == 0)
    {
      offlinedrift = 0;
    }
  }
}//end updatecurrentTime


///////////wifi/////////

int flag_online;

//SSIDs to be presented to the user
String SSID1 = "";
String SSID2 = "";
String SSID3 = "";
String SSID4 = "";
String SSID5 = "";

#define WIFI_CONNECTION_ATTEMPTS 2


DNSServer dnsServer;
AsyncWebServer server(80);

String saved_ssid;
String saved_password;

bool valid_ssid_received = false;
bool valid_password_received = false;
bool wifi_timeout = false;

int flag_rescanwifi=0;
IPAddress Ip(192,168,0,1);    //setto IP Access Point




//Webpage HTML
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Captive Portal Demo</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <h3>Captive Portal Demo</h3>
  <br><br>
  <form action="/get">
    <br>
    SSID: <select name = "ssid">
      <option value=%SSID1%>%SSID1%</option>
      <option value=%SSID2%>%SSID2%</option>
      <option value=%SSID3%>%SSID3%</option>
      <option value=%SSID4%>%SSID4%</option>
      <option value=%SSID5%>%SSID5%</option>
    </select>
    <br>
    <br>
    Password: <input type="text" name="password">
    <input type="submit" value="Submit">
  </form>
</body></html>)rawliteral";

//Processor for adding values to the HTML
String processor(const String& var) {
  //Serial.println(var);
  if (var == "SSID1") {
    return SSID1;
  }
  else if (var == "SSID2") {
    return SSID2;
  }
  else if (var == "SSID3") {
    return SSID3;
  }
  else if (var == "SSID4") {
    return SSID4;
  }
  else if (var == "SSID5") {
    return SSID5;
  }

  else
  {
    return SSID1;
  }
}

void scanWiFi()
{
  WiFi.mode(WIFI_AP_STA); 
  //WiFi.disconnect();
  delay(100);
  Serial.println("scan start");
  int k = 5;

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  if (n < k) k = n;
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      delay(10);
      if (i == 0)
      {
        SSID1 = WiFi.SSID(i);
      }
      else if (i == 1)
      {
        SSID2 = WiFi.SSID(i);
      }
      else if (i == 2)
      {
        SSID3 = WiFi.SSID(i);
      }
      else if (i == 3)
      {
        SSID4 = WiFi.SSID(i);
      }
      else if (i == 4)
      {
        SSID5 = WiFi.SSID(i);
      }
    }
  }
  Serial.println("");

}//end scan


class CaptiveRequestHandler : public AsyncWebHandler {
  public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request) {
      //request->addInterestingHeader("ANY");
      return true;
    }

    void handleRequest(AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", index_html, processor);
    }
};

void setupServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html, processor);
    Serial.println("Client Connected");
   // flag_rescanwifi=1;
  });

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    String inputMessage;
    String inputParam;

    if (request->hasParam("ssid")) {
      inputMessage = request->getParam("ssid")->value();
      inputParam = "ssid";
      ssid = inputMessage;
      Serial.println(inputMessage);
      valid_ssid_received = true;
    }

    if (request->hasParam("password")) {
      inputMessage = request->getParam("password")->value();
      inputParam = "password";
      password = inputMessage;
      Serial.println(inputMessage);
      valid_password_received = true;
    }
    request->send(200, "text/html", "The values entered by you have been successfully sent to the device. It will now attempt WiFi connection");
  });
}

void WiFiSoftAPSetup()
{
  scanWiFi();

  //https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/
  WiFi.mode(WIFI_AP);


  WiFi.softAP("XOC");
  


  delay(100);
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(Ip, Ip, NMask);
  Serial.print("AP IP address: "); Serial.println(WiFi.softAPIP());
}

void StartCaptivePortal() {
  Serial.println("Setting up AP Mode");
  WiFiSoftAPSetup();
  Serial.println("Setting up Async WebServer");
  setupServer();
  Serial.println("Starting DNS Server");
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP
  server.begin();
  dnsServer.processNextRequest();
}

void WiFiStationSetup(String rec_ssid, String rec_password)
{
  wifi_timeout = false;
  WiFi.mode(WIFI_STA);
  char ssid_arr[20];
  char password_arr[20];
  rec_ssid.toCharArray(ssid_arr, rec_ssid.length() + 1);
  rec_password.toCharArray(password_arr, rec_password.length() + 1);
  Serial.print("Received SSID: "); Serial.println(ssid_arr); Serial.print("password: "); Serial.println(password_arr);
  WiFi.begin(ssid_arr, password_arr);

  uint32_t t1 = millis();

  int n_attempts_left = WIFI_CONNECTION_ATTEMPTS;
  int k=3;
  while (k>0)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      k--;
      delay(3000);
      digitalWrite(WDT, HIGH);
      delay(10);
      digitalWrite(WDT, LOW);
      Serial.print("'");
      if (millis() - t1 > 3000) // 15 seconds elapsed connecting to WiFi
      {
        n_attempts_left -= 1;
        Serial.println();
        Serial.println("Timeout connecting to WiFi. The SSID and Password seem incorrect.");
        Serial.print("Number of attempts left is: ");
        Serial.println(n_attempts_left);
        t1 = millis();

        if (n_attempts_left == 0)
        {
          Serial.println();
          Serial.println("All attempts exhausted");
          valid_ssid_received = false;
          valid_password_received = false;
          StartCaptivePortal();
          wifi_timeout = true;
          break;
        }
      }
    }
  }
  if (!wifi_timeout)
  {
    preferences.begin("my-pref", false);

    Serial.println("");
    Serial.print("WiFi connected to: ");
    Serial.println(rec_ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (rec_ssid != saved_ssid)
    {
      Serial.print("Updating SSID to: ");
      Serial.println(rec_ssid);
      preferences.putString("rec_ssid", rec_ssid);
    }

    if (rec_password != saved_password)
    {
      Serial.print("Updating Password to: ");
      Serial.println(rec_password);
      preferences.putString("rec_password", password);
      saved_password = rec_password;
    }

    Serial.print("Verifying storage of credentials: ");
    ssid = preferences.getString("rec_ssid", WIFI_SSID);
    Serial.print("rec_ssid: ");
    Serial.print(ssid);
    password = preferences.getString("rec_password", WIFI_PASSWORD);
    Serial.print(" rec_password: ");
    Serial.println(password);
    preferences.end();
  }
}//end WiFiStationSetup

void WiFiStationSetup2(String rec_ssid, String rec_password)
{
  wifi_timeout = false;
  WiFi.mode(WIFI_STA);
  char ssid_arr[20];
  char password_arr[20];
  rec_ssid.toCharArray(ssid_arr, rec_ssid.length() + 1);
  rec_password.toCharArray(password_arr, rec_password.length() + 1);
  Serial.print("Received SSID: "); Serial.println(ssid_arr); Serial.print("And password: "); Serial.println(password_arr);
  WiFi.begin(ssid_arr, password_arr);

  uint32_t t1 = millis();

  int n_attempts_left = WIFI_CONNECTION_ATTEMPTS;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(3000);
    digitalWrite(WDT, HIGH);
    delay(10);
    digitalWrite(WDT, LOW);
    Serial.print("'");
    if (millis() - t1 > 5000) // 15 seconds elapsed connecting to WiFi
    {
      n_attempts_left -= 1;
      Serial.println();
      Serial.println("Timeout connecting to WiFi. The SSID and Password seem incorrect.");
      Serial.print("Number of attempts left is: "); Serial.println(n_attempts_left);
      t1 = millis();

      if (n_attempts_left == 0) {
        Serial.println();
        Serial.println("All attempts exhausted");
        valid_ssid_received = false;
        valid_password_received = false;
        //StartCaptivePortal();
        wifi_timeout = true;
        break;
      }
    }
  }
}//end WiFiStationSetup2 _no prefefrences store

void ping()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("pinging.. ");
        //esp_task_wdt_init(20, true); //enable panic so ESP32 restarts
        //esp_task_wdt_add(NULL); //add current thread to WDT watch
        if (Ping.ping("www.google.com", 2) == 1) //bool ret = Ping.ping("www.google.com",10); //repeticiones
        {
            //esp_task_wdt_disable();
            // disableCore0WDT();
            int avg_time_ms = Ping.averageTime();
            Serial.print(avg_time_ms);
            //zeit2 = 10000 + millis();
            if (avg_time_ms > 1000)
            {
               
                digitalWrite(ledgreen, LOW);
                digitalWrite(ledred, HIGH);
                
                flag_online = 0;
            }
            else
            {
               
                digitalWrite(ledgreen, HIGH);
                digitalWrite(ledred, LOW);
                flag_online = 1;
            }
        }
        else
        {
            Serial.print("no pong");
            flag_online = 0;
          
                digitalWrite(ledgreen, LOW);
                digitalWrite(ledred, HIGH);
        }
    }
    else {
        flag_online=0;
    
                digitalWrite(ledgreen, LOW);
                digitalWrite(ledred, HIGH);
    }
} //end ping

int offlinecounter=0;
void connecttohardcodedwifis(){
  Serial.print(" connecttohardcodedwifis ");

  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.disconnect();
    WiFi.mode( WIFI_MODE_NULL );//off
    delay(500);
    WiFi.mode(WIFI_STA);
    Serial.println("Trying to connect to XOC factory wifi..");
    Serial.print("SSID: ");
    Serial.println(WIFI_SSID_xoc);
    Serial.print("Pass: ");
    Serial.println(WIFI_PASSWORD2);
    for (int i = 2; i > 0; i--) // launch portal and try to connect to previous wifi at the same time, repeat every 60 sec
    {
      WiFi.begin(WIFI_SSID_xoc, WIFI_PASSWORD2);
      delay(3000);
      digitalWrite(WDT, HIGH);
      delay(10);
      digitalWrite(WDT, LOW);
      Serial.println(i);
      if (WiFi.status() == WL_CONNECTED)
      {
        delay(1000);
        ping();
        if(flag_online == 1){
        i = 0;
        offlinecounter=0;
        }
      }
    }
  }

   //tentativa para que se conecte mejor. este metodo se ve mas robusto, probarlo en vez de las lineas de abajo, falta ver cuanto tarda toda la secuencia.

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Trying to connect to manually added wifi..");
    WiFiStationSetup2(ssid, password);

    if (WiFi.status() == WL_CONNECTED)
    {
      ping();
      if (flag_online == 1)
      {
        //i = 0;
        offlinecounter = 0;
      }
    }
  }


  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Trying to connect to default coded wifi..");
    WiFiStationSetup2(WIFI_SSID, WIFI_PASSWORD);


    if (WiFi.status() == WL_CONNECTED)
    {
      ping();
      if (flag_online == 1)
      {
        //i = 0;
        offlinecounter = 0;
      }
    }
  }





  if (WiFi.status() == WL_CONNECTED)
  {

    if (flag_online == 0)
    {
      offlinecounter++;
      Serial.print("offlinecounter: "); Serial.println(offlinecounter);
    }
  }


  if (WiFi.status() != WL_CONNECTED)
  {
    offlinecounter++;
  }

  Serial.println(" end connecttohardcodedwifis");
}//end connecttohardcodedwifis

void wificheck()
{
  Serial.println(" wificheck");
  preferences.begin("my-pref", false);

  connecttohardcodedwifis();

  char ssid_arr[20];
  char password_arr[20];
  if (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(ledgreen, LOW);
    digitalWrite(ledred, HIGH);
    
    flag_online=0;
    WiFi.disconnect();
    
    ssid = preferences.getString("rec_ssid", "xoc");
    password = preferences.getString("rec_password", "12345678");
    Serial.print("Reconnecting to WiFi with SSID: ");Serial.print(ssid);Serial.print(" pass:");Serial.println(password);
    ssid.toCharArray(ssid_arr, ssid.length() + 1);
    password.toCharArray(password_arr, password.length() + 1);
    //WiFi.begin(ssid_arr, password_arr);
  }



  if (WiFi.status() != WL_CONNECTED) //if still offline
  {
    WiFi.disconnect();
    Serial.println("Searching for known wifi and starting portal now...");
    StartCaptivePortal();

    for (int i = 40; i > 0; i--) // launch portal and try to connect to previous wifi at the same time, repeat every 60 sec
    {
      // connecttohardcodedwifis();
      WiFi.begin(ssid_arr, password_arr);
      delay(2000);
      digitalWrite(WDT, HIGH);
      delay(10);
      digitalWrite(WDT, LOW);
      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.println("WiFi ok, pinging...");
        i = 0;
        ping(); 
        //flag_online=2; //wifi ok but still needs to test ping
        server.end();
        WiFi.mode(WIFI_STA);  //stop portal
        Serial.println("WiFi ok");
        digitalWrite(ledgreen, HIGH);
        digitalWrite(ledred, LOW);
      }
      Serial.print(i); 
      Serial.print(" ");
      dnsServer.processNextRequest();
      delay(500);
      if (valid_ssid_received && valid_password_received)
      {
        Serial.println("Attempting WiFi Connection!");
        WiFiStationSetup(ssid, password);
        i=0;
      }
      if(flag_rescanwifi==1) //when client refreshes page do a re-scan of wifis
      {

        flag_rescanwifi=0;
        WiFi.disconnect();
        StartCaptivePortal();
      }
    }

    //next: end captive portal after 60 seconds unless a clients has connected to the esp32 in which case keep portal active unless clients disconnects, in which case resume 60 sec countdown.
  }



  preferences.end();
}//end wificheck



////////wifi end///////








void addtobasket() //checa que pueda subor datos a internet, si no entonces guarda en ram cada medicion hasta que pueda subir los datos pendientes
{
  Serial.print("addtobasket ");

  updatecurrentTime();

  if (caso == 0)
  {
    counterstatus++;
    if (counterstatus > 9999)
    {
      counterstatus = 0;
    }
  }

  if (flag_online == 0)
  {
    intcounterstatus = counterstatus;
    counterstatus = intcounterstatus;
    counterstatus = counterstatus + 0.1;
  }
  else
  {
    intcounterstatus = counterstatus;
    counterstatus = intcounterstatus;
  }

  o++;
  Serial.print("location 'o': ");
  Serial.println(o);

  if (o == BACKUPSIZE - 1)
  {
       // si se llena entonces suma los Wh y los guarda en la primera casilla de BACKUPSIZE, si se vuelve a llenar guarda todo en la segunda casilla etc.
            // if (bigbackup==BACKUPSIZE-2){bigbackup=0;}//ahora si sobreescribe el backup
            
            // float Wh1acum,Wh2acum,Wh3acum,Wh4acum,Wh5acum=0.0;
            // for(int p=bigbackup;p<BACKUPSIZE;p++){
            //     Wh1acum=Wh1acum+payloadbag[p][13];
            //     Wh2acum=Wh2acum+payloadbag[p][14];
            //     Wh3acum=Wh3acum+payloadbag[p][15];
            //     Wh4acum=Wh4acum+payloadbag[p][16];
            //     Wh5acum=Wh5acum+payloadbag[p][17];
            //     payloadbag[p][19]=0;
            // }
            // payloadbag[bigbackup][13]=Wh1acum;
            // payloadbag[bigbackup][14]=Wh2acum;
            // payloadbag[bigbackup][15]=Wh3acum;
            // payloadbag[bigbackup][16]=Wh4acum;
            // payloadbag[bigbackup][17]=Wh5acum;
            // payloadbag[bigbackup][19]=1;
            
            // bigbackup++;
            // o=bigbackup;
            o=0;
            payloadbag[o][0] = 0;
  }
  else
  {
    //antes
    // while (payloadbag[o][0] == 1) // encuentra un slot libre
    // {
    //   o++;Serial.print(o);
    // } 

    //ahora
    int q=0;
    while (payloadbag[q][0] == 1) // encuentra un slot libre
    {
      q++;Serial.print(q);
    } 
    o=q;

    payloadbag[o][0] = 1;
    payloadbag[o][2] = counterstatus;
    currenttimearray[o] = currentTime; //se necesita usar unsigned long en vez de float para que funcione bien
    payloadbag[o][3] = V1;
    payloadbag[o][4] = V2;
    payloadbag[o][5] = V3;

    for (byte j = 0; j < 12; j++)
    {
      payloadbag[o][j + 6] = Ix[j];
      payloadbag[o][j + 18] = Whx[j];
      payloadbag[o][j + 30] = maxamps[j];
      payloadbag[o][j + 42] = pf[j]; 
    }

  } //end else
} //end addtobasket

void empaquetador()
{
//Serial.print("emp");
  byte flaginit = 0;
  byte flagpub = 0;
  if (initialmessages == 0)
  {
    flaginit = 1;
  } // publica varias mediciones inicialmente para confirmar llegada de datos a la nube
  else
  {
    flaginit = 0;
    initialmessages--;
    delay(3000);
  }

  const int periodo = 10000;
  //                   10000       120         initialmessages     button o casos especiales
  if ((zeit - prezeit) > periodo * reportacada * flaginit * publishnow)
  { // cada x milisegundos promedia las mediciones que ha estado agregando a addup() para promediar
    Serial.println("preparing to send... ");
    average_new(); // promedia todas las lecturas realizadas durante el periodo
    //prezeit = zeit; //se paso abajo
    // cosas para variabilidad=1:
    //  float temp1 = fabs(V1pre-V1);
    //  float temp2 = fabs(V2pre-V2);
    //  float temp3 = fabs(V3pre-V3);

    // for (int j=0;j<12;j++)
    // {
    //    float tempx[j] = fabs(Ixpre[j]-Ix[j]);

    // }

    if (publishnow == 0 || flaginit==0)
    {
      for (byte j = 0; j < 12; j++)
      {
        if (iconfig[j] > 0)
        {
          Whx[j] = Whx[j] + Px[j] * (zeit - prezeit) / 1000 / 3600;
        }
      }
      publishnow = 1;
      flagpub = 2;
    }
    else // if not publishnow
    {
      for (byte j = 0; j < 12; j++)
      {
        if (iconfig[j] > 0)
        {
          Whx[j] = Whx[j] + Px[j] * periodo / 1000 / 3600 * reportacada;
        }
      }
    }

    if (flagpub != 2)
    {
      flagpub = 0;
    } // solo en caso de initialmessages (publishnow=0)

    // if(variabilidad==1){
    //     byte add=0;

    //     if      (temp1 > 3.0) {flagpub++; }             // Volts de variación
    //     else if (temp2 > deltai) {flagpub++; digitalWrite(ledgreen,LOW);}      // Amps de variación
    //     else     {add=1;}                               // si pasan mas x veces sin reportar cambios, manda medicion de todas formas

    //     if      (temp7 > deltai) {flagpub++; }             // Volts de variación
    //     else if (temp8 > deltai) {flagpub++; }      // Amps de variación
    //     else     {add=1;}                               // si pasan mas x veces sin reportar cambios, manda medicion de todas formas

    //     if(add==1){flagpubcount++;}
    //     if (flagpubcount > maxwait)
    //     {
    //       flagpub = 1;
    //       flagpubcount = 0;
    //     }
    // } // end if variablilidad
    // else
    // {
    prezeit = zeit;
    addtobasket();

    flagpubcount = 0; // para cosas de varibilidad=1 creo

   

    // reset variables
    for (byte j = 0; j < 12; j++)
    {
      Ixprom[j] = 0;
      Px[j] = 0;
      Whx[j] = 0;
      maxamps[j] = 0;
    }
    V1prom = V2prom = V3prom = 0;

  } // end if
} // end empaquetador


void buttoncheck() //if pushbutton pressed for more than x times delay(millis), then clear wifi credentials and launch softAP
{
  int d = 0;
  int led = 1;
  if (digitalRead(bot) == LOW)
  { //if1
    buttonflag = 1;
    Serial.println("button ");
    while (d < 30) //x times
    {
      //esp_task_wdt_reset();
      if (digitalRead(bot) == LOW)
      {
        Serial.print("! ");
        if (led == 1)
        {
          digitalWrite(ledred, HIGH);
          digitalWrite(ledgreen, LOW);
          led = 0;
        }
        else
        {
          digitalWrite(ledred, LOW);
          digitalWrite(ledgreen, LOW);
          led = 1;
        }
        delay(50); //delay(millis)
        d++;
      }
      else
      {
        buttonflag = 0;
        d = 51;
        Serial.print(" clear button ");
      }
    } //end while

    if (buttonflag == 0)
    {
      Serial.println(" returning..");
      digitalWrite(ledred, LOW);
      digitalWrite(ledgreen, HIGH);
      publishnow = 0; //if publishnow==1 then publish normally, if publishnow==0 then publish now.
                      // attachInterrupt(bot,interruptbutton, FALLING);
      return;
    }

    if (buttonflag == 1) //if button still pressed blink faster
    {
      d = 0;
      while (d < 100) //x times
      {
        //esp_task_wdt_reset();
        if (digitalRead(bot) == LOW)
        {
          Serial.print("! ");
          if (led == 1)
          {
            digitalWrite(ledred, HIGH);
            digitalWrite(ledgreen, LOW);
            led = 0;
          }
          else
          {
            digitalWrite(ledred, LOW);
            digitalWrite(ledgreen, LOW);
            led = 1;
          }
          delay(20); //delay(millis)
          d++;
        }

        else
        {
          buttonflag = 0;
          d = 100;
          Serial.print(" clear button ");
        }
      } //END WHILE
    }

    if (buttonflag == 0)
    {
      Serial.print(" flowacum reset to cero,"); //if released go back but clear flowacum
     
      Serial.println(" returning..");
      digitalWrite(ledred, LOW);
      digitalWrite(ledgreen, HIGH);
      publishnow = 0; //if publishnow==1 then publish normally, if publishnow==0 then publish now.
                      // attachInterrupt(bot,interruptbutton, FALLING);
      return;
    }
    if (buttonflag == 1)
    { //if still pressed, clear wifi credentials
      digitalWrite(ledred, LOW);
      digitalWrite(ledgreen, LOW);
      delay(1000);
      //detachInterrupt(bot);
      //resetWifi();
    }
  }
}//end buttoncheck

//Se realiza un traductor para enviar a Google Sheets; debido al tipo de formato. Modificar loopushlisher tomaría mucho tiempo:
String convertirParamsAJson(String params) {
    String json = "{";
    int start = 0;
    while (start < params.length()) {
        int eq = params.indexOf('=', start);
        int amp = params.indexOf('&', start);
        if (amp == -1) amp = params.length();

        // Extraemos la "llave" (nombre de columna) y el "valor"
        String key = params.substring(start, eq);
        String val = params.substring(eq + 1, amp);

        json += "\"" + key + "\":" + val; // Los números en JSON no llevan comillas

        if (amp < params.length()) json += ",";
        start = amp + 1;
    }
    json += "}";
    return json;
} //Aqui finaliza el traductor. 

void send_universal_log(String params_univ)
{
  Serial.println(" ");
  Serial.print("Enviando a Superbase...");
  HTTPClient http; //AQUI POR PRIMERA VEZ INTENTO MANDAR DATOS. 
  String server = "https://miqrnsiajbplihvicydr.supabase.co/rest/v1/mediciones";

  digitalWrite(ledgreen, LOW);
  digitalWrite(ledred, LOW);
  digitalWrite(WDT, HIGH);
  delay(10);
  digitalWrite(WDT, LOW);

  Serial.print(server); //Se refiere a la URL que ya definimos arriba. 
  Serial.print("Realizando una solicitud:");
  //http.begin(url, root_ca); // Specify the URL and certificate

  http.begin(server); // Specify the URL and certificate

  Serial.print(" http.server:");
  //esp_task_wdt_init(20, true); //enable panic so ESP32 restarts}
  //hacer un timer manual, si pasan mas de unos 20 seg haz backup de currenttimer 
  //esp_task_wdt_init(10, true); //enable panic so ESP32 restarts

  //Configurar las etiquetas del paquete:
    http.addHeader("apikey", apiKey);
    http.addHeader("Authorization", "Bearer " + String(apiKey));
    http.addHeader("Content-Type", "application/json");

  //Se realiza la traducción a JSON:
  String jsonPayload = convertirParamsAJson(params_univ);

  httpCode = http.POST(jsonPayload);
  Serial.print ("HECHO");

  //esp_task_wdt_init(3, true); //enable panic so ESP32 restarts
  //disableCore0WDT();
  
  Serial.print("Listo. Respuesta de Superbase: ");
  Serial.println(httpCode);
  http.end();

  if (httpCode == 201 || httpCode == 200 || httpCode==302 || httpCode == -11)
  {
    digitalWrite(ledgreen, HIGH);
    digitalWrite(ledred, LOW);
    Serial.println("universal log sent.");
  }
  else
  {
    digitalWrite(WDT, HIGH);
    delay(10);
    digitalWrite(WDT, LOW);
    // flag_online=0;
    // counterstatus--;
    // onlinecheck3();
    Serial.print("sendData sending again...");
    HTTPClient http; //AQUI VUELVE A INICIAR EL PROCESO PARA INTENTAR MANDAR DATOS. 
    String server = "https://miqrnsiajbplihvicydr.supabase.co/rest/v1/mediciones"; //Talvez no se necesite. 

    Serial.print(server);

    Serial.print("Realizando una solicitud");
    //http.begin(url, root_ca); // Specify the URL and certificate

    http.begin(server); // Specify the URL and certificate

    Serial.print(" http.server:");
    // esp_task_wdt_init(20, true); //enable panic so ESP32 restarts}
    // hacer un timer manual, si pasan mas de unos 20 seg haz backup de currenttimer
    // esp_task_wdt_init(10, true); //enable panic so ESP32 restarts

    //Credenciales:
    http.addHeader("apikey", apiKey);
    http.addHeader("Authorization", "Bearer " + String(apiKey));
    http.addHeader("Content-Type", "application/json");

    httpCode = http.POST(jsonPayload);

    Serial.print(" DONE");
    // esp_task_wdt_init(3, true); //enable panic so ESP32 restarts
    // disableCore0WDT();
    Serial.print("Listo: Respuesta de superbase: ");
    Serial.println(httpCode);
    http.end();
  }

  if (httpCode == 201 || httpCode == 200 || httpCode == 302 || httpCode == -11)
  {
    digitalWrite(ledgreen, HIGH);
    digitalWrite(ledred, LOW);
  }
  else
  {
    digitalWrite(ledgreen, LOW);
    digitalWrite(ledred, HIGH);
    flag_online = 0;
    Serial.println(" could not sendData, marked as unsent");
    // counterstatus--;
    // onlinecheck3();
  }

}//end send_universal_log


void sendData(String params)
{
  Serial.println("sendData sending...");
  HTTPClient http;
  String server = "https://miqrnsiajbplihvicydr.supabase.co/rest/v1/mediciones"; 

  digitalWrite(ledgreen, LOW);
  digitalWrite(ledred, LOW);
  digitalWrite(WDT, HIGH);
  delay(10);
  digitalWrite(WDT, LOW);

  Serial.print(server);
  Serial.print("Realizando una solicitud:");
  //http.begin(url, root_ca); // Specify the URL and certificate

  http.begin(server); // Specify the URL and certificate

  //Credenciales para la base de datos. 
    http.addHeader("apikey", apiKey);
    http.addHeader("Authorization", "Bearer " + String(apiKey));
    http.addHeader("Content-Type", "application/json");

String jsonPayload = convertirParamsAJson(params);

  Serial.print("http.server:");
  //esp_task_wdt_init(20, true); //enable panic so ESP32 restarts}
  //hacer un timer manual, si pasan mas de unos 20 seg haz backup de currenttimer 
  //esp_task_wdt_init(10, true); //enable panic so ESP32 restarts}

  httpCode = http.POST(jsonPayload);

  Serial.print (" DONE");
  //esp_task_wdt_init(3, true); //enable panic so ESP32 restarts
  //disableCore0WDT();
  Serial.print("Hecho. Respuesta de superbase: ");
  Serial.println (httpCode);
  http.end();

  // if (httpCode == 302 || httpCode == 200)
  // {
  //  // flowtotal = 0; //reset flowshort (flowtotal) para que pueda ir contando nuevamente mientras google responde
  // }
  // else
  // {
  // Serial.print(url);
  // Serial.print("sending same message agaiin...");
  // http.begin(url, root_ca); //Specify the URL and certificate
  // Serial.print("http.get:");
  // //esp_task_wdt_init(20, true); //enable panic so ESP32 restarts}
  // //hacer un timer manual, si pasan mas de unos 20 seg haz backup de currenttimer
  // //esp_task_wdt_init(10, true); //enable panic so ESP32 restarts
  // httpCode = http.GET();
  // Serial.println("A VER");
  // //esp_task_wdt_init(3, true); //enable panic so ESP32 restarts
  // //disableCore0WDT();
  // http.end();
  // }

  if (httpCode == 201 || httpCode == 200 || httpCode==302 || httpCode == -11)
  {
    digitalWrite(ledgreen, HIGH);
    digitalWrite(ledred, LOW);
  }
  else
  {
    digitalWrite(WDT, HIGH);
    delay(10);
    digitalWrite(WDT, LOW);
    // flag_online=0;
    // counterstatus--;
    // onlinecheck3();
    Serial.println("Tratando de enviar de nuevo...");
    HTTPClient http; //Vuelve a intentar a enviar la información. 

    Serial.print(server);
    Serial.print("Realizando una solicitud:");
    //http.begin(url, root_ca); // Specify the URL and certificate

    http.begin(server); // Specify the URL and certificate

    http.addHeader("apikey", apiKey);
    http.addHeader("Authorization", "Bearer " + String(apiKey));
    http.addHeader("Content-Type", "application/json");
    
    Serial.print("http.server:");
    // esp_task_wdt_init(20, true); //enable panic so ESP32 restarts}
    // hacer un timer manual, si pasan mas de unos 20 seg haz backup de currenttimer
    // esp_task_wdt_init(10, true); //enable panic so ESP32 restarts
    httpCode = http.POST(jsonPayload);
    Serial.print("HECHO");
    // esp_task_wdt_init(3, true); //enable panic so ESP32 restarts
    // disableCore0WDT();
    Serial.print("Hecho. Respuesta de superbase: ");
    Serial.println(httpCode);
    http.end();
  }

  if (httpCode == 201 || httpCode == 200 || httpCode==302 || httpCode == -11)
  {
    digitalWrite(ledgreen, HIGH);
    digitalWrite(ledred, LOW);
  }
  else
  {
    digitalWrite(ledgreen, LOW);
    digitalWrite(ledred, HIGH);
    flag_online = 0;
    Serial.println("could not sendData, marked as unsent");
    // counterstatus--;
    // onlinecheck3();
  }

} // end senddata
void looppublisher()
{
  for (int i = BACKUPSIZE; i >= 0; i--)
  {
    if (payloadbag[i][0] == 1)//si hay un dato guardado
    {
      payloadbag[i][0] = 0;
      unsigned long p_currenttime = currenttimearray[i];
      //OJO: payloadbag[i][1] no se esta usando
      float p_counterstatus = payloadbag[i][2];
      float p_V1 = payloadbag[i][3];
      float p_V2 = payloadbag[i][4];
      float p_V3 = payloadbag[i][5];

      float p_Ix0 = payloadbag[i][6];//confirmar si empiueza en cero o en uno
      float p_Ix1 = payloadbag[i][7];
      float p_Ix2 = payloadbag[i][8];
      float p_Ix3 = payloadbag[i][9];
      float p_Ix4 = payloadbag[i][10];
      float p_Ix5 = payloadbag[i][11];
      float p_Ix6 = payloadbag[i][12];
      float p_Ix7 = payloadbag[i][13];
      float p_Ix8 = payloadbag[i][14];
      float p_Ix9 = payloadbag[i][15];
      float p_Ix10 = payloadbag[i][16];
      float p_Ix11 = payloadbag[i][17];

      float p_Whx0 = payloadbag[i][18];//confirmar si empiueza en cero o en uno
      float p_Whx1 = payloadbag[i][19];
      float p_Whx2 = payloadbag[i][20];
      float p_Whx3 = payloadbag[i][21];
      float p_Whx4 = payloadbag[i][22];
      float p_Whx5 = payloadbag[i][23];
      float p_Whx6 = payloadbag[i][24];
      float p_Whx7 = payloadbag[i][25];
      float p_Whx8 = payloadbag[i][26];
      float p_Whx9 = payloadbag[i][27];
      float p_Whx10 = payloadbag[i][28];
      float p_Whx11 = payloadbag[i][29];

      float p_maxamps0 = payloadbag[i][30];//confirmar si empiueza en cero o en uno
      float p_maxamps1 = payloadbag[i][31];
      float p_maxamps2 = payloadbag[i][32];
      float p_maxamps3 = payloadbag[i][33];
      float p_maxamps4 = payloadbag[i][34];
      float p_maxamps5 = payloadbag[i][35];
      float p_maxamps6 = payloadbag[i][36];
      float p_maxamps7 = payloadbag[i][37];
      float p_maxamps8 = payloadbag[i][38];
      float p_maxamps9 = payloadbag[i][39];
      float p_maxamps10 = payloadbag[i][40];
      float p_maxamps11 = payloadbag[i][41];

      float p_pfx0 = payloadbag[i][42];//confirmar si empiueza en cero o en uno
      float p_pfx1 = payloadbag[i][43];
      float p_pfx2 = payloadbag[i][44];
      float p_pfx3 = payloadbag[i][45];
      float p_pfx4 = payloadbag[i][46];
      float p_pfx5 = payloadbag[i][47];
      float p_pfx6 = payloadbag[i][48];
      float p_pfx7 = payloadbag[i][49];
      float p_pfx8 = payloadbag[i][50];
      float p_pfx9 = payloadbag[i][51];
      float p_pfx10 = payloadbag[i][52];
      float p_pfx11 = payloadbag[i][53];
      


      send_universal_log("Timestamp_Device=" + String(p_currenttime)+ "&config_id=" + String(caso) + "&XOC_id=" + String(XOCid,3)  + "&counterstatus=" + String(p_counterstatus) );// + "&count=" + String(0)); //count todavia no esta soportado, no se si se necesita
      
      //old sendData("Timestamp_Device=" + String(currentTimebkp) + "&config_id=" + String(caso) + "&flow=" + String(hotflowbkp) + "&tempIN=" + String(hottempbkpIN)+ "&tempOUT=" + String(hottempbkpOUT) + "&vbat=" + String(solarbatbkp)+ "&count=" + String(solarcountbkp));// + "&count=" + String(0)); //count todavia no esta soportado, no se si se necesita
      
      //version para sheets xoc1xx corriendo en vscode xoc002clone
      //sendData("Timestamp_Device=" + String(p_currenttime)+ "&config_id=" + String(caso) + "&XOC_id=" + String(XOCid,3)  + "&counterstatus=" + String(p_counterstatus) + "&V1=" + String(p_V1)+ "&V2=" + String(p_V2) + "&V3=" + String(p_V3)+ "&I1=" + String(p_Ix0)+ "&I2=" + String(p_Ix1)  + "&I3=" + String(p_Ix2)+ "&I4=" + String(p_Ix3) + "&I5=" + String(p_Ix4)+ "&I6=" + String(p_Ix5)+ "&I7=" + String(p_Ix6)+ "&I8=" + String(p_Ix7)+ "&I9=" + String(p_Ix8)+ "&I10=" + String(p_Ix9)+ "&I11=" + String(p_Ix10)+ "&I12=" + String(p_Ix11)+ "&Wh1=" + String(p_Whx0)+ "&Wh2=" + String(p_Whx1) + "&Wh3=" + String(p_Whx2)+ "&Wh4=" + String(p_Whx3)+ "&Wh5=" + String(p_Whx4)+ "&Wh6=" + String(p_Whx5)+ "&Wh7=" + String(p_Whx6)+ "&Wh8=" + String(p_Whx7)+ "&Wh9=" + String(p_Whx8)+ "&Wh10=" + String(p_Whx9)+ "&Wh11=" + String(p_Whx10)+ "&Wh12=" + String(p_Whx11)+ "&pf1=" + String(p_pfx0)+ "&pf2=" + String(p_pfx1)+ "&pf3=" + String(p_pfx2)+ "&pf4=" + String(p_pfx3)+ "&pf5=" + String(p_pfx4)+ "&pf6=" + String(p_pfx5)+ "&pf7=" + String(p_pfx6)+ "&pf8=" + String(p_pfx7)+ "&pf9=" + String(p_pfx8)+ "&pf10=" + String(p_pfx9)+ "&pf11=" + String(p_pfx10)+ "&pf12=" + String(p_pfx11) );// + "&count=" + String(0)); //count todavia no esta soportado, no se si se necesita
      


      //version xoc2xx corriendo en vscode xoc003 ahora con payload extendida para incluir maxpot1-12 para mostrar potencia pico
      //sendData("Timestamp_Device=" + String(p_currenttime)+ "&config_id=" + String(caso) + "&XOC_id=" + String(XOCid,3)  + "&counterstatus=" + String(p_counterstatus) + "&V1=" + String(p_V1)+ "&V2=" + String(p_V2) + "&V3=" + String(p_V3)+ "&I1=" + String(p_Ix0)+ "&I2=" + String(p_Ix1)  + "&I3=" + String(p_Ix2)+ "&I4=" + String(p_Ix3) + "&I5=" + String(p_Ix4)+ "&I6=" + String(p_Ix5)+ "&I7=" + String(p_Ix6)+ "&I8=" + String(p_Ix7)+ "&I9=" + String(p_Ix8)+ "&I10=" + String(p_Ix9)+ "&I11=" + String(p_Ix10)+ "&I12=" + String(p_Ix11)+ "&Wh1=" + String(p_Whx0)+ "&Wh2=" + String(p_Whx1) + "&Wh3=" + String(p_Whx2)+ "&Wh4=" + String(p_Whx3)+ "&Wh5=" + String(p_Whx4)+ "&Wh6=" + String(p_Whx5)+ "&Wh7=" + String(p_Whx6)+ "&Wh8=" + String(p_Whx7)+ "&Wh9=" + String(p_Whx8)+ "&Wh10=" + String(p_Whx9)+ "&Wh11=" + String(p_Whx10)+ "&Wh12=" + String(p_Whx11)    + "&maxamps1=" + String(p_maxamps0)+ "&maxamps2=" + String(p_maxamps1)+"&maxamps3=" + String(p_maxamps2)+"&maxamps4=" + String(p_maxamps3)+"&maxamps5=" + String(p_maxamps4)+"&maxamps6=" + String(p_maxamps5)+"&maxamps7=" + String(p_maxamps6)+"&maxamps8=" + String(p_maxamps7)+"&maxamps9=" + String(p_maxamps8)+"&maxamps10=" + String(p_maxamps9)+"&maxamps11=" + String(p_maxamps10)+"&maxamps12=" + String(p_maxamps11)+   "&pf1=" + String(p_pfx0)+ "&pf2=" + String(p_pfx1)+ "&pf3=" + String(p_pfx2)+ "&pf4=" + String(p_pfx3)+ "&pf5=" + String(p_pfx4)+ "&pf6=" + String(p_pfx5)+ "&pf7=" + String(p_pfx6)+ "&pf8=" + String(p_pfx7)+ "&pf9=" + String(p_pfx8)+ "&pf10=" + String(p_pfx9)+ "&pf11=" + String(p_pfx10)+ "&pf12=" + String(p_pfx11) );// + "&count=" + String(0)); //count todavia no esta soportado, no se si se necesita
      //******* */
      //p_Ix4=flag_error; //temp. COMENTADA POR RECOMENDACIÓN DE IA. 

       sendData("Timestamp_Device=" + String(p_currenttime)+ "&config_id=" + String(caso) + "&XOC_id=" + String(XOCid,3)  + "&counterstatus=" + String(p_counterstatus) + "&V1=" + String(p_V1)+ "&V2=" + String(p_V2) + "&V3=" + String(p_V3)+ "&I1=" + String(p_Ix0)+ "&I2=" + String(p_Ix1)  + "&I3=" + String(p_Ix2)+ "&I4=" + String(p_Ix3) + "&I5=" + String(p_Ix4)+ "&I6=" + String(p_Ix5)+ "&I7=" + String(p_Ix6)+ "&I8=" + String(p_Ix7)+ "&I9=" + String(p_Ix8)+ "&I10=" + String(p_Ix9)+ "&I11=" + String(p_Ix10)+ "&I12=" + String(p_Ix11)+ "&Wh1=" + String(p_Whx0)+ "&Wh2=" + String(p_Whx1) + "&Wh3=" + String(p_Whx2)+ "&Wh4=" + String(p_Whx3)+ "&Wh5=" + String(p_Whx4)+ "&Wh6=" + String(p_Whx5)+ "&Wh7=" + String(p_Whx6)+ "&Wh8=" + String(p_Whx7)+ "&Wh9=" + String(p_Whx8)+ "&Wh10=" + String(p_Whx9)+ "&Wh11=" + String(p_Whx10)+ "&Wh12=" + String(p_Whx11)    + "&maxamps1=" + String(p_maxamps0)+ "&maxamps2=" + String(p_maxamps1)+"&maxamps3=" + String(p_maxamps2)+"&maxamps4=" + String(p_maxamps3)+"&maxamps5=" + String(p_maxamps4)+"&maxamps6=" + String(p_maxamps5)+"&maxamps7=" + String(p_maxamps6)+"&maxamps8=" + String(p_maxamps7)+"&maxamps9=" + String(p_maxamps8)+"&maxamps10=" + String(p_maxamps9)+"&maxamps11=" + String(p_maxamps10)+"&maxamps12=" + String(p_maxamps11)+   "&pf1=" + String(p_pfx0)+ "&pf2=" + String(p_pfx1)+ "&pf3=" + String(p_pfx2)+ "&pf4=" + String(p_pfx3)+ "&pf5=" + String(p_pfx4)+ "&pf6=" + String(p_pfx5)+ "&pf7=" + String(p_pfx6)+ "&pf8=" + String(p_pfx7)+ "&pf9=" + String(p_pfx8)+ "&pf10=" + String(p_pfx9)+ "&pf11=" + String(p_pfx10)+ "&pf12=" + String(p_pfx11) );// + "&count=" + String(0)); //count todavia no esta soportado, no se si se necesita
       

      if (flag_online == 1)
      {
        o--;
        if(o==0)
        {
          i = 0;
          //WiFi.mode( WIFI_MODE_NULL );
        }//exit

      }
      else
      {
        payloadbag[i][0] = 1; //mark as unsent
      }
    }
    else
    {
     // WiFi.mode( WIFI_MODE_NULL );
      //Serial.print("nadaaaaaa maaaaas");
    }
  }//end for
}//end looppublisher

void OTAcheck()
{
  ///////////////////////////////////////// ota //////////////////////////////////////////
  int x = XOCver;
  Serial.println(">>> OTA XOC ID: " + String(XOC_unique_id) + " , version: " + String(x));

  // String bin = "/firmware" + String(version + 1) + ".bin";
  String bin = "/xoc" + String(XOC_unique_id) + "." + String(x + 1) + ".bin"; // ejemplo: xoc1.102.bin
  Serial.print("Looking for version: ");
  Serial.println(String(x + 1) + "on firmware: " + bin);

  OTA.update(bin);
}

void Task2code( void * pvParameters ){
  delay(200);
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());
  //Serial.println("Configuring WDT core 1...");
  //esp_task_wdt_init(3, true); //enable panic so ESP32 restarts
  //esp_task_wdt_add(NULL); //add current thread to WDT watch
  //esp_task_wdt_reset();
  for (;;)
  {
    Serial.print(",");
    
    // zeit=millis();                  //timer para guardar dato o publicar
    // readcycle();    //
    // analysis(); //cleanup, voltaje , corrientes, powerfactor
    // addup();
    // empaquetador(); // average, addtobasket temp descomentar
    // delay(100);

    // new:
    zeit = millis(); // timer para guardar dato o publicar

    for (byte i = 0; i < 12; i++)
    {
      flag_wdt=1;//wdt set
      //                                                           Serial.println(" "); 
      // Serial.print("                                                in:");
      //Serial.print(i);


      //                                                           Serial.println(" "); 
      // Serial.print("                                                readcycle_new:");
      readcycle_new(i);



          //                                                       Serial.println(" "); 
          // Serial.print("                                                anaysis_new:");
      analysis_new(i); // cleanup, voltaje , corrientes, powerfactor
      // //temp:
      //         V=100;
      //         I=1;
      //         pfactor_new=0.9;

      //                                                     Serial.println(" "); 
      // Serial.print("                                                  addup_new:");
      addup_new(i);
      //Serial.print(" ");
      delay(10);
    }
                                                              //Serial.println(" "); 
    empaquetador(); // average, addtobasket
    delay(100);
  }
}//end task2code

void relaunchTask2()
{
  vTaskDelete(Task2);
  delay(500);
  Serial.println("Configuring Task2Core...");
  xTaskCreatePinnedToCore(
      Task2code, /* Task function. */
      "Task2",   /* name of task. */
      10000,     /* Stack size of task */
      NULL,      /* parameter of the task */
      1,         /* priority of the task */
      &Task2,    /* Task handle to keep track of created task */
      1);        /* pin task to core 1 */
  delay(100);
}

void doevery()
{
  currentMillis = millis();
  // wificheck
  if (currentMillis - previousMillis_wificheck >= 10000 || currentMillis < previousMillis_wificheck)
  {
    wificheck();
    previousMillis_wificheck = currentMillis;
  }

  // ping
  if (currentMillis - previousMillis_ping >= 20000 || currentMillis < previousMillis_ping)
  {
    ping();
    previousMillis_ping = currentMillis;
  }

  // updatecurrentTime
  if (currentMillis - previousMillis_timerbkp >= 3600000) // cada hora=3600000
  {
    previousMillis_timerbkp = currentMillis;
    updatecurrentTime();
    preferences.begin("my-pref", false);
    preferences.putULong("currentTime", currentTime);
    preferences.end();
    // flag_timebackup = 1;
  }

  // OTA , LoRasetup, resarttolerance
  if (currentMillis - previousMillistimerOTA >= 60000 || currentMillis < previousMillistimerOTA)
  {
    previousMillistimerOTA = currentMillis;


  if (WiFi.status() == WL_CONNECTED)
  {
    OTAcheck();
  }
    //LoRaSetup(); //se hace el setup periodicamente para que en caso de que haya un fail de SPI en setup se pueda corregir.

    // restarttolerance
    // if (flag_online == 1)
    // {
    //   restarttolerance = 3600 * 24 * 1000 * 5; // every 5 days
    // }
    // else
    // {
    //   restarttolerance = restarttolerance / 2; // ve reduciendo el tiempo de espera hasta que se reinicie.
    //   Serial.print("restarttolerance: ");
    //   Serial.println(restarttolerance);
    // }
  }

  // ESP.restart:
  //if (currentMillis - previousMillisRestart >= restarttolerance || currentMillis < previousMillisRestart) // cada 24 hr reinicia
  
  if (currentMillis > restarttolerance) // cada 24 hr reinicia
  {
    // previousMillisRestart = currentMillis;
    updatecurrentTime();
    preferences.begin("my-pref", false);
    preferences.putULong("currentTime", currentTime);
    preferences.end();
    Serial.println("");
    Serial.print("RESTARTING NOW...");
    Serial.flush();
    ESP.restart(); //aguanta todavia no quiero reiniciarlo
  }


  currentMillis = millis();
  // external wdt reset
  
  if (currentMillis - previousMillis_wdt >= 5000 || currentMillis < previousMillis_wdt)
  {
    // do stuff for wdt, check both cores
    if (flag_wdt == 1)
    {
      //Serial.print("----------------WDT--------------------------------");
      digitalWrite(WDT, HIGH);
      delay(10);
      digitalWrite(WDT, LOW);
      flag_wdt = 0;
    }
    else
    {
      flag_relauchTask2++;
      Serial.print("flag_relauchTask2:");Serial.println(flag_relauchTask2);
      if(flag_relauchTask2>3)
      {
        relaunchTask2();
        flag_relauchTask2=0;
      }
    }
    previousMillis_wdt = currentMillis;
  }

} // end doevery






void setup() /////////////////    SETUP    ///////////////////
{


  delay(100);
   //set pin modes 
 pinMode(ADCCS1, OUTPUT);
 pinMode(ADCCS2, OUTPUT);
 pinMode(MOSI, OUTPUT);
 pinMode(MISO, INPUT);
 pinMode(SCK, OUTPUT);
 pinMode(WDT, OUTPUT);
 pinMode(ledred, OUTPUT);
 pinMode(ledgreen, OUTPUT);
 pinMode(bot, INPUT_PULLUP);
 // disable device to start with
 digitalWrite(ADCCS1, HIGH);
 digitalWrite(ADCCS2, HIGH);
 digitalWrite(MOSI, LOW);
 digitalWrite(SCK, LOW);
 digitalWrite(WDT, HIGH);
 delay(50);
 digitalWrite(WDT, LOW);
 digitalWrite(ledred, HIGH);
 digitalWrite(ledgreen, LOW);

 Serial.begin(115200);
 delay(20);
 Serial.print("\n\n");
 Serial.print(">>>>> Bienvenido a XOC32 ID.version: ");
 Serial.println(XOCid,3);

 Serial.println("-----------SETUP-----------");
 /////pinMode(flowpin, INPUT_PULLUP);
 /////digitalWrite(enserialport, LOW); //LOW=ON

 //   //SPI
 //   // configure PIN mode
 //   //NOTA: SPI solo se usa para LoRa, los MCP3208 tambien usan SPI pero se esta bitbangeando entonces no usa librerias.
 // //  pinMode(ADCCS1, OUTPUT);
 //  // pinMode(ADCCS2, OUTPUT);
 //   pinMode(CSLORA, OUTPUT);
 //   //digitalWrite(ADCCS1, HIGH);
 //   //digitalWrite(ADCCS2, HIGH);
 //   digitalWrite(CSLORA, HIGH);
 //   SPISettings settings(ADC_CLK, MSBFIRST, SPI_MODE0);
 //   SPI.begin();
 //   SPI.beginTransaction(settings);

 // isocom hardware serial via RS485
 atSerial.begin(9600, SERIAL_8N1, 17, 16);
 delay(100);
 atSerial.print("Bienvenido enviado por atSerial");

 // para mensajes de LoRa
 whiteID[0] = XOC_unique_id;

 // stored preferences:
 Serial.println("loading stored data from preferences...");
 preferences.begin("my-pref", false);

 currentTime = preferences.getULong("currentTime", 0);
 Serial.print("currentTime: ");
 Serial.println(currentTime);

 ssid = preferences.getString("rec_ssid", WIFI_SSID);
 Serial.print("rec_ssid: ");
 Serial.println(ssid);

 password = preferences.getString("rec_password", WIFI_PASSWORD);
 Serial.print("rec_password: ");
 Serial.println(password);

 preferences.end();

 // create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
 Serial.println("Configuring Task2Core...");
 xTaskCreatePinnedToCore(
     Task2code, /* Task function. */
     "Task2",   /* name of task. */
     10000,     /* Stack size of task */
     NULL,      /* parameter of the task */
     1,         /* priority of the task */
     &Task2,    /* Task handle to keep track of created task */
     1);        /* pin task to core 1 */
 delay(500);

for (byte j = 0; j < 12; j++)
 {
   pf[j] = 0.0;
 }

  // connectToAWS();
  delay(50);

  // LoRaSetup();
  delay(50);

  //wifi stuff
  // WiFiStationSetup2(ssid, password);
  // if (WiFi.status() != WL_CONNECTED)
  // {
    WiFi.begin(WIFI_SSID_xoc, WIFI_PASSWORD2); // try one time factory wifi
    delay(3000);
  // }

  connecttohardcodedwifis();
  delay(100);
  updatecurrentTime();
  // wificheck();
  if (WiFi.status() == WL_CONNECTED)
  {
   send_universal_log("Timestamp_Device=" + String(currentTime) + "&config_id=" + String(caso) + "&XOC_id=" + String(XOCid, 3) + "&counterstatus=" + String(0));

  }
} // end setup

void loop()
{
  if (flag_online == 1)
  {
    looppublisher();
  }
  doevery();

  buttoncheck();

  // Serial.println("");
  // Serial.print("!");
  // zeit=millis();                  //timer para guardar dato o publicar
  // readcycle();    //
  // analysis(); //cleanup, voltaje , corrientes, powerfactor
  // addup();
  // empaquetador(); // average, addtobasket temp descomentar
  //

  //zeit = millis();
  delay(100);
  Serial.print("'");
}

//2do:



//concepto xocmon3 features:
// lee desde dos mcp3208 onboard, con puerto para expansion de otros dos mcp3208
//    mcp3208_1: 3 voltajes, 3 corrientes , vbat, vref = 8 inputs. mcp3208_2: 8 corrientes. mcp3208_3 y 4 8 corrientes.
// usa rtc
// usa sd card  https://randomnerdtutorials.com/esp32-microsd-card-arduino/
// OTA
// bateria backup para un par de horas de respaldo, despues apaga todo y se pone en hibernacion, solo revisa cada x seg si ya regreso la luz (falta ver casos especificos)
// lora capable para sensores inalambricos
// servidor web / osciloscopio via wifi o serial  https://randomnerdtutorials.com/esp32-web-server-microsd-card/
// autoconnect o de preferencia un portal mas ligero y que sirva mejor
// espnow para hablar con xocev y otros productos futuros


//corrientes 
// array "iconfig" para asignar nr de corrientes y asignacion a fase:
// las corrientes con 0 no se asignan a ninguna fase (no se lee pf ni wh, solo i) 
// iconfig se asigna al momento deprogramar y no cambia hasta que se reprograme el esp32 via ota o usb.

//sensores inalambricos
//solo pueden leer corriente, por lo tanto solo pueden reportar "i" y "Ah"




//otras cosas or hacer comunicaicon con xocev via serial o espnow o wifi o lora (pero no es tan seguro)
//millis overflow/rollover: https://arduino.stackexchange.com/questions/12587/how-can-i-handle-the-millis-rollover



//falta: 
//pasar todo a un core, por que talvez tas2 esta haciendo ruido
//upload a sheet: OK
//mejorar el calculo de iarea ya que si detecta jump>1000 le suma cero en vez de un valor mas realista
//powerfactor
//hacer que mande cero cuando no hay clamp: OK
//  solucion: poner jumper para activar el burden resistor de 1k para todas las entradas sin usar o para clamos sin burden interno, quitar jumper cuando se usen clamps con burden interno como el SCT013
//confirmar que lea bien voltaje y corrientes
//ajustar sheet para procesar toda la data
//configconfig


/*
update:
ver por que a vveces se tarda entre media y una hora en mandar el siguiente dato, sera que algo tiene  zeit = millis();? probar pasarlo a loop
revisar updatecurrenttime 

falta vver por que a veces se queda en led rojo

falta soldar el pin2 de WDT attiny a EN de esp32 y hacer codigo para eso
falta revisar updatecurrenttime y arreglarlo
falta revisar powerfactor_new
falta probar con variabilidad=1
falta soldar WDT pin y hacer codigo (y arreglar en eagle)
falta probar modo offline, hay un bug por ahi

a futuro:
SD card
RTC

*/