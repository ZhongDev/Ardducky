// HID input libraries
#include <Keyboard.h>
#include <Mouse.h>
// SD card support libraries
#include <SPI.h>
#include <SD.h>

// Dip Switch Pin Definitions
int dipPins[6] = {4, 5, 6, 7, 8, 9};
int dipSwitchLen = sizeof(dipPins)/sizeof(int);

void setup()
{
  // HID init
  Keyboard.begin();
  Mouse.begin();
  // Serial init
  Serial.begin(9600);       // Turn on serial connection
  while (!Serial);          // Wait while serial connects
  Serial.println("Initialize Serial Monitor");

  // Define dip switch pins as inputs
  for (int i = 0; i < dipSwitchLen; i++) {
    pinMode(dipPins[i], INPUT_PULLUP);
  }

  // Read out dip switches as a string
  String dipStateStr = "";
  for (int i = 0; i < dipSwitchLen; i++) {
    int readVal = digitalRead(dipPins[i]);
    String state = "0";
    if (readVal == 0) {
      state = "1";
    }
    dipStateStr = dipStateStr + state;
  }

  Serial.println(dipStateStr);
}

void loop()
{
  Serial.println("-----");
  delay(500);
}
