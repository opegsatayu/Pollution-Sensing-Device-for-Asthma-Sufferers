#include <Arduino.h>
#include <Wire.h>     // allow Arduino to communicate with I2C
#include <SensirionI2CSen5x.h>     // for PM sensor
#include <DFRobot_MICS.h>         // for NO₂ sensor
#include "DFRobot_OzoneSensor.h"  // for Ozone sensor

// Ozone Sensor Setup 
#define OZONE_I2C_ADDRESS 0x73
#define COLLECT_NUMBER    20
DFRobot_OzoneSensor Ozone(&Wire);

//  NO₂ Sensor Setup 
#define CALIBRATION_TIME   0.5    // in minutes
#define Mics_I2C_ADDRESS   0x75
DFRobot_MICS_I2C mics(&Wire, Mics_I2C_ADDRESS);

// #define potPin A5

//  PM Sensor Setup 
SensirionI2CSen5x sen5x;

// Output Pin Definitions 
const int greenLed = 10;
const int redLed   = 11;
const int buzzer   = 9;


// pot

 //const int threshold_pot = 500;
 //const unsigned long threshold_pot_timer = 4000; 



////// Thresholds for each sensor //////
// Ozone (in ppb)
const int threshold_Ozone               =   400;           //200;     // 200 ppb = 0.2 ppm 
const unsigned long threshold_ozone_timer =  3000;              //7200000;  // 2 hours

// NO₂ (in ppm) – using a 1-hour exposure for green warning
const float threshold_no2 = 0.7;
const unsigned long threshold_no2_timer = 14400000;  // 4 hour in ms

// PM2.5 (in µg/m³)
const int threshold_pm25 = 50;
const unsigned long threshold_pm25_timer = 86400000;  // 24 hours in ms

// PM10 (in µg/m³)
const int threshold_pm10 = 150;
const unsigned long threshold_pm10_timer = 86400000;  // 24 hours in ms
////////////////////////////////////////////////////////////////////////////

// ***** Universal Timing for Warning Logic *****
const unsigned long extra_overtime   = 6000; //3600000 ;   // extra time 60 min after green to trigger red warning
const unsigned long time_in_safety   = 2000; //1800000;  // 30 min in safety required to deem safe and clear the warning
const unsigned long danger_again     = 3000; //900000;   // 15 min of re-exposure to re-trigger warning

///////////////////////////////////////////////////////////////////////
//  Ozone warning state variables 
unsigned long total_time_above_ozone = 0;
unsigned long total_time_below_ozone = 0;
unsigned long time_reexpose_ozone = 0;
bool mark_ozone = false;    // becomes true once red warning is triggered
   

//  NO₂ warning state variables 
unsigned long total_time_above_no2 = 0;
unsigned long total_time_below_no2 = 0;
unsigned long time_reexpose_no2 = 0;
bool mark_no2 = false;


//  PM2.5 warning state variables  
unsigned long total_time_above_pm25 = 0;
unsigned long total_time_below_pm25 = 0;
unsigned long time_reexpose_pm25 = 0;
bool mark_pm25 = false;


// PM10 warning state variables  
unsigned long total_time_above_pm10 = 0;
unsigned long total_time_below_pm10 = 0;
unsigned long time_reexpose_pm10 = 0;
bool mark_pm10 = false;

//////////////////////////////////// POT test//////////////
//pot warning state variables 
//  unsigned long total_time_above_pot  = 0;
 // unsigned long total_time_below_pot  = 0;
 // unsigned long time_reexpose_pot     = 0;
 // bool mark_pot = false;

unsigned long previous_time = 0;

bool green_led_on = false;
bool red_led_on = false;
bool safe = false;




void setup() {
  Serial.begin(115200);
  Wire.begin();


  // Ozone sensor initiation 


Serial.println("Ozone connected"); while(!Ozone.begin(OZONE_I2C_ADDRESS)) {
  Serial.println("I2C Ozone error");
  delay(1000);
}
Serial.println("Ozone connected");
  Ozone.setModes(MEASURE_MODE_PASSIVE);
///////////////////////////////////////////////////////////////////////////

// pm initiation 
  sen5x.begin(Wire); //start I2C communication 

  uint16_t error;
  char errorMessage[256];

  error = sen5x.deviceReset(); // reset the sensor everystart to have a clean startup
  if (error) {
    Serial.print("Error trying to execute deviceReset(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  float tempOffset = 0.0;  // for compensating for the heat generated from the sensor itself 
  error = sen5x.setTemperatureOffsetSimple(tempOffset);
  if (error) {
    Serial.print("Error trying to execute setTemperatureOffsetSimple(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.print("Temperature Offset set to ");
    Serial.print(tempOffset);
    Serial.println(" deg. Celsius (SEN54/SEN55 only)");
  }

  error = sen5x.startMeasurement(); //start collecting data 
  if (error) {
    Serial.print("Error trying to execute startMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }
////////////////////////////////////////////////////////////////////

//no2 initiation 

  while (!mics.begin()) {
    Serial.println("I2C no2 error");
    delay(1000);
  }
  Serial.println("no2 connected");

  uint8_t mode = mics.getPowerState();
  if (mode == SLEEP_MODE) {
    mics.wakeUpMode();
    Serial.println("Wake-up sensor success!");
  } else {
    Serial.println("The sensor is already in wake-up mode.");
  }

  // Preheat the sensor
  while (!mics.warmUpTime(CALIBRATION_TIME)) {
    Serial.println("Please wait until the warm-up time is over!");
    delay(1000);
  }
  pinMode(greenLed, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(buzzer, OUTPUT);
}

/////////////////////////////////////////////////////////////
  





void loop() { 
/////////////////// read sensor values  ////////////////////////////////////////////////////
//read pot 
// int potValue = analogRead(potPin);

// read ozone
int16_t ozoneValue = Ozone.readOzoneData(COLLECT_NUMBER);

// no2 read
float no2Value = mics.getGasData(NO2); // in PPM

// pm + temp read 
 uint16_t error;
  char errorMessage[256];

  float massConcentrationPm1p0;
  float pm25Value;
  float massConcentrationPm4p0;
  float pm10Value;
  float ambientHumidity;
  float temperature;
  float vocIndex;
  float noxIndex;

  error = sen5x.readMeasuredValues(
             massConcentrationPm1p0,
             pm25Value,
             massConcentrationPm4p0,
             pm10Value,
             ambientHumidity,
             temperature,
             vocIndex,
             noxIndex
          );
          
////////////////////////// print out reading in 1 string /////////////////
 Serial.print(ozoneValue / 1000.0, 3);   // convert ppb into ppm for ozone
  Serial.print(",");                   
  Serial.print(no2Value, 3);  // no2 in PPM (3 decimal places)
  Serial.print(",");

  if (!error) { // Check if PM sensor read is successful
    Serial.print(pm25Value, 2);  // PM2.5 in ug/m3 (2 decimal places)
    Serial.print(",");
    Serial.print(pm10Value, 2); // PM10 in ug/m3 (2 decimal places)
    Serial.print(",");
    Serial.print(temperature, 2);      // temp in celcius (2 decimal places)
   // Serial.print(",");
    
  } 
  //print pot 
  //Serial.print(potValue);
  Serial.println();  

// make time (universal)
  unsigned long current_time = millis();
  unsigned long time_passed  = current_time - previous_time; 
 previous_time = current_time;


////////// warning logic////////////////////////////

/////////////// ozone /////////////////
// start timer ozone 
  if (ozoneValue >= threshold_Ozone) {
    total_time_above_ozone = total_time_above_ozone + time_passed;
    total_time_below_ozone = 0; //no safe anymore 
  } else{
    total_time_below_ozone = total_time_below_ozone + time_passed;  
  }

// green LED 
  if ((total_time_above_ozone >= threshold_ozone_timer) && mark_ozone == false && safe == false) {  // prevent this loop from conflicting with the safety logic when the reading went down as green led lit 
    digitalWrite(greenLed, HIGH);
    green_led_on = true; 
     
  } 

// red LED 
  if ((total_time_above_ozone >= extra_overtime + threshold_ozone_timer) && mark_ozone == false && safe == false) { 
    digitalWrite(redLed, HIGH);
    digitalWrite(buzzer, HIGH);
    red_led_on = true;
    mark_ozone = true; 
  }
// --------------------------------------------------------------------------------------

/////// no2 warning logic ///////////////

 if (no2Value >= threshold_no2) {
    total_time_above_no2 = total_time_above_no2 + time_passed;
    total_time_below_no2 = 0;
  } else{
    total_time_below_no2 = total_time_below_no2 + time_passed;  //do i need
  }

// green LED 
  if ((total_time_above_no2 >= threshold_no2_timer) && mark_no2 == false && safe == false) {
    digitalWrite(greenLed, HIGH);
    green_led_on = true; 
     
  } 

// red LED 
  if ((total_time_above_no2 >= extra_overtime + threshold_no2_timer) && mark_no2 == false && safe == false) { 
    digitalWrite(redLed, HIGH);
    digitalWrite(buzzer, HIGH);
    red_led_on = true;
    mark_no2 = true; 
  }
// ----------------------------------------------------------------------------------------------

///////////////////// pm2.5 warning logic////////////////////
if (pm25Value >= threshold_pm25) {
    total_time_above_pm25 = total_time_above_pm25 + time_passed;
    total_time_below_pm25 = 0;
  } else{
    total_time_below_pm25 = total_time_below_pm25 + time_passed;  // for safety mechanism 
  }

// green LED 
  if ((total_time_above_pm25 >= threshold_pm25_timer) && mark_pm25 == false && safe == false) {
    digitalWrite(greenLed, HIGH);
    green_led_on = true; 
     
  } 

// red 
  if ((total_time_above_pm25 >= extra_overtime + threshold_pm25_timer) && mark_pm25 == false && safe == false) { 
    digitalWrite(redLed, HIGH);
    digitalWrite(buzzer, HIGH);
    red_led_on = true;
    mark_pm25 = true; 
  }
//-----------------------------------------------------------

/////////////// pm10 warning logic////////////////////////

if (pm10Value >= threshold_pm10) {
    total_time_above_pm10 = total_time_above_pm10 + time_passed;
    total_time_below_pm10 = 0;
  } else{
    total_time_below_pm10 = total_time_below_pm10 + time_passed;  //do i need
  }

// green LED 
  if ((total_time_above_pm10 >= threshold_pm10_timer) && mark_pm10 == false && safe == false) {
    digitalWrite(greenLed, HIGH);
    green_led_on = true; 
     
  } 

// red LED 
  if ((total_time_above_pm10 >= extra_overtime + threshold_pm10_timer) && mark_pm10 == false && safe == false) { 
    digitalWrite(redLed, HIGH);
    digitalWrite(buzzer, HIGH);
    red_led_on = true;
    mark_pm10 = true; 
  }


/////////////////// pot warning logic //////////////
 //if (potValue >= threshold_pot) {
  //  total_time_above_pot = total_time_above_pot + time_passed;
  //  total_time_below_pot = 0;
 // } else{
  //  total_time_below_pot = total_time_below_pot + time_passed;  
 // }

// green LED 
 // if ((total_time_above_pot >= threshold_pot_timer) && mark_pot == false && safe == false) {
 //   digitalWrite(greenLed, HIGH);
 //   green_led_on = true; 
     
 // } 

// red LED 
 //  if ((total_time_above_pot >= extra_overtime + threshold_pot_timer) && mark_pot == false && safe == false) { 
  //   digitalWrite(redLed, HIGH);
  //   digitalWrite(buzzer, HIGH);
   //  red_led_on = true;
  //   mark_pot = true; 
  //}



// --------------------------------------------------------------------------------------------------
  //////////////// safety checker logic universal //////////////////
//if green OR red led is one AND the time that user is in safe space is-
  // -long enough to be deemed safe space (time_in_safety), turn all warning off 
   if ((green_led_on || red_led_on) && 
    ((total_time_below_ozone >= time_in_safety) && (total_time_below_no2 >= time_in_safety) && (total_time_below_pm25 >= time_in_safety) && (total_time_below_pm10 >= time_in_safety))) {  
     
    
   
    total_time_below_ozone = 0;
    total_time_below_no2 = 0;
    total_time_below_pm25 = 0;
    total_time_below_pm10 = 0;
   // total_time_below_pot = 0;
    safe = true ;  // mark user in safe
    time_reexpose_ozone = 0; // user did not re-expose 
    time_reexpose_no2 = 0;
    time_reexpose_pm25 = 0;
    time_reexpose_pm10 = 0;
   // time_reexpose_pot = 0;

     digitalWrite(greenLed, LOW);
    digitalWrite(redLed, LOW);
    digitalWrite(buzzer, LOW);
  }

// --------------------------------------------------------------------------------
////////// ozone re-exposure logic ////////////

  // if mark and safe are true 
  if (ozoneValue >= threshold_Ozone && safe == true && mark_ozone == true){   //user reexposure 
  time_reexpose_ozone = time_reexpose_ozone + time_passed; // start timer for re-exposure 
  } 

  if (mark_ozone == true && safe == true && time_reexpose_ozone >= danger_again) {
    digitalWrite(redLed, HIGH);
    digitalWrite(buzzer, HIGH);
  }
// ------------------------------------------------------------------------

//////////// no2 re-expose logic //////////////////
  if (no2Value >= threshold_no2 && safe == true && mark_no2 == true){  // re-expose 
  time_reexpose_no2 = time_reexpose_no2 + time_passed; //start a timer for no2 re-expose 
  } 

  if (mark_no2 == true && safe == true && time_reexpose_no2 >= danger_again) {
    digitalWrite(redLed, HIGH);
    digitalWrite(buzzer, HIGH);
  }
// ------------------------------------------------------------------------

//////////////// pm2.5 re-expose logic //////////////////
  if (pm25Value >= threshold_pm25 && safe == true && mark_pm25 == true){  // re-expose 
  time_reexpose_pm25 = time_reexpose_pm25 + time_passed; //start a timer for no2 re-expose 
  } 

  if (mark_pm25 == true && safe == true && time_reexpose_pm25 >= danger_again) {
    digitalWrite(redLed, HIGH);
    digitalWrite(buzzer, HIGH);
  }
// ------------------------------------------------------------------------

/////////////// pm10 re-expose logic //////////////////////
 if (pm10Value >= threshold_pm10 && safe == true && mark_pm10 == true){  // re-expose 
  time_reexpose_pm10 = time_reexpose_pm10 + time_passed; //start a timer for no2 re-expose 
  } 

  if (mark_pm10 == true && safe == true && time_reexpose_pm10 >= danger_again) {
    digitalWrite(redLed, HIGH);
    digitalWrite(buzzer, HIGH);
  }
//----------------------------------------------------------------------------

///////////// pot re-expose logic//////////////
 // if (potValue >= threshold_pot && safe == true && mark_pot == true){  // re-expose 
   //time_reexpose_pot = time_reexpose_pot + time_passed; //start a timer for no2 re-expose 
  // } 

 //  if (mark_pm10 == true && safe == true && time_reexpose_pot >= danger_again) {
 //  digitalWrite(redLed, HIGH);
 //   digitalWrite(buzzer, HIGH);
 // }

  }  //close loop bracket