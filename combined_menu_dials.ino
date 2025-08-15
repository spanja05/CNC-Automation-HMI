#include <TFT_eSPI.h>
#include <AiEsp32RotaryEncoder.h>
#include "dials.h"
#include "NotoSansMonoSCB20.h"
#include "NotoSansBold15.h"
#include <Preferences.h>

TFT_eSPI tft = TFT_eSPI();

// Sprites for dial page
TFT_eSprite bck = TFT_eSprite(&tft);     // background sprite where tank (left) dial's needle and circle with reading is pushed onto
TFT_eSprite sprite1 = TFT_eSprite(&tft); // needle for the tank dial
TFT_eSprite sprite2 = TFT_eSprite(&tft); // circle with number for tank dial
TFT_eSprite bck2 = TFT_eSprite(&tft);    // background sprite where table (right) dial's needle and circle with reading is pushed onto
TFT_eSprite sprite3 = TFT_eSprite(&tft); // needle for table dial
TFT_eSprite sprite4 = TFT_eSprite(&tft); // circle with number for table dial

// Rotary encoder pins
#define ROTARY_ENCODER_CLK_PIN 33
#define ROTARY_ENCODER_DT_PIN 17
#define ROTARY_ENCODER_BUTTON_PIN 32
#define ROTARY_ENCODER_VCC_PIN -1
#define ROTARY_ENCODER_STEPS 4

// Preferences object for storing upper and lower limit values set by the user to ESP32 flash memory. 
Preferences preferences;
// Rotary encoder object for detecting user input from rotary encoder. 
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_CLK_PIN, ROTARY_ENCODER_DT_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

/* Indicates which page the display is on. 
   Two options with one displaying the tank and table dials, and the other for allowing user to set upper and lower limits. */ 
enum Page {
  DIAL_PAGE,
  MENU_PAGE
};

Page currentPage = DIAL_PAGE;  // Display starts with the dial page

// Dial page variables
uint16_t angle1 = 0;                    // Reads the angle at which the potentiometer is at for the tank (left) dial
uint16_t angle2 = 0;                    // Reads the angle at which the potentiometer is at for the table (right) dial
const int circleRadius = 23;            // Radius of the center circle which displays the texts for both dials
const int needleLength = 20;            // Length of the needle that rotates and matches the dials
const int compressed_air_reading = 19;  // Currently hard coded, but should map to the actual compressed air to get the readings for the amount of compressed air. 
const int table_dial_threshold = 20;    // A hardcoded threshold that indicates when the table dial should flash red if below this threshold. 

// Menu page variables
int menuCounter = 0;          // Indicates which menu option the user is on
float upperLimitVal = 25.0;   // Initial upper limit variable. This variable (as changed by the user) is stored permanently on the ESP32 flash memory
float lowerLimitVal = 19.0;   // Initial lower limit variable. This variable (as changed by the user) is stored permanently on the ESP32 flash memory

bool upperSelected = false;   // Indicates whether the upper limit menu option is selected
bool lowerSelected = false;   // Indicates whether the lower limit menu option is selected
bool backToDialSelected = false;  // Indicates whether the back to dial menu option is selected

// Screen dimensions and layout constants for the menu page
const int SCREEN_WIDTH = 320;   // width of the display
const int SCREEN_HEIGHT = 480;  // height of the display
const int LINE_HEIGHT = 30;     // determines the vertical spacing between lines of text
const int TEXT_SIZE = 2;        // text size for the options
const int HEADER_Y = 20;        // y-coordinate for Vacuum Gauge Sensor header
const int UPPER_Y = 70;         // y-coordinate for the upper limit option
const int LOWER_Y = 120;        // y-coordinate for the lower limit option
const int BACK_Y = 170;         // y-coordinate for the back to display option
const int STATUS_Y = 220;       // y-coordinate for indicating if the pump is on or off
const int CURRENT_Y = 270;      // y-coordinate for the current reading: output

bool refreshDisplay = true;     // Boolean indicating if the menu page has been updated and should be refreshed.
bool firstTimeDial = true;      // Boolean indicating if the user switched back to the dial page. If so then the background image is pushed again. This avoids repushing images, thus avoiding glitches. 

/*
  Indicates whether the compressed air pump is on or not. 
*/
enum State {
  COMPRESSED_AIR_OFF,
  COMPRESSED_AIR_ON,
};

// The pump is initially turned off. 
State currentState = COMPRESSED_AIR_OFF;

/*
  If a user is on the Dial page and clicks on the rotary encoder, the page changes to the Menu page.
  If a user is on the Menu page and is on the upper limit menu option and presses the button, the user can edit the number. 
  When pressed again they are able to rotate between the menu options. Same applies to the lower limit menu option.
  If a user presses on the back to dial menu option the Dial page is displayed again. 
*/
void rotary_onButtonClick() {
  //Serial.print("Button clicked! Page: "); Serial.print(currentPage);
  //Serial.print(", Menu: "); Serial.println(menuCounter);

  if (currentPage == DIAL_PAGE) {
    // Switch to menu page
    currentPage = MENU_PAGE;
    rotaryEncoder.setBoundaries(0, 2, true); // 3 menu items (0-2), circling
    rotaryEncoder.setEncoderValue(0);
    menuCounter = 0;
    refreshDisplay = true;
    //Serial.println("Switched to MENU_PAGE");
    return;
  }

  // Handle menu page button clicks
  switch(menuCounter) {
    case 0: // Upper Limit
      upperSelected = !upperSelected;
      //Serial.print("Upper selected: "); Serial.println(upperSelected);
      if (upperSelected) {
        rotaryEncoder.setBoundaries(235, 270, false); // 23.5 to 27.0 (multiply by 10)
        rotaryEncoder.setEncoderValue((int)(upperLimitVal * 10));
      }
      break;
    case 1: // Lower Limit
      lowerSelected = !lowerSelected;
      Serial.print("Lower selected: "); Serial.println(lowerSelected);
      if (lowerSelected) {
        rotaryEncoder.setBoundaries(155, 225, false); // 15.5 to 22.5 (multiply by 10)
        rotaryEncoder.setEncoderValue((int)(lowerLimitVal * 10));
      }
      break;
    case 2: // Back to Dial
      backToDialSelected = !backToDialSelected;
      Serial.print("Back to dial selected: "); Serial.println(backToDialSelected);
      if (backToDialSelected) {
        // Switch back to dial page
        currentPage = DIAL_PAGE;
        backToDialSelected = false;
        firstTimeDial = true;
        //Serial.println("Switched to DIAL_PAGE");
        return;
      }
      break;
  }
  
  if (!upperSelected && !lowerSelected && !backToDialSelected) {
    // Back to menu navigation mode
    //Serial.println("Back to menu mode");
    rotaryEncoder.setBoundaries(0, 2, true); // Menu has 3 items (0-2)
    rotaryEncoder.setEncoderValue(menuCounter);
  }
  refreshDisplay = true;
}

/*
  Function does not apply if the user is on the Dial page.
  On the menu page, the user is able to navigate between the three menu options with the rotary encoder. 
*/
void rotary_loop() {
  if (rotaryEncoder.isEncoderButtonClicked()) {
    rotary_onButtonClick();
  }

  if (rotaryEncoder.encoderChanged()) {
    int encoderValue = rotaryEncoder.readEncoder();
    //Serial.print("Encoder changed to: "); Serial.println(encoderValue);
    
    if (currentPage == DIAL_PAGE) {
      return;
    }
    
    // Handle menu page encoder changes
    if (upperSelected) {
      upperLimitVal = encoderValue / 10.0;
      preferences.putFloat("upperLimitVal", upperLimitVal);
      //Serial.print("Upper limit now: "); Serial.println(upperLimitVal);
      refreshDisplay = true;
    }
    else if (lowerSelected) {
      lowerLimitVal = encoderValue / 10.0;
      preferences.putFloat("lowerLimitVal", lowerLimitVal);
      //Serial.print("Lower limit now: "); Serial.println(lowerLimitVal);
      refreshDisplay = true;
    }
    else {
      // Menu navigation
      menuCounter = encoderValue;
      //Serial.print("Menu counter: "); Serial.println(menuCounter);
      refreshDisplay = true;
    }
  }
}

/*
  Called every time when an encoder pulse occurs. 
*/
void IRAM_ATTR readEncoderISR() {
  rotaryEncoder.readEncoder_ISR();
}

/*
  Start of the program. 
  Initializes display at the Dial page. 
*/
void setup() {
  Serial.begin(115200);
  
  preferences.begin("limit-vals", false);
  upperLimitVal = preferences.getFloat("upperLimitVal", upperLimitVal);
  lowerLimitVal = preferences.getFloat("lowerLimitVal", lowerLimitVal);

  // Initialize rotary encoder
  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  rotaryEncoder.disableAcceleration();
  
  // Initialize TFT display
  tft.init();
  tft.setRotation(1);
  tft.setSwapBytes(true);

  // Initialize sprites for dial page
  bck.createSprite(90, 90);
  bck.setSwapBytes(true);
  bck2.createSprite(90, 90);
  bck2.setSwapBytes(true);

  sprite1.createSprite(120, 120);
  sprite1.setPivot(60, 60);
  sprite1.setSwapBytes(true);

  sprite3.createSprite(120, 120);
  sprite3.setPivot(60, 60);
  sprite3.setSwapBytes(true);

  sprite2.createSprite(120, 120);
  sprite2.setSwapBytes(true);
  sprite2.loadFont(NotoSansMonoSCB20);
  
  sprite4.createSprite(120, 120);
  sprite4.setSwapBytes(true);
  sprite4.loadFont(NotoSansMonoSCB20);

  // Start with dial page
  currentPage = DIAL_PAGE;
  refreshDisplay = true;
}

/*
  Continously checks for user input.
  If the compressed air reading is less than the lower limit, the pump is turned on.
  It is turned off once it reaches the upper limit. 
  Reads tank and table pressure input if user is on Dial page. 
  Refreshes page if user is on Menu page if changes have been made. 
*/
void loop() {
  rotary_loop();
  
  // Control compressed air based on readings
  if (compressed_air_reading < lowerLimitVal && currentState == COMPRESSED_AIR_OFF) {
    currentState = COMPRESSED_AIR_ON;
    // Turn on pin here. 
    //Serial.println("Turned on compressed air valve");
    refreshDisplay = true;
  }
  else if (compressed_air_reading >= upperLimitVal && currentState == COMPRESSED_AIR_ON) {
    currentState = COMPRESSED_AIR_OFF;
    // Turn off pin here. 
    //Serial.println("Turned off compressed air valve");
    refreshDisplay = true;
  }
  
  if (currentPage == DIAL_PAGE){
    displayDialPage();
  }

  if (refreshDisplay) {
    if (currentPage != DIAL_PAGE) {
      displayMenuPage();
      refreshDisplay = false;
    }
  }
  
  //delay(10); // Small delay to prevent excessive updates
}

/*
  Reads tank and table pressure readings. 
  Maps the readings to values that match the dials. 
  Displays current compressed air reading. 
*/
void displayDialPage() {
  // Read ADC values for dial positions
  int rawADC1 = analogRead(39); // 0–4095, vacuum dial
  int rawADC2 = analogRead(36); // 0–4095, table fixture dial

  // Map ADC to needle rotation
  angle1 = map(rawADC1, 0, 4095, -120, 120);
  angle2 = map(rawADC2, 0, 4095, -120, 120);

  // Map ADC to number to display
  int displayed_value1 = map(rawADC1, 0, 4095, 0, 30);
  int displayed_value2 = map(rawADC2, 0, 4095, 0, 30);

  drawDials(displayed_value1, displayed_value2); 

  tft.loadFont(NotoSansMonoSCB20);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Air Pressure (psi):  ", 120, 20);
  tft.drawString(String(compressed_air_reading), 320, 20);
}

/*
  Displays menu page and calls updates to the page. 
*/
void displayMenuPage() {
  static bool firstTime = true;
  
  if (firstTime || refreshDisplay) {
    printMenuLCD();
    firstTime = false;
  }
  
  updateMenuLCD();
  updateCursorPosition();
  updateSelection();
}

/*
  Only pushes background image if user switches to Dial page, not continously.
  Formats the dials and readings using tft sprite objects. 
  If table reading is below a threshold then circle displaying the amount will flash red. 
*/
void drawDials(int displayed_value1, int displayed_value2) {
  // Clear background
  if (firstTimeDial){
    tft.pushImage(0, 0, 480, 320, dial);
    firstTimeDial = false;
  }
  
  bck.fillSprite(TFT_BLACK);
  bck2.fillSprite(TFT_BLACK);

  sprite2.fillSprite(TFT_BLACK);
  sprite4.fillSprite(TFT_BLACK);

  sprite1.fillSprite(TFT_BLACK);
  sprite3.fillSprite(TFT_BLACK);

  // Start point: just outside the circle edge
  int startY = 60 - circleRadius;
  int endY = startY - needleLength;

  // Draw needle (red)
  sprite1.drawLine(60, startY, 60, endY, TFT_RED);
  sprite3.drawLine(60, startY, 60, endY, TFT_RED);

  // Draw circle border
  sprite2.drawCircle(60, 60, circleRadius, TFT_WHITE);
  sprite4.drawCircle(60, 60, circleRadius, TFT_WHITE);

  // Draw number in center of cicrle
  String numStr1 = String(displayed_value1);
  String numStr2 = String(displayed_value2);
  int16_t x1 = (sprite2.width() - sprite2.textWidth(numStr1)) / 2;
  int16_t y1 = (sprite2.height() - sprite2.fontHeight()) / 2;
  int16_t x2 = (sprite4.width() - sprite4.textWidth(numStr2)) / 2;
  int16_t y2 = (sprite4.height() - sprite4.fontHeight()) / 2;
  sprite2.setTextColor(TFT_WHITE, TFT_BLACK);
  sprite2.drawString(numStr1, x1, y1);

  if (displayed_value2 < table_dial_threshold){
    static unsigned long prevTime = 0;
    static bool flash = false;
    const unsigned long FLASH_INTERVAL = 350;

    if (millis() - prevTime >= FLASH_INTERVAL){
      prevTime = millis();
      flash = !flash;
    }

    if (flash){
      sprite4.setTextColor(TFT_WHITE, TFT_WHITE);
      sprite4.fillCircle(60, 60, circleRadius, TFT_RED);
      sprite4.drawString(numStr2, x2, y2);
    }
    else{
      sprite4.setTextColor(TFT_WHITE, TFT_BLACK);
      sprite4.drawString(numStr2, x2, y2);
    }
  }
  else {
    sprite4.setTextColor(TFT_WHITE, TFT_BLACK);
    sprite4.drawString(numStr2, x2, y2);
  }

  // Push rotated circle with number sprite
  sprite2.pushRotated(&bck, 0, TFT_BLACK);
  sprite4.pushRotated(&bck2, 0, TFT_BLACK);

  // Push rotated needle sprite
  sprite1.pushRotated(&bck, angle1, TFT_BLACK);
  sprite3.pushRotated(&bck2, angle2, TFT_BLACK);

  // Display final image
  bck.pushSprite(70, 110);
  bck2.pushSprite(310, 110);

  // Add text labels below each circle
  tft.loadFont(NotoSansBold15);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  int circle1CenterX = 70 + 48;
  int circle2CenterX = 310 + 48;
  int textY = 110 + 90;
  
  String label1 = "(inHg)";
  int label1Width = tft.textWidth(label1);
  
  tft.drawString(label1, circle1CenterX - label1Width/2, textY);
  tft.drawString(label1, circle2CenterX - label1Width/2, textY);
  if (currentState == COMPRESSED_AIR_ON){
    tft.loadFont(NotoSansMonoSCB20);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("PUMP ON", 0, 300);
  }
}

/*
  Prints menu options and formats the menu. 
*/
void printMenuLCD() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextSize(TEXT_SIZE);
  tft.loadFont(NotoSansMonoSCB20);

  // Header
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("Vacuum Gauge Sensor", SCREEN_WIDTH/2, HEADER_Y);
  
  // Menu labels
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("  Upper Limit:", 20, UPPER_Y);
  tft.drawString("  Lower Limit:", 20, LOWER_Y);
  tft.drawString("  Back to Dial", 20, BACK_Y);
  
  // Status labels
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.drawString("Pump Status:", 20, STATUS_Y);
  tft.drawString("Current Reading:", 20, CURRENT_Y);
}

/*
  Update menu page as user makes changes. 
*/
void updateMenuLCD() {
  tft.setTextSize(TEXT_SIZE);
  
  // Clear and update upper limit value
  tft.fillRect(260, UPPER_Y, 120, LINE_HEIGHT, TFT_BLACK);
  tft.setTextColor(upperSelected ? TFT_GREEN : TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(String(upperLimitVal, 1), 260, UPPER_Y);
  
  // Clear and update lower limit value
  tft.fillRect(260, LOWER_Y, 120, LINE_HEIGHT, TFT_BLACK);
  tft.setTextColor(lowerSelected ? TFT_GREEN : TFT_WHITE, TFT_BLACK);
  tft.drawString(String(lowerLimitVal, 1), 260, LOWER_Y);
  
  // Update pump status
  tft.fillRect(280, STATUS_Y, 150, LINE_HEIGHT, TFT_BLACK);
  tft.setTextColor(currentState == COMPRESSED_AIR_ON ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.drawString(currentState == COMPRESSED_AIR_ON ? "ON" : "OFF", 280, STATUS_Y);
  
  // Update current reading
  tft.fillRect(280, CURRENT_Y, 120, LINE_HEIGHT, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(compressed_air_reading), 280, CURRENT_Y);
}

/*
  Display the cursor according to where the user is while rotating between menu options. 
*/
void updateCursorPosition() {
  if (!upperSelected && !lowerSelected && !backToDialSelected) {
    tft.setTextSize(TEXT_SIZE);
    
    // Clear previous cursor positions
    tft.fillRect(0, UPPER_Y, 25, LINE_HEIGHT, TFT_BLACK);
    tft.fillRect(0, LOWER_Y, 25, LINE_HEIGHT, TFT_BLACK);
    tft.fillRect(0, BACK_Y, 25, LINE_HEIGHT, TFT_BLACK);
    
    // Draw cursor at new position
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    
    switch(menuCounter) {
      case 0:
        tft.drawString(">", 0, UPPER_Y);
        break;
      case 1:
        tft.drawString(">", 0, LOWER_Y);
        break;
      case 2:
        tft.drawString(">", 0, BACK_Y);
        break;
    }
  }
}

/*
  If a menu option is selected change the cursor to an X.
*/
void updateSelection() {
  tft.setTextSize(TEXT_SIZE);
  
  // When a menu is selected ">" becomes "X"
  if (upperSelected) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("X", 0, UPPER_Y);
  }
  
  if (lowerSelected) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("X", 0, LOWER_Y);
  }
  
  if (backToDialSelected) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("X", 0, BACK_Y);
  }
}
