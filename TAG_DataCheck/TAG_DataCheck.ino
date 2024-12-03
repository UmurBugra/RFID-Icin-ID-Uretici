#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         22
#define SS_PIN          5

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println("TAG okuma testi...");
  Serial.println("Bir kart yaklaştırın...");
}

void loop() {
  
  if (!mfrc522.PICC_IsNewCardPresent()) {
    delay(50);
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  Serial.println("\n---Kart Bilgileri---");
  
  Serial.print("Kart UID: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  byte buffer[18];
  byte blok = 4; 
  byte size = sizeof(buffer);

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blok, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.println("Kimlik doğrulama hatası!");
    delay(1000);
    return;
  }

  status = mfrc522.MIFARE_Read(blok, buffer, &size);
  if (status == MFRC522::STATUS_OK) {
    Serial.print("Blok ");
    Serial.print(blok);
    Serial.print(" içeriği: ");
    
    for (uint8_t i = 0; i < 16; i++) {
      if (buffer[i] >= 32 && buffer[i] <= 126) { 
        Serial.write(buffer[i]);
      } else {
        Serial.print(".");
      }
    }
    Serial.println();

    Serial.print("HEX format: ");
    for (uint8_t i = 0; i < 16; i++) {
      Serial.print(buffer[i] < 0x10 ? " 0" : " ");
      Serial.print(buffer[i], HEX);
    }
    Serial.println();
  } else {
    Serial.println("Okuma hatası!");
  }
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(1000);
}