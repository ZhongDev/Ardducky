// HID input libraries
#include <Keyboard.h>
#include <Mouse.h>
// SD card support libraries
#include <SPI.h>
#include <SD.h>

const String validKeys = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ`~!@#$%^&*()-_=+[]{};:'\",.<>/?";

// delay between keypresses in ms
const int keyDelay = 8;
=
// SD fluff
File fd;
String fileName = "000000.txt"; // SD library only supports up to 8.3 names
// SD card Pins
const uint8_t chipSelect = 3;
const uint8_t cardDetect = 2;
// More SD fluff
bool alreadyBegan = false;  // SD.begin() misbehaves if not first call

// Dip Switch Pin Definitions
int dipPins[6] = {4, 5, 6, 7, 8, 9};
int dipSwitchLen = sizeof(dipPins)/sizeof(int);

void setup()
{
  // HID init
  Keyboard.begin();
  Mouse.begin();

  // Define SD card detection
  pinMode(cardDetect, INPUT);

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

  // Set file to read to dipswitch state
  fileName = dipStateStr + ".txt";

  // Init SD library
  initializeCard();

  unsigned long endTime = millis();

  // Read file line by line
  fd = SD.open(fileName, FILE_READ);
  if (fd) {

    char lineBuffer[512]; // Buffer to store the characters in a line
    int bufferIndex = 0;
    
    // Read from the file until it's finished
    while (fd.available()) {
      char c = fd.read(); // Read a character from the file

      if (c == '\n' || c == '\r') {
        // End of line character found, process the line
        // Ignore CRLF
        if (bufferIndex != 0) {
          lineBuffer[bufferIndex] = '\0'; // Null-terminate the string
          String line(lineBuffer);
          handleDucky(line);
          endTime = millis();
          bufferIndex = 0; // Reset buffer index
        }
      } else {
        // Add character to buffer if it's not a carriage return (to handle CRLF)
        if (c != '\r') {
          if (bufferIndex < sizeof(lineBuffer) - 1) {
            lineBuffer[bufferIndex++] = c; // Add to buffer
          } else {
            // Buffer overflow, handle it or reset the buffer index
            bufferIndex = 0; // For this example, we simply clear the buffer
            lineBuffer[bufferIndex] = '\0';
          }
        }
      }
    }

    // check and process last line
    if (bufferIndex != 0) {
      lineBuffer[bufferIndex] = '\0'; // Null-terminate the string
      String line(lineBuffer);
      handleDucky(line);
      endTime = millis();
      bufferIndex = 0; // Reset buffer index
    }
    
    // Close the file
    fd.close();
  }
}

void loop()
{
  // do nothing
}

// unsigned long startTime;
bool isRemBlock = false;
bool isStringBlock = false;
bool isStringLnBlock = false;

///
/// Interpret and run a single line of duckyscript (1.0)
///
void handleDucky(String line)
{
  // Grab first word
  int fstSpaceIdx = line.indexOf(" ");
  String command = line.substring(0, fstSpaceIdx);
  String postCommand;
  String fstArg;
  if (fstSpaceIdx == -1) {
    postCommand = "";
    fstArg = "";
  } else {
    postCommand = line.substring(fstSpaceIdx + 1);
    int secSpaceIdx = postCommand.indexOf(" ");
    fstArg = postCommand.substring(0, secSpaceIdx);
  }
  
  // startTime = millis();

  // Switch based on command
  // Check for Comment
  if (isRemBlock) {
    if (command.equals("END_REM")) {
      isRemBlock = false;
    }
  }
  else if (isStringBlock) {
    if (command.equals("END_STRING")) {
      isStringBlock = false;
    } else {
      int startI = 0;
      if (line[0] == '\t') {startI = 1;}
      int argLen = line.length();
      for (int i = startI; i < argLen; i++) {
        Keyboard.write(line[i]);
        delay(keyDelay);
      }
    }
  }
  else if (isStringLnBlock) {
    if (command.equals("END_STRINGLN")) {
      isStringLnBlock = false;
    } else {
      int startI = 0;
      if (line[0] == '\t') {startI = 1;}
      int argLen = line.length();
      for (int i = startI; i < argLen; i++) {
        Keyboard.write(line[i]);
        delay(keyDelay);
      }
      Keyboard.write(KEY_RETURN);
      delay(keyDelay);
    }
  }
  else if (command.equals("REM")) {/* do nothing */}
  else if (command.equals("REM_BLOCK")) {
    isRemBlock = true;
  }
  // Check for Delay command
  else if (command.equals("DELAY")) {
    int delayVal = fstArg.toInt();
    // Make sure delay is at least 20ms
    if (delayVal < 20) {
      delayVal = 20;
    }
    delay(delayVal);
  }
  // Print (no return)
  else if (command.equals("STRING")) {
    int argLen = postCommand.length();
    if (argLen > 0) {
      for (int i = 0; i < argLen; i++) {
        Keyboard.write(postCommand[i]);
        delay(keyDelay);
      }
    } else {
      isStringBlock = true;
    }
  }
  else if (command.equals("STRINGLN")) {
    int argLen = postCommand.length();
    if (argLen > 0) {
      for (int i = 0; i < argLen; i++) {
        Keyboard.write(postCommand[i]);
        delay(keyDelay);
      }
      Keyboard.write(KEY_RETURN);
      delay(keyDelay);
    } else {
      isStringLnBlock = true;
    }
  }
  // Check if is a Modifer Keycombo
  else {
    if (line.length() > 0) {
      modifKeydown(line);
      delay(keyDelay);
    }
    Keyboard.releaseAll();
    delay(keyDelay);
  }
}

///
/// Handle Modifier Keys Keydown
///
void modifKeydown(String postCommand) {
  int fstSpaceIdx = postCommand.indexOf(" ");
  int fstDashIdx = postCommand.indexOf("-");
  int fstIdx = -1;
  if (fstSpaceIdx < fstDashIdx && fstSpaceIdx >= 0) {
    fstIdx = fstSpaceIdx;
  } else if (fstDashIdx >= 0) {
    fstIdx = fstDashIdx;
  } else {
    fstIdx = fstSpaceIdx;
  }
  String modif = postCommand.substring(0, fstIdx);
  String modifPostCommand;
  if (fstIdx != -1) { modifPostCommand = postCommand.substring(fstIdx + 1); }
  else { modifPostCommand = ""; }

  //Basic Modifier Keys
  if (modif.equals("SHIFT")) {Keyboard.press(KEY_LEFT_SHIFT);}
  else if (modif.equals("ALT")) {Keyboard.press(KEY_LEFT_ALT);}
  else if (modif.equals("CONTROL") || modif.equals("CTRL")) {Keyboard.press(KEY_LEFT_CTRL);}
  else if (modif.equals("COMMAND") || modif.equals("WINDOWS") || modif.equals("GUI")) {Keyboard.press(KEY_LEFT_GUI);}
  //System Keys
  else if (modif.equals("ENTER")) {Keyboard.press(KEY_RETURN);}
  else if (modif.equals("ESCAPE")) {Keyboard.press(KEY_ESC);}
  else if (modif.equals("PAUSE") || modif.equals("BREAK")) {Keyboard.press(KEY_PAUSE);}
  else if (modif.equals("PRINTSCREEN")) {Keyboard.press(KEY_PRINT_SCREEN);}
  else if (modif.equals("MENU") || modif.equals("APP")) {Keyboard.press(KEY_MENU);}
  else if (modif.length() <= 3 && modif.startsWith("F") && modif.substring(1).toInt() <= 24 && modif.substring(1).toInt() > 0) {
    int fKeycode = 193 + modif.substring(1).toInt();
    Keyboard.press(fKeycode);
  }
  //Cursor Keys
  else if (modif.equals("UPARROW")) {Keyboard.press(KEY_UP_ARROW);}
  else if (modif.equals("DOWNARROW")) {Keyboard.press(KEY_DOWN_ARROW);}
  else if (modif.equals("LEFTARROW")) {Keyboard.press(KEY_LEFT_ARROW);}
  else if (modif.equals("RIGHTARROW")) {Keyboard.press(KEY_RIGHT_ARROW);}
  else if (modif.equals("PAGEUP")) {Keyboard.press(KEY_PAGE_UP);}
  else if (modif.equals("PAGEDOWN")) {Keyboard.press(KEY_PAGE_DOWN);}
  else if (modif.equals("HOME")) {Keyboard.press(KEY_HOME);}
  else if (modif.equals("END")) {Keyboard.press(KEY_END);}
  else if (modif.equals("INSERT")) {Keyboard.press(KEY_INSERT);}
  else if (modif.equals("DELETE")) {Keyboard.press(KEY_DELETE);}
  else if (modif.equals("BACKSPACE")) {Keyboard.press(KEY_BACKSPACE);}
  else if (modif.equals("TAB")) {Keyboard.press(KEY_TAB);}
  else if (modif.equals("SPACE")) {Keyboard.press(' ');}
  //Lock Keys
  else if (modif.equals("CAPSLOCK")) {Keyboard.press(KEY_CAPS_LOCK);}
  else if (modif.equals("NUMLOCK")) {Keyboard.press(KEY_NUM_LOCK);}
  else if (modif.equals("SCROLLLOCK")) {Keyboard.press(KEY_SCROLL_LOCK);}

  //Any other singular key
  else if (modif.length() == 1 && validKeys.indexOf(modif[0]) > -1) {
    Keyboard.press(modif[0]);
  }

  if (modifPostCommand.length() > 0) {
    delay(keyDelay);
    modifKeydown(modifPostCommand);
  }
}

///
/// Ready Card for reading.
///
void initializeCard(void){
  // Wait for SD to exist
  if (!digitalRead(cardDetect))
  {
    while (!digitalRead(cardDetect));
    delay(250); // 'Debounce insertion'
  }
  // Card seems to exist.  begin() returns failure
  // even if it worked if it's not the first call.
  if (!SD.begin(chipSelect) && !alreadyBegan)  // begin uses half-speed...
  {
    initializeCard(); // Possible infinite retry loop is as valid as anything
  } else {
    alreadyBegan = true;
  }

  if (SD.exists(fileName))
  {}
  else
  {
    File newFile = SD.open(fileName, FILE_WRITE);
    if (newFile) {
      newFile.close();
    } else {
    }
  }
}