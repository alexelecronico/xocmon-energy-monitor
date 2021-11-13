#ifndef configs_h
#define configs_h

//google publish
 #include <HTTPClient.h>
//String GOOGLE_SCRIPT_ID = "AKfycbyZuLmLX83geo0c0nQnzlz3w-uQtoSzJZZNBtQ7vGNWw62HEhAx"; // sheet: pupu2 
//String GOOGLE_SCRIPT_ID = "AKfycbwKJ4eFVA7c7tfNMuVmW7criLQvSzuioWzPl05NjJLABL6l0W79"; // m001 Replace by your GAS service id
//String GOOGLE_SCRIPT_ID = "AKfycbwvHLFtKlmxn1JLKSxQUXQtdGqMFZi6u8bgFvPCfZ_WAnRFYIVL";   // 01 
//String GOOGLE_SCRIPT_ID = "AKfycbz7PZjFy7pxur9boSm50U3tCV5JKOieMUfiJDPS";             // 02
//String GOOGLE_SCRIPT_ID = "AKfycbydrQCU67gRJiyRCfhZBjQmjzSCFpmc5nGzjDBE";             // 03
//String GOOGLE_SCRIPT_ID = "AKfycbwgalsXzeVeO8QHh8E_kSYRllym3W6jJj2x16DpUDXUa_deztIh7aRKg45I4_2l4E9X";               //3.1
//String GOOGLE_SCRIPT_ID = "AKfycbyY0Kdl07u8Zjr1M7kntPsn45algjEQf6YY2c6lfUuTmzHaij0";  // 04
//String GOOGLE_SCRIPT_ID = "AKfycbz_pcCj8ovohrlNnCZdJ2IwtwzR-uG23ejIsSIlm56_a1PpTMLy";   //4.1
String GOOGLE_SCRIPT_ID = "AKfycbyfigPBkOwgQqVVMdyOYU5DwJRTJNTbfVAJqpoU2n84GX3bieQKKhRrPvTdnI6CCzJC";              //4.2
//String GOOGLE_SCRIPT_ID = "AKfycbxq6K9hSCv-Lvjgm-MFzRNTcbTdU4Dq-7M0AnGs33pGTxB7arfNXPNb";   //5.1
//String GOOGLE_SCRIPT_ID = "AKfycbzS0dr03JAKFMKoE8I4a-Kjfl--5MfiH9HUFCo2On3cRGcsPP-7cBuaRP2e5LtlhOnS";   //new 5.1



const byte M3TR_unique_id = 4;      //ID

const float M3TRver = 11;            //version, debe ser igual a la que se exporta p.ej firmware4.6.bin en https://s3.console.aws.amazon.com/s3/buckets/otabucketm?region=us-east-1&tab=objects    aguapasapormicasaaws3!


const float M3TRid = M3TR_unique_id + M3TRver/100;  //p ej: 1.03;
//local AP url: http://172.217.28.1/_ac/

//////////////////// Default Wifi credentials ///////////////////////////

//const char * WIFI_SSID = "INFINITUMA17B_2.4";
//const char * WIFI_PASSWORD = "ZbuQ54AsD4";
//const char * WIFI_SSID = "WiFi Casaclub";
//const char * WIFI_PASSWORD = "Cclub$21";
const char * WIFI_SSID = "AGUACATEx";
const char * WIFI_PASSWORD = "chipotle";

const char * WIFI_SSID_m3tr = "m3tr";
const char * WIFI_PASSWORD2 = "12345678";

const byte whitelistlength=1;
int whiteID[whitelistlength];
void whiteIDlist(){
whiteID[0]=M3TR_unique_id;
}




// MQTT information4
//#define DEVICE_NAME "M3TR"
// #define AWS_IOT_TOPIC "gprod_test"
// #define AWS_IOT_ENDPOINT "a3ed10vql0i9ty-ats.iot.us-east-1.amazonaws.com"
// #define AWS_MAX_RECONNECT_TRIES 5

// // LoRa params
// #define SCK 5   // GPIO5  SCK
// #define MISO 19 // GPIO19 MISO
// #define MOSI 27 // GPIO27 MOSI
// #define SS 18   // GPIO18 CS
// #define RST 14  // GPIO14 RESET

// #define BAND 915E6
// #define SPREADING_FACTOR 12
// #define BAND_WINDTH 500E3

#endif







//


//versiones:
//01 no existe
//02 version base que incluye offline basico, upload, autoconnect que NO sirve
//03 se agrega un segundo intento a senddata en caso de que no reciba un 200,302,-11 o lo guarda en backup (para casos donde la wifi es debil) FALTA probar
//   se agrega railenable para apagar el sensor cuando no hay luz y la bateria esta baja FALTA PROBAR
//04 se agrega OTA, si se quiere reflashear por serial un ESP que hizo OTA se tiene que borrar la flash con esptool.py --port /dev/cu.SLAB_USBtoUART erase_flash
//   se arregla bug que hacia que publicara constantemente (creo que porque publishnor=0 cada vez que fallaba wifi)
//05 se reestructura la publicacion de mensajes para optimizar que no se pierdan
//06 nueva estructura de payload para sensores lora (solar, boiler bkp, heater todo y nivo)
//07 se incluye en la publicacion a google el contador de cada sensor extra (para ver que lleguen bien)
//08 bug fix "cadena demasiado larga", se incrementa lengthpayload de 31 a 40
//09 y 10 nada, solo se uso para probar ota en 4.2