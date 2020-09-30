#ifndef configs_h
#define configs_h

//google publish
 #include <HTTPClient.h>
//String GOOGLE_SCRIPT_ID = "AKfycbyZuLmLX83geo0c0nQnzlz3w-uQtoSzJZZNBtQ7vGNWw62HEhAx"; // sheet: pupu2 
//String GOOGLE_SCRIPT_ID = "AKfycbwKJ4eFVA7c7tfNMuVmW7criLQvSzuioWzPl05NjJLABL6l0W79"; // m001 Replace by your GAS service id
//String GOOGLE_SCRIPT_ID = "AKfycbwvHLFtKlmxn1JLKSxQUXQtdGqMFZi6u8bgFvPCfZ_WAnRFYIVL"; // id:1 sheet:pupu 
//String GOOGLE_SCRIPT_ID = "AKfycbydrQCU67gRJiyRCfhZBjQmjzSCFpmc5nGzjDBE"; //id:3 sheet 03
String GOOGLE_SCRIPT_ID = "AKfycbxZCbWVgx4cfmQ-cR0cVU6uUWT1yoOyShJTtqArcPNjZTbVY6E"; //04
//String GOOGLE_SCRIPT_ID = "AKfycbzphLuRdrU4BjnvHl4kYuuMkAB059wXw0wQ2yoZTw"; //05

const float M3TRid = 4.02;
//local AP url: http://172.217.28.1/_ac/

//////////////////// Default Wifi credentials ///////////////////////////

//const char * WIFI_SSID = "INFINITUMA17B_2.4";
//const char * WIFI_PASSWORD = "ZbuQ54AsD4";
//const char *WIFI_SSID = "Mm1";
//const char *WIFI_PASSWORD = "12481632";
const char * WIFI_SSID = "AGUACATE";
const char * WIFI_PASSWORD = "chipotle";




// MQTT information4
//#define DEVICE_NAME "M3TR"
// #define AWS_IOT_TOPIC "gprod_test"
// #define AWS_IOT_ENDPOINT "a3ed10vql0i9ty-ats.iot.us-east-1.amazonaws.com"
// #define AWS_MAX_RECONNECT_TRIES 5

// LoRa params
#define SCK 5   // GPIO5  SCK
#define MISO 19 // GPIO19 MISO
#define MOSI 27 // GPIO27 MOSI
#define SS 18   // GPIO18 CS
#define RST 14  // GPIO14 RESET

#define BAND 915E6
#define SPREADING_FACTOR 12
#define BAND_WINDTH 500E3

#endif







//esptool.py --port /dev/cu.SLAB_USBtoUART erase_flash


//versiones:
//01 no existe
//02 version base que incluye offline basico, upload, autoconnect que NO sirve
//03 se agrega un segundo intento a senddata en caso de que no reciba un 200 o un 302 (para casos donde la wifi es debil)


