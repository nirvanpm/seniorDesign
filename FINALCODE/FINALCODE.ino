// LIBRARIES
#include <AStar32U4.h> // library for the Pololu microcontroller
#include <DS3231.h> // for RTC "RealTime Clock" (must add zipped library from Sketch>Include Library)
#include <SPI.h> // for communication with MicroSD card adapter
#include <SD.h> // additional tools for MicroSD read/write

// MAKE RTC PIN
DS3231 rtc(SDApin, SCLpin);

// https://www.gammon.com.au/power
// reference for saving power ^

// INPUT PINS
const int baudRate = 115200; // baudrate for arduino readings <- how many bits/second
const int pressure_pin = A0; // input from A0 port <- pressure input in bits
const int temperature_pin = A1; // input from A1 port <- temperature input in bits
const int SDApin = 2; // serial DATA pin 
const int SCLpin = 3; // serial CLOCK pin
const int microSDpin = 4; // microSD pin

// TIME RECORDING INTERVAL SETUP
// intervals
const unsigned int period = 5000; // [ms], recording period
const unsigned int span = 250; // [ms], time span of measurements
const unsigned int numMeasures = 10; // number of data points before sleeping
const unsigned int sleepTime = 10000; // [ms], gap between recording periods
// average obtaining code
const unsigned int weightLength = 20; // number of weights used in the weighted average (this happens within ~1ms)
int weightPointer = 0;
float weights[weightLength]; // float array of length weightLength

// TIME LOGGING VARIABLES
String dataString; // this will be what is printed to SD card, will join time, temp, and pressure data
unsigned long prevWriteMillis; // [milliseconds] time of previous write
unsigned long totalSec = 0; // [sec] total time device has been on
unsigned long uncoveredSec = 0; // [sec] total time radiello has been uncovered


// PRESSURE SENSOR SETUP AND CALIBRATION
// bit and volt range
const int num_bits = 1023;  // bit range
const int num_volt = 5;     // volt range

// pressure sensor linear voltage range
const float pressure_voltage_min = 0.5; //volts
const float pressure_voltage_max = 4.5; //volts

// pressure sensor minimum and maximum psi, atmospheric pressure
const int pressure_min = 0;           //psi
const int pressure_max = 100;         //psi
const float pressure_atm = 14.7;      //psi
float pressure_abs = 0;               //psi, this variable will change over time

// calibrate number of bits at pressure min and max
const float pressure_min_bits = num_bits * (pressure_voltage_min/num_volt);
const float pressure_max_bits = num_bits * (pressure_voltage_max/num_volt);


// SET UP LOOP, CONNECT TO SD CARD, INITIALIZE RTC
void setup() {
  
  // INITIALIZE SERIAL CONNECTION
  Serial.begin(baudRate); // setup serial

  // INITIALIZE REAL TIME CLOCK
  rtc.begin();                // starts rtc
  rtc.setDOW(SUNDAY);         // sets day of week
  rtc.setTime(20, 25, 0);     // sets time of day
  rtc.setDate(29, 10, 2023);  // sets day of year

  // INITIALIZE SD CARD
  Serial.print("Initializing SD card...");  // show on serial monitor the microprocessor is looking for sd card existence
  
  // this loop checks to see if SD card exists, if card not present code won't start
  if (!SD.begin(microSDpin)) {
    Serial.println("Card failed, or not present");
    while (1);
  }
  Serial.println("card initialized.");
}

// START RECORDING DATA
void loop() {
  unsigned long startMain = millis();               // starts counting
  unsigned long endMain = startMain + period;       // establishes how long the recording period is
  unsigned int numMeasured = 0;                     // counts number of discrete measurements
  
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
        pressure = pressure_max * ((pressure-min_bits)/(max_bits-min_bits)); // convert bits to gauge pressure, assume linear relationship
        pressure_abs = pressure_atm + pressure; // output absolute pressure, assume atm 14.7 psi
        weights[weightPointer] = pressure_abs;
      }
      weightPointer++; //collect N data points, where N is weightlength
    }
    weightPointer = 0;

    // find the weighted average of the collected data
    float sum = 0;
    for (int i = 0; i < weightLength; i++) {
      sum += weights[i];
    }
    pressure_abs = (float) sum / weightLength;

    // STRING THAT CONCATENATES TIME AND PRESSURE DATA
    dataString = "";
    dataString += rtc.getDOWStr() + String(",");
    dataString += rtc.getDateStr() + String(",");
    dataString += rtc.getTimeStr() + String(", ");
    dataString += String("Pressure: ");
    dataString += String(pressure_abs);
    dataString += String(" PSI");

    // CONNECT TO SD CARD 
    File dataFile;                                   // creates file datatype
    dataFile = SD.open("datalog.txt", FILE_WRITE);   // opens datafile in arduino

    // UPLOAD TO SD CARD
    if (dataFile){
      Serial.print("Uploading: ");    // let's serial know it is running, can delete later
      Serial.print(dataString);       // shows us what is being uploaded, can delete later
      Serial.println(" now!");        // definitely can be deleted later
      dataFile.println(dataString);   // prints the dataString to the SD card .txt file
      dataFile.close();               // closes the SD card to stop writing on it
      Serial.println("Done!  :)");    // lets user know upload complete
    } else{
      Serial.println("Error uploading data!"); // will print if failed upload
    }

    numMeasured++;
  }
  delay(sleepTime); 
}
