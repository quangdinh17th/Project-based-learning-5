#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// LCD initialization
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Pin definitions for ESP32
#define RX_PIN 17
#define TX_PIN 16
HardwareSerial mySerial(2); // Serial2 for ESP32
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// 4x4 Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27}; // ESP32 GPIO pins connected to keypad rows
byte colPins[COLS] = {26, 25, 33, 32}; // ESP32 GPIO pins connected to keypad columns
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

bool attendance[100] = {false};  // Keeps track of attendance for each person
int totalPeople = 100;           // Total number of people to enroll

bool inMainMenu = true;        // Tracks if the system is in the main menu
bool isEnrolling = false;      // Tracks if the system is currently enrolling fingerprints
const String managerPassword = "1709"; // Define the manager password

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  // Initialize fingerprint sensor
  lcd.setCursor(0, 0);
  lcd.print("Fingerprint setup");
  mySerial.begin(57600);

  if (finger.verifyPassword()) {
    lcd.setCursor(0, 1);
    lcd.print("Sensor detected");
    Serial.println("Fingerprint sensor detected!");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Sensor not found");
    Serial.println("Fingerprint sensor not found :(");
    while (1);
  }

  finger.getTemplateCount();
  lcd.setCursor(0, 2);
  lcd.print("Templates: ");
  lcd.print(finger.templateCount);
  Serial.print("Number of templates stored: ");
  Serial.println(finger.templateCount);
}

void loop() {
  if (!isEnrolling && inMainMenu) {
    showMainMenu();
    char key = keypad.getKey();  // Get the key pressed on the keypad

    if (key == 'A') {
      if (checkManagerPassword()) { // Check if the entered password is correct
        lcd.clear();
        lcd.print("Enroll New Finger");
        delay(2000);
        
        isEnrolling = true;  // Set enrolling state to true to block other actions
        for (int i = 1; i <= totalPeople; i++) {
          if (!attendance[i - 1]) {
            enrollFingerprint(i);
            break;
          }
        }
        isEnrolling = false;  // Reset enrolling state after completion
      } else {
        lcd.clear();
        lcd.print("Incorrect Password");
        delay(2000);
      }
    } else if (key == 'D') {
      if (checkManagerPassword()) { // Check if the entered password is correct
        lcd.clear();
        lcd.print("Enter ID to Delete:");
        
        char idToDelete = keypad.waitForKey();  // Wait for ID to delete
        int id = idToDelete - '0';  // Convert char to int

        if (id >= 0 && id <= totalPeople) {
          deleteFingerprint(id);
        } else {
          lcd.setCursor(0, 1);
          lcd.print("Invalid ID");
          delay(2000);
        }
      } else {
        lcd.clear();
        lcd.print("Incorrect Password");
        delay(2000);
      }
    } else if (key == 'C') {
      // Exit the main menu and enter attendance mode
      lcd.clear();
      lcd.print("Attendance Mode");
      delay(2000);
      inMainMenu = false;
    }

  } else if (!isEnrolling && !inMainMenu) {
    handleAttendance();  // Enter the attendance mode

    // Press 'C' to go back to the main menu
    char key = keypad.getKey();
    if (key == 'C') {
      lcd.clear();
      lcd.print("Back to Menu");
      delay(2000);
      inMainMenu = true;
    }
  }

  delay(100);
}

void showMainMenu() {
  lcd.setCursor(0, 0);
  lcd.print("Press A:Add D:Del");
  lcd.setCursor(0, 1);
  lcd.print("Press C:Attendance");
}

// Function to check manager password
bool checkManagerPassword() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Password:");
  String inputPassword = "";
  char key;
  
  // Wait for 4 digits
  for (int i = 0; i < 4; i++) {
    key = keypad.waitForKey();
    if (key != NO_KEY) {
      inputPassword += key; // Append the entered key to password
      lcd.setCursor(i, 1);
      lcd.print(key); // Display the entered key on the LCD
    }
  }

  return inputPassword == managerPassword; // Check if the entered password matches
}

// Function to enroll fingerprints
void enrollFingerprint(int id) {
  int p = -1;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enrolling ID: ");
  lcd.print(id);
  lcd.setCursor(0, 1);
  lcd.print("Place finger...");

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
      lcd.setCursor(0, 1);
      lcd.print("Waiting for finger...");
      delay(500);
    } else if (p == FINGERPRINT_OK) {
      lcd.setCursor(0, 1);
      lcd.print("Image taken    ");
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Error capturing ");
    }
  }

  p = finger.image2Tz(1);
  if (p == FINGERPRINT_OK) {
    lcd.setCursor(0, 2);
    lcd.print("Finger processed");
  } else {
    lcd.setCursor(0, 2);
    lcd.print("Processing error");
    isEnrolling = false;
    return;
  }

  lcd.setCursor(0, 3);
  lcd.print("Remove finger");
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  lcd.setCursor(0, 3);
  lcd.print("Place same finger");

  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
      lcd.setCursor(0, 3);
      lcd.print("Waiting for finger...");
      delay(500);
    } else if (p == FINGERPRINT_OK) {
      lcd.setCursor(0, 3);
      lcd.print("Image taken    ");
    } else {
      lcd.setCursor(0, 3);
      lcd.print("Error capturing ");
    }
  }

  p = finger.image2Tz(2);
  if (p == FINGERPRINT_OK) {
    lcd.setCursor(0, 3);
    lcd.print("Finger processed");
  } else {
    lcd.setCursor(0, 3);
    lcd.print("Processing error");
    isEnrolling = false;
    return;
  }

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    lcd.setCursor(0, 3);
    lcd.print("Prints matched  ");
  } else {
    lcd.setCursor(0, 3);
    lcd.print("Match failed    ");
    isEnrolling = false;
    return;
  }

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    lcd.setCursor(0, 3);
    lcd.print("ID: ");
    lcd.print(id);
    lcd.print(" stored");
    attendance[id - 1] = true;  // Mark attendance after enrollment
  } else {
    lcd.setCursor(0, 3);
    lcd.print("Store failed    ");
  }

  delay(2000);
  isEnrolling = false;  // Reset enrolling state after completion
}

// Function to delete a fingerprint
void deleteFingerprint(int id) {
  int p = finger.deleteModel(id);
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Deleted ID: ");
    lcd.print(id);
    attendance[id - 1] = false;  // Clear attendance
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Failed to delete");
  }
  delay(2000);
}

// Function to handle attendance
void handleAttendance() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place finger for");
  lcd.setCursor(0, 1);
  lcd.print("attendance check");

  int id = getFingerprintIDez();
  if (id != -1 && id <= totalPeople && !attendance[id - 1]) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Attendance marked");
    lcd.setCursor(0, 1);
    lcd.print("ID: ");
    lcd.print(id);
    Serial.println("Attendance marked for ID: " + String(id));

    attendance[id - 1] = true; // Mark attendance for that person
    delay(3000); // Display success message for 3 seconds
    lcd.clear();
  } else if (id != -1 && attendance[id - 1]) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Already attended");
    lcd.setCursor(0, 1);
    lcd.print("ID: ");
    lcd.print(id);
    delay(3000); // Display message for 3 seconds
    lcd.clear();
  }
  delay(100); // Small delay to prevent over-processing
}

// Simplified function to get fingerprint ID
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return -1;

  return finger.fingerID;
}
