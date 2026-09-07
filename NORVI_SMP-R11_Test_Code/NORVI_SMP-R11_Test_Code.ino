//5318_NORVI-SMP-R11-V9.1 


#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ModbusMaster.h>
ModbusMaster node;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LED_PIN 23     // Pin connected to the data input of the LED
#define NUM_LEDS 1     // Number of LEDs (1 in this case)

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

#define SDA   21     
#define SCL   22     

#define RELAY 12

#define PB_IN 16

#define GSM_RX 14
#define GSM_TX 15
#define GSM_RESET 13

#define JSY_TX   26     
#define JSY_RX   25

bool touch_input = 0;
bool temp_relay = 0;

unsigned long int timer1 = 0;

float voltageData;
float currentData;
float activepower;
float activeenergy;
float powerfactor;
float frequency;


unsigned long int millis_start_modbus = 0;
unsigned long int modbus_interval = 2000;

const char* ssid     = "ICONIC DEVICES (PVT) LTD"; // Change this to your WiFi SSID
const char* password = "bb2057756"; // Change this to your WiFi password


void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, GSM_RX, GSM_TX);
  delay(1000);

  Serial2.begin(9600, SERIAL_8N1, JSY_RX, JSY_TX);
  node.begin(1, Serial2);  // Initialize Modbus communication

  Serial.println("5318_NORVI-SMP-R11-V9.1");
  delay(1000);

  Wire.begin(SDA, SCL);

  I2C_SCAN();
  delay(1000);

  pinMode(PB_IN,  INPUT);

  pinMode(RELAY, OUTPUT);
  pinMode(GSM_RESET, OUTPUT); 

  digitalWrite(GSM_RESET, LOW);
  delay(1000); 

  digitalWrite(RELAY,HIGH);   /* Turn ON output*/ 
  delay(1000);
  digitalWrite(RELAY,LOW);   /* Turn Off output*/ 

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.display();
  display.clearDisplay();
  display.display();

  Serial.print("Connecting to ");
  Serial.println(ssid);

  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.println(F("WELCOME!"));
  display.display();

 WiFi.begin(ssid, password);

 while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  RGB_DISP();
}

void loop() {
  
  strip.setPixelColor(0, strip.Color(0, 255, 255)); // Cyan
  strip.show();
  delay(200); // Wait for 1 second

  strip.setPixelColor(0, strip.Color(0, 0, 0)); // Off
  strip.show();
  
  if((millis()-millis_start_modbus)>modbus_interval){
    millis_start_modbus=millis();
    modbus_read();
  }

  while (Serial.available()) {
    int inByte = Serial.read();
    Serial1.write(inByte);
  }

  while (Serial1.available()) {
    int inByte = Serial1.read();
    Serial.write(inByte);
  }

  Read_IO();
  Output_control();
  OLED_Update();
  
}

float convertToValue(uint16_t rawData, float scaleFactor) {
  return static_cast<float>(rawData) * scaleFactor; // Convert to desired unit
}

void modbus_read() {

  uint8_t result;  // Variable to store the Modbus communication result
  uint16_t data[10]; // Array to store received data

  // Send Modbus request (01 03 01 00 00 03 04 37)
  result = node.readHoldingRegisters(0x0048, 10);

  // Check if Modbus communication was successful
  if (result == node.ku8MBSuccess) {
    // Get the received data
    data[0] = node.getResponseBuffer(0);
    data[1] = node.getResponseBuffer(1);
    data[2] = node.getResponseBuffer(2);

    data[3] = node.getResponseBuffer(3);
    data[4] = node.getResponseBuffer(4);
    data[5] = node.getResponseBuffer(5);
    data[6] = node.getResponseBuffer(6);
    data[7] = node.getResponseBuffer(7);
    data[8] = node.getResponseBuffer(8);
    
    // Convert received data to voltage values
    voltageData   = 0;
    currentData   = 0;
    activepower   = 0;
    activeenergy  = 0;
    powerfactor   = 0;
    frequency     = 0;

    voltageData = convertToValue(data[0], 0.01);
    currentData = convertToValue(data[1]+data[2], 0.0001);
    activepower = convertToValue(data[3]+data[4], 0.01);
    activeenergy = convertToValue(data[5]+data[6], 0.01);
    powerfactor  = convertToValue(data[7], 0.001);
    frequency  = convertToValue(data[8], 0.01);
    
    // Convert received data to current values
    Serial.print("Voltage "); Serial.print(voltageData);Serial.print(" currentData "); Serial.print(currentData);Serial.print(" activepower "); Serial.print(activepower);
    Serial.print(" activeenergy "); Serial.print(activeenergy);Serial.print(" powerfactor "); Serial.print(powerfactor);Serial.print(" frequency "); Serial.print(frequency);

    Serial.println("");
    Serial.println("");
    Serial.println("");


  } else {
    // Print an error message if communication failed
    Serial.print("Modbus communication error: ");
    Serial.println(result);
  }
}

void Read_IO(){
  Serial.print("PB Value : "); Serial.println(digitalRead(PB_IN));
  
}

void Output_control(){
  if((digitalRead(PB_IN)==0)&&(temp_relay==0)){
    temp_relay=1;
    digitalWrite(RELAY, HIGH);
    delay(500);
  }
  else if((digitalRead(PB_IN)==0)&&(temp_relay==1)){
    temp_relay=0;
    digitalWrite(RELAY, LOW);
    delay(500);
  }
}

void OLED_Update(){
  display.clearDisplay();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.print(F("Touch Value: "));
  display.println(digitalRead(PB_IN));
  display.print(F("Voltage: "));
  display.println(voltageData);
  display.print(F("Current: "));
  display.println(currentData);
  
  display.display();

}

void I2C_SCAN() {
  byte error, address;
  int deviceCount = 0;

  Serial.println("Scanning...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);
      Serial.println("  !");

      deviceCount++;
      delay(1);  // Wait for a moment to avoid overloading the I2C bus
    }
    else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }

  if (deviceCount == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("Scanning complete\n");
  }
}

void RGB_DISP(){
//  Cycle through colors
  strip.setPixelColor(0, strip.Color(255, 0, 0)); // Red
  strip.show();
  delay(500); // Wait for 1 second

  strip.setPixelColor(0, strip.Color(0, 255, 0)); // Green
  strip.show();
  delay(500); // Wait for 1 second

  strip.setPixelColor(0, strip.Color(0, 0, 255)); // Blue
  strip.show();
  delay(500); // Wait for 1 second

  strip.setPixelColor(0, strip.Color(255, 255, 0)); // Yellow
  strip.show();
  delay(500); // Wait for 1 second

  strip.setPixelColor(0, strip.Color(0, 255, 255)); // Cyan
  strip.show();
  delay(500); // Wait for 1 second

  strip.setPixelColor(0, strip.Color(255, 0, 255)); // Magenta
  strip.show();
  delay(500); // Wait for 1 second

  strip.setPixelColor(0, strip.Color(255, 255, 255)); // White
  strip.show();
  delay(500); // Wait for 1 second

  strip.setPixelColor(0, strip.Color(0, 0, 0)); // Off
  strip.show();
  delay(500); // Wait for 1 second
}
