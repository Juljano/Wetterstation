#include "FirebaseESP8266.h"
#include <ESP8266WiFi.h>
#include <Adafruit_MLX90614.h>
#include  <Adafruit_BMP280.h>
#include "DHT.h" 
#include <ESP8266HTTPClient.h>
#define DHT_TYPE DHT22 

#define FIREBASE_HOST "https://wetterstation-9a964-default-rtdb.europe-west1.firebasedatabase.app/" 
#define FIREBASE_AUTH "AIzaSyCV2oxI6kTA2i34GOublb2xbL1syrrppx0"
#define WIFI_SSID "Fritz Routi" 
#define WIFI_PASSWORD "Elias01.12.2020" 
const char* host = "www.schrotthandel-moeller.de";
const int httpsPort = 443;
int batteryinPercent;
double dewPoint;

FirebaseData fb;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

Adafruit_BMP280 bmp; 

const int DHT_PIN = 14; 
DHT dht(DHT_PIN, DHT_TYPE);

void setup() {

 Serial.begin(115200);
 
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wi-Fi");
  WiFi.mode(WIFI_STA); 
  
 int retries = 0;
while (WiFi.status() != WL_CONNECTED)
{
  Serial.print(".");
  delay(300);
  retries++;
  if (retries > 10) {
    Serial.println("Connection failed");
    ESP.reset();
  }
}

  Serial.println("Verbunden!");

  
 Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  //Enable auto reconnect the WiFi when connection lost
   Firebase.reconnectWiFi(true);


  /* Default settings from datasheet.  */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500);  /* Standby time. */



  dht.begin();
  mlx.begin();
  bmp.begin();


}

void loop() {

  //calculate the dewPoint
  double gamma = log(dht.readHumidity() / 100) + ((17.62 * bmp.readTemperature()) / (243.5 + bmp.readTemperature()));
  dewPoint = 243.5 * gamma / (17.62 - gamma);

  batteryinPercent = batteryCheck(A0); //get Percent from Battery back
  callWebsite();
  updateFirebase();

   
   
}


void updateFirebase() {
  Firebase.pushFloat(fb, "/MLX/Wolkentemperatur",mlx.readObjectTempC());
  Firebase.pushFloat(fb, "/DHT22/Temperature",dht.readTemperature());
  Firebase.pushFloat(fb, "/DHT22/Humidity", dht.readHumidity());
  Firebase.pushFloat(fb, "/BMP280/Pressure", bmp.readPressure()/100000);
  Firebase.pushFloat(fb, "/BMP280/Temperature", bmp.readTemperature());
  Firebase.pushFloat(fb, "/Altitude BMP", bmp.readAltitude(1013.25));
  Firebase.pushFloat(fb, "Others/dew Point", dewPoint);
  Firebase.pushFloat(fb, "Station/Battery" , batteryinPercent);

  startdeepSleep();
}

void startdeepSleep(){


   Serial.println("Tiefschlaf-Modus");
   ESP.deepSleep(1800*1e6); //Für eine halbe Stunde geht der ESP schlafen
  

 
}
