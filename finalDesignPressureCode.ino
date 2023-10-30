#include <DS3231.h>  // for RTC "RealTime Clock" (must add zipped library from Sketch>Include Library)
#include <SPI.h>     // for communication with MicroSD card adapter
#include <SD.h>      // additional tools for MicroSD read/write


// pins
const int baudRate = 115200; // baudrate for arduino readings
const int pressure_pin = A0; // input from A0 port
const int SDApin = 2;       // serial DATA pin
const int SCLpin = 3;       // serial CLOCK pin

// TIME RECORDING INTERVAL SETUP
const unsigned long period = 1000; // [ms], recording period
const unsigned long span = 1000;  // [ms], time span of measurements
const unsigned long sleepTime = 100; // [ms], gap between spans
// TIME LOGGING VARIABLES
String dataString;
unsigned long prevWriteMillis;   // [milliseconds] time of previous write
unsigned long totalSec = 0;      // [sec] total time device has been on
unsigned long uncoveredSec = 0;  // [sec] total time radiello has been uncovered

// MAKE RTC PIN
DS3231 rtc(SDApin, SCLpin);

// PRESSURE SENSOR SETUP AND CALIBRATION
// bit range, voltage range
// analogRead() takes voltage output from 0 to 5 from pressure sensor and returns bit value from 0-1023
const int num_bits = 1023; //bits
const int num_volt = 5; //volts
// pressure sensor values
const float v_min = 0.5; //volts
const int p_min = 0; //psi
const float v_max = 4.5; //volts
const int p_max = 100; //psi
const float p_atm = 14.7; //psi
// calibrate number of bits at pressure min and max
const float min_bits = num_bits * (v_min/num_volt);
const float max_bits = num_bits * (v_max/num_volt);
// establish variable to change over time
float p_abs = 0;
// initialize connection
void setup() {
  // setup serial connection
  Serial.begin(baudRate); // setup serial

  // initialize rtc object
  rtc.begin();
  // set time and date
  rtc.setDOW(THURSDAY);
  rtc.setTime(15, 35, 0);
  rtc.setDate(5, 10, 2014);

}

// start recording
void loop() {

  // timing
  unsigned long start = millis();
  unsigned long end = start + span;
  //begin measuring period, will do so until 20 mins has elapsed
  while (millis() < end){
    // take pressure readings
    float p = analogRead(pressure_pin); // reads pressure from sensor
    p = p_max * ((p-min_bits)/(max_bits-min_bits)); // convert bits to gauge pressure, assume linear relationship
    p_abs = p_atm + p; // output absolute pressure, assume atm 14.7 psi
    // read out pressure readings
    
    dataString = "";
    dataString += rtc.getDOWStr() + String(",");
    dataString += rtc.getDateStr() + String(",");
    dataString += rtc.getTimeStr() + String(",");
    dataString += String("Pressure,");
    dataString += String(p_abs);
    dataString += String(" PSI");
    Serial.println(dataString);
    // pause between recording
    delay(period);
  }
  // wait till next time interval
  delay(sleepTime);
}