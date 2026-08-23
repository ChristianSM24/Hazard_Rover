#include <SPI.h>
#include <RF24.h>

// RF Module 
RF24 radio(48, 53);  // CE, CSN
const byte address[6] = "rover";


const int JOY_X   = A1;
const int JOY_Y   = A0;
const int JOY_BTN = 12;
const int DEADZONE = 25;

struct DataPacket {
  int joyX;
  int joyY;
  bool button;
};

struct ReceivePacket {
  float temperature;
  float humidity;
};

void setup() {
  pinMode(JOY_BTN, INPUT_PULLUP);
  Serial.begin(9600);

  radio.begin();
  radio.enableAckPayload();
  radio.enableDynamicPayloads();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening();  // transmitter mode

  Serial.println("Controller ready!");
}

void loop() {
  DataPacket data;

  data.joyX = map(analogRead(JOY_X), 0, 1023, -100, 100);
  data.joyY = map(analogRead(JOY_Y), 0, 1023, -100, 100);
  data.button = (digitalRead(JOY_BTN) == LOW);

  if (abs(data.joyX) < DEADZONE) data.joyX = 0;
  if (abs(data.joyY) < DEADZONE) data.joyY = 0;

  bool sent = radio.write(&data, sizeof(data));


  Serial.print("Sent: "); Serial.print(sent);
  Serial.print("  Button: "); Serial.print(data.button);
  Serial.print("  AckAvailable: "); Serial.println(radio.isAckPayloadAvailable());

  if (data.button && sent && radio.isAckPayloadAvailable()) {
    ReceivePacket response;
    radio.read(&response, sizeof(response));
    Serial.println("--- TEMPERATURE READING ---");
    Serial.print("Temperature: "); Serial.print(response.temperature); Serial.println(" °C");
    Serial.println("---------------------------");
  }

  delay(20);
}
