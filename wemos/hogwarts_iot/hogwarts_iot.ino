#include "WEMOS_Motor.h";
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>

// ^ will need to configure your client to include the above libraries

const char* ssid = "network_SSID";
const char* password = "password";


//config
int hallThreshold = 600;
int homingSpeed = 40;
int waitDelay = 100;
int reverseSpeed = -50;
int reverseDelay = 100;

//state
bool  isReturningHome = false;
int   motorSpeed = 0;

ESP8266WebServer server(80);

//Motor shield I2C Address: 0x30
//PWM frequency: 1000Hz(1kHz)
Motor M1(0x30,_MOTOR_A, 1000);

// pin variables
const int pinHall = A0;
const int headlights = D6; 

//wifi client init
WiFiClient client;

void setup() {
  //only used for debugging
  Serial.begin(250000);

  //pin config
  pinMode(pinHall, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(headlights, OUTPUT);

  digitalWrite(LED_BUILTIN,HIGH);

  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  //routing setup for endpoints
  server.on("/", handleRoot); // what happens in the root
  server.on("/go", HTTP_GET, getGo);
  server.on("/go", HTTP_POST, setGo);
  server.on("/gohome", goHome); 
  server.on("/hall", hall); 

  server.on("/state", HTTP_POST, setState);
  server.on("/state", HTTP_GET, getState);
  server.on("/status", HTTP_GET, boolState);

  server.begin();
  Serial.println("HTTP server started");

  
}

long getHallVal(){
  long measure = analogRead(pinHall);
  return measure;
}

void hall(){
  long mHall = getHallVal();
  server.send(200, "application/json", "{\"hall\": "+String(mHall)+"}");
}

void setMotor(int speed){
  motorSpeed = speed;
  digitalWrite(headlights, HIGH);
  if(speed == 0){
    M1.setmotor(_STOP);
    digitalWrite(headlights, LOW);
    digitalWrite(LED_BUILTIN,HIGH);
  }else if(speed > 0){
    M1.setmotor(_CCW, speed);
  }else{
    M1.setmotor(_CW, abs(speed));
  }
}

void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266! (blinking LED)");
  int led_state = 0;
  for (int ii=0;ii<10;ii++){
    digitalWrite(LED_BUILTIN,led_state);
    digitalWrite(headlights, led_state);
    led_state = !led_state;
    delay(100);
  }
  digitalWrite(headlights, LOW);
}

void setGo(){
  digitalWrite(LED_BUILTIN,LOW);
  isReturningHome = false;
  setMotor(server.arg("speed").toInt());
  Serial.println(server.arg("speed"));
  getGo();
}

void getGo(){
  server.send(200, "application/json", "{\"motorSpeed\": "+String(motorSpeed)+"}");
}

void goHome(){
  server.send(200, "text/plain", "going home");
  digitalWrite(LED_BUILTIN,HIGH);
  digitalWrite(headlights, LOW);
  isReturningHome = true; 
}

void boolState(){
  bool state = motorSpeed > 0;
  server.send(200, "application/json", "{\"on\": "+String(state)+"}");
}

void getState(){
  Serial.println("getting state");
  const size_t capacity = JSON_OBJECT_SIZE(5);
  DynamicJsonDocument doc(capacity);
  Serial.println("setting vars");
  doc["hallThreshold"]  = hallThreshold;
  doc["homingSpeed"]    = homingSpeed;
  doc["waitDelay"]      = waitDelay;
  doc["reverseSpeed"]   = reverseSpeed;
  doc["reverseDelay"]   = reverseDelay;
  
  Serial.println("printing client");
  serializeJson(doc, Serial);
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

//designed for tweaking the detection of the magnet and what to do afterwards.
void setState(){
  if(server.arg("hallThreshold") != NULL){
    hallThreshold = server.arg("hallThreshold").toInt();
  }

  if(server.arg("homingSpeed") != NULL){
    homingSpeed = server.arg("homingSpeed").toInt();
  }

  if(server.arg("waitDelay") != NULL){
    waitDelay = server.arg("waitDelay").toInt();
  }

  if(server.arg("reverseSpeed") != NULL){
    reverseSpeed = server.arg("reverseSpeed").toInt();
  }
  
  if(server.arg("reverseDelay") != NULL){
    reverseDelay = server.arg("reverseDelay").toInt();
  }
  getState();
}

void loop() {
 server.handleClient(); 

 //if told to return home, slow down and look for the magnet
 //TODO: timeout if not discovered otherwise train may run until dead or derail and burn out.
 if(isReturningHome){
    long mHall = getHallVal();
    setMotor(homingSpeed);
    digitalWrite(headlights, LOW);
    if(mHall >= hallThreshold){
      setMotor(0);
      delay(waitDelay);
      //backup
      setMotor(reverseSpeed);
      delay(reverseDelay);
      setMotor(0);
      isReturningHome = false;
    }
  }
}
