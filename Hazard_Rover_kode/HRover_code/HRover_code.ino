#include <SPI.h>
#include <RF24.h>
#include <Servo.h>
#include <DHT.h>

// RF Module
RF24 radio(48, 53);
const byte address[6] = "rover";

// DHT22
#define DHT_PIN 22
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

// Motor Pins
const int motor1pin1 = 8;
const int motor1pin2 = 7;
const int motor2pin1 = 5;
const int motor2pin2 = 4;
const int ena = 9;
const int enb = 3;

// Servo Pins
Servo shoulder;
Servo elbow;
Servo gripper;
const int SHOULDER_PIN = 11;
const int ELBOW_PIN    = 10;
const int GRIPPER_PIN  = 6;

// Servo Positions
int shoulderPos = 90;
int elbowPos    = 90;

// IR Sensor Pins
const int IR_FRONT_LEFT  = 32;
const int IR_FRONT_RIGHT = 33;
const int IR_REAR_LEFT   = 34;
const int IR_REAR_RIGHT  = 35;

// Ultrasonic Pins
const int TRIG_PIN = 30;
const int ECHO_PIN = 31;

// Gas Sensor Pins
const int MQ135_PIN = A6;
const int MQ2_PIN   = A7;

// Obstacle thresholds
const int ULTRASONIC_THRESHOLD = 20;  // cm
const int LIDAR_THRESHOLD      = 30;  // cm

// Timing
unsigned long lastSensorSend = 0;
const long SENSOR_INTERVAL = 2000;

// Data packets
struct DataPacket {
    int joyX;
    int joyY;
    bool button;
    int armX;
    int armY;
    bool armBtn;
};

struct ReceivePacket {
    float temperature;
    float humidity;
};


// TF-Luna LiDAR via Serial1 

void setupTFLuna() {
    Serial1.begin(115200);
    delay(100);
}

int getTFLunaDistance() {
    // Flush buffer first
    while (Serial1.available() > 9) Serial1.read();
    
    if (Serial1.available() >= 9) {
        if (Serial1.read() == 0x59) {
            if (Serial1.read() == 0x59) {
                int low  = Serial1.read();
                int high = Serial1.read();
                for (int i = 0; i < 5; i++) Serial1.read();
                int distance = low + high * 256;
                if (distance > 0 && distance < 800) {
                    return distance;
                }
            }
        }
    }
    return -1;
}


// Ultrasonic

long getUltrasonicDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (duration == 0) return -1;
    return duration * 0.034 / 2;
}


// Motor functions

void stopMotors() {
    analogWrite(ena, 0);
    analogWrite(enb, 0);
}

void moveForward(int speed) {
    analogWrite(ena, speed);
    analogWrite(enb, speed);
    digitalWrite(motor1pin1, HIGH);
    digitalWrite(motor1pin2, LOW);
    digitalWrite(motor2pin1, LOW);
    digitalWrite(motor2pin2, HIGH);
}

void moveBackward(int speed) {
    analogWrite(ena, speed);
    analogWrite(enb, speed);
    digitalWrite(motor1pin1, LOW);
    digitalWrite(motor1pin2, HIGH);
    digitalWrite(motor2pin1, HIGH);
    digitalWrite(motor2pin2, LOW);
}

void turnRight(int speed) {
    analogWrite(ena, speed);
    analogWrite(enb, speed);
    digitalWrite(motor1pin1, HIGH);
    digitalWrite(motor1pin2, LOW);
    digitalWrite(motor2pin1, HIGH);
    digitalWrite(motor2pin2, LOW);
}

void turnLeft(int speed) {
    analogWrite(ena, speed);
    analogWrite(enb, speed);
    digitalWrite(motor1pin1, LOW);
    digitalWrite(motor1pin2, HIGH);
    digitalWrite(motor2pin1, LOW);
    digitalWrite(motor2pin2, HIGH);
}


// Setup

void setup() {
    // Motor pins
    pinMode(motor1pin1, OUTPUT);
    pinMode(motor1pin2, OUTPUT);
    pinMode(motor2pin1, OUTPUT);
    pinMode(motor2pin2, OUTPUT);
    pinMode(ena, OUTPUT);
    pinMode(enb, OUTPUT);

    // IR pins
    pinMode(IR_FRONT_LEFT,  INPUT);
    pinMode(IR_FRONT_RIGHT, INPUT);
    pinMode(IR_REAR_LEFT,   INPUT);
    pinMode(IR_REAR_RIGHT,  INPUT);

    // Ultrasonic pins
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    // Servos
    shoulder.attach(SHOULDER_PIN);
    elbow.attach(ELBOW_PIN);
    gripper.attach(GRIPPER_PIN);
    shoulder.write(shoulderPos);
    elbow.write(elbowPos);
    gripper.write(0);

    // DHT
    dht.begin();

    // TF-Luna
    setupTFLuna();

    // RF
    radio.begin();
    radio.enableAckPayload();
    radio.enableDynamicPayloads();
    radio.openReadingPipe(0, address);
    radio.setPALevel(RF24_PA_LOW);
    radio.startListening();

    Serial.begin(9600);
    Serial.println("Rover klar!");
}


// Loop

void loop() {

    // READ ALL SENSORS
    float temperature = dht.readTemperature();
    float humidity    = dht.readHumidity();
    int   mq135Value  = analogRead(MQ135_PIN);
    int   mq2Value    = analogRead(MQ2_PIN);
    long  ultraDist   = getUltrasonicDistance();
    int   lidarDist   = getTFLunaDistance();

    // IR sensors (LOW = obstacle detected)
    bool irFrontLeft  = digitalRead(IR_FRONT_LEFT)  == LOW;
    bool irFrontRight = digitalRead(IR_FRONT_RIGHT) == LOW;
    bool irRearLeft   = digitalRead(IR_REAR_LEFT)   == LOW;
    bool irRearRight  = digitalRead(IR_REAR_RIGHT)  == LOW;

    // SEND SENSOR DATA TO PI VIA SERIAL
    unsigned long now = millis();
    if (now - lastSensorSend >= SENSOR_INTERVAL) {
        lastSensorSend = now;
        if (!isnan(temperature) && !isnan(humidity)) {
            Serial.print("TEMP:");
            Serial.print(temperature);
            Serial.print(",HUM:");
            Serial.print(humidity);
            Serial.print(",GAS:");
            Serial.print(mq135Value);
            Serial.print(",MQ2:");
            Serial.println(mq2Value);
        }
    }

    // PRINT SENSOR STATUS
    if (lidarDist > 0) {
        Serial.print("LiDAR afstand: ");
        Serial.print(lidarDist);
        Serial.println(" cm");
    }
    if (ultraDist > 0) {
        Serial.print("Ultralyd afstand: ");
        Serial.print(ultraDist);
        Serial.println(" cm");
    }

    Serial.print("IR sensorer — FL:");
    Serial.print(irFrontLeft ? "BLOKERET" : "FRI");
    Serial.print(" FR:");
    Serial.print(irFrontRight ? "BLOKERET" : "FRI");
    Serial.print(" RL:");
    Serial.print(irRearLeft ? "BLOKERET" : "FRI");
    Serial.print(" RR:");
    Serial.println(irRearRight ? "BLOKERET" : "FRI");

    // OBSTACLE AVOIDANCE
    bool obstacleDetected = false;

    // Priority 1
    if (lidarDist > 0 && lidarDist < LIDAR_THRESHOLD) {
        obstacleDetected = true;
        Serial.println("FORHINDRING (LiDAR: " + String(lidarDist) + "cm) — BAKKER");
        moveBackward(200);
        delay(500);
        stopMotors();
        Serial.println("DREJER HØJRE for at undvige");
        turnRight(200);
        delay(400);
        stopMotors();
    }
    // Priority 2 
    else if (ultraDist > 0 && ultraDist < ULTRASONIC_THRESHOLD) {
        obstacleDetected = true;
        Serial.println("FORHINDRING (Ultralyd: " + String(ultraDist) + "cm) — BAKKER");
        moveBackward(200);
        delay(500);
        stopMotors();
    }
    // Priority 3 — IR sensors
    else if (irFrontLeft && irFrontRight) {
        obstacleDetected = true;
        Serial.println("FORHINDRING FORUDE (IR begge) — BAKKER");
        moveBackward(200);
        delay(500);
        stopMotors();
    }
    else if (irFrontLeft) {
        obstacleDetected = true;
        Serial.println("FORHINDRING VENSTRE (IR) — DREJER HØJRE");
        turnRight(200);
        delay(400);
        stopMotors();
    }
    else if (irFrontRight) {
        obstacleDetected = true;
        Serial.println("FORHINDRING HØJRE (IR) — DREJER VENSTRE");
        turnLeft(200);
        delay(400);
        stopMotors();
    }
    else if (irRearLeft || irRearRight) {
        obstacleDetected = true;
        Serial.println("FORHINDRING BAGVED (IR) — STOPPER");
        stopMotors();
    }

    // RF JOYSTICK CONTROL
    ReceivePacket response;
    response.temperature = isnan(temperature) ? 0 : temperature;
    response.humidity    = isnan(humidity)    ? 0 : humidity;
    radio.writeAckPayload(0, &response, sizeof(response));

    if (radio.available() && !obstacleDetected) {
        DataPacket data;
        radio.read(&data, sizeof(data));

        // Drive control
        if (data.joyY > 0) {
            int speed = map(data.joyY, 0, 100, 100, 255);
            Serial.println("BEVÆGELSE: FREMAD");
            moveForward(speed);
        }
        else if (data.joyY < 0) {
            int speed = map(abs(data.joyY), 0, 100, 100, 255);
            Serial.println("BEVÆGELSE: BAGLÆNS");
            moveBackward(speed);
        }
        else if (data.joyX > 0) {
            int speed = map(data.joyX, 0, 100, 100, 255);
            Serial.println("BEVÆGELSE: DREJER MED URET");
            turnRight(speed);
        }
        else if (data.joyX < 0) {
            int speed = map(abs(data.joyX), 0, 100, 100, 255);
            Serial.println("BEVÆGELSE: DREJER MOD URET");
            turnLeft(speed);
        }
        else {
            stopMotors();
        }

        // Arm control
        if (data.armY > 20) {
            shoulderPos = constrain(shoulderPos + 2, 0, 180);
            shoulder.write(shoulderPos);
            Serial.println("ARM: SKULDER OP");
        }
        else if (data.armY < -20) {
            shoulderPos = constrain(shoulderPos - 2, 0, 180);
            shoulder.write(shoulderPos);
            Serial.println("ARM: SKULDER NED");
        }

        if (data.armX > 20) {
            elbowPos = constrain(elbowPos + 2, 0, 180);
            elbow.write(elbowPos);
            Serial.println("ARM: ALBUE UD");
        }
        else if (data.armX < -20) {
            elbowPos = constrain(elbowPos - 2, 0, 180);
            elbow.write(elbowPos);
            Serial.println("ARM: ALBUE NED");
        }

        // Gripper
        if (data.armBtn) {
            gripper.write(90);
            Serial.println("GRIBER: LUKKET");
        } else {
            gripper.write(0);
            Serial.println("GRIBER: ÅBEN");
        }
    }

    delay(20);
}