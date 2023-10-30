#include <AStar32U4.h>

#include <DS3231.h> // for RTC "RealTime Clock" (must add zipped library from Sketch>Include Library)
#include <SPI.h> // for communication with MicroSD card adapter
#include <SD.h> // additional tools for MicroSD read/write

// https://www.gammon.com.au/power
// reference for saving power ^


// pins
const int baudRate = 115200; // baudrate for arduino readings <- how many bits/second
const int pressure_pin = A0; // input from A0 port <- pressure input in bits
const int SDApin = 2; // serial DATA pin 
const int SCLpin = 3; // serial CLOCK pin
const int microSDpin = 4; // microSD pin


// TIME RECORDING INTERVAL SETUP
const unsigned int period = 5000; // [ms], recording period
const unsigned int span = 250; // [ms], time span of measurements
const unsigned int numMeasures = 10; // number of data points before sleeping
const unsigned int sleepTime = 10000; // [ms], gap between recording periods

const unsigned int weightLength = 20; // number of weights used in the weighted average (this happens within ~1ms)
int weightPointer = 0;
float weights[weightLength]; // float array of length weightLength

// TIME LOGGING VARIABLES
String dataString;
unsigned long prevWriteMillis; // [milliseconds] time of previous write
unsigned long totalSec = 0; // [sec] total time device has been on
unsigned long uncoveredSec = 0; // [sec] total time radiello has been uncovered

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
  rtc.setDOW(SUNDAY);
  rtc.setTime(20, 25, 0);
  rtc.setDate(29, 10, 2023);

  // set up sd card
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(microSDpin)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");
}

// start recording
void loop() {
  // sleep between measures
  unsigned long startMain = millis();
  unsigned long endMain = startMain + period;
  unsigned int numMeasured = 0;
  // begin measuring period
  // switch between time based (period) or measurement based (numMeasures)
  while (millis() < endMain){

    // timing
    unsigned long start = millis();
    unsigned long end = start + span;
    // begin gathering data
    while (millis() < end){
      if(weightPointer < weightLength){
        // take pressure readings
        float p = analogRead(pressure_pin); // reads pressure from sensor
        p = p_max * ((p-min_bits)/(max_bits-min_bits)); // convert bits to gauge pressure, assume linear relationship
        p_abs = p_atm + p; // output absolute pressure, assume atm 14.7 psi
        weights[weightPointer] = p_abs;
      }
      weightPointer++; //collect N data points, where N is weightlength
    }
    weightPointer = 0;

    // find the weighted average of the collected data
    float sum = 0;
    for (int i = 0; i < weightLength; i++) {
      sum += weights[i];
    }
    p_abs = (float) sum / weightLength;

    // read out pressure readings, string to input to SD card
    dataString = "";
    dataString += rtc.getDOWStr() + String(",");
    dataString += rtc.getDateStr() + String(",");
    dataString += rtc.getTimeStr() + String(", ");
    dataString += String("Pressure: ");
    dataString += String(p_abs);
    dataString += String(" PSI");

    
    File dataFile;
    dataFile = SD.open("datalog.txt", FILE_WRITE);
    
    if (dataFile){
      Serial.print("Uploading: ");
      Serial.print(dataString);
      Serial.println(" now!");
      dataFile.println(dataString);
      dataFile.close();
      Serial.println("Done!  :)");
    } else{
      Serial.println("Error uploading data!");
    }



    numMeasured++;
  }
  delay(sleepTime);
}