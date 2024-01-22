/*
Code written for IOT based Smart HOME.

Written by - Mayank Lodhi 

© Copywright - @ml_024

 https://github.com/Eviltr0N/Iot-based-smart-home

*/

#define BLYNK_PRINT Serial // For printing Blynk Cloud State
#include <MFRC522.h> //library responsible for communicating with the module RFID-RC522
#include <SPI.h> //library responsible for communicating of SPI bus
#include <WiFi.h> 
#include <WiFiClient.h>  //library responsible for WiFi Connection
#include <BlynkSimpleEsp32.h> //library for Blynk Cloud Server
#include <ESP32Servo.h> //library responsible for Controlling Servo Motor
#include <HTTPClient.h>

#define SS_PIN    21 //SPI pin defination for RFID Module
#define RST_PIN   22
#define SIZE_BUFFER     18
#define MAX_SIZE_BLOCK  16


#define gas_pin            34 
#define temp_pin           35 
#define main_servo_pin     13  
#define kitchen_servo_pin  4
#define water_pump         26    // pin D6 cant be used as output
#define buzzer_pin         25
#define green_led          32
#define red_led            33
#define gas_threshold      900 //For LPG
#define temp_threshold     1200 // equivalent to 70 degree celcius
#define normal_temp        800 // equivalent to 40 degree celcius

            
char auth[] = "_OQIDJ43SF-goAqGG6TrmPj0-89x70Io"; // Auth token for Blynk local server
BlynkTimer timer;

char ssid[] = "WIFI_NAME"; //WiFi Credentials
char pass[] = "WIFI_PASS";

MFRC522 mfrc522(SS_PIN, RST_PIN); // configuring RFID instance
Servo main_servo; // configuring Main Door Servo
Servo kitchen_servo; // configuring Kitchen Door Servo
int pos = 0; // variable for storing servo position

void setup()
{
Serial.begin(115200);
SPI.begin();
mfrc522.PCD_Init(); 

main_servo.setPeriodHertz(50);
kitchen_servo.setPeriodHertz(50);
main_servo.attach(main_servo_pin, 500, 2400);
kitchen_servo.attach(kitchen_servo_pin, 500, 2400);
main_servo.write(0);
kitchen_servo.write(90);

Blynk.begin(auth, ssid, pass, IPAddress(XX,XX,XX,XX), 8080); // Connecting to Blynk Local Server 

timer.setInterval(1000L , sensor_update); // this will run sensor_update function in every 1 second
timer.setInterval(1000L , alarm);         // this will run alarm function in every 1 second
timer.setInterval(2000L , rfid);    //// this will run rfid function in every 1 second
 
pinMode(gas_pin, INPUT);
pinMode(temp_pin , INPUT);
pinMode(water_pump , OUTPUT);
pinMode(buzzer_pin , OUTPUT);
pinMode(green_led , OUTPUT);
pinMode(red_led , OUTPUT);

 
}

void loop()
{
  Blynk.run();
  timer.run();
}

void sensor_update()
{
  int gas_value = analogRead(gas_pin);
  int temp_value = analogRead(temp_pin);
  Blynk.virtualWrite(V0 , gas_value);
  Blynk.virtualWrite(V1 , temp_value/20);

  Serial.println("Temp: " + String(temp_value) + "    " + "Gas: " + String(gas_value));
}

 
void alarm()
{
  digitalWrite(water_pump, LOW);
  digitalWrite(buzzer_pin, LOW);
  int gas_value = analogRead(gas_pin);
  int temp_value = analogRead(temp_pin);
  while (gas_value > gas_threshold)
    {
      Serial.println("GAS Leaked... Open your Door & window");
      digitalWrite(buzzer_pin , HIGH);
      delay(400);
      digitalWrite(buzzer_pin , LOW);
      gas_value = analogRead(gas_pin);
    }
  if (temp_value > temp_threshold)
    {
      Serial.println("Fire Detected...");
      webhook();
      Serial.println("Locking Kitchen to prevent spreding fire.");
      for (pos = 90; pos >= 0; pos -= 1)
        { 
        kitchen_servo.write(pos);    // tell servo to go to position in variable 'pos'
        delay(5);             // waits 5ms for the servo to reach the position
        }
 
      
      Serial.println("Turning ON water pump...");
          while(temp_value >= normal_temp)
          {
            digitalWrite(water_pump , LOW); //Turning on water pump as pump is connected to Active LOW Relay.
            digitalWrite(buzzer_pin , HIGH);
            temp_value = analogRead(temp_pin);
            delay(500);
          }  
      for (pos = 0; pos <= 90; pos += 1)
        { 
        kitchen_servo.write(pos);    // tell servo to go to position in variable 'pos'
        delay(5);             // waits 5ms for the servo to reach the position
        }
     digitalWrite(water_pump , HIGH);
     digitalWrite(buzzer_pin , LOW);
      
    }

}    

void rfid(){
Serial.println("Reading RFID...");
     //waiting the card approach
if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    
    return;
  }
  // Select a card
if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
  
  //instructs the PICC when in the ACTIVE state to go to a "STOP" state
mfrc522.PICC_HaltA(); 

 Serial.print("UID tag :");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println("");
  content.toUpperCase();
  if (content.substring(1) == "60 26 4E 21") //change here the UID of the card/cards that you want to give access
  {
  Serial.println("Access Granted... Welcome to Smart Home");
    
  for (pos = 0; pos <= 90; pos += 1) 
  { 
    main_servo.write(pos);    // tell servo to go to position in variable 'pos'
    delay(5);             // waits 15ms for the servo to reach the position
  }
  Serial.println("Waiting for 5 sec...");
  delay(5000);
  for (pos = 90; pos >= 0; pos -= 1)
    { 
    main_servo.write(pos);    // tell servo to go to position in variable 'pos'
    delay(5);  
    }
  }
 else
    {
    Serial.println("Access denied...");
    delay(100);
    }
 
}

BLYNK_WRITE(V5)
{   
  int valuee = param.asInt(); // Get value as integer
  main_servo.write(valuee);

}
BLYNK_WRITE(V6)
{   
  int valuee = param.asInt(); // Get value as integer
  kitchen_servo.write(valuee);

}

void webhook(){
  
HTTPClient http;
http.begin("https://maker.ifttt.com/trigger/fire/json/with/key/xxxxxxxxxxx");
Serial.println("[HTTP] GET...\n");
int httpCode = http.GET();
if(httpCode > 0) {
Serial.printf("[HTTP] GET... code: %d\n", httpCode);
if(httpCode == HTTP_CODE_OK) {
 String payload = http.getString();
Serial.println(payload);
}
} else {
Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
}
http.end();
  }
