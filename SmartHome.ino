#include <WiFi.h>
#include <MQTT.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <HCSR04.h>

#define RST_PIN         22          // Configurable, see typical pin layout above
#define SS_PIN          21         // Configurable, see typical pin layout above

const char ssid[] = "AND4NG";
const char pass[] = "Bujangga2021";
static const int servoPin = 12;


String pesan = "";
bool cekSystem = false;
int newDistance;
int oldDistance;
int count = 0;
bool cekSensorTongSampah = false;
bool cekSystemServo = true;

WiFiClient net;
MQTTClient client;
Servo servo1;

UltraSonicDistanceSensor distanceSensor(15, 2); //initialisation class HCSR04 (trig pin , echo pin)
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

unsigned long lastMillis = 0;

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("ESP32", "alvrialdo12", "1qa2w3ed4r")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  client.subscribe("Pintu");
  client.subscribe("Servo");
  client.subscribe("SensorTongSampah");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  if (cekSystem)
  {
    if (topic == "Pintu") {
      if (payload == "kunci") {
        digitalWrite(4, HIGH);
        pesan = "Terkunci";
      } else if (payload == "buka") {
        digitalWrite(4, LOW);
        pesan = "Terbuka";
      }
    }

    if (topic == "Servo") {
      if (cekSystemServo) {
        if (payload == "buka") {
          for (int posDegrees = 0; posDegrees <= 230; posDegrees++) {
            servo1.write(posDegrees);
            Serial.println(posDegrees);
            delay(20);
          }
        } else if (payload == "tutup") {
          for (int posDegrees = 230; posDegrees >= 0; posDegrees--) {
            servo1.write(posDegrees);
            Serial.println(posDegrees);
            delay(20);
          }
        }
      }
    }

    if (topic == "SensorTongSampah") {
      if (payload == "aktif") {
        cekSensorTongSampah = true;
        cekSystemServo = false;
        pesan = "Sensor Aktif";
      } else {
        cekSensorTongSampah = false;
        cekSystemServo = true;
        pesan = "Sensor Tidak Aktif";
      }
    }

    client.publish("info", pesan);
  }

}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  pinMode(13, OUTPUT);
  pinMode(4, OUTPUT);
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();
  servo1.attach(servoPin);
  oldDistance = distanceSensor.measureDistanceCm();

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.
  client.begin("broker.shiftr.io", net);
  client.onMessage(messageReceived);
  connect();


  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)

}

void loop() {

  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    connect();
  }

  if (cekSensorTongSampah) {
    newDistance = distanceSensor.measureDistanceCm();
    if (newDistance != oldDistance) {
      oldDistance = newDistance;
    }
    Serial.print("nilai jarak: ");
    Serial.println(oldDistance);
    delay(100);

    if (oldDistance <= 10 && oldDistance > -1 ) {
      if (count == 1) {
        for (int posDegrees = 0; posDegrees <= 230; posDegrees++) {
          servo1.write(posDegrees);
          Serial.println(posDegrees);
          delay(20);
        }
        delay(5000);
        for (int posDegrees = 230; posDegrees >= 0; posDegrees--) {
          servo1.write(posDegrees);
          Serial.println(posDegrees);
          delay(20);
        }
        count = 0;
      }
    } else {
      count = 1;

    }

  }

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  String UID = "";

  for (byte i = 0; i < mfrc522.uid.size; i++) {
    String satu =  mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ";
    String dua = String ( mfrc522.uid.uidByte[i], HEX);
    String tampung = satu + dua;
    UID = UID + tampung;
  }


  Serial.println(UID);

  if (UID == " 7a ae c1 16") {
    if (cekSystem) {
      Serial.println("Sistem di Non Aktifkan");
      cekSystem = false;
      cekSensorTongSampah = false;
      digitalWrite(13, LOW);
    } else {
      Serial.println("Sistem di Aktifkan");
      cekSystem = true;
      digitalWrite(13, HIGH);
    }
  } else {
    Serial.println("Kartu tidak terdaftar");
  }
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

}
