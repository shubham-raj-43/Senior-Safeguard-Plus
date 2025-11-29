#define BLYNK_TEMPLATE_ID "TMPL3qjD2u4I3"
#define BLYNK_TEMPLATE_NAME "Fall detection"
#define BLYNK_AUTH_TOKEN "Fwbvo8eY6ENv3ZsLVO_p39gMNBVqUwbR"

#include <HX711.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

#define LOADCELL_DOUT1 4  // Data pins for HX711s (Load Cell 1)
#define LOADCELL_SCK1  5  // Clock pins for HX711s (Load Cell 1)
#define LOADCELL_DOUT2 18 // Load Cell 2
#define LOADCELL_SCK2  19 // Load Cell 2
#define LOADCELL_DOUT3 21 // Load Cell 3
#define LOADCELL_SCK3  22 // Load Cell 3
#define LOADCELL_DOUT4 25 // Load Cell 4
#define LOADCELL_SCK4  26 // Load Cell 4

#define BUZZER_PIN 15      // Buzzer Pin
#define RED_LED_PIN 13     // Red LED
#define GREEN_LED_PIN 12   // Green LED
#define RELAY_PIN 27       // Relay for Solenoid Lock
#define BUTTON_PIN 14      // Push Button

// Blynk credentials
char auth[] = "Fwbvo8eY6ENv3ZsLVO_p39gMNBVqUwbR";
char ssid[] = "ROBOTECH";
char pass[] = "90909090";

HX711 scale1, scale2, scale3, scale4; // Initialize HX711 objects for load cells

bool fall_detected = false;
bool button_pressed = false;
unsigned long fall_time = 0;
const int fall_delay = 5000; // 5 seconds delay to unlock solenoid
const float weight_threshold = 0.1; // Small threshold for weight detection (0.1 kg)
bool weight_changed = false; // Track if the weight has changed from 0

void setup() {
  Serial.begin(115200);
  
  // Initialize Blynk
  Blynk.begin(auth, ssid, pass);

  // Initialize HX711s for all load cells
  scale1.begin(LOADCELL_DOUT1, LOADCELL_SCK1);
  scale2.begin(LOADCELL_DOUT2, LOADCELL_SCK2);
  scale3.begin(LOADCELL_DOUT3, LOADCELL_SCK3);
  scale4.begin(LOADCELL_DOUT4, LOADCELL_SCK4);

  // Calibrate scales (tare)
  scale1.tare();
  scale2.tare();
  scale3.tare();
  scale4.tare();

  // Set scale factor for each scale (adjust this based on calibration weight)
  scale1.set_scale(2280.f);  // Change '2280.f' based on your load cell calibration
  scale2.set_scale(2280.f);
  scale3.set_scale(2280.f);
  scale4.set_scale(2280.f);

  // Initialize Pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Default state: Green LED ON (waiting for fall detection)
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW); // Solenoid locked
  digitalWrite(BUZZER_PIN, LOW); // Buzzer off
}

void loop() {
  Blynk.run();

  // Read weight from all 4 load cells with reduced averaging for faster response
  float weight1 = scale1.get_units(1);
  float weight2 = scale2.get_units(1);
  float weight3 = scale3.get_units(1);
  float weight4 = scale4.get_units(1);

  // Sum the weight from all load cells
  float total_weight = weight1 + weight2 + weight3 + weight4;

  // Print total weight quickly to the Serial Monitor
  Serial.print("Total Weight: ");
  Serial.println(total_weight);

  // Send total weight to Blynk virtual pin for live monitoring
  Blynk.virtualWrite(V0, total_weight);

  // Check if the weight has changed from 0 grams
  if (!weight_changed && total_weight > 0) {
    weight_changed = true; // Mark that weight has changed from 0
    Serial.println("Weight has changed from 0 grams.");
  }

  // Trigger fall detection condition with a smaller threshold
  if (weight_changed && total_weight > weight_threshold && !fall_detected) {
    fall_detected = true;
    fall_time = millis();  // Record time of fall detection

    // Blynk notification for fall detection
    Blynk.logEvent("fall_detection", "Someone has fallen, help him quickly!");

    // Activate buzzer, red LED, deactivate green LED
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);

    Serial.println("Fall detected! Sending alert...");
  }

  // If button is pressed within 5 seconds, fall condition is canceled
  if (digitalRead(BUTTON_PIN) == LOW && fall_detected && (millis() - fall_time < fall_delay)) {
    button_pressed = true;

    // Blynk notification for false alarm
    Blynk.logEvent("fall_detected", "Fall detection but everything okay");

    // Reset buzzer and LEDs
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);

    Serial.println("Button pressed, false alarm.");

    fall_detected = false;  // Reset fall detection
  }

  // If 5 seconds have passed and button not pressed, unlock solenoid
  if (fall_detected && !button_pressed && (millis() - fall_time > fall_delay)) {
    Serial.println("Unlocking solenoid...");
    digitalWrite(RELAY_PIN, HIGH);  // Unlock solenoid

    delay(1000); // Solenoid stays unlocked for 1 second
    digitalWrite(RELAY_PIN, LOW);   // Relock solenoid
    fall_detected = false;          // Reset fall detection

    // Turn off buzzer, reset LEDs
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
  }
}
