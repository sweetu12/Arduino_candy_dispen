// Auto door open with ultrasonic and servo 
// libraries used :-  servo.h for servo motor (SG90), LiquidCrystal.h for LCD 1602 Display, DHT11.h for Dht11 sensor by Author: Dhruba Saha | Version: 2.1.0 | License: MIT

#include <Servo.h> 
#include <DHT11.h>
#include <LiquidCrystal.h>

// pins

int triger_pin = 12;
int echo_pin = 2;
int servo_pin = 8;
int led_pin  = 4;
int dht_pin = 7;

LiquidCrystal lcd(3, 6, A0, A1, A2, A3); // LCD 1602 Display

Servo door;

DHT11 dht(dht_pin); // Temp and Hum. sensor


// ultrasonic data constants
const unsigned long interval   = 20;   // T  = 0.020 s
const double        t         = 0.10; // t = 0.1 s
const double        omega       = 6.2;  
// gesture capture constants
const unsigned long gesture_time    = 5000;// Time to accept wave
const int           min_dist       = 20;   // Min. wave dist
const int           max_dist        = 35;   // Distance Limit for counter
const int           toggles_req       = 4;    // Number of waves required to open
// other constants
const unsigned long auto_close = 5000; // door close after 5 sec
const int           light_dist      = 40;   // light on <40cm
const unsigned long light_time = 3000; // On for 3 sec after >40cm
const unsigned long dht_refresh    = 1000; // DHT11 Refresh rate

// Varaiables
double distFilt = 0.0;           // filtered cm
int    dist_cm  = 0;             // last raw cm
int    toggles=0;                // Wave counter
int    temp=0;                   // temperature (celcius)
int    hum=0;                    // Humidity (in %)

// timers
unsigned long tPing=0;
unsigned long tGesture=0;
unsigned long tDoorOpen=0;
unsigned long tLight=0;
unsigned long tDht=0;

// gesture track conditionals 
bool tracking=false;
bool inZone=false;
// conditionals for door and light
bool doorOpen=false;
bool lightOn=false;

// functions
void updateDistance();
void detectGesture();
void controlDoor();
void LEDcontrol();
void readDHT();
void printLCD();



void setup()
{
  pinMode(triger_pin, OUTPUT);
  pinMode(echo_pin, INPUT);
  pinMode(led_pin,  OUTPUT);

  door.attach(servo_pin, 500, 2500);  // sevo initialise,  min and max pulse widths for 0 degree and 180 degree
  door.write(0);                      // Servo home position 0 degrees

  lcd.begin(16,2);                    // 16 columns and 2 rows
  lcd.print("Starting");
  Serial.begin(115200);
}



void loop()
{
 // function calling in main loop
 
  updateDistance();
  detectGesture();
  controlDoor();
  LEDcontrol();
  readDHT();
  printLCD();

  Serial.print(distFilt);
  Serial.print(", ");  
  Serial.print(tGesture);  
  Serial.print(", ");  
  Serial.print(tracking);
  Serial.print(", ");
  Serial.println(toggles);
}





// 1) Ultrasonic distance + low-pass filter

void updateDistance()
{
  unsigned long now = millis();
  if (now - tPing < interval) return;
  tPing = now;

  digitalWrite(triger_pin, LOW); 
  delayMicroseconds(2);
  digitalWrite(triger_pin, HIGH); 
  delayMicroseconds(10);
  digitalWrite(triger_pin, LOW);

  long raw_dist = pulseIn(echo_pin, HIGH, 11500);
  
  if (raw_dist == 0) 
    return;                         // ignore 0 sample
  else         
    dist_cm = (int)(raw_dist / 58);       // convert to centimeter

  distFilt = (1.0 - omega * t) * distFilt + (omega * t) * (double)dist_cm; // low pass filter

  
}



// 2) Gesture track

void detectGesture()
{
  if (!tracking && distFilt < min_dist) {
    tracking = true;  
    inZone=true; 
    toggles=0; 
    tGesture=millis();
  }
  if (!tracking) 
    return;

  if (inZone && distFilt > max_dist) { 
    toggles++; 
    inZone=false; 
    }
  if (!inZone && distFilt < min_dist) { 
    toggles++; 
    inZone=true; 
    }

  if (toggles >= toggles_req) {
    door.write(90);  
    doorOpen=true;  
    tDoorOpen=millis();
    tracking=false;
  }
  if (millis() - tGesture > gesture_time) 
    tracking=false;

}



// 3) Auto closing door

void controlDoor()
{
  if (doorOpen && millis()-tDoorOpen >= auto_close) {
    door.write(0); 
    doorOpen=false;
  }
}



// 4) LED control

void LEDcontrol()
{
  if (distFilt <= light_dist) {
    digitalWrite(led_pin, HIGH);
    lightOn=true;  
    tLight=millis();
  } 
   else if (lightOn && millis()-tLight >= light_time) {
    digitalWrite(led_pin, LOW); 
    lightOn=false;
  }
}




// 5) Reading temp sensor

void readDHT()
{
  if (millis() - tDht < dht_refresh) 
    return;
    
  tDht = millis();
  dht.readTemperatureHumidity(temp, hum);
}



// 6) Printing LCD

void printLCD()
{
  lcd.setCursor(0,0);            // column 0, line 0
  lcd.print("T:"); 
  lcd.print(temp); 
  lcd.print("C ");
  lcd.print("H:"); 
  lcd.print(hum); 
  lcd.print("% ");
  lcd.setCursor(0,1);            // column 0, line 1                      
  lcd.print("Dist:"); 
  lcd.print((int)distFilt); 
  lcd.print("cm  ");
}
