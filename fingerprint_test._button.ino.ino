#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

#define RX_PIN 17
#define TX_PIN 16
#define BUTTON_ENROLL_PIN 12  // Button pin for enrollment
#define BUTTON_RANDOM_DELETE_PIN 13  // Button pin for random deletion
#define BUTTON_ORDER_PIN 14  // Button pin for ordering fingerprints

HardwareSerial mySerial(2); // Serial2 for ESP32
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

bool attendance[3] = {false};
int totalPeople = 3; // Maximum number of fingerprints that can be enrolled

void setup() 
{
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  pinMode(BUTTON_ENROLL_PIN, INPUT_PULLUP);  // Enrollment button
  pinMode(BUTTON_RANDOM_DELETE_PIN, INPUT_PULLUP);  // Random deletion button
  pinMode(BUTTON_ORDER_PIN, INPUT_PULLUP);  // Ordering button

  // Initialize fingerprint sensor
  lcd.setCursor(0, 0);
  lcd.print("Fingerprint setup");
  mySerial.begin(57600);

  if (finger.verifyPassword()) 
  {
    lcd.setCursor(0, 1);
    lcd.print("Sensor detected");
    Serial.println("Fingerprint sensor detected!");
  }
  else 
  {
    lcd.setCursor(0, 1);
    lcd.print("Sensor not found");
    Serial.println("Fingerprint sensor not found :(");
    while (1);
  }

  finger.getTemplateCount();
  lcd.setCursor(0, 2);
  lcd.print("Templates: ");
  lcd.print(finger.templateCount);

  // Enroll initial fingerprints
  for (int i = 1; i <= totalPeople; i++) 
  {
    enrollFingerprint(i); // Enroll fingerprint for each ID from 1 to totalPeople
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enrollment done!");
  delay(2000);
}

void loop() 
{
  // Attendance check
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

  // Check if the button for new fingerprint enrollment is pressed
  if (digitalRead(BUTTON_ENROLL_PIN) == LOW) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enroll new finger");
    totalPeople++;  // Increment totalPeople and enroll new fingerprint
    enrollFingerprint(totalPeople);
  }

  // Check if the button for random deletion is pressed
  if (digitalRead(BUTTON_RANDOM_DELETE_PIN) == LOW) {
    deleteRandomFingerprint(); // Call function to delete a random fingerprint
    delay(200); // Debounce delay
  }

  // Check if the button for ordering fingerprints is pressed
  if (digitalRead(BUTTON_ORDER_PIN) == LOW) {
    orderFingerprints(); // Call function to order fingerprints
  }

  delay(100); // Small delay to prevent over-processing
}

// Function to enroll fingerprints
void enrollFingerprint(int id) 
{
  int p = -1;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enrolling ID: ");
  lcd.print(id);
  lcd.setCursor(0, 1);
  lcd.print("Place finger...");

  while (p != FINGERPRINT_OK) 
  {
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

  // Convert the image to a template
  p = finger.image2Tz(1);
  if (p == FINGERPRINT_OK) {
    lcd.setCursor(0, 2);
    lcd.print("Finger processed");
  } else {
    lcd.setCursor(0, 2);
    lcd.print("Processing error");
    return;
  }

  // Ask the user to remove the finger
  lcd.setCursor(0, 3);
  lcd.print("Remove finger");
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  // Ask the user to place the same finger again
  lcd.setCursor(0, 3);
  lcd.print("Place same finger");

  // Capture the same finger again
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

  // Convert the second image to a template
  p = finger.image2Tz(2);
  if (p == FINGERPRINT_OK) {
    lcd.setCursor(0, 3);
    lcd.print("Finger processed");
  } else {
    lcd.setCursor(0, 3);
    lcd.print("Processing error");
    return;
  }

  // Create a model from both images and store it in the sensor's memory
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    lcd.setCursor(0, 3);
    lcd.print("Prints matched  ");
  } else {
    lcd.setCursor(0, 3);
    lcd.print("Match failed    ");
    return;
  }

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    lcd.setCursor(0, 3);
    lcd.print("ID: ");
    lcd.print(id);
    lcd.print(" stored");
  } else {
    lcd.setCursor(0, 3);
    lcd.print("Store failed    ");
  }

  delay(2000);
}

// Function to delete a random fingerprint
void deleteRandomFingerprint() 
{
  if (totalPeople <= 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No fingerprints!");
    delay(2000);
    return;
  }

  // Generate a random ID to delete
  int randomID = random(1, totalPeople + 1); // Random number between 1 and totalPeople
  int p = finger.deleteModel(randomID);

  // Display deletion result
  lcd.clear();
  lcd.setCursor(0, 0);
  if (p == FINGERPRINT_OK) {
    lcd.print("Deleted ID: ");
    lcd.print(randomID);
    Serial.println("Deleted fingerprint with ID: " + String(randomID));
  } else {
    lcd.print("Delete failed");
    Serial.println("Failed to delete fingerprint.");
  }

  delay(3000); // Delay for user readability
}

// Function to order fingerprints
void orderFingerprints() 
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ordering fingers");
  
  // Placeholder for actual ordering logic.
  lcd.setCursor(0, 1);
  for (int i = 1; i <= totalPeople; i++) {
    lcd.print("ID: ");
    lcd.print(i);
    delay(500);  // Delay for readability
    lcd.clear(); // Clear before showing the next ID
  }

  lcd.setCursor(0, 0);
  lcd.print("Ordering done!");
  delay(2000); // Delay for the user to read
}

// Simplified function to get fingerprint ID
int getFingerprintIDez() 
{
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return -1;

  return finger.fingerID; // Return the recognized fingerprint ID
}
