#include <Arduino.h>
#include <analogWrite.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>

const int AIN1 = 27;
const int AIN2 = 33;
const int PWMA = 12;
const int BIN1 = 15;
const int BIN2 = 32;
const int PWMB = 14;

// define RGB pins
const int pin_red = 25;
const int pin_green = 4;
const int pin_blue = 26; 

// define counter
int count = 0;

// define MFRC522 pins
#define RST_PIN         23         // Configurable, see typical pin layout above
#define SS_PIN          22         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  

// define default sentence array
String sentence[5] = {"C3 DF 17 13", "B0 16 10 22", "97 05 B3 1F", "95 D8 B0 1F", "2C EB B2 1F"};

// MQTT server setup
const char* ssid = "USC Guest Wireless";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";

volatile bool start = false;

WiFiClient espClient;
PubSubClient client(espClient);

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
  for (int i = 0; i < length; i++)
    messageTemp += (char)message[i];
  if (messageTemp == "start")
    start = true;
  else {
    for (int i = 0; i < length; i++) {
      switch((char)messageTemp[i]) {
        case 'e' :
          Serial.println("Letter l received.");
          sentence[i] = "C3 DF 17 13";
          break;
        case 'l' :
          Serial.println("Letter o received.");
          sentence[i] = "B0 16 10 22";
          break;
        case 'o' :
          Serial.println("Letter v received.");
          sentence[i] = "97 05 B3 1F";
          break;
        case 'v' :
          Serial.println("Letter e received.");
          sentence[i] = "95 D8 B0 1F";
          break;
        case 'r' :
          Serial.println("Letter r received.");
          sentence[i] = "2C EB B2 1F";
          break;
        default :
          Serial.println("invalid input, defaulting to a");
          sentence[i] = "C3 DF 17 13";
          break;
      }
    }
  }
  Serial.println();
}

void setup() {
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);
  // initialize RGB pins as outputs
  pinMode(pin_blue, OUTPUT);
  pinMode(pin_green, OUTPUT);
  pinMode(pin_red, OUTPUT);

  // intiate serial monitor, SPI, MFRC522
  Serial.begin(9600);
  while (!Serial);
  SPI.begin();			// Init SPI bus
	mfrc522.PCD_Init();		// Init MFRC522
	mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
	Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  
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
  client.loop();
  if (start) {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);

    analogWrite(PWMA, 95, 255);
    analogWrite(PWMB, 95, 255);
    // scan for cards
    if ( ! mfrc522.PICC_IsNewCardPresent())
      return;

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial())
      return;

    // Dump debug info about the card; PICC_HaltA() is automatically called
    mfrc522.PICC_DumpToSerial(&(mfrc522.uid));

    // show UID on serial monitor
    Serial.println("________________");
    Serial.println();
    Serial.print("Scanned track :");
    String content= "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
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
      //delay(000);

      count++;
      Serial.println(count);
      Serial.println(sizeof(sentence)/sizeof(sentence[0]));
    } else {
      Serial.println("incorrect track");
      // make LED red
      analogWrite(pin_red,255);
      analogWrite(pin_blue,0);
      analogWrite(pin_green,0);
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, LOW);
      digitalWrite(BIN1, LOW);
      digitalWrite(BIN2, LOW);

      analogWrite(PWMA, 0, 255);
      analogWrite(PWMB, 0, 255);
      start = false;
      count = 0;
      //delay(2000);
    }

    Serial.println("________________");
    Serial.println();

    // when sentence is complete
    if (count == sizeof(sentence)/sizeof(sentence[0])) { 
      Serial.println("Congratulation! You have correctly arranged all the tiles! ");
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, LOW);
      digitalWrite(BIN1, LOW);
      digitalWrite(BIN2, LOW);

      analogWrite(PWMA, 0, 255);
      analogWrite(PWMB, 0, 255);
      start = 0;
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
  }
} 