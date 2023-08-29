#include "RTClib.h"
#include <Wire.h>
#include <EEPROM.h>

#define LIGHT_PIN 9
#define FAN_PIN 8

#define LIGHT_UP_HOURS_ADDRESS 0
#define LIGHT_DOWN_HOURS_ADDRESS 10

#define FAN_UP_HOURS_ADDRESS 20
#define FAN_DOWN_HOURS_ADDRESS 30

const int AirValue = 455;
const int WaterValue = 180;

int marcSoilmoisturepercent = 0;
int marc2Soilmoisturepercent = 0;

RTC_DS1307 rtc;
char t[32];

String read_String(char add);
void writeString(char add,String data);
void automaticLightControl();
void automaticFanControl();

void setup() {
  // Initialize pins
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  Serial.begin(9600);

  // Default pins values
  digitalWrite(LIGHT_PIN, HIGH);
  digitalWrite(FAN_PIN, HIGH);

  // Default initial values stored in EEPROM
  // writeString(LIGHT_UP_HOURS_ADDRESS, "06:00");
  // writeString(LIGHT_DOWN_HOURS_ADDRESS, "00:00");
  // writeString(FAN_UP_HOURS_ADDRESS, "23:00");
  // writeString(FAN_DOWN_HOURS_ADDRESS, "00:00");

  // Initialize RTC
  Wire.begin();
  rtc.begin();
  //rtc.adjust(DateTime(2023, 8, 29, 1, 51, 0));

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }
}

void loop() {
  // Send actual time
  DateTime now = rtc.now();
  sprintf(t, "%02d:%02d", now.hour(), now.minute());
  Serial.print(t);
  Serial.print(" ");

  // Send light state
  if (digitalRead(LIGHT_PIN) == LOW) {
    Serial.print(1);
  } else {
    Serial.print(0);
  }
  Serial.print(" ");

  // Send light desired time
  String lightUpHours = read_String(LIGHT_UP_HOURS_ADDRESS);
  String lightDownHours = read_String(LIGHT_DOWN_HOURS_ADDRESS);
  Serial.print(lightUpHours);
  Serial.print("/");
  Serial.print(lightDownHours);
  Serial.print(" ");

  // Send fan state
  if (digitalRead(FAN_PIN) == LOW) {
    Serial.print(1);
  } else {
    Serial.print(0);
  }
  Serial.print(" ");

  // Send fan desired time
  String fanUpHours = read_String(FAN_UP_HOURS_ADDRESS);
  String fanDownHours = read_String(FAN_DOWN_HOURS_ADDRESS);
  Serial.print(fanUpHours);
  Serial.print("/");
  Serial.print(fanDownHours);
  Serial.print(" ");

  // Send marc1 soil moisture percent
  marcSoilmoisturepercent = map(analogRead(A8), AirValue, WaterValue, 0, 100);
  Serial.print(marcSoilmoisturepercent);
  Serial.print(" ");

  // Send marc2 soil moisture percent
  marc2Soilmoisturepercent = map(analogRead(A9), AirValue, WaterValue, 0, 100);
  Serial.print(marc2Soilmoisturepercent);
  Serial.println(" ");

  // Get info from Raspberry Pi
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');

    if (command == "LIGHT_ON" && digitalRead(LIGHT_PIN) == HIGH) {
      digitalWrite(LIGHT_PIN, LOW);
    } else if (command == "LIGHT_OFF" && digitalRead(LIGHT_PIN) == LOW) {
      digitalWrite(LIGHT_PIN, HIGH);
    } else if (command == "FAN_ON" && digitalRead(FAN_PIN) == HIGH) {
      digitalWrite(FAN_PIN, LOW);
    } else if (command == "FAN_OFF" && digitalRead(FAN_PIN) == LOW) {
      digitalWrite(FAN_PIN, HIGH);
    }

    if (command.startsWith("LIGHT_HOURS")) {
      String hoursValue = command.substring(11);

      // Split hoursValue into two parts using the delimiter '/'
      int delimiterIndex = hoursValue.indexOf('/');
      if (delimiterIndex != -1) {
        String up_hour = hoursValue.substring(0, delimiterIndex);
        String down_hour = hoursValue.substring(delimiterIndex + 1);

        // Store hours values in EEPROM
        writeString(LIGHT_UP_HOURS_ADDRESS, up_hour);
        writeString(LIGHT_DOWN_HOURS_ADDRESS, down_hour);
      }
    } else if (command.startsWith("FAN_HOURS")) {
      String hoursValue = command.substring(9);

      // Split hoursValue into two parts using the delimiter '/'
      int delimiterIndex = hoursValue.indexOf('/');
      if (delimiterIndex != -1) {
        String up_hour = hoursValue.substring(0, delimiterIndex);
        String down_hour = hoursValue.substring(delimiterIndex + 1);

        // Store hours values in EEPROM
        writeString(FAN_UP_HOURS_ADDRESS, up_hour);
        writeString(FAN_DOWN_HOURS_ADDRESS, down_hour);
      }
    }
  }

  automaticLightControl();
  automaticFanControl();
  delay(2000);
}

void writeString(char add,String data) {
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

void automaticLightControl() {
  DateTime now = rtc.now();
  
  String upHours = read_String(LIGHT_UP_HOURS_ADDRESS);
  String downHours = read_String(LIGHT_DOWN_HOURS_ADDRESS);

  // Extract hours and minutes
  int upHour = atoi(upHours.substring(0, 2).c_str());
  int upMinute = atoi(upHours.substring(3, 5).c_str());

  int downHour = atoi(downHours.substring(0, 2).c_str());
  int downMinute = atoi(downHours.substring(3, 5).c_str());

  // Compare hours and minutes for automatic light control
  if (upHour == now.hour() && downMinute == now.minute()) {
    digitalWrite(LIGHT_PIN, LOW);
  } else if (downHour == now.hour() && upMinute == now.minute()) {
    digitalWrite(LIGHT_PIN, HIGH);
  }
}

void automaticFanControl() {
  DateTime now = rtc.now();
  
  String upHours = read_String(FAN_UP_HOURS_ADDRESS);
  String downHours = read_String(FAN_DOWN_HOURS_ADDRESS);

  // Extract hours and minutes
  int upHour = atoi(upHours.substring(0, 2).c_str());
  int upMinute = atoi(upHours.substring(3, 5).c_str());

  int downHour = atoi(downHours.substring(0, 2).c_str());
  int downMinute = atoi(downHours.substring(3, 5).c_str());

  // Compare hours and minutes for automatic light control
  if (upHour == now.hour() && downMinute == now.minute()) {
    digitalWrite(FAN_PIN, LOW);
  } else if (downHour == now.hour() && upMinute == now.minute()) {
    digitalWrite(FAN_PIN, HIGH);
  }
}