#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(-1);

#define PIN 14
#define BUZZER 15
#define TRIG 13
#define ECHO 12
#define BUTTON 0

Adafruit_NeoPixel ring = Adafruit_NeoPixel(16, PIN, NEO_GRB + NEO_KHZ800);

/* Declare && Initialize global variables*/
float threshold_max = 160.00;
float web_dist = 0;
unsigned long pixels_on = 0;
int value=0;

const char* ssid = "MAKE_JEFF_NOT_LOVE";
ESP8266WebServer server(80);

/* Home page of web server */
void send_dist(){
  String html="<html>";
  html+="<h1>Sensor Status</h1>";
  html+="<b>Distance: ";
  html+=web_dist;
  html+="</b>";
  html+="<meta http-equiv=\"refresh\" content=\"1;url=/\" /></html>";
  server.send(200, "text/html", html);
}

void setup() {
  // Initialize distance sensor stuff
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(BUZZER, LOW);

  // Inititalize buttons
  pinMode(BUTTON, INPUT_PULLUP);

  // Initialize display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.display();

  // Initialize Serial Output
  Serial.begin(9600);

  // Initialize LED ring
  ring.begin();
  ring.setBrightness(32);
  ring.clear(); // clear all pixels
  ring.show();  // show all pixels

  // Setup WiFi
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);
  server.on("/", send_dist);
  server.begin();
}

/* Method takes distance in from distance sensor and returns the distance in centimeters */
float readDistance() {
  digitalWrite(TRIG, LOW); delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  unsigned long timeout = micros() + 26233L;
  while ((digitalRead(ECHO) == LOW) && (micros() < timeout));
  unsigned long start_time = micros();
  timeout = start_time + 26233L;
  while ((digitalRead(ECHO) == HIGH) && (micros() < timeout));
  unsigned long lapse = micros() - start_time;
  return lapse * 0.01716f;
}

/* Method takes distance ad lights up the ring depending on distance of target */
void glow(float distance, float pixel_gap) {
  unsigned long pixels;
  // if all pixels filled -- TOO CLOSE, so flash
  if (distance / pixel_gap < 1) {
    flash();
  }
  else {
    ring.clear();
    pixels = 17 - distance / pixel_gap;
    if (pixels > pixels_on) {
      for (int i = 0; i < pixels; i++) {
        if (i <= ring.numPixels() / 3)
          ring.setPixelColor(i, ring.Color(0, 255, 0));
        else if (i > ring.numPixels() / 3 && i < ring.numPixels() - ring.numPixels() / 3)
          ring.setPixelColor(i, ring.Color(255, 140, 0));
        else
          ring.setPixelColor(i, ring.Color(255, 0, 0));
      }
    }
    else {
      for (int i = pixels_on; i >= pixels; i--) {
        ring.setPixelColor(i, ring.Color(0, 0, 0));
      }
    }

    pixels_on = pixels;
  }

  ring.show();
}

void flash() {
  tone(BUZZER, 880);
  for (int j = 0; j < ring.numPixels(); j++)
    ring.setPixelColor(j, ring.Color(255, 0, 0));

  ring.show();
  delay(100);
  ring.clear();
  delay(100);

  noTone(BUZZER);
  pixels_on = 0;
}
byte prev_state;
unsigned long int lastclick_time=0;
void button(){
  byte curr_state = digitalRead(BUTTON);
  if(curr_state == 0 && prev_state ==1){
    if(millis() > lastclick_time + 100){
      if(threshold_max <= 450){
        threshold_max += 10;  
      }
      if(threshold_max > 450){
        threshold_max = 10;  
      }
      lastclick_time = millis();  
    }  
  }
  prev_state = curr_state;
}

void loop() {
  // Declare local variables
  float dist = readDistance();
  float pixel_gap;

  //button();

  pixel_gap = (float)threshold_max / (float)16;
  glow(dist, pixel_gap);

  /* Display distance away */
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Distance:");
  display.setCursor(32, 18);
  display.print((int)dist);
  display.println(" cm");
  display.setCursor(0, 35);
  display.print("Max:");
  display.setCursor(32, 48);
  display.print((int)threshold_max);
  display.println(" cm");
  display.display();

  /* Update distance in website */
  if(dist < threshold_max)
    web_dist = dist;

  /* Handle client requests */
  server.handleClient();
}
