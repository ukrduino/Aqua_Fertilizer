/* Контроллер внесения удобрений в аквариум на 3 помпы */
#include <DS3231.h>


// To use the hardware I2C (TWI) interface of the Arduino you must connect
// the pins as follows:
//
// Arduino Uno/2009:
// ----------------------
// DS3231:  SDA pin   -> Arduino Analog 4 or the dedicated SDA pin
//          SCL pin   -> Arduino Analog 5 or the dedicated SCL pin
//
// Arduino Leonardo:
// ----------------------
// DS3231:  SDA pin   -> Arduino Digital 2 or the dedicated SDA pin
//          SCL pin   -> Arduino Digital 3 or the dedicated SCL pin
//
// Arduino Mega:
// ----------------------
// DS3231:  SDA pin   -> Arduino Digital 20 (SDA) or the dedicated SDA pin
//          SCL pin   -> Arduino Digital 21 (SCL) or the dedicated SCL pin
//
// Arduino Due:
// ----------------------
// DS3231:  SDA pin   -> Arduino Digital 20 (SDA) or the dedicated SDA1 (Digital 70) pin
//          SCL pin   -> Arduino Digital 21 (SCL) or the dedicated SCL1 (Digital 71) pin
//
// The internal pull-up resistors will be activated when using the
// hardware I2C interfaces.
//
// You can connect the DS3231 to any available pin but if you use any
// other than what is described above the library will fall back to
// a software-based, TWI-like protocol which will require exclusive access
// to the pins used, and you will also have to use appropriate, external
// pull-up resistors on the data and clock signals.
//

// Init the DS3231 using the hardware interface
DS3231  rtc(SDA, SCL);
// Init a Time-data structures
Time  currentTime; //time from RTC


//Объект внесения удобрения
struct Udobrenie {
  String _name;
  bool _active; // Для включения или отключения данного внесения
  bool _pump_active; // Для датчика уровня удобрения
  byte _hour; // Время внесения
  byte _pump; // Номер помпы
  byte _seconds; // Секунды прокачки примерно 4 сек - 3,5 мл
  byte _speed; // Скорость работы мотора PWM  0- 255
  bool _done;
};

Udobrenie udobrenia[6] = {
  //String _name, bool _active; bool _pump_active; byte _hour; byte _pump; byte _seconds; byte _speed; byte _period; bool _done;
  {"macro", true, true, 21, 1, 2, 255, false}, // 2 seconds
  {"micro", true, true, 6, 2, 4, 255, false}, //4 seconds
  {"al_co2", false, true, 7, 3, 0, 255, false},
  {"macro", false, true, 0, 1, 0, 255, false},
  {"micro", false, true, 12, 2, 3, 255, false},
  {"al_co2", false, true, 12, 3, 0, 255, false}
};

const byte pump1Pin = 8;    // the number of the pin for pump #1
const byte pump2Pin = 9;    // the number of the pin for pump #2
const byte pump3Pin = 10;    // the number of the pin for pump #3
const byte ledsPin = 11;    // the number of the pin for leds
const byte pump1ButtonPin = 48;    // the number of the pushbutton pin for pump #1
const byte pump2ButtonPin = 50;    // the number of the pushbutton pin for pump #2
const byte pump3ButtonPin = 52;    // the number of the pushbutton pin for pump #3



void setup() {
  pinMode(pump1ButtonPin, INPUT_PULLUP);
  pinMode(pump2ButtonPin, INPUT_PULLUP);
  pinMode(pump3ButtonPin, INPUT_PULLUP);
  pinMode(pump1Pin, OUTPUT);
  pinMode(pump2Pin, OUTPUT);
  pinMode(pump3Pin, OUTPUT);
  pinMode(ledsPin, OUTPUT);
  analogWrite(pump1Pin, 0);
  analogWrite(pump2Pin, 0);
  analogWrite(pump3Pin, 0);
  analogWrite(ledsPin, 50);
  // Setup Serial connection
  Serial.begin(115200);
  // Initialize the rtc object
  rtc.begin();
  currentTime = rtc.getTime();
  delay(1000);              // wait for a second
  Serial.print(rtc.getTimeStr());
  Serial.print(" ");
  Serial.print(rtc.getDateStr());
  Serial.print(" ");
  Serial.print("Day of week: ");
  Serial.println(currentTime.dow);

  // The following lines can be uncommented to set the date and time
  //rtc.setDOW(SUNDAY);     // Set Day-of-Week to MONDAY
  //rtc.setTime(18, 54, 00);     // Set the time to 08:54:00 (24hr format)
  //rtc.setDate(26, 3, 2017);   // Set the date to 17 March 2017

  updateUdobreniaAfterReset(udobrenia);
  printUdobrenia(udobrenia);
}

void updateUdobreniaAfterReset (Udobrenie *udobrenia) {
  for (int i = 0; i < 6; i++) {
    if (udobrenia[i]._hour <= currentTime.hour) {
      udobrenia[i]._done = true;
    };
  };
}

void updateUdobreniaAfterMidnight () {
  if (currentTime.hour == 0) {
    for (int i = 0; i < 6; i++) {
      udobrenia[i]._done = false;
    };
  };
}

void printUdobrenia(Udobrenie *udobrenia) {
  for (int i = 0; i < 6; i++) {
    printUdobrenie(udobrenia[i]);
  }
}


void printUdobrenie(Udobrenie udo) {
  Serial.println("--------Udobrenie: " + udo._name + "----------");
  Serial.print("_active: ");
  Serial.println(udo._active);
  Serial.print("_pump_active: ");
  Serial.println(udo._pump_active);
  Serial.print("_hour: ");
  Serial.println(udo._hour);
  Serial.print("_pump: ");
  Serial.println(udo._pump);
  Serial.print("_seconds: ");
  Serial.println(udo._seconds);
  Serial.print("_speed: ");
  Serial.println(udo._speed);
  Serial.print("_done: ");
  Serial.println(udo._done);
}


void loop() {
  currentTime = rtc.getTime();
  readButtons();
  checkUdobrenia(udobrenia);
  updateUdobreniaAfterMidnight();
}

void checkUdobrenia(Udobrenie* udobrenia) {
  for (int i = 0; i < 6; i++) {
    if (udobrenia[i]._hour == currentTime.hour
        && udobrenia[i]._active == true
        && udobrenia[i]._pump_active == true
        && udobrenia[i]._done == false) {
      vnestyUdobreni(udobrenia[i], i);
    };
  };
}

void vnestyUdobreni(Udobrenie udo, int index) {
  if (udo._pump == 1) {
    analogWrite(pump1Pin, udo._speed);
    delay(udo._seconds * 1000);
    analogWrite(pump1Pin, 0);
  }
  if (udo._pump == 2) {
    analogWrite(pump2Pin, udo._speed);
    delay(udo._seconds * 1000);
    analogWrite(pump2Pin, 0);
  }
  if (udo._pump == 3) {
    analogWrite(pump3Pin, udo._speed);
    delay(udo._seconds * 1000);
    analogWrite(pump3Pin, 0);
  }
  udobrenia[index]._done = true;
  printUdobrenia(udobrenia);
}

void  readButtons() {
  if (digitalRead(pump1ButtonPin) == LOW)
  {
    Serial.println("pump1ButtonPin pressed");
    delay(100);
    analogWrite(pump1Pin, 255);
    do {
      Serial.println("Pump 1 working");
    } while (digitalRead(pump1ButtonPin) == LOW);
    analogWrite(pump1Pin, 0);
    return;
  };
  if (digitalRead(pump2ButtonPin) == LOW)
  {
    Serial.println("pump2ButtonPin pressed");
    delay(100);
    analogWrite(pump2Pin, 255);
    do {
      Serial.println("Pump 2 working");
    } while (digitalRead(pump2ButtonPin) == LOW);
    analogWrite(pump2Pin, 0);
    return;
  };
  if (digitalRead(pump3ButtonPin) == LOW)
  {
    Serial.println("pump3ButtonPin pressed");
    delay(100);
    analogWrite(pump3Pin, 255);
    do {
      Serial.println("Pump 3 working");
    } while (digitalRead(pump3ButtonPin) == LOW);
    analogWrite(pump3Pin, 0);
    return;
  };
}
