#include <ESP32Servo.h>
#include <Keypad.h>
#include <Ultrasonic.h>

#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""

// Insert Firebase project API Key
#define API_KEY "AIzaSyCREfPuEnVGNbEULqMfsyaf_NbCGFPSZog"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://esp32-garage-5da19-default-rtdb.europe-west1.firebasedatabase.app/"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

// Keypad
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {13, 12, 14, 27}; // Rows: 13, 12, 14, 27
byte colPins[COLS] = {5, 18, 19, 21};  // Columns: 5, 18, 19, 21
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Servo motor
#define SERVO_PIN 23

#define trigPin 25
#define echoPin 26
#define ULTRASONIC_UPDATE_INTERVAL 2000

// define sound speed in cm/uS
#define SOUND_SPEED 0.034

Servo servo;
Ultrasonic ultrasonic(trigPin, echoPin);

float distanceCm;

// Initialize servo motor
void setupServo()
{
  servo.attach(SERVO_PIN);
}

void lockDoor()
{
  servo.write(0);
}

void unlockDoor()
{
  servo.write(90);
}

void setupFirebase()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("ok");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void setup()
{
  Serial.begin(9600);
  setupServo();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  setupFirebase();
}

void loop()
{
  char key = keypad.getKey();

  if (key != NO_KEY)
  {
    if (key == '1' && authenticateUser())
    {
      Serial.println("User authenticated");
      if (clearSpot())
      {
        unlockDoor();
        Serial.println("Door unlocked");
        delay(2000); // Delay to avoid multiple unlocks with a single key press
        lockDoor();
        Serial.println("Door locked");
        delay(1000);
      }
      else
      {
        Serial.println("No spot");
      }
    }
    else
    {
      Serial.println("Invalid code");
    }
  }
}

bool clearSpot()
{
  unsigned long currentMillis = millis();
  static unsigned long previousMillis = 0;

  if (currentMillis - previousMillis >= ULTRASONIC_UPDATE_INTERVAL)
  {
    previousMillis = currentMillis;
    auto distance = ultrasonic.distanceRead(CM);
    registerReading(distance, "us/spot1/reading");
    return distance > 70;
  }
}

void registerReading(double reading, String path) {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.setInt(&fbdo, path, reading))
    {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

bool authenticateUser()
{
  String enteredCode = "";
  while (enteredCode.length() < 4)
  {
    char key = keypad.getKey();
    if (key != NO_KEY && isDigit(key))
    {
      enteredCode += key;
      delay(200);        // Delay to avoid reading the same key multiple times
      Serial.print("*"); // Print asterisks to simulate masking of the entered code
    }
  }
  Serial.println();
  return enteredCode.equals("1234");
}
