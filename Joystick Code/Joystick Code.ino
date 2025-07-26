#include <Mouse.h>
#include <Keyboard.h>

// Pin definitions
const int buttonLeft = 2;         // Left mouse button
const int buttonRight = 3;        // Right mouse button
const int buttonBack = 4;         // Back browser button
const int buttonForward = 5;      // Forward browser button

const int joystickX = A0;         // Joystick X-axis
const int joystickY = A1;         // Joystick Y-axis
const int joystickButton = 6;     // Joystick button (SW pin)
const int speedControl = A2;      // NEW: Potentiometer for speed control

// Joystick config
const int deadZone = 10;
const int joyCenter = 512;
const int maxSpeed = 8;           // This is now the maximum possible speed
const int maxScroll = 3;          // Max scroll steps per loop

// Button states
int leftButtonState = HIGH;       // Current state of left button
int prevLeftButtonState = HIGH;   // Previous state of left button 
int rightButtonState = HIGH;      // Current state of right button
int prevRightButtonState = HIGH;  // Previous state of right button
int backButtonState = HIGH;       // Current state of back button
int prevBackButtonState = HIGH;   // Previous state of back button
int forwardButtonState = HIGH;    // Current state of forward button
int prevForwardButtonState = HIGH;// Previous state of forward button
int joyButtonState = HIGH;        // Current state of joystick button  
int prevJoyButtonState = HIGH;    // Previous state of joystick button

// Scrolling state
bool scrollMode = false;

// NEW: Speed factor variable
float speedFactor = 1.0;          // Will be adjusted by potentiometer

void setup() {
  // Initialize pin modes
  pinMode(buttonLeft, INPUT_PULLUP);  // Comment out if external pull-down is used
  pinMode(buttonRight, INPUT_PULLUP); // Comment out if external pull-down is used
  pinMode(buttonBack, INPUT_PULLUP);  // Back button
  pinMode(buttonForward, INPUT_PULLUP); // Forward button
  pinMode(joystickButton, INPUT_PULLUP);
  // No need to set pinMode for analog inputs (speedControl)
  
  // Optional debugging
  Serial.begin(9600);
  Serial.println("Pro Micro Mouse Controller Ready");
  
  // Initialize control interfaces
  Mouse.begin();
  Keyboard.begin();
  
  // Make sure all buttons are released at startup
  Mouse.release(MOUSE_LEFT);
  Mouse.release(MOUSE_RIGHT);
  
  // Give the computer time to recognize the device
  delay(1000);
}

void loop() {
  // Read the current state of inputs
  int xValue = analogRead(joystickX);
  int yValue = analogRead(joystickY);
  
  // NEW: Read potentiometer value and calculate speed factor
 
int potValue = analogRead(speedControl);
speedFactor = map(potValue, 0, 1023, 100, 300) / 100.0;  // Map to 1.0 - 3.0 range
  
  // Store previous button states before reading new ones
  prevLeftButtonState = leftButtonState;
  prevRightButtonState = rightButtonState;
  prevBackButtonState = backButtonState;
  prevForwardButtonState = forwardButtonState;
  prevJoyButtonState = joyButtonState;
  
  // Read all button states (LOW when pressed with INPUT_PULLUP, HIGH when pressed with your wiring)
  leftButtonState = digitalRead(buttonLeft);
  rightButtonState = digitalRead(buttonRight);
  backButtonState = digitalRead(buttonBack);
  forwardButtonState = digitalRead(buttonForward);
  joyButtonState = digitalRead(joystickButton);

  // ---- HANDLE JOYSTICK BUTTON FOR SCROLLING ----
  // Toggle scroll mode on button press (not hold)
  if (joyButtonState != prevJoyButtonState) {
    if (joyButtonState == HIGH) {  // REVERSED: HIGH means pressed with this wiring
      // Toggle scroll mode
      scrollMode = !scrollMode;
      // Update LED indicator
    
      Serial.print("Scroll mode: ");
      Serial.println(scrollMode ? "ON" : "OFF");
      delay(50); // Debounce delay
    }
  }
  
  // ---- HANDLE MOVEMENT OR SCROLLING ----
  if (scrollMode) {
    // Scroll mode: Y-axis controls scrolling
    int scrollAmount = calculateScroll(yValue, joyCenter);
    if (scrollAmount != 0) {
      Mouse.move(0, 0, scrollAmount);
    }
  } else {
    // Normal mode: move mouse cursor
    int xMove = calculateMovement(xValue, joyCenter);
    int yMove = calculateMovement(yValue, joyCenter);
    if (abs(xMove) > 0 || abs(yMove) > 0) {
      Mouse.move(xMove, yMove, 0);
    }
  }

  // ---- HANDLE LEFT MOUSE BUTTON (ONLY ON STATE CHANGE) ----
  if (leftButtonState != prevLeftButtonState) {
    if (leftButtonState == HIGH) {  // REVERSED: HIGH means pressed with this wiring
      // Button pressed - send a press command
      Serial.println("LEFT PRESSED");
      Mouse.press(MOUSE_LEFT);
    } else {  // LOW means released with this wiring
      // Button released - send a release command
      Serial.println("LEFT RELEASED");
      Mouse.release(MOUSE_LEFT);
    }
  }

  // ---- HANDLE RIGHT MOUSE BUTTON (ONLY ON STATE CHANGE) ----
  if (rightButtonState != prevRightButtonState) {
    if (rightButtonState == HIGH) {  // REVERSED: HIGH means pressed with this wiring
      // Button pressed - send a press command
      Serial.println("RIGHT PRESSED");
      Mouse.press(MOUSE_RIGHT);
    } else {  // LOW means released with this wiring
      // Button released - send a release command
      Serial.println("RIGHT RELEASED");
      Mouse.release(MOUSE_RIGHT);
    }
  }
  
  // ---- HANDLE BACK BUTTON (BROWSER BACK) ----
  if (backButtonState != prevBackButtonState) {
    if (backButtonState == HIGH) {  // REVERSED: HIGH means pressed with this wiring
      Serial.println("BACK PRESSED");
      // Send Alt+Left Arrow keyboard shortcut for browser back
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press(KEY_LEFT_ARROW);
      delay(50);
      Keyboard.releaseAll();
    }
  }
  
  // ---- HANDLE FORWARD BUTTON (BROWSER FORWARD) ----
  if (forwardButtonState != prevForwardButtonState) {
    if (forwardButtonState == HIGH) {  // REVERSED: HIGH means pressed with this wiring
      Serial.println("FORWARD PRESSED");
      // Send Alt+Right Arrow keyboard shortcut for browser forward
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press(KEY_RIGHT_ARROW);
      delay(50);
      Keyboard.releaseAll();
    }
  }
  
  // Small delay for stability
  delay(20);
}

// Function to calculate cursor movement based on joystick position
int calculateMovement(int value, int center) {
  int movement = 0;
  
  // Calculate distance from center
  int distance = value - center;
  
  // Apply deadzone
  if (abs(distance) > deadZone) {
    // Map to cursor speed, applying non-linear curve for better control
    if (distance > 0) {
      movement = map(distance, deadZone, 1023-center, 1, maxSpeed);
    } else {
      movement = map(distance, -deadZone, -center, -1, -maxSpeed);
    }
    
    // Apply some exponential response for more precise control
    movement = (movement * abs(movement)) / maxSpeed;
    
    // NEW: Apply speed factor from potentiometer
    movement = movement * speedFactor;
  }
  
  return movement;
}

// Function to calculate scroll movement with reversed logic
int calculateScroll(int value, int center) {
  int scrollAmount = 0;
  
  // Calculate distance from center
  int distance = value - center;
  
  // Apply deadzone
  if (abs(distance) > deadZone) {
    // REVERSED LOGIC: For scrolling
    if (distance > 0) {
      scrollAmount = -1;  // Scroll up (reversed)
    } else {
      scrollAmount = 1;   // Scroll down (reversed)
    }
  }
  
  return scrollAmount;
}