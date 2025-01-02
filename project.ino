include <Wire.h>                   // Include Wire library for I2C communication
#include "DHT.h"                    // Include DHT sensor library for temperature and humidity
#include <WiFi.h>                   // Include WiFi library for ESP32 WiFi functions
#include <WiFiClientSecure.h>       // Include WiFiClientSecure library for secure WiFi communication
#include <UniversalTelegramBot.h>   // Include UniversalTelegramBot library for Telegram bot functionality

// WiFi credentials
#define WIFI_SSID "reeree"        // Replace with your WiFi SSID
#define WIFI_PASSWORD "sam99sam99"         // Replace with your WiFi password

// Telegram bot token
#define BOT_TOKEN "6649287060:AAFaXZnguPGJrJRZN-axunHKfvWsKroH3w8"  // Replace with your Telegram bot token
String chat_id = "694564907";     // Replace with your chat ID

WiFiClientSecure secured_client;    // Secure WiFi client
UniversalTelegramBot bot(BOT_TOKEN, secured_client); // Initialize bot with token and secure client

const unsigned long BOT_MTBS = 500; // Mean time between scan messages (500 milliseconds)
unsigned long bot_lasttime;         // Last time messages were scanned
bool Start = false;                 // Start flag (not used)

// Pin definitions
#define DHTPIN 18                   // Pin for DHT sensor
#define DHTTYPE DHT11               // DHT sensor type (DHT11)
DHT dht(DHTPIN, DHTTYPE);           // Initialize DHT sensor

int FlamePin = 35;                  // Pin for flame sensor
int SoilDigitalPin = 34;            // Pin for soil moisture sensor
const int relyFan = 4;              // Pin for fan relay
//const int relyLight = 32;         // Pin for light relay (commented out)
const int relyPump = 21;            // Pin for pump relay

int flag = 0;                       // Flag to control pump state
int sensorDigitalValue = 0;         // Variable for soil moisture sensor value
float t = 0;                        // Variable for temperature
float h = 0;                        // Variable for humidity
int sensorReading = 0;              // Variable for flame sensor value

// Function to handle new messages from Telegram
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));
 
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    if (from_name == "") from_name = "Guest";

    // Handle /send_test_action command
    if (text == "/send_test_action") {
      bot.sendChatAction(chat_id, "typing");
      delay(4000);
      bot.sendMessage(chat_id, "Did you see the action message?");
    }

    // Handle /fan_on command
    if (text == "/fan_on") {
      bot.sendMessage(chat_id, "fan is on");
      digitalWrite(relyFan, HIGH); // Turn fan on
    }

    // Handle /fan_off command
    if (text == "/fan_off") { 
      bot.sendMessage(chat_id, "fan is off");
      digitalWrite(relyFan, LOW); // Turn fan off
    }

    // Handle /pump_on command
    if (text == "/pump_on") {
      bot.sendMessage(chat_id, "pump is on");
      if (sensorDigitalValue < 50) {
        flag = 1;
        digitalWrite(relyPump, HIGH); // Turn pump on
      } else {
        digitalWrite(relyPump, LOW);
        bot.sendMessage(chat_id, "High Humidity pump is off ");
      }
    }

    // Handle /pump_off command
    if (text == "/pump_off") {
      bot.sendMessage(chat_id, "pump is off");
      digitalWrite(relyPump, LOW); // Turn pump off
    }

    // Handle /data command
    if (text == "/data") {
      String temp_str = "Temp: " + String(t) + "°C \n";
      String hum_str = "Hum: " + String(h) + "% \n";
      String Soil_str = "Soil Hum:" + String(sensorDigitalValue) + "% \n";
      bot.sendMessage(chat_id, temp_str);
      bot.sendMessage(chat_id, hum_str);
      bot.sendMessage(chat_id, Soil_str);
    }

    // Handle /options command
    if (text == "/options") {
      String keyboardJson = "[[\"/pump_on\", \"/pump_off\"],[\"/fan_on\", \"/fan_off\"],[\"/data\"]]";
      bot.sendMessageWithReplyKeyboard(chat_id, "Choose from one of the following options", "", keyboardJson, true);
    }

    // Handle /start command
    if (text == "/start") {
      String welcome = "Welcome to Universal Arduino Telegram Bot library, " + from_name + ".\n";
      welcome += "This is Chat Action Bot example.\n\n";
      welcome += "/send_test_action : to send test chat action message\n";
      bot.sendMessage(chat_id, welcome);
    }
  }
}

// Setup function
void setup() {
  Serial.begin(9600);             // Initialize serial communication at 9600 baud rate
  Serial.println(F("DHTxx test!"));
  dht.begin();                    // Initialize DHT sensor
  pinMode(FlamePin, INPUT);       // Set flame sensor pin as input
  pinMode(SoilDigitalPin, INPUT); // Set soil moisture sensor pin as input
  pinMode(relyFan, OUTPUT);       // Set fan relay pin as output
  pinMode(relyPump, OUTPUT);      // Set pump relay pin as output
  // pinMode(relyLight, OUTPUT);   // Set light relay pin as output (commented out)
  connectToWiFi();                // Connect to WiFi
  bot.sendMessage(chat_id, "Project Online");
  digitalWrite(relyPump, LOW);    // Turn off pump relay
  digitalWrite(relyFan, LOW);     // Turn off fan relay
  // digitalWrite(relyLight, LOW); // Turn off light relay (commented out)
}

// Main loop function
void loop() {
  if (sensorDigitalValue > 50 && flag == 1) {
    flag = 0;
    digitalWrite(relyPump, LOW); // Turn off pump if high humidity
    bot.sendMessage(chat_id, "High Humidity pump is off ");
  }

  sensorDigitalValue = analogRead(SoilDigitalPin); // Read soil moisture sensor value
  h = dht.readHumidity();        // Read humidity from DHT sensor
  t = dht.readTemperature();     // Read temperature from DHT sensor
  sensorReading = analogRead(FlamePin); // Read flame sensor value
  sensorDigitalValue = map(sensorDigitalValue, 0, 4095, 100, 0); // Map soil moisture value to percentage

  // Print sensor values to serial monitor
  Serial.print("Soil sensor digital value: ");
  Serial.println(sensorDigitalValue);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.println(F("°C "));
  Serial.print(F("%  Humidity: "));
  Serial.println(h);
  Serial.print(F("Flame sensor reading: "));
  Serial.println(sensorReading);

  // Check for new messages from Telegram bot
  if (millis() - bot_lasttime > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    bot_lasttime = millis();
  }

  // Handle flame sensor detection
  if (sensorReading < 500) {
    bot.sendMessage(chat_id, "Fire detected");
    digitalWrite(relyPump, HIGH); // Turn on pump if fire detected
    delay(500);
  } else if (sensorReading > 500 && flag == 0) {
    digitalWrite(relyPump, LOW); // Turn off pump if no fire detected
  }
}

// Function to connect to WiFi
void connectToWiFi() {
  Serial.print("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Set root certificate for secure connection
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\n");
  Serial.print("WiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  // Retrieve time from NTP server
  Serial.print("Retrieving time: ");
  configTime(0, 0, "pool.ntp.org");
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);
}
