#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "DHTesp.h" 
#include <NTPClient.h>
#ifdef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP8266 ONLY!)
#error Select ESP8266 board.
#endif

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

#include "DHT.h"
#define ON_Board_LED 2
#define dht_pin 14 

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

unsigned long currentTime ;
unsigned long sleep_time = 1800e6 ; // here change the 1800e6 represents the amount of seconds the board woudl be in sleep , here its 1800secs

const float CF          = 6.59e-3 ; //This is the correction factor for 
const int analogInPin = A0 ;

float h;          //first  variable send to the google sheets  , you can change this to send anything else
float t;          //second variable send to the google sheets 
float bat = 0  ;  //Third  variabel send to the google sheets 
String sheetHumid = "";
String sheetTemp  = "";
String sheetbatt  = "";
bool flag = 0 ;

const char* ssid = "Note 10s"; //--> Your wifi name or SSID.
const char* password = "abcd1234"; //--> Your wifi password.
//----------------------------------------Host & httpsPort
const char* host = "script.google.com";
const int httpsPort = 443;
//----------------------------------------

WiFiClientSecure client; //--> Create a WiFiClientSecure object.

String GAS_ID = "AKfycbwkAkxcM0x2mY5fZGTYZqjRlnGIMhAG1UOj9BauQ7ZMY5JvTrA0jqt937Fq7-cG2KJ1_g"; //--> when you succesfulyl deploy you would get a link and in that link after script.google.com and beofre execute/ is your gasid



DHTesp dht;


/* Declare LCD object for SPI
  Adafruit_PCD8544(CLK,DIN,D/C,CE,RST);*/
Adafruit_PCD8544 display = Adafruit_PCD8544(D4, D3, D2, D1, D7); /*  equiv to gpios 2,0,4,5,13 */ 
int contrastValue = 60; /* Default Contrast Value */


void setup()
{
  if(((analogRead(analogInPin)  * CF )  ) < 3.3 ){
    ESP.deepSleep(0);
    
    
    }
  Serial.begin(115200);

  dht.setup(dht_pin, DHTesp::DHT11);

  h = dht.getHumidity();                                              // Reading temperature or humidity takes about 250 milliseconds!
  t = dht.getTemperature();
  bat = (analogRead(analogInPin)  * CF ) ;


  const char* ntpServerName = "pool.ntp.org";
  const int ntpServerPort = 123;
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP, ntpServerName, ntpServerPort);


  display.begin();

  /* Change the contrast using the following API*/
  display.setContrast(contrastValue);

  /* Clear the buffer */
  display.clearDisplay();
  display.display();
  delay(100);

  /* Now let us display some text */
  display.setTextColor(WHITE,BLACK);
 
  display.setCursor(0, 0);
  display.println(" PHOTON BOARD");
   display.setTextColor(BLACK);
  display.setCursor(8, 10);
  display.println("UPDATING..");

  display.display();
  delay(20);


  WiFi.begin(ssid, password); //--> Connect to your WiFi router
  Serial.println("");

  pinMode(2 , OUTPUT); //--> On Board LED port Direction output
  digitalWrite(2, HIGH); //--> Turn off Led On Board

  //----------------------------------------Wait for connection
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    //----------------------------------------Make the On Board Flashing LED on the process of connecting to the wifi router.
    digitalWrite(2, HIGH );
    delay(250);
    digitalWrite(2, LOW);
    delay(25);
    //----------------------------------------
  }
  //----------------------------------------
  digitalWrite(2, HIGH); //--> Turn off the On Board LED when it is connected to the wifi router.
  if (Serial.available() == 1) {
    Serial.println("");
    Serial.print("Successfully connected to : ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println();

  }
  //----------------------------------------

  client.setInsecure();
  timeClient.begin();


}

void st_cloud() {

  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);

  //----------------------------------------Connect to Google host
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }
  //----------------------------------------

  //----------------------------------------Processing data and sending data
  String string_temperature =  String(t);
  // String string_temperature =  String(tem, DEC);
  String string_humidity =  String(h);
  String string_batt =  String(bat) + "V";

  // retrying if the data is not posted to the server , maybe it failed on first try at max we try 5 times


  unsigned int counter = 0 ;


  while (flag == 0 ) {

    String url = "/macros/s/" + GAS_ID + "/exec?value1=" + string_temperature + "&value2=" + string_humidity + "&value3=" + string_batt  ;
    Serial.print("requesting URL: ");
    Serial.println(url);



    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: BuildFailureDetectorESP8266\r\n" +
                 "Connection: close\r\n\r\n");

    Serial.println("request sent");
    //----------------------------------------

    //----------------------------------------Checking whether the data was sent successfully or not
    update_display();
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("headers received");
        break;
      }
    }
    String line = client.readStringUntil('\n');
    if (line.startsWith("Content-Type")) {
      Serial.println("esp8266/Arduino CI successfull!");
      flag = 1 ;
    } else {
      Serial.println("esp8266/Arduino CI has failed");
      flag = 0 ;
    }
    Serial.print("reply was : ");
    Serial.println(line);
    Serial.println("closing connection");
    Serial.println("==========");
    Serial.println();


    if (counter == 5 ) {

      break ;
    }
    counter = counter + 1 ;
    if(counter == 5 ){
      break ; 
      }

  }



  timeClient.update();

}


String TIME() {
  unsigned long epochTime = timeClient.getEpochTime() + 19800;

  unsigned long days = epochTime / 86400;
  unsigned long seconds = epochTime % 86400;

  unsigned int hours = seconds / 3600  ; 
  seconds %= 3600;
  unsigned int minutes = seconds / 60  ;
  seconds %= 60;

  unsigned int year = 1970;
  while (days >= 365) {
    if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
      if (days >= 366) {
        days -= 366;
        year++;
      } else {
        break;
      }
    } else {
      days -= 365;
      year++;
    }
  }

  unsigned char monthTable[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  unsigned int month = 0;
  while (days >= monthTable[month]) {
    if (month == 1 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))) {
      if (days >= 29) {
        days -= 29;
        month++;
      } else {
        break;
      }
    } else {
      days -= monthTable[month];
      month++;
    }
  }

  unsigned int day = days + 1;

  char buffer[20];
  sprintf(buffer, "%02u/%02u/%02u %02u:%02u:%02u", day, month + 1, year % 100, hours, minutes, seconds);
  return String(buffer);
}

void update_display() {

  display.clearDisplay();
  display.setCursor(0, 0);

    
  display.print(" PHOTON BOARD");
  

  display.setCursor(0, 7);
  display.print("Temp:");
  display.print(t);
  display.print("C");

  display.setCursor(0, 15);
  display.print("Humi:");
  display.print(h);
  display.print("%");

  display.setCursor(0, 23);
  display.setTextColor(WHITE,BLACK);
  display.print(" Last updated:");
  display.setTextColor(BLACK);
  display.setCursor(0, 31);
  display.print(TIME().substring(0, 8));
  display.print(" VBAT");
  display.setCursor(0, 39);
  display.print(TIME().substring(9, 17));
  display.print(" ");

  display.print(bat,2);
  display.display();




}

void loop() {
  
  delay(100) ;
  
  h = dht.getHumidity();
  t = dht.getTemperature();

  st_cloud();
  update_display();
  ESP.deepSleep(sleep_time);


}
