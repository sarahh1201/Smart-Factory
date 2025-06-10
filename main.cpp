#include "DHT11.h"
#include "hcsr04.h"
#include "mbed.h"
#include "sMotor.h"


///   INITIALIZING
DigitalOut buzzer(p6);     // Buzzer
DigitalOut fan(p7);        // Fan
Serial bluetooth(p9, p10); // Bluetooth for app
DHT11 Temp_Hum(p16);       // DHT11 - Humidity + Temperature Sensor
DigitalIn PIR(p20);        // PIR Sensor
HCSR04 WaterLVL(p21, p22); // Ultrasonic Sensor
sMotor motor(p11, p12, p13, p14);
DigitalOut led(p24);     // LED through relay
Serial pc(USBTX, USBRX); // PC for debugging


/// DECLARING VARIABLES
Timer lightTimer; // Timer to handle the delay
float chemLevel = 0;
float requiredLevel = 3; // Suppose the required level should be at 20cm
char state = 0;
bool manual_fan = false;
int DHT11status = 0; // DHT11 Sensor status
float distance = 0;


/// BLUETOOTH FUNCTION
void checkBluetooth() {
  if (bluetooth.readable()) {
    state = bluetooth.getc();
    pc.printf("State = %c\n\r", state);
  }
}


/// FUNCTION TO CONTROL DOOR WITH THE STEPPER MOTOR
void Door() {
  int step_speed = 1200; // set default motor speed
  int numstep = 512;     // defines full turn of 360 degree


  if (state == '1') { // Door Open
    motor.step(numstep / 2, 0, step_speed);
  }
  if (state == '2') { // Door Close
    motor.step(numstep / 2, 1, step_speed);
  }
}


/// FUNCTION TO CONTROL DC FAN WITH RELAY AND MANUAL USER INPUT THROUGH THE APP
void Fan() {
  if (state == '3') {
    fan = 1;
    manual_fan = true;
    return;
  } else if (state == '4') {
    fan = 0;
    manual_fan = true;
    return;
  }


  if (manual_fan)
    return; // If manually set, skip auto control


  DHT11status = Temp_Hum.readData();
  if (DHT11status != DHT11::OK) {
    printf("DHT11 not ready\r\n");
    return; // Exit early if the sensor is not ready
  }


  checkBluetooth();


  // Read and store values
  int temperature = Temp_Hum.readTemperature();
  int humidity = Temp_Hum.readHumidity();


  printf("DHT11: Temperature: %d C\r\n", temperature);
  printf("DHT11: Humidity: %d %%\r\n", humidity);


  static bool fan_on = false;


  if (!fan_on && (humidity >= 70 || temperature >= 28)) {
    fan = 1;
    fan_on = true;
  } else if (fan_on &&
             temperature <= 25) { // Lower limit to prevent rapid switching
    fan = 0;
    fan_on = false;
  }
}


/// FUNCTION TO CHECK CHEMICAL LEVEL
void Level() {
  buzzer = 0;
  WaterLVL.start();
  distance = WaterLVL.get_dist_cm();
  chemLevel = 10 - distance; // Calculating chemical level
  pc.printf("Chemical level = %.2f cm\r\n", chemLevel);
  pc.printf("Distance = %.2f cm\r\n", distance);


    if(chemLevel < requiredLevel) {
    pc.printf("WARNING: Leakage Detected!\r\n");
    buzzer = 1;
    wait(1);    // Buzzer on for 1 sec
    buzzer = 0; // Turn off buzzer after 1 minute
    bluetooth.printf("%d\n", "8");
    if (state == 7) {
      break;
    }
  }
}


/// FUNCTION TO CONTROL LIGHTS WITH PIR SENSOR
void Light() {
  if (PIR.read() == 1) { // If motion is detected
    led = 1;             // Turn on the LED
    lightTimer.reset();  // Reset the timer whenever motion is detected
    lightTimer.start();  // Start or restart the timer
  } else if (lightTimer.read() >= 5) { // Check if 5 seconds have passed
    led = 0; // Turn off the LED if 5 seconds have passed without motion
    lightTimer.stop();  // Stop the timer
    lightTimer.reset(); // Reset the timer to prevent multiple triggers
  }
}


int main() {
  lightTimer.start();
  bluetooth.baud(9600);


  // Start the timer initially
  while (1) {
    wait(2);


    checkBluetooth();
    Door();
    Fan();
    Level();


    Light();
    wait(10);
    manual_fan = false;
    state = 0;
  }
}


