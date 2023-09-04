/***************************************************
 This code is for an automatic planting project made in Arduino
 with the ability to send and get datas from Serial port

 Created 2023-08-13
 By Alexis KNOB <https://www.bonko.fr>

/***********Notice and Trouble shooting***************
 1.Connection and Diagram can be found here: https://www.bonko.fr
 2.This code is tested on Arduino Mega.
 3.Sensors are connect to Analog 8 & 9 port.
 ****************************************************/

#include "RTClib.h"
#include <Wire.h>
#include <EEPROM.h>
#include <avr/wdt.h>

// General delay time
#define DELAY_TIME 50

// Define the pins for light and fan
#define LIGHT_PIN 9
#define FAN_PIN 8

// EEPROM storage addresses for light and fan timing
#define LIGHT_UP_HOURS_ADDRESS 0
#define LIGHT_DOWN_HOURS_ADDRESS 10
#define FAN_UP_HOURS_ADDRESS 20
#define FAN_DOWN_HOURS_ADDRESS 30

// Soil moisture sensor calibration values
const int AirValue = 510;
const int WaterValue = 180;

// Initialize RTC
RTC_DS1307 rtc;
char t[32];

// Function prototypes
String read_String(int add);
void writeString(int add, const String& data);
void automaticDeviceControl(int pin, int upHourAddress, int downHourAddress);
void checkPinState(int pin, int upHourAddress, int downHourAddress);

// Handler GET function prototypes for incoming commands
void handleGetTime();
void handleGetMarc1();
void handleGetMarc2();
void handleGetLightState();
void handleGetFanState();
void handleGetLightHours();
void handleGetFanHours();

// Handler SET function prototypes for incoming commands
void handleSetTime(const String& datas);
void handleSetLightState(const String& datas);
void handleSetFanState(const String& datas);
void handleSetLightHours(const String& datas);
void handleSetFanHours(const String& datas);

// Define GET command strings
const String getCommands[] = {
  "GET_TIME",
  "GET_MARC1",
  "GET_MARC2",
  "GET_LIGHTSTATE",
  "GET_FANSTATE",
  "GET_LIGHTHOURS",
  "GET_FANHOURS",
};

// Define SET command strings
const String setCommands[] = {
  "SET_TIME",
  "SET_LIGHTSTATE",
  "SET_FANSTATE",
  "SET_LIGHTHOURS",
  "SET_FANHOURS",
};

// Map command strings to handler functions
typedef void (*GetCommandHandler)();
const GetCommandHandler getHandlers[] = {
  handleGetTime,
  handleGetMarc1,
  handleGetMarc2,
  handleGetLightState,
  handleGetLightHours,
  handleGetFanState,
  handleGetFanHours,
};

// Map command strings to handler functions
typedef void (*SetCommandHandler)(String command);
const SetCommandHandler setHandlers[] = {
  handleSetTime,
  handleSetLightState,
  handleSetFanState,
  handleSetLightHours,
  handleSetFanHours,
};

// Calculate the number of commands
const int numGetCommands = sizeof(getCommands) / sizeof(getCommands[0]);
const int numSetCommands = sizeof(setCommands) / sizeof(setCommands[0]);

void setup() {
  // Initialize PINs
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  Serial.begin(9600);
  wdt_disable();

  // Initialize RTC
  Wire.begin();
  rtc.begin();
  
  // Vérifier si le système a redémarré à cause du watchdog timer
  if (MCUSR & _BV(WDRF)) {
    MCUSR &= ~_BV(WDRF);
    Serial.println("System restarted using watchdog timer");
  }

  // Check if RTC is running
  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }
  //rtc.adjust(DateTime(2023, 9, 19, 17, 3, 0));
  
  // Set initial state of PINs based on EEPROM values and current time
  checkPinState(LIGHT_PIN, LIGHT_UP_HOURS_ADDRESS, LIGHT_DOWN_HOURS_ADDRESS);
  checkPinState(FAN_PIN, FAN_UP_HOURS_ADDRESS, FAN_DOWN_HOURS_ADDRESS);

  // Active and set Watchdog timer on 8 seconds
  wdt_enable(WDTO_8S);
}

void loop() {
  wdt_reset();

  // Check for incoming commands from Raspberry Pi
  if (Serial.available()) {
    String dataRead = Serial.readStringUntil('\n');
    String command = "";
    String datas = "";

    // Extract set command and datas (format: SET_XXX:XXX)
    int idx = dataRead.lastIndexOf(":");
    if (idx != -1) {
      command = dataRead.substring(0, idx);
      datas = dataRead.substring(idx + 1, dataRead.length());
    }
    
    // Match command with its handler
    for (int i = 0; i < numSetCommands; i++) {
      if (command == setCommands[i]) {
        setHandlers[i](datas);
        break;
      }
    }
  }

  // Lanch all get functions by order
  for (int i = 0; i < numGetCommands; i++) {
    getHandlers[i]();
    Serial.print(" ");
  }
  Serial.println();

  // Automatic control of light and fan based on time
  automaticDeviceControl(LIGHT_PIN, LIGHT_UP_HOURS_ADDRESS, LIGHT_DOWN_HOURS_ADDRESS);
  automaticDeviceControl(FAN_PIN, FAN_UP_HOURS_ADDRESS, FAN_DOWN_HOURS_ADDRESS);
  
  delay(DELAY_TIME);
}

void writeString(int add, const String& data) {
  int len = data.length();
  for (int i = 0; i < len; i++) {
    EEPROM.write(add + i, data[i]);
  }
  EEPROM.write(add + len, '\0');
}

String read_String(int add) {
  char data[100];
  int len = 0;
  char k;
  do {
    k = EEPROM.read(add + len);
    data[len] = k;
    len++;
  } while (k != '\0' && len < 100); // Added upper limit
  data[len] = '\0';
  return String(data);
}

void automaticDeviceControl(int pin, int upHourAddress, int downHourAddress) {
  DateTime now = rtc.now();
  
  String upHours = read_String(upHourAddress);
  String downHours = read_String(downHourAddress);

  // Extract hours and minutes
  int upHour = atoi(upHours.substring(0, 2).c_str());
  int upMinute = atoi(upHours.substring(3, 5).c_str());
  int downHour = atoi(downHours.substring(0, 2).c_str());
  int downMinute = atoi(downHours.substring(3, 5).c_str());

  // Compare hours and minutes for automatic light control
  if (upHour == now.hour() && upMinute == now.minute()) {
    digitalWrite(pin, LOW);
  } else if (downHour == now.hour() && downMinute == now.minute()) {
    digitalWrite(pin, HIGH);
  }
}

void checkPinState(int pin, int upHourAddress, int downHourAddress) {
  DateTime now = rtc.now();
  
  String upHours = read_String(upHourAddress);
  String downHours = read_String(downHourAddress);

  // Extract hours and minutes
  int upHour = atoi(upHours.substring(0, 2).c_str());
  int upMinute = atoi(upHours.substring(3, 5).c_str());

  int downHour = atoi(downHours.substring(0, 2).c_str());
  int downMinute = atoi(downHours.substring(3, 5).c_str());

  // Compare hours and minutes for automatic light control
  if (upHour < now.hour() || (upHour == now.hour() && upMinute <= now.minute())) {
    digitalWrite(pin, LOW);
  } else if (downHour < now.hour() || (downHour == now.hour() && downMinute <= now.minute())) {
    digitalWrite(pin, HIGH);
  }
}

// GET FUNCTIONS
void handleGetTime() {
  DateTime now = rtc.now();
  sprintf(t, "%02d:%02d", now.hour(), now.minute());
  Serial.print(t);
}

void handleGetMarc1() {
  Serial.print(map(analogRead(A8), AirValue, WaterValue, 0, 100));
}

void handleGetMarc2() {
  Serial.print(map(analogRead(A9), AirValue, WaterValue, 0, 100));
}

void handleGetLightState() {
  if (digitalRead(LIGHT_PIN) == LOW) {
    Serial.print(1);
  } else {
    Serial.print(0);
  }
}

void handleGetFanState() {
  if (digitalRead(FAN_PIN) == LOW) {
    Serial.print(1);
  } else {
    Serial.print(0);
  }
}

void handleGetLightHours() {
  Serial.print(read_String(LIGHT_UP_HOURS_ADDRESS));
  Serial.print("/");
  Serial.print(read_String(LIGHT_DOWN_HOURS_ADDRESS));
}

void handleGetFanHours() {
  Serial.print(read_String(FAN_UP_HOURS_ADDRESS));
  Serial.print("/");
  Serial.print(read_String(FAN_DOWN_HOURS_ADDRESS));
}

// SET FUNCTIONS
void handleSetTime(const String& datas) {
  if (datas.length() == 20) {
    int year = datas.substring(0, 4).toInt();
    int month = datas.substring(5, 7).toInt();
    int day = datas.substring(8, 10).toInt();
    int hour = datas.substring(11, 13).toInt();
    int minute = datas.substring(14, 16).toInt();
    int second = datas.substring(17, 19).toInt();

    rtc.adjust(DateTime(year, month, day, hour, minute, second));
  } else {
    return;
  }
}


void handleSetLightState(const String& datas) {
  digitalWrite(LIGHT_PIN, datas == "1" ? LOW : HIGH);
}

void handleSetFanState(const String& datas) {
  digitalWrite(FAN_PIN, datas == "1" ? LOW : HIGH);
}

void handleSetLightHours(const String& datas) {
  int delimiterIndex = datas.indexOf('/');
  if (delimiterIndex != -1) {
    String up_hour = datas.substring(0, delimiterIndex);
    String down_hour = datas.substring(delimiterIndex + 1);
    writeString(LIGHT_UP_HOURS_ADDRESS, up_hour);
    writeString(LIGHT_DOWN_HOURS_ADDRESS, down_hour);
  }
}

void handleSetFanHours(const String& datas) {
  int delimiterIndex = datas.indexOf('/');
  if (delimiterIndex != -1) {
    String up_hour = datas.substring(0, delimiterIndex);
    String down_hour = datas.substring(delimiterIndex + 1);
    writeString(FAN_UP_HOURS_ADDRESS, up_hour);
    writeString(FAN_DOWN_HOURS_ADDRESS, down_hour);
  }
}
