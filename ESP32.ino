#include <EEPROM.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HMC5883L.h>
//#define magPin A0 //Acceso al magnetometro
//#define actionActivateDevice D7
#define actionConfig D3 //Iniciar conexiòn WiFi
#define actionReset D6 //eliminar configuración
#define actionDisplay D4 // Mostrar datos en pantalla

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
HMC5883L compass;

int val=0;
 
bool auxDisplay = false;

unsigned long lastMillis;


/**
 * Vars to connect with wifi and send data to server
 */
  int buttonState=0;
  int incomingByte = 0; // for incoming serial data
  
  WiFiClientSecure clientTemp;
  HTTPClient httpClientTemp;
  
  
  String serverName = "https://megahx.com.mx/api/v1/device/mini/update/content";
  String urlRequestTemp = "https://megahx.com.mx/api/v1/device/mini/weather";
  String urlRequest = "https://megahx.com.mx/api/v1/device/mini/update";//Ruta final
  String device_uuid = "MEGAH6091967769f35";//Token correspondiente
  
  const char ssid[] = "MEGAH6091967769f35";
  const char password[] = "123456789";
  char ssidc[30];
  char passwordc[30];
  int ip_group=0;//sets what number set of the IP address we are collecting. 5 in total
  String string_IP="";//temporarily stores a IP group number
  IPAddress wap_ip(192,168,1,1);
  IPAddress wap_gateway(192,168,1,254);
  IPAddress subnet(255, 255, 255, 0); 
  String ssid_Arg;// Error message for ssid input
  String password_Arg;// Error message for password input
  String ip_Arg;// Error message for ip input
  String gw_Arg;// Error message for gateway input
  boolean actionPross = true;
  bool auxWifiRunService = false;

  AsyncWebServer server(80);
  /**
   * Status response WiFi Connection
   * Value        Info
   * 255          No Shield
   * 0            IDLE Status
   * 1            NO SSID disponibles
   * 2            Escaneo completo
   * 3            Conectado
   * 4            Fallo al intentar conectarse
   * 5            Conexión perdida
   * 6            Desconectado
   */

//creates int out of a string for your IP address
int ip_convert(String convert){
  int number;
  if (convert.toInt()){
      number=convert.toInt();
      string_IP="";
      return number;
     }else{
      Serial.println("error. ONe Not an IP address");
      return 400;
     }
 }
//Reads a string out of memory
String read_string(int l, int p){
  String temp;
  for (int n = p; n < l+p; ++n)
    {
     if(char(EEPROM.read(n))!=';'){
        if(isWhitespace(char(EEPROM.read(n)))){
          //do nothing
        }else temp += String(char(EEPROM.read(n)));
      
     }else n=l+p;
     
    }
  return temp;
}

bool startWPSPBC() {
  Serial.println("Inicio de configuración de WPS");
  bool wpsSuccess = WiFi.beginWPSConfig();
  if (wpsSuccess) {
    // Well this means not always success :-/ in case of a timeout we have an empty ssid
    String newSSID = WiFi.SSID();
    if (newSSID.length() > 0) {
      // WPSConfig has already connected in STA mode successfully to the new station.
      Serial.printf("WPS terminado. Conectado con éxito al SSID '%s'\n", newSSID.c_str());
    } else {
      wpsSuccess = false;
    }
  }
  return wpsSuccess;
}

void WPSConfig(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str()); // reading data from EPROM, last saved credentials
  /*
   while (WiFi.status() == WL_DISCONNECTED) {
    delay(500);
    Serial.print(".");
  }
   */

  wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    Serial.printf("\nConectado con éxito al SSID '%s'\n", WiFi.SSID().c_str());
  }else {
    Serial.printf("\nNo se ha podido conectar al WiFi. estado='%d'", status);
    //Serial.println("Por favor, pulse el botón WPS de su router.\nPulse cualquier tecla para continuar...");

      if (startWPSPBC()) {
       Serial.print("Conectado por WPS");
        WiFi.persistent(true);
      }else{
        Serial.println("WiFi UI Config");
        String string_Ssid="";
        string_Ssid= read_string(30,0); 
        Serial.println("ssid: "+string_Ssid);
        if (read_string(30,0).length() != 0){
          WiFiConnection();
        }else{
          WAP();
        }
        delay(200);
      }
   
           
      
    }
  
}
  void connectWithNetwork(){
   WiFi.mode(WIFI_STA);
    WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str()); // reading data from EPROM, last saved credentials
  }

void setup() {
 Serial.begin(9600); 
 //Serial.begin(115200);
 EEPROM.begin(512);

  while (!compass.begin()){
    Serial.println("Nie odnaleziono HMC5883L, sprawdz polaczenie!");
    delay(500);
  }
 
 if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); 
 }
   //pinMode(actionActivateDevice, OUTPUT);
  //connectWithNetwork();

   // Ustawienie zakresu pomiarowego
  // +/- 0.88 Ga: HMC5883L_RANGE_0_88GA
  // +/- 1.30 Ga: HMC5883L_RANGE_1_3GA (domyslny)
  // +/- 1.90 Ga: HMC5883L_RANGE_1_9GA
  // +/- 2.50 Ga: HMC5883L_RANGE_2_5GA
  // +/- 4.00 Ga: HMC5883L_RANGE_4GA
  // +/- 4.70 Ga: HMC5883L_RANGE_4_7GA
  // +/- 5.60 Ga: HMC5883L_RANGE_5_6GA
  // +/- 8.10 Ga: HMC5883L_RANGE_8_1GA
  compass.setRange(HMC5883L_RANGE_1_3GA);
 
  // Ustawienie trybu pracy
  // Uspienie:              HMC5883L_IDLE
  // Pojedynczy pomiar:     HMC5883L_SINGLE
  // Ciagly pomiar: HMC5883L_CONTINOUS (domyslny)
  compass.setMeasurementMode(HMC5883L_CONTINOUS);
 
  // Ustawienie czestotliwosci pomiarow
  //  0.75Hz: HMC5883L_DATARATE_0_75HZ
  //  1.50Hz: HMC5883L_DATARATE_1_5HZ
  //  3.00Hz: HMC5883L_DATARATE_3HZ
  //  7.50Hz: HMC5883L_DATARATE_7_50HZ
  // 15.00Hz: HMC5883L_DATARATE_15HZ (domyslny)
  // 30.00Hz: HMC5883L_DATARATE_30HZ
  // 75.00Hz: HMC5883L_DATARATE_75HZ
  compass.setDataRate(HMC5883L_DATARATE_15HZ);
 
  // Liczba usrednionych probek
  // 1 probka:  HMC5883L_SAMPLES_1 (domyslny)
  // 2 probki: HMC5883L_SAMPLES_2
  // 4 probki: HMC5883L_SAMPLES_4
  // 8 probki: HMC5883L_SAMPLES_8
  compass.setSamples(HMC5883L_SAMPLES_1);
  
  display.display();
  delay(2000); 

  WPSConfig();
  // Clear the buffer
  display.clearDisplay();

  //pinMode(8, OUTPUT);//bus

  
  pinMode(actionDisplay, INPUT);
  pinMode(actionConfig, INPUT);
  pinMode(actionReset, INPUT);
  powerOnDisplay();
  

}

void loop() {
  unsigned long currentMillis = millis();
   if(digitalRead(actionDisplay) == 0){
    auxDisplay = true;
    powerOnDisplay();
   }

   if(digitalRead(actionConfig) == 0){
      Serial.println("WPS WiFi Config");
      WPSConfig();
      delay(200);
   }


   // Pobranie pomiarow surowych
    Vector raw = compass.readRaw();
   
    // Pobranie pomiarow znormalizowanych
    Vector norm = compass.readNormalize();
   
    // Wyswielnie wynikow
     
    //Serial.print(" Xraw = ");
    //Serial.print(raw.XAxis);
   // Serial.print(" Yraw = ");
    //Serial.print(raw.YAxis);
    Serial.print(" Zraw = ");
    Serial.print(raw.ZAxis);
    //Serial.print(" Xnorm = ");
    //Serial.print(norm.XAxis);
    //Serial.print(" Ynorm = ");
    //Serial.print(norm.YAxis);
    Serial.print(" ZNorm = ");
    Serial.print(norm.ZAxis);
    Serial.println();
    
   

   if (millis() - lastMillis >= 2*60*1000UL) 
    {
     lastMillis = millis();  //get ready for the next iteration
      //Serial.println("Enviar datos al servidor");
      //sendDataToServer();
    }


  
  
   
  resetOption();
   
   
}


  void resetOption(){
    if(digitalRead(actionReset) == 0){
       Serial.println("Reset Config");
       erase_memory();
       delay(200);
   } 
  }

  void sendDataToServer(){
    if (Serial.available() > 0) {
      getTempViaServer();
    }
 
  }


  String getValueCylinderlStationary(int valueProgress){

      if (valueProgress >= 1) {
            if (valueProgress == 97) {//0.5%
                return "0 %";
            } else if (valueProgress == 96) {
                return "0 %";
            } else if (valueProgress == 95) {
                 return "0 %";
            } else if (valueProgress == 94) {
                return "0 %";
            } else if (valueProgress == 93) {
                return "0 %";
            } else if (valueProgress == 92) {
                 return "0 %";
            } else if (valueProgress == 91) {
                 return "0 %";
            } else if (valueProgress == 90) {
                 return "0 %";
            } else if (valueProgress == 89) {
                 return "0 %";
            } else if (valueProgress == 88) {
                 return "0 %";
            } else if (valueProgress == 87) {
                 return "0 %";
            } else if (valueProgress == 86) {
                 return "0 %";
            } else if (valueProgress == 85) {
                 return "0 %";
            } else if (valueProgress == 84) {
                 return "0 %";
            } else if (valueProgress == 83) {
                 return "0 %";
            } else if (valueProgress == 82) {
                 return "0 %";
            } else if (valueProgress == 81) {
                 return "0 %";
            } else if (valueProgress == 80) {
                 return "0 %";
            } else if (valueProgress == 79) {
                 return "0 %";
            } else if (valueProgress == 78) {
                 return "0 %";
            } else if (valueProgress == 77) {//0 valor sensor
                 return "0 %";
            } else if (valueProgress == 76) {//10%
                 return "0 %";
            } else if (valueProgress == 75) {//10%
                 return "0 %";
            } else if (valueProgress == 74) {//10%
                 return "1 %";
            } else if (valueProgress == 73) {//10%
                 return "1 %";
            } else if (valueProgress == 72) {//10%
                 return "1 %";
            } else if (valueProgress == 71) {//10%
                 return "1 %";
            } else if (valueProgress == 70) {//10%
                return "2 %";
            } else if (valueProgress == 69) {//10%
                return "2 %";
            } else if (valueProgress == 68) {//10%
                return "2 %";
            } else if (valueProgress == 67) {//10%
                return "2 %";
            } else if (valueProgress == 66) {//10%
                return "3 %";
            } else if (valueProgress == 65) {//10%
                return "3 %";
            } else if (valueProgress == 64) {//10%
                return "3 %";
            } else if (valueProgress == 63) {//10%
                return "3 %";
            } else if (valueProgress == 62) {//10%
                return "4 %";
            } else if (valueProgress == 61) {//10%
                return "4 %";
            } else if (valueProgress == 60) {//10%
                return "4 %";
            } else if (valueProgress == 59) {//10%
                return "4 %";
            } else if (valueProgress == 58) {//5% valor de sensor ////////////////////////////
                return "5 %";
            } else if (valueProgress == 57) {//10%
                return "5 %";
            } else if (valueProgress == 56) {//10%
                return "5 %";
            } else if (valueProgress == 55) {//10%
                return "5 %";
            } else if (valueProgress == 54) {//10%
                return "5 %";
            } else if (valueProgress == 53) {//10%
                return "5 %";
            } else if (valueProgress == 52) {//10%
                return "5 %";
            } else if (valueProgress == 51) {//10%
                return "6 %";
            } else if (valueProgress == 50) {//10%
                return "6 %";
            } else if (valueProgress == 49) {//10%
                return "6 %";
            } else if (valueProgress == 48) {//10%
                return "6 %";
            } else if (valueProgress == 47) {//10%
                return "6 %";
            } else if (valueProgress == 46) {//10%
                return "6 %";
            } else if (valueProgress == 45) {//10%
                return "6 %";
            } else if (valueProgress == 44) {//10%
                return "6 %";
            } else if (valueProgress == 43) {//10%
                return "7 %";
            } else if (valueProgress == 42) {//10%
                return "7 %";
            } else if (valueProgress == 41) {//10%
                return "7 %";
            } else if (valueProgress == 40) {//10%
                return "7 %";
            } else if (valueProgress == 39) {//10%
                return "7 %";
            } else if (valueProgress == 38) {//10%
                return "7 %";
            } else if (valueProgress == 37) {//10%
                return "8 %";
            } else if (valueProgress == 36) {//10%
                return "8 %";
            } else if (valueProgress == 35) {//10%
                return "8 %";
            } else if (valueProgress == 34) {//10%
                return "8 %";
            } else if (valueProgress == 33) {//10%
                return "8 %";
            } else if (valueProgress == 32) {//10%
                return "8 %";
            } else if (valueProgress == 31) {//10%
                return "8 %";
            } else if (valueProgress == 30) {//10%
                return "9 %";
            } else if (valueProgress == 29) {//10%
                return "9 %";
            } else if (valueProgress == 28) {//10%
                return "9 %";
            } else if (valueProgress == 27) {//10%
                return "9 %";
            } else if (valueProgress == 26) {//10%
                return "9 %";
            } else if (valueProgress == 25) {//10% valor sensor/////////////
                return "10 %";
            } else if (valueProgress == 24) {//10%
                return "10 %";
            } else if (valueProgress == 23) {//10%
                return "10 %";
            } else if (valueProgress == 22) {//10%
                return "10 %";
            } else if (valueProgress == 21) {//10%
                return "11 %";
            } else if (valueProgress == 20) {//10%
                return "11 %";
            } else if (valueProgress == 19) {//10%
                return "11 %";
            } else if (valueProgress == 18) {//10%
                return "11 %";
            } else if (valueProgress == 17) {//10%
                return "12 %";
            } else if (valueProgress == 16) {//10%
                return "12 %";
            } else if (valueProgress == 15) {//10%
                return "12 %";
            } else if (valueProgress == 14) {//10%
                return "12 %";
            } else if (valueProgress == 13) {//10%
                return "12 %";
            } else if (valueProgress == 12) {//10%
                return "13 %";
            } else if (valueProgress == 11) {//10%
                return "13 %";
            } else if (valueProgress == 10) {//10%
                return "13 %";
            } else if (valueProgress == 9) {//10%
                return "14 %";
            } else if (valueProgress == 8) {//10%
                return "14 %";
            } else if (valueProgress == 7) {//10%
                return "14 %";
            } else if (valueProgress == 6) {//10%
                return "14 %";
            } else if (valueProgress == 5) {//15% valor de sensor/////////////////////////
                return "15 %";
            } else if (valueProgress == 4) {//10%
                return "15 %";
            } else if (valueProgress == 3) {//10%
                return "15 %";
            } else if (valueProgress == 2) {//10%
                return "15 %";
            } else if (valueProgress == 1) {//10%
                return "15 %";
            } else if (valueProgress == 360) {//10%
                return "16 %";
            } else if (valueProgress == 359) {//10%
                return "16 %";
            } else if (valueProgress == 358) {//10%
                return "16 %";
            } else if (valueProgress == 357) {//10%
                return "16 %";
            } else if (valueProgress == 356) {//10%
                return "17 %";
            } else if (valueProgress == 355) {//10%
                return "17 %";
            } else if (valueProgress == 354) {//10%
                return "17 %";
            } else if (valueProgress == 353) {//10%
                return "18 %";
            } else if (valueProgress == 352) {//10%
                return "18 %";
            } else if (valueProgress == 351) {//10%
                return "18 %";
            } else if (valueProgress == 350) {//10%
                return "18 %";
            } else if (valueProgress == 349) {//10%
                return "19 %";
            } else if (valueProgress == 348) {//10%
                return "19 %";
            } else if (valueProgress == 347) {//10%
                return "19 %";
            } else if (valueProgress == 346) {//10%
                return "19 %";
            } else if (valueProgress == 345) {//10%
                return "19 %";
            } else if (valueProgress == 344) {//20% valor sensor/////////////////////////
                return "20 %";
            } else if (valueProgress == 343) {//20%
                return "20 %";
            } else if (valueProgress == 342) {//20%
                return "20 %";
            } else if (valueProgress == 341) {//20%
                return "20 %";
            } else if (valueProgress == 340) {//20%
                return "21 %";
            } else if (valueProgress == 339) {//20%
                return "21 %";
            } else if (valueProgress == 338) {//20%
                return "21 %";
            } else if (valueProgress == 337) {//20%
                return "22 %";
            } else if (valueProgress == 336) {//20%
                return "22 %";
            } else if (valueProgress == 335) {//20%
                return "23 %";
            } else if (valueProgress == 334) {//20%
                return "23 %";
            } else if (valueProgress == 333) {//20%
                return "24 %";
            } else if (valueProgress == 332) {//20%
                return "24 %";
            } else if (valueProgress == 331) {//25% valor sensor///////////////////////
                return "25 %";
            } else if (valueProgress == 330) {//40%
                return "25 %";
            } else if (valueProgress == 329) {//40%
                return "25 %";
            } else if (valueProgress == 328) {//40%
                return "25 %";
            } else if (valueProgress == 327) {//40%
                return "26 %";
            } else if (valueProgress == 326) {//40%
                return "26 %";
            } else if (valueProgress == 325) {//40%
                return "27 %";
            } else if (valueProgress == 324) {//40%
                return "27 %";
            } else if (valueProgress == 323) {//40%
                return "28 %";
            } else if (valueProgress == 322) {//40%
                return "28 %";
            } else if (valueProgress == 321) {//40%
                return "29 %";
            } else if (valueProgress == 320) {//40%
                return "29 %";
            } else if (valueProgress == 319) {//40%
                return "29 %";
            } else if (valueProgress == 318) {//30%/////////////////////////////////
                return "30 %";
            } else if (valueProgress == 317) {//40%
                return "30 %";
            } else if (valueProgress == 316) {//40%
                return "30 %";
            } else if (valueProgress == 315) {//40%
                return "30 %";
            } else if (valueProgress == 314) {//40%
                return "30 %";
            } else if (valueProgress == 313) {//40%
                return "30 %";
            } else if (valueProgress == 312) {//40%
                return "30 %";
            } else if (valueProgress == 311) {//40%
                return "30 %";
            } else if (valueProgress == 310) {//40%
                return "30 %";
            } else if (valueProgress == 309) {//40%
                return "30 %";
            } else if (valueProgress == 308) {//40%
                return "30 %";
            } else if (valueProgress == 307) {//40%
                return "30 %";
            } else if (valueProgress == 306) {//40%
                return "30 %";
            } else if (valueProgress == 305) {//35% valor sensor///////////////////
                return "35%";
            } else if (valueProgress == 304) {//40%
                return "35%";
            } else if (valueProgress == 303) {//40%
                return "35%";
            } else if (valueProgress == 302) {//40%
                return "35%";
            } else if (valueProgress == 301) {//40%
                return "35%";
            } else if (valueProgress == 300) {//40%
                return "35%";
            } else if (valueProgress == 299) {//40%
                return "35%";
            } else if (valueProgress == 298) {//40%
                return "35%";
            } else if (valueProgress == 297) {//40%
                return "35%";
            } else if (valueProgress == 296) {//40%
                return "35%";
            } else if (valueProgress == 295) {//40%
                return "35%";
            } else if (valueProgress == 294) {//40%
                return "35%";
            } else if (valueProgress == 293) {//40%
                return "35%";
            } else if (valueProgress == 292) {//40%
                return "35%";
            } else if (valueProgress == 291) {//40 valos sensor////////////////////
                return "40 %";
            } else if (valueProgress == 290) {
                return "40 %";
            } else if (valueProgress == 289) {
                return "40 %";
            } else if (valueProgress == 288) {
                return "40 %";
            } else if (valueProgress == 287) {
                return "40 %";
            } else if (valueProgress == 286) {
                return "40 %";
            } else if (valueProgress == 285) {
                return "40 %";
            } else if (valueProgress == 284) {
                return "40 %";
            } else if (valueProgress == 283) {
                return "40 %";
            } else if (valueProgress == 282) {
                return "40 %";
            } else if (valueProgress == 281) {
                return "40 %";
            } else if (valueProgress == 280) {
                return "40 %";
            } else if (valueProgress == 279) {
                return "40 %";
            } else if (valueProgress == 278) {//45 valor sensor////
                return "45 %";
            } else if (valueProgress == 277) {
                return "45 %";
            } else if (valueProgress == 276) {//100%
                return "45 %";
            } else if (valueProgress == 275) {//100%
                return "45 %";
            } else if (valueProgress == 274) {//100%
                return "46 %";
            } else if (valueProgress == 273) {//100%
                return "46 %";
            } else if (valueProgress == 272) {//100%
                return "46 %";
            } else if (valueProgress == 271) {//100%
                return "47 %";
            } else if (valueProgress == 270) {//100%
                return "47 %";
            } else if (valueProgress == 269) {//100%
                return "47 %";
            } else if (valueProgress == 268) {//100%
                return "48 %";
            } else if (valueProgress == 267) {//100%
                return "48 %";
            } else if (valueProgress == 266) {//100%
                return "48 %";
            } else if (valueProgress == 265) {//100%
                return "49 %";
            } else if (valueProgress == 264) {//100%
                return "49 %";
            } else if (valueProgress == 263) {//100%
                return "49 %";
            } else if (valueProgress == 262) {//50% valor sensor////////////////
                return "50 %";
            } else if (valueProgress == 261) {//100%
                return "50 %";
            } else if (valueProgress == 260) {//100%
                return "50 %";
            } else if (valueProgress == 259) {//100%
                return "50 %";
            } else if (valueProgress == 258) {//100%
                return "51 %";
            } else if (valueProgress == 257) {//100%
                return "51 %";
            } else if (valueProgress == 256) {//100%
                return "51 %";
            } else if (valueProgress == 255) {//100%
                return "52 %";
            } else if (valueProgress == 254) {//100%
                return "52 %";
            } else if (valueProgress == 253) {//100%
                return "53 %";
            } else if (valueProgress == 252) {//100%
                return "53 %";
            } else if (valueProgress == 251) {//100%
                return "54 %";
            } else if (valueProgress == 250) {//100%
                return "54 %";
            } else if (valueProgress == 249) {//100%
                return "54 %";
            } else if (valueProgress == 248) {//55% valor sensor////////////
                return "55 %";
            } else if (valueProgress == 247) {//100%
                return "55 %";
            } else if (valueProgress == 246) {//100%
                return "55 %";
            } else if (valueProgress == 245) {//100%
                return "56 %";
            } else if (valueProgress == 244) {//100%
                return "56 %";
            } else if (valueProgress == 243) {//100%
                return "57 %";
            } else if (valueProgress == 242) {//100%
                return "57 %";
            } else if (valueProgress == 241) {//100%
                return "58 %";
            } else if (valueProgress == 240) {//100%
                return "58 %";
            } else if (valueProgress == 239) {//100%
                return "59 %";
            } else if (valueProgress == 238) {//100%
                return "59 %";
            } else if (valueProgress == 237) {//60% valor sensor//////////
                return "60 %";
            } else if (valueProgress == 236) {//100%
                return "60 %";
            } else if (valueProgress == 235) {//100%
                return "60 %";
            } else if (valueProgress == 234) {//100%
                return "61 %";
            } else if (valueProgress == 233) {//100%
                return "61 %";
            } else if (valueProgress == 232) {//100%
                return "62 %";
            } else if (valueProgress == 231) {//100%
                return "62 %";
            } else if (valueProgress == 230) {//100%
                return "63 %";
            } else if (valueProgress == 229) {//100%
                return "64 %";
            } else if (valueProgress == 228) {//100%
                return "64 %";
            } else if (valueProgress == 227) {//100%
                return "64 %";
            } else if (valueProgress == 226) {////valor sensor//65%///////////////////
                return "65 %";
            } else if (valueProgress == 225) {//100%
                return "65 %";
            } else if (valueProgress == 224) {//100%
                return "65 %";
            } else if (valueProgress == 223) {//100%
                return "65 %";
            } else if (valueProgress == 222) {
                return "66 %";
            } else if (valueProgress == 221) {
                return "66 %";
            } else if (valueProgress == 220) {//100%
                return "66 %";
            } else if (valueProgress == 219) {//100%
                return "67 %";
            } else if (valueProgress == 218) {//100%
                return "67 %";
            } else if (valueProgress == 217) {//100%
                return "67 %";
            } else if (valueProgress == 216) {//100%
                return "68 %";
            } else if (valueProgress == 215) {//100%
                return "68 %";
            } else if (valueProgress == 214) {//100%
                return "68 %";
            } else if (valueProgress == 213) {//100%
                return "69 %";
            } else if (valueProgress == 212) {//100%
                return "69 %";
            } else if (valueProgress == 211) {//100%
                return "69 %";
            } else if (valueProgress == 210) {//70%//////////////////////////////
                return "70 %";
            } else if (valueProgress == 209) {//100%
                return "70 %";
            } else if (valueProgress == 208) {//100%
                return "70 %";
            } else if (valueProgress == 207) {//100%
                return "71 %";
            } else if (valueProgress == 206) {//100%
                return "71 %";
            } else if (valueProgress == 205) {//100%
                return "72 %";
            } else if (valueProgress == 204) {//100%
                return "72 %";
            } else if (valueProgress == 203) {//100%
                return "72 %";
            } else if (valueProgress == 202) {//100%
                return "73 %";
            } else if (valueProgress == 201) {//100%
                return "73 %";
            } else if (valueProgress == 200) {//100%
                return "74 %";
            } else if (valueProgress == 199) {//100%
                return "74 %";
            } else if (valueProgress == 198) {//75%//////////////////////////
                return "75 %";
            } else if (valueProgress == 197) {//100%
                return "75 %";
            } else if (valueProgress == 196) {//100%
                return "75 %";
            } else if (valueProgress == 195) {//100%
                return "76 %";
            } else if (valueProgress == 194) {//100%
                return "76 %";
            } else if (valueProgress == 193) {//100%
                return "77 %";
            } else if (valueProgress == 192) {//100%
                return "77 %";
            } else if (valueProgress == 191) {//100%
                return "77 %";
            } else if (valueProgress == 190) {//100%
                return "78 %";
            } else if (valueProgress == 189) {//100%
                return "78 %";
            } else if (valueProgress == 188) {//100%
                return "78 %";
            } else if (valueProgress == 187) {//100%
                return "79 %";
            } else if (valueProgress == 186) {//100%
                return "79 %";
            } else if (valueProgress == 185) {//100%
                return "79 %";
            } else if (valueProgress == 184) {//80%////////////////////////////////////////////////////////////////////////////////////////////////////////
                return "80 %";
            } else if (valueProgress == 183) {//100%
                return "80 %";
            } else if (valueProgress == 182) {//100%
                return "80 %";
            } else if (valueProgress == 181) {//100%
                return "81 %";
            } else if (valueProgress == 180) {//100%
                return "81 %";
            } else if (valueProgress == 179) {//100%
                return "81 %";
            } else if (valueProgress == 178) {//100%
                return "81 %";
            } else if (valueProgress == 177) {//100%
                return "82 %";
            } else if (valueProgress == 176) {//100%
                return "82 %";
            } else if (valueProgress == 175) {//100%
                return "82 %";
            } else if (valueProgress == 174) {//100%
                return "83 %";
            } else if (valueProgress == 173) {//100%
                return "83 %";
            } else if (valueProgress == 172) {//100%
                return "83 %";
            } else if (valueProgress == 171) {//100%
                return "84 %";
            } else if (valueProgress == 170) {//100%
                return "84 %";
            } else if (valueProgress == 169) {//100%
                return "84 %";
            } else if (valueProgress == 168) {//100%///////////////////////////////////////////////////////////////////////////////
                return "85 %";
            } else if (valueProgress == 167) {
                return "85 %";
            } else if (valueProgress == 166) {//100%
                return "85 %";
            } else if (valueProgress == 165) {//100%
                return "85 %";
            } else if (valueProgress == 164) {//100%
                return "86 %";
            } else if (valueProgress == 163) {//100%
                return "86 %";
            } else if (valueProgress == 162) {//100%
                return "86 %";
            } else if (valueProgress == 161) {//100%
                return "86 %";
            } else if (valueProgress == 160) {//100%
                return "87 %";
            } else if (valueProgress == 159) {//100%
                return "87 %";
            } else if (valueProgress == 158) {//100%
                return "87 %";
            } else if (valueProgress == 157) {//100%
                return "87 %";
            } else if (valueProgress == 156) {//100%
                return "88 %";
            } else if (valueProgress == 155) {//100%
                return "88 %";
            } else if (valueProgress == 154) {//100%
                return "88 %";
            } else if (valueProgress == 153) {//100%
                return "88 %";
            } else if (valueProgress == 152) {//100%
                return "89 %";
            } else if (valueProgress == 151) {//100%
                return "89 %";
            } else if (valueProgress == 150) {//100%
                return "89 %";
            } else if (valueProgress == 149) {//100%
                return "89 %";
            } else if (valueProgress == 148) {//100%  punto exacto//////////////////////////////////////////
                return "90 %";
            } else if (valueProgress == 147) {//100%
                return "90 %";
            } else if (valueProgress == 146) {//100%
                return "90 %";
            } else if (valueProgress == 145) {//100%
                return "90 %";
            } else if (valueProgress == 144) {//100%
                return "90 %";
            } else if (valueProgress == 143) {//100%
                return "90 %";
            } else if (valueProgress == 142) {//100%
                return "91 %";
            } else if (valueProgress == 141) {//100%
                return "91 %";
            } else if (valueProgress == 140) {//100%
                return "91 %";
            } else if (valueProgress == 139) {//100%
                return "91 %";
            } else if (valueProgress == 138) {//100%
                return "91 %";
            } else if (valueProgress == 137) {//100%
                return "91 %";
            } else if (valueProgress == 136) {//100%
                return "92 %";
            } else if (valueProgress == 135) {//100%
                return "92 %";
            } else if (valueProgress == 134) {//100%
                return "92 %";
            } else if (valueProgress == 133) {//100%
                return "92 %";
            } else if (valueProgress == 132) {//100%
                return "92 %";
            } else if (valueProgress == 131) {//100%
                return "92 %";
            } else if (valueProgress == 130) {//100%
                return "92 %";
            } else if (valueProgress == 129) {//100%
                return "93 %";
            } else if (valueProgress == 128) {//100%
                return "93 %";
            } else if (valueProgress == 127) {//100%
                return "93 %";
            } else if (valueProgress == 126) {//100%
                return "93 %";
            } else if (valueProgress == 125) {//100%
                return "93 %";
            } else if (valueProgress == 124) {//100%
                return "93 %";
            } else if (valueProgress == 123) {//100%
                return "93 %";
            } else if (valueProgress == 122) {//100%
                return "93 %";
            } else if (valueProgress == 121) {//100%
                return "93 %";
            } else if (valueProgress == 120) {//100%
                return "94 %";
            } else if (valueProgress == 119) {//100%
                return "94 %";
            } else if (valueProgress == 118) {//100%
                return "94 %";
            } else if (valueProgress == 117) {//100%
                return "94 %";
            } else if (valueProgress == 116) {//100%
                return "94 %";
            } else if (valueProgress == 115) {//100%
                return "94 %";
            } else if (valueProgress == 114) {//100%
                return "94 %";
            } else if (valueProgress == 113) {//100%
                return "94 %";
            } else if (valueProgress == 112) {//95% valor exacto
                return "95 %";
            } else if (valueProgress == 111) {//100%
                return "95 %";
            } else if (valueProgress == 110) {//100%
                return "96 %";
            } else if (valueProgress == 109) {//100%
                return "96 %";
            } else if (valueProgress == 108) {//100%
                return "97 %";
            } else if (valueProgress == 107) {//100%
                return "97 %";
            } else if (valueProgress == 106) {//100%
                return "97 %";
            } else if (valueProgress == 105) {//100%
                return "98 %";
            } else if (valueProgress == 104) {//100%
                return "98 %";
            } else if (valueProgress == 103) {//100%
                return "98 %";
            } else if (valueProgress == 102) {//100%
                return "99 %";
            } else if (valueProgress == 101) {//100%
                return "99 %";
            } else if (valueProgress == 100) {//100%
                return "100 %";
            } else if (valueProgress == 99) {//100%
                return "100 %";
            } else if (valueProgress == 98) {//100%
                return "100 %";
            } else if (valueProgress == 97) {//100%
                return "100 %";
            }


        }
  
}


String getValueCylinder(int valueProgress){

     if (valueProgress <= 354 && valueProgress <= 355) {
            return "0 ";
        }else if (valueProgress >= 356 && valueProgress <= 357){
            return "5 ";
        }else if (valueProgress >= 358 && valueProgress <= 359){
            return "10 ";
        }else if (valueProgress >= 360 && valueProgress <= 361){
            return "20 ";
        }else if(valueProgress >= 362 && valueProgress <= 364){
            return "30 ";
        }else if(valueProgress >= 365 && valueProgress <= 368){
            return "40 ";
        }else if(valueProgress >= 369 && valueProgress <= 371){
            return "50 ";
        }else if(valueProgress >= 372 && valueProgress <= 373){
            return "60 ";
        }else if(valueProgress >= 374 && valueProgress <= 375){
            return "70 ";
        }else if(valueProgress >= 376 && valueProgress <= 378){
            return "80 ";
        }else if(valueProgress >= 379 && valueProgress <= 381){
            return "90 ";
        }else if(valueProgress >= 382 && valueProgress <= 384){
            return "100 ";
        }else if(valueProgress >386){
            return "*SP* ";
        }else{
            return "*SP*";
        }
  
}


  String getTempViaServer(){
   
    String val = Serial.readString(); 
    String part01 = getValue(val,':',0);
    
    clientTemp.setInsecure(); //the magic line, use with caution
    clientTemp.connect(urlRequest, 443);
    httpClientTemp.begin(clientTemp, urlRequest);

    
    httpClientTemp.addHeader("Content-Type", "application/json");
     int httpResponseCode = httpClientTemp.POST("{\"device_uuid\":\""+device_uuid+"\",\"contentDevice\":\""+part01+"\"}");
    String payload = "0";
     //httpClientTemp.writeToStream(&Serial);
    if  (httpResponseCode > 0){
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, httpClientTemp.getStream());
      payload = String(doc["temp"].as<double>(), 2);

      Serial.print("ResponseCode: ");
      Serial.println(httpResponseCode);
      Serial.println("GG: "+payload);
     }
    httpClientTemp.end();        
    delay(2000);
    return payload;
  }

/**    
 *Mostrar     
 */
void  displayTemp () {
   
   //val = analogRead (magPin);      // salida del sensor al pin A0 analógico arduino
   //int magValue = val;//analogRead(magPin);
  
    
   // int valueProgress = 576; //Obtenerlo de manera automatica
    display.clearDisplay ();
    display.setTextColor (WHITE);
    display.setTextSize ( 2 );
    display.setCursor (  0, 0 );
    display.print(getTempViaServer()); //Llamar al api del clima getTempViaServer()
    display.setTextSize ( 2 );
  
    display.setCursor (  62, 0 );
    display.print ("C");


    //show wifi
    
  if (WiFi.status() == WL_CONNECTED) {
    long rssi = WiFi.RSSI();
    int bars = getBarsSignal(rssi);
    for (int b=0; b <= bars; b++) {
        display.fillRect(80 + (b*3),15 - (b*3),2,b*3,WHITE); 
      }
  }
    

    display.setTextSize ( 2 );
    display.setCursor (  0, 18 );
    display.print ( "Nivel" );
    display.setTextSize ( 3 );
    display.setCursor (  0, 35 );
    if (Serial.available() > 0) {
       String val = Serial.readString();
       String part01 = getValue(val,':',0); 
       //display.print(part01);
       Serial.print("Mag value: ");
       Serial.println(part01);
       Serial.println("");
   
       display.print(getValueCylinder(part01.toInt()));
   }
    display.setTextSize ( 2 );
    display.setCursor (  90, 35 );
    
    display.print("%");
    display.setTextSize ( 1 );
    display.setCursor (  80, 55 );
    display.print("Aprox");
    
   drawPercentBateryBar(106,4,20,10,val);
   drawPointBatery();
  
  
   display.display();
 
   delay ( 1000 );
  
}

int getBarsSignal(long rssi){
  // 5. High quality: 90% ~= -55db
  // 4. Good quality: 75% ~= -65db
  // 3. Medium quality: 50% ~= -75db
  // 2. Low quality: 30% ~= -85db
  // 1. Unusable quality: 8% ~= -96db
  // 0. No signal
  int bars;
  
  if (rssi > -55) { 
    bars = 5;
  } else if (rssi < -55 & rssi > -65) {
    bars = 4;
  } else if (rssi < -65 & rssi > -75) {
    bars = 3;
  } else if (rssi < -75 & rssi > -85) {
    bars = 2;
  } else if (rssi < -85 & rssi > -96) {
    bars = 1;
  } else {
    bars = 0;
  }
  return bars;
}


/**
 * Mostrar barra de porcentaje de bateria
 */
void drawPercentBateryBar(int x,int y, int width,int height, int progress)
{

   progress = progress > 20 ? 20 : progress;
   progress = progress < 0 ? 0 :progress;

   float bar = ((float)(width-1) / 20) * progress;
 
   display.drawRect(x, y, width, height, WHITE);
   display.fillRect(x+2, y+2, bar , height-4, WHITE);


  // Display progress text
  if( height >= 15){
      display.setCursor((width/2) -3, y+5 );
      display.setTextSize(1);
      display.setTextColor(WHITE);
     if( progress >=50){
       display.setTextColor(BLACK, WHITE); // 'inverted' text
       display.print(progress);
       display.print("%");
    }
  }
}

void drawPointBatery(){
   display.drawPixel(126,6, SSD1306_WHITE); 
   display.drawPixel(126,7, SSD1306_WHITE);
   display.drawPixel(126,8, SSD1306_WHITE);
   display.drawPixel(126,9, SSD1306_WHITE);
   display.drawPixel(126,10, SSD1306_WHITE);  

   display.drawPixel(127,6, SSD1306_WHITE);
   display.drawPixel(127,7, SSD1306_WHITE);
   display.drawPixel(127,8, SSD1306_WHITE);
   display.drawPixel(127,9, SSD1306_WHITE);
   display.drawPixel(127,10, SSD1306_WHITE);
}





  /**
   * Método para enecender y apagar la pantalla del dispositivo
   * y mostrar la información recolectada
   * 
   */
  void powerOnDisplay(){

    if(auxDisplay){//encender pantalla y mostrar datos
      display.ssd1306_command(SSD1306_DISPLAYON);
      auxDisplay = false;
      //digitalWrite(actionActivateDevice, HIGH);
      displayTemp();
      //delay(15000);
      delay(7000);
      powerOnDisplay();
     }else{//Apagar
      display.ssd1306_command(SSD1306_DISPLAYOFF);
     }
    
  }


  void WiFiConnection(){
   delay(3000);
    String string_Ssid="";
    String string_Password="";
    string_Ssid= read_string(30,0); 
    string_Password= read_string(30,100); 
    Serial.println("ssid: "+string_Ssid);
    Serial.println("Password:"+string_Password);
    string_Password.toCharArray(passwordc,30);
    string_Ssid.toCharArray(ssidc,30); 
    //Reading IP information out of memmory
    int ip_Info[5];//holds all converted IP integer information
    for (int n = 200; n < 216; ++n)
    {
     if(char(EEPROM.read(n))!='.'&& char(EEPROM.read(n))!=';'){//
      string_IP += char(EEPROM.read(n));
     }else if(char(EEPROM.read(n))=='.'){//if(char(EEPROM.read(n))=='.')
        ip_Info[ip_group]=ip_convert(string_IP);
        ip_group++;
     }else if(char(EEPROM.read(n))==';'){
        ip_Info[ip_group]=ip_convert(string_IP);
        ip_group++;
        n=217;
     }
  }

    string_IP="";
    string_IP=read_string(4,220);
    Serial.println("GW: "+String(string_IP));
    ip_Info[ip_group]=ip_convert(string_IP);
    Serial.println("IP:"+String(ip_Info[0])+"."+String(ip_Info[1])+"."+String(ip_Info[2])+"."+String(ip_Info[3]));

     //configuring conection parameters, and connecting to the WiFi router
    IPAddress  IOT_ip(ip_Info[0], ip_Info[1], ip_Info[2], ip_Info[3]); // defining ip address
    IPAddress IOT_gateway(ip_Info[0], ip_Info[1],ip_Info[2],ip_Info[4]); // set gateway to match your network
   //IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your network
    WiFi.mode(WIFI_STA);
   WiFi.begin(ssidc, passwordc);
   delay(1000);
    
   while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("Wifi Status:");
    Serial.println(WiFi.status());
    resetOption();
    
  }
  Serial.println("Connected");
     
}

/*The WAP() function configures and calls all functions and
 * variables needed for creating a WiFi Access Poit
 *
 */
void WAP(){
  if  (WiFi.status() != WL_CONNECTED){
  
 
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.println();
  Serial.println("Direccion IP Access Point - por defecto");
  Serial.println(WiFi.softAPIP());
  Serial.println("Direccion MAC Access Point: ");
  Serial.println(WiFi.softAPmacAddress());

 
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    //response->addHeader("Server","ESP Async Web Server");
    
    response->print("<!DOCTYPE html><html lang='es'><head><meta charset='utf-8' /><meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'/><style>");
 
    response->print(":root{--blue:#007bff;--indigo:#6610f2;--purple:#6f42c1;--pink:#e83e8c;--red:#dc3545;--orange:#fd7e14;--yellow:#ffc107;--green:#28a745;--teal:#20c997;--cyan:#17a2b8;--white:#fff;--gray:#6c757d;--gray-dark:#343a40;--primary:#007bff;--secondary:#6c757d;--success:#28a745;--info:#17a2b8;--warning:#ffc107;--danger:#dc3545;--light:#f8f9fa;--dark:#343a40;--breakpoint-xs:0;--breakpoint-sm:576px;--breakpoint-md:768px;--breakpoint-lg:992px;--breakpoint-xl:1200px;--font-family-sans-serif:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans',sans-serif,'Apple Color Emoji','Segoe UI Emoji','Segoe UI Symbol','Noto Color Emoji';--font-family-monospace:SFMono-Regular,Menlo,Monaco,Consolas,'Liberation Mono','Courier New',monospace}*,*::before,*::after{box-sizing:border-box}html{font-family:sans-serif;line-height:1.15;-webkit-text-size-adjust:100%;-webkit-tap-highlight-color:rgba(0,0,0,0)}article,aside,figcaption,figure,footer,header,hgroup,main,nav,section{display:block}body{margin:0;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans',sans-serif,'Apple Color Emoji','Segoe UI Emoji','Segoe UI Symbol','Noto Color Emoji';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;text-align:left;background-color:#fff}[tabindex='-1']:focus:not(:focus-visible){outline:0!important}label{display:inline-block;margin-bottom:.5rem}button{border-radius:0}button:focus:not(:focus-visible){outline:0}input,button,select,optgroup,textarea{margin:0;font-family:inherit;font-size:inherit;line-height:inherit}button,input{overflow:visible}button,select{text-transform:none}[role='button']{cursor:pointer}select{word-wrap:normal}button,[type='button'],[type='reset'],[type='submit']{-webkit-appearance:button}.container{width:100%;padding-right:15px;padding-left:15px;margin-right:auto;margin-left:auto}.row{display:-ms-flexbox;display:flex;-ms-flex-wrap:wrap;flex-wrap:wrap;margin-right:-15px;margin-left:-15px}.no-gutters{margin-right:0;margin-left:0}.no-gutters>.col,.no-gutters>[class*='col-']{padding-right:0;padding-left:0}.col-12,.col{position:relative;width:100%;padding-right:15px;padding-left:15px}.col{-ms-flex-preferred-size:0;flex-basis:0%;-ms-flex-positive:1;flex-grow:1;max-width:100%}.col-12{-ms-flex:0 0 100%;flex:0 0 100%;max-width:100%}.form-control{display:block;width:100%;height:calc(1.5em + 0.75rem + 2px);padding:.375rem .75rem;font-size:1rem;font-weight:400;line-height:1.5;color:#495057;background-color:#fff;background-clip:padding-box;border:1px solid #ced4da;border-radius:.25rem;transition:border-color 0.15s ease-in-out,box-shadow 0.15s ease-in-out}.form-control::-ms-expand{background-color:transparent;border:0}.form-control:-moz-focusring{color:transparent;text-shadow:0 0 0 #495057}.form-control:focus{color:#495057;background-color:#fff;border-color:#80bdff;outline:0;box-shadow:0 0 0 .2rem rgba(0,123,255,.25)}.form-control::-webkit-input-placeholder{color:#6c757d;opacity:1}.form-control::-moz-placeholder{color:#6c757d;opacity:1}.form-control:-ms-input-placeholder{color:#6c757d;opacity:1}.form-control::-ms-input-placeholder{color:#6c757d;opacity:1}.form-control::placeholder{color:#6c757d;opacity:1}select.form-control:focus::-ms-value{color:#495057;background-color:#fff}select.form-control[size],select.form-control[multiple]{height:auto}.form-group{margin-bottom:1rem}.btn{display:inline-block;font-weight:400;color:#212529;text-align:center;vertical-align:middle;-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none;background-color:transparent;border:1px solid transparent;padding:.375rem .75rem;font-size:1rem;line-height:1.5;border-radius:.25rem;transition:color 0.15s ease-in-out,background-color 0.15s ease-in-out,border-color 0.15s ease-in-out,box-shadow 0.15s ease-in-out}.btn:hover{color:#212529;text-decoration:none}.btn:focus,.btn.focus{outline:0;box-shadow:0 0 0 .2rem rgba(0,123,255,.25)}.btn-success{color:#fff;background-color:#28a745;border-color:#28a745}.btn-success:hover{color:#fff;background-color:#218838;border-color:#1e7e34}.btn-success:focus,.btn-success.focus{color:#fff;background-color:#218838;border-color:#1e7e34;box-shadow:0 0 0 .2rem rgba(72,180,97,.5)}.btn-block{display:block;width:100%}.btn-block+.btn-block{margin-top:.5rem}.fade{transition:opacity 0.15s linear}.navbar{position:relative;display:-ms-flexbox;display:flex;-ms-flex-wrap:wrap;flex-wrap:wrap;-ms-flex-align:center;align-items:center;-ms-flex-pack:justify;justify-content:space-between;padding:.5rem 1rem}.navbar .container{display:-ms-flexbox;display:flex;-ms-flex-wrap:wrap;flex-wrap:wrap;-ms-flex-align:center;align-items:center;-ms-flex-pack:justify;justify-content:space-between}.navbar-brand{display:inline-block;padding-top:.3125rem;padding-bottom:.3125rem;margin-right:1rem;font-size:1.25rem;line-height:inherit;white-space:nowrap}.navbar-brand:hover,.navbar-brand:focus{text-decoration:none}.navbar-text{display:inline-block;padding-top:.5rem;padding-bottom:.5rem}.navbar-dark .navbar-brand{color:#fff}.navbar-dark .navbar-brand:hover,.navbar-dark .navbar-brand:focus{color:#fff}.card{position:relative;display:-ms-flexbox;display:flex;-ms-flex-direction:column;flex-direction:column;min-width:0;word-wrap:break-word;background-color:#fff;background-clip:border-box;border:1px solid rgba(0,0,0,.125);border-radius:.25rem}.card-body{-ms-flex:1 1 auto;flex:1 1 auto;min-height:1px;padding:1.25rem}.bg-dark{background-color:#343a40!important}.mb-0,.my-0{margin-bottom:0!important}.mt-4,.my-4{margin-top:1.5rem!important}");
    response->print("</style><title>MEGAHX</title></head><body onload='loadData();'><nav class='navbar navbar-dark bg-dark'><span class='navbar-brand mb-0 h1'>MEGAHX</span></nav><div class='container mt-4'><div class='row'><div class='col-md-12'><div class='card'><div class='card-body'><form action='/save' id='formActions' method='POST'><div class='form-group'><label for='ssid'>SSID</label><select required name='ssid' id='ssid' class='form-control'><option disabled>Selecciona una red</option></select></div><div class='form-group'><label for='password'>Contraseña</label><input required type='password' name='password' id='password' class='form-control'/></div><div class='form-group'><button onclick='loadData();' type='button' class='btn btn-block btn-success'>Actualizar redes</button></div><div class='form-group'><button type='submit' class='btn btn-block btn-success'>Conectar</button></div></form></div></div></div></div></div><script>let ssid = document.getElementById('ssid'); let password = document.getElementById('password'); let formActions = document.getElementById('formActions'); function loadData() { var options = document.querySelectorAll('#ssid option'); options.forEach(o => o.remove()); fetch('/scan').then((res) => res.json()).then((res) => { if (res.length <= 0) { loadData(); } for (let i = 0; i < res.length; i++) { var option = document.createElement('option'); option.innerHTML = res[i].ssid + ' - ' + Math.abs(res[i].rssi) + '%';option.value = res[i].ssid;ssid.appendChild(option, ssid.lastChild);}}).catch(function (err) { console.error(err);});}</script></body></html>");
    Serial.println("Html request");
    request->send(response);
  });

  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
  String json = "[";
  int n = WiFi.scanComplete();
  if(n == -2){
    WiFi.scanNetworks(true);
  } else if(n){
    for (int i = 0; i < n; ++i){
      if(i) json += ",";
      json += "{";
      json += "\"rssi\":"+String(WiFi.RSSI(i));
      json += ",\"ssid\":\""+WiFi.SSID(i)+"\"";
      json += ",\"bssid\":\""+WiFi.BSSIDstr(i)+"\"";
      json += ",\"channel\":"+String(WiFi.channel(i));
      json += ",\"secure\":"+String(WiFi.encryptionType(i));
      json += ",\"hidden\":"+String(WiFi.isHidden(i)?"true":"false");
      json += "}";
    }
    WiFi.scanDelete();
    if(WiFi.scanComplete() == -2){
      WiFi.scanNetworks(true);
    }
  }
  json += "]";
  Serial.println("Network request");
  request->send(200, "application/json", json);
  json = String();
});


server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
  int params = request->params();
 
  Serial.println(request->arg("ssid"));
  Serial.println(request->arg("password"));

  
  write_to_Memory(String(request->arg("ssid")), String(request->arg("password")), "192.168.1.100", "192.168.1.254");
  WiFi.persistent(true);
  delay(1500);
  
  request->send(200, "application/json", "¡Información guardada con exito!");
  delay(1500);
  ESP.reset();
});
 
  
 
  server.begin();

    
  }
}


String getValue(String data, char separator, int index){
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}


//Checks for correct construction of the SSID  
int ssid_error_Check(String content){
 int error=0;
 if(content.length()<2 || content.length()>30){
  ssid_Arg="Your SSID can't be smaller than 2 characters, and not bigger then 30";
  error=1; 
 }else if(content.indexOf(';')!=-1){
  ssid_Arg="The SSID can't contain ;";
  error=1;
 }else{
  ssid_Arg="";
  error=0;
 }
 return error;
}





//Checks for the correct construction of the password
int password_error_Check(String content){
  int error=0;
 if(content.length()<8 || content.length()>30){
  password_Arg="Your password can't be smaller than 8 characters, and not bigger then 30";
  error=1; 
 }else if(content.indexOf(';')!=-1){
  password_Arg="The password can't contain ;";
  error=1;
 }else if(content.indexOf('"')!=-1){
  password_Arg="The password can't contain \"";
  error=1;
 }else if(content.indexOf(';')!=-1){
  password_Arg="The password can't contain \'";
  error=1;
 }else{
  password_Arg="";
  error=0;
 }
 return error;
}


//Checks for the correct construction of the ip address  
int ip_error_Check(String content){
 int error=0;
 if(content.length()<7 || content.length()>15){
  ip_Arg="Your IP can't be smaller than 7 characters, and not bigger then 15";
  error=1; 
 }else {
   ip_Arg="";
    error=0;
 }
 if(error==0){
 int ip_Info=0;//holds all converted IP integer information
  content+=";";
  for (int n = 0; n < content.length(); ++n)
  {
    if(content[n] != '.' && content[n] != ';') { //
      if(isAlpha(content[n])){
        ip_Arg="Missformed IP. It can only contain  4 sets of numbers between 0 and 254 seperated by '.' 1.0";
        error=1;
        return error;
        break;
      }
      string_IP += content[n];
    } else if (content[n] == '.') { 
      ip_Info = ip_convert(string_IP);
    } else if (content[n] == ';') {
      ip_Info = ip_convert(string_IP);
      n = content.length();
    }
    if(ip_Info>254){
      Serial.println("ip_Info: "+String(ip_Info));
      ip_Arg="Missformed IP. It can only contain  4 sets of numbers between 0 and 254 seperated by '.'1.1";
      error=1;
      n = content.length();
    }
  }
 }
 return error;
}
//Checks for the correct construction of the gateway address
int gw_error_Check(String content){
  int error=0;
  
  for(int n=0;n<content.length();n++){
    if(isAlpha(content[n])){
        gw_Arg="Missformed Gateway address. Your Gateway can only contain numbers between 0 and 254 2.0";
        error=1;
        return error;
        break;
      }
  }
  int gwn=ip_convert(content);
  if(content.length()<1 || content.length()>3){
      gw_Arg="Your Gateway can't be smaller than 1 characters, and not bigger then 3";
      error=1; 
 }else if(gwn>254 || gwn<0){
      gw_Arg="Your Gateway can only contain numbers between 0 and 254";
      error=1;
 }else{
    error=0;
    gw_Arg="";
 }
  return error;
}


void erase_memory(){
   for (int i = 0; i < 512; i++) {
    EEPROM.write(i, 0);
  }
   EEPROM.commit();
   WiFi.persistent(false);
   ESP.reset();
}

//Write data to memory
/**
   We prepping the data strings by adding the end of line symbol I decided to use ";".
   Then we pass it off to the write_EEPROM function to actually write it to memmory
*/
void write_to_Memory(String s, String p, String i, String g) {
  s += ";";
  write_EEPROM(s, 0);
  p += ";";
  write_EEPROM(p, 100);
  i += ";";
  write_EEPROM(i, 200);
  g += ";";
  write_EEPROM(g, 220);
  EEPROM.commit();
}
//write to memory
void write_EEPROM(String x, int pos) {
  for (int n = pos; n < x.length() + pos; n++) {
    EEPROM.write(n, x[n - pos]);
  }
}
  
