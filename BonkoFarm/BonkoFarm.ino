#include "RTClib.h"
#include <Wire.h>
#include <EEPROM.h>

// Define the pins for light and fan
#define LIGHT_PIN 9
#define FAN_PIN 8

// EEPROM storage addresses for light and fan timing
#define LIGHT_UP_HOURS_ADDRESS 0
#define LIGHT_DOWN_HOURS_ADDRESS 10
#define FAN_UP_HOURS_ADDRESS 20
#define FAN_DOWN_HOURS_ADDRESS 30

// Soil moisture sensor calibration values
const int AirValue = 455;
const int WaterValue = 180;

// General delay time
const int DelayTime = 0;

// Initialize RTC
RTC_DS1307 rtc;
char t[32];

// Function prototypes
String read_String(char add);
void writeString(char add, String data);
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
void handleSetTime(String command);
void handleSetLightState(String command);
void handleSetFanState(String command);
void handleSetLightHours(String command);
void handleSetFanHours(String command);

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

  // Initialize RTC
  Wire.begin();
  rtc.begin();
  
  // Check if RTC is running
  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }
  
  // Set initial state of PINs based on EEPROM values and current time
  checkPinState(LIGHT_PIN, LIGHT_UP_HOURS_ADDRESS, LIGHT_DOWN_HOURS_ADDRESS);
  checkPinState(FAN_PIN, FAN_UP_HOURS_ADDRESS, FAN_DOWN_HOURS_ADDRESS);
}

void loop() {
  // Check for incoming commands from Raspberry Pi
  if (Serial.available()) {
    String dataRead = Serial.readStringUntil('\n');
    String command = "";
    String datas = "";

    // Extract set command and datas (format: SET_XXX:XXX)
    int idx = dataRead.lastIndexOf(":");
    if (idx != -1) {
      command = dataRead.substring(0, idx);
      datas = dataRead.substring(idx);
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
  
  delay(DelayTime);
}

void writeString(char add, String data) {
  int _size = data.length();
  int i;
  for(i=0;i<_size;i++)
  {
    EEPROM.write(add+i,data[i]);
  }
  EEPROM.write(add+_size,'\0');
}

String read_String(char add) {
  int i;
  char data[100];
  int len=0;
  unsigned char k;
  k=EEPROM.read(add);
  while(k != '\0' && len<500)
  {    
    k=EEPROM.read(add+len);
    data[len]=k;
    len++;
  }
  data[len]='\0';
  return String(data);
}

void automaticDeviceControl(int pin, int upHourAddress, int downHourAddress) {
  DateTime now = rtc.now();
  
  String upHours = read_String(upHourAddress);
  String downHours = read_String(downHourAddress);

  // Extraire les heures et les minutes
  int upHour = atoi(upHours.substring(0, 2).c_str());
  int upMinute = atoi(upHours.substring(3, 5).c_str());
  int downHour = atoi(downHours.substring(0, 2).c_str());
  int downMinute = atoi(downHours.substring(3, 5).c_str());

  if (upHour == now.hour() && upMinute == now.minute()) {
    digitalWrite(pin, LOW);  // Mettre le PIN à LOW pour allumer le dispositif
  } else if (downHour == now.hour() && downMinute == now.minute()) {
    digitalWrite(pin, HIGH);  // Mettre le PIN à HIGH pour éteindre le dispositif
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
void handleSetTime(String command) {
  int years = atoi(command.substring(0, 2).c_str());
  int months = atoi(command.substring(3, 5).c_str());
  int days = atoi(command.substring(6, 8).c_str());
  int hour = atoi(command.substring(9, 11).c_str());
  int minutes = atoi(command.substring(12, 14).c_str());
  int secondes = atoi(command.substring(15, 17).c_str());
  rtc.adjust(DateTime(years, months, days, hour, minutes, secondes));
}

void handleSetLightState(String command) {
  if (digitalRead(LIGHT_PIN) == HIGH) {
    digitalWrite(LIGHT_PIN, LOW);
  } else {
    digitalWrite(LIGHT_PIN, HIGH);
  }
}

void handleSetFanState(String command) {
  if (digitalRead(FAN_PIN) == HIGH) {
    digitalWrite(FAN_PIN, LOW);
  } else {
    digitalWrite(FAN_PIN, HIGH);
  }
}

void handleSetLightHours(String command) {
  int delimiterIndex = command.indexOf('/');
  if (delimiterIndex != -1) {
    String up_hour = command.substring(0, delimiterIndex);
    String down_hour = command.substring(delimiterIndex + 1);
    writeString(LIGHT_UP_HOURS_ADDRESS, up_hour);
    writeString(LIGHT_DOWN_HOURS_ADDRESS, down_hour);
  }
}

void handleSetFanHours(String command) {
  int delimiterIndex = command.indexOf('/');
  if (delimiterIndex != -1) {
    String up_hour = command.substring(0, delimiterIndex);
    String down_hour = command.substring(delimiterIndex + 1);
    writeString(FAN_UP_HOURS_ADDRESS, up_hour);
    writeString(FAN_DOWN_HOURS_ADDRESS, down_hour);
  }
}
