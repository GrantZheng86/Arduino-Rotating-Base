// Some libraries needed for this project

#include <ArducamSSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include <Math.h>

// Needed for the display
#define OLED_RESET  16
#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16
ArducamSSD1306 display(OLED_RESET);

const byte chanA = 2; // Encoder A channel
const byte chanB = 4; // Encoder B channel
const byte button = 3;// Button
const byte dir = 8;   // Direction channel for the stepper motor
const byte stepCmd = 9; // Steppind command channal for the stepper motor, steps on rising edge



// Menu stuff
volatile int tic;
int8_t menuLevel;   // Menu Level, this indicates if we should display the main page or the detailed next page
int8_t menuSelection; // This is indicates which tab on the main menu that we are on
uint8_t delayedTime;  // This is the time to delay for count down delay functionaily for the base
int finalAngle;       // That angle this thing should rotate
uint8_t timeDuration; // Rotate the "finalAngle" in this amount of time
uint8_t selectedMenu; // When button is pressed, the "selectedMenu" will tell the system what sub menu to display.



void setup() {

  // Some basic step up
  display.begin();
  Serial.begin(9600);
  pinMode(chanA, INPUT);
  pinMode(chanB, INPUT);
  pinMode(button, INPUT);
  pinMode(dir, OUTPUT);
  pinMode(stepCmd, OUTPUT);

  // Attach interrupt to the A channel and Button
  attachInterrupt(digitalPinToInterrupt(chanA), isrEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(button), isrButton, RISING);

  // Initialize variables
  menuLevel = 0;
  menuSelection = 0;
  delayedTime = 0;
  finalAngle = 0;
  tic = 0;
  timeDuration = 0;

}

void loop() {
  // The loop handles what to display and what action to take
  if (menuLevel == 0) {
    menuSelection = tic % 4;
    displayMainMenu(menuSelection);
  } else if (menuLevel == 1) {
    switch (selectedMenu) {
      case 0:
        displayDelayedTimeMenu();
        break;
      case 1:
        displayAngleMenu();
        break;
      case 2:
        displayTimeDurationMenu();
        break;
      case 3:
        startRotation();
        break;
    }
  }


}


// The interrupt service routine for the encoder
void isrEncoder() {
  static byte increaseClick = 0;
  static byte decreaseClick = 0;

  // This is uaually how encoder works, two different channels with a phase shift
  // This ISR must observe a full falling and rising cycle for the tic count to change
  // Which is the fame as the click filling from the encoder
  if (!digitalRead(chanA)) {
    if (digitalRead(chanB)) {
      increaseClick ++;
    } else {
      decreaseClick ++;
    }
  } else {
    if (!digitalRead(chanB)) {
      increaseClick ++;
    } else {
      decreaseClick ++;
    }
  }

  // Only when a cycle has been finished, i.e. decrease or increase click = 2, can the tic 
  // number change. Depends on the selected menu and menu level, different variables will
  // be changed in this section
  if (decreaseClick == 2) {
    tic --;
    if (menuLevel == 1 && menuSelection == 0) {
      delayedTime --;
    } else if (menuLevel == 1 && menuSelection == 1) {
      finalAngle --;
    } else if (menuLevel == 1 && menuSelection == 2) {
      timeDuration --;
    }
    increaseClick = 0;
    decreaseClick = 0;

  } else if (increaseClick == 2) {
    tic ++;
    if (menuLevel == 1 && menuSelection == 0) {
      delayedTime ++;
    } else if (menuLevel == 1 && menuSelection == 1) {
      finalAngle ++;
    } else if (menuLevel == 1 && menuSelection == 2) {
      timeDuration ++;
    }
    increaseClick = 0;
    decreaseClick = 0;

  }
  
  


  if (tic <= 0) {
    tic = 0;
  }

  Serial.println(tic);
}


// Button ISR with commonly applied time delay debounce, each click will increase the menu level
// and therefore, different content will be displayed on the screen.
void isrButton() {
  static unsigned long lastButtonTime = 0;
  unsigned long buttonTime = millis();
  if (buttonTime - lastButtonTime > 20) {
    selectedMenu = menuSelection;
    menuLevel ++;
    if (menuLevel > 1 ) {
      menuLevel = 0;
    }

  }
  lastButtonTime = buttonTime;
}

// The function that handles the main menu display, the selected item will be displayed in dark colored
// text and the bkackground for that line will be in white color.
// INPUT: currItem, and uint8_t number represent which line to invert the display color
void displayMainMenu(uint8_t currItem) {
  char firstChoice[] = "\t Set Time Delay";
  char secondChoice[] = "\t Set Rotation Angle";
  char theirChoice[] = "\t Set Time Duration";
  char execute[] = "\t Start";
  char *mainMenuStrings[] = {firstChoice, secondChoice, theirChoice, execute};
  uint8_t l = sizeof(mainMenuStrings) / sizeof(mainMenuStrings[0]);

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.setTextSize(1);
  display.println("Turn Table Menu\n");

  for (int i = 0; i < l; i ++) {
    display.setCursor(0, 8 * (i + 2));
    if (i != currItem) {
      display.setTextColor(WHITE);
    } else {
      display.setTextColor(BLACK, WHITE);
    }

    display.println(mainMenuStrings[i]);

  }
  display.display();
}


// Display the menu to let user set time delay
void displayDelayedTimeMenu() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.println("Set Time To Delay\n");

  display.print(delayedTime);
  display.print(" seconds");
  display.display();
}

// Display the menu to let user set angle of rotation
void displayAngleMenu() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.println("Set Rotation Angle\n");

  display.print(finalAngle);
  display.println(" degrees");
  display.display();
}


// Display the menu to let user set duration of 
// rotation of the device. In another word, the speed.
void displayTimeDurationMenu() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.println("Set Time Duration\n");

  display.print(timeDuration);
  display.println(" seconds");
  display.display();
}

// Starts the actural rotation
void startRotation() {
  if (finalAngle < 0) {
    digitalWrite(dir, 0);
  } else {
    digitalWrite(dir, 1);
  }

  unsigned int steps = abs(round((float)finalAngle / 0.09371f));
  unsigned int timeStep = timeDuration * 1000 / steps;
  countDown();
  for (int i = 0; i < steps; i ++) {
    digitalWrite(stepCmd, LOW);
    delay(timeStep);
    digitalWrite(stepCmd, HIGH);
  }
  menuLevel = 0;
}

// A count down feature to let user know how many seconds are left in the time delay
void countDown() {
  for (int i = 0; i <= delayedTime; i ++) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setCursor(10, 0);
    display.setTextSize(1);
    display.println("Rotation Count Down\n");

    display.setCursor(30, 20);
    display.setTextSize(3);
    display.println(delayedTime - i);
    delay(1000);
    display.display();
  }
}
