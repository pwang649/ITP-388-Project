#include <Arduino.h>
#include <analogWrite.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

// define RGB pins
const int pin_red = 4;
const int pin_green = 3;
const int pin_blue = 5; 

// define RGB colors
int red = 0;
int green = 0;
int blue = 0;

// define counter
int count = 0;

// define MFRC522 pins
const int RST_PIN = 9;
const int SS_PIN = 10;

// create MFRC522 instance
MFRC522 mfrc522(SS_PIN, RST_PIN);

// create a MIFARE_Key structure
MFRC522::MIFARE_Key key;          

// setting read/write block number for MFRC522
int block = 2;  

// array used to read blocks
byte readbackblock[18];

// define default sentence array
String sentence[5] = {"C3 DF 17 13", "B0 16 10 22", "97 05 B3 1F", "95 D8 B0 1F", "2C EB B2 1F"};


// MQTT server setup
const char* ssid = "SpectrumSetup-50";
const char* password = "brightrabbit840";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
    switch((char)message[i]) {
      case 'a' :
        Serial.println("Letter a received.");
        sentence[i] = "C3 DF 17 13";
        break;
      case 'b' :
        Serial.println("Letter b received.");
        sentence[i] = "B0 16 10 22";
        break;
      case 'c' :
        Serial.println("Letter c received.");
        sentence[i] = "97 05 B3 1F";
        break;
      case 'd' :
        Serial.println("Letter d received.");
        sentence[i] = "95 D8 B0 1F";
        break;
      case 'e' :
        Serial.println("Letter e received.");
        sentence[i] = "2C EB B2 1F";
        break;
      default :
        Serial.println("invalid input, defaulting to a");
        sentence[i] = "C3 DF 17 13";
        break;
   }
  }
  Serial.println();
}

void setup() {
  // initialize RGB pins as outputs
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);

  // intiate serial monitor, SPI, MFRC522
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println();
  Serial.println("scanning for track");
  Serial.println();

  // setup wifi and mqtt
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  while (!client.connected()) {
     String client_id = "esp32-client-";
     client_id += String(WiFi.macAddress());
     Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
     if (client.connect(client_id.c_str())) {
         Serial.println("Public emqx mqtt broker connected");
     } else {
         Serial.print("failed with state ");
         Serial.print(client.state());
         delay(2000);
     }
  }
  // publish and subscribe
  client.subscribe("ITP388/test");
}

void loop() {
  // set LED to default light 
  analogWrite(pin_red,230);
  analogWrite(pin_blue,250);
  analogWrite(pin_green,250);

  // scan for cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }

  // select card
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }

  // show UID on serial monitor
  Serial.println("________________");
  Serial.println();
  Serial.print("Scanned track :");
  String content= "";
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  // expected track
  Serial.println();
  Serial.println("Expected track :");
  Serial.print(sentence[count]);

  // print results
  Serial.println();
  Serial.print("Message: ");
  content.toUpperCase();

  //checking if track is correct
  if (content.substring(1) == sentence[count]) {
    Serial.println("correct track");
    Serial.println();

    // make LED green
    analogWrite(pin_red,0);
    analogWrite(pin_blue,0);
    analogWrite(pin_green,255);
    delay(3000);

    count++;
    Serial.println(count);
    Serial.println(sizeof(sentence));
  } else {
    Serial.println("incorrect track");
    // make LED red
    analogWrite(pin_red,255);
    analogWrite(pin_blue,0);
    analogWrite(pin_green,0);
    delay(3000);
  }

  Serial.println("________________");
  Serial.println();

  // when sentence is complete
  if (count == sizeof(sentence)/sizeof(sentence[0])) { 
    int timer = 0;
    while (timer < 5) {
      analogWrite(pin_red,0);
      analogWrite(pin_blue,0);
      analogWrite(pin_green,255);
      delay(500);
      
      analogWrite(pin_red,0);
      analogWrite(pin_blue,0);
      analogWrite(pin_green,0);
      delay(500);

      timer++;
    }
    count = 0;
  }
  client.loop();
} 