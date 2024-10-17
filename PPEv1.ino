#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
#include <avr/power.h>
#endif

#define DHTTYPE DHT11
#define LDR A0
#define PIN_NEO_PIXEL  0   // Arduino pin that connects to NeoPixel
#define NUM_PIXELS     256  // The number of LEDs (pixels) on NeoPixel
#define IFR 5

Adafruit_NeoPixel NeoPixel(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);
const int DHTPin = 4;
DHT dht(DHTPin, DHTTYPE);

const char* ssid = "*********";
const char* password = "**********";

// Change the variable to your Raspberry Pi IP address, so it connects to your MQTT broker
const char* mqtt_server = "X.X.X.X";

// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
WiFiClient espClient;
PubSubClient client(espClient);

int value = 0;
long now = millis();
long lastMeasure = 0;

unsigned long now1 = millis();
unsigned long lastMeasure1 = 0;
boolean mod = false;

unsigned long now2 = millis();
unsigned long lastMeasure2 = 0;
int choix=0;

void allumer(){
   
  NeoPixel.setBrightness(value);
  for(int i=0;i<NUM_PIXELS;i++)
  {
    NeoPixel.setPixelColor(i, NeoPixel.Color(255, 255, 255)); 
  }
  NeoPixel.show();
}

ICACHE_RAM_ATTR void Move() {
  client.publish("TD02_GP01/pres", String("Il y a quelqu'un dans la salle").c_str());
  mod = true;
  lastMeasure1 = millis();
}

// Don't change the function below. This functions connects your ESP8266 to your router
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
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  const char *s=messageTemp.c_str();
  value=atoi(s);
  Serial.println();

  if(topic=="TD02_GP01/led"){
    if(messageTemp=="on")
    {
      choix=1;
    }
    if(messageTemp=="off")
    {
      choix=0;
    }
    //allumer();
  }
  Serial.println();
}

// This functions reconnects your ESP8266 to your MQTT broker
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
 
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");  
      client.subscribe("TD02_GP01/led");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
    pinMode(LDR,INPUT);
    pinMode(IFR,INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(IFR), Move, RISING);
    dht.begin();
    NeoPixel.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

    Serial.begin(9600);
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP8266Client");

  now = millis();
  // Publishes new temperature and humidity every 1 second
  if (now - lastMeasure > 1000) {
    lastMeasure = now;
    
    float HumiditySensorAir = dht.readHumidity();
    float TemperatureSensor = dht.readTemperature();

    // Publishes Temperature and Humidity values
    client.publish("TD02_GP01/temp", String(TemperatureSensor).c_str());
    client.publish("TD02_GP01/relhum", String(HumiditySensorAir).c_str());

    int LumSensor = map(analogRead(LDR),-50,550,0,1024);
    client.publish("TD02_GP01/lux", String(LumSensor).c_str());
    Serial.println(analogRead(LDR));
  }
  now1 = millis();
  
  // Turn off the LED after the number of seconds defined in the timeSeconds variable
  if(mod && (now1 - lastMeasure1 > (10000))) {
    client.publish("TD02_GP01/pres", String("Il y a personne dans la salle").c_str());
    NeoPixel.clear();
    NeoPixel.show();
    mod = false;
  }
  else
  {
    now2 = millis();
    if(now2 - lastMeasure2 > 1000 && mod)
    {
      lastMeasure2 = now2;
      if(choix==1)
      {
        allumer();
      }
      if(choix==0)
      {
         if(((-analogRead(LDR)/1.67)+135)>=0){
        //Serial.println((-analogRead(LDR)/4)+135);
        NeoPixel.setBrightness((-analogRead(LDR)/1.67)+135);
        for(int i=0;i<NUM_PIXELS;i++)
        {
          NeoPixel.setPixelColor(i, NeoPixel.Color(255, 255, 255));
        }
        NeoPixel.show();
        }
        else
        {
          NeoPixel.setBrightness(0);
          for(int i=0;i<NUM_PIXELS;i++)
          {
            NeoPixel.setPixelColor(i, NeoPixel.Color(255, 255, 255));
          }
          NeoPixel.show();
        }
      }
    }
  }
} 
