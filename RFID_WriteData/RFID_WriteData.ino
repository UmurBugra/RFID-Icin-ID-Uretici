#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN     22
#define SS_PIN      5

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  SPI.begin();
  mfrc522.PCD_Init();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println("RFID okuyucu hazır...");
}

void loop() {
  
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  Serial.println("YENI_KART");

  while (!Serial.available()) {
    delay(100);
  }

  String yeni_numara = Serial.readStringUntil('\n');
  yeni_numara.trim();

  if (kartaYaz(yeni_numara)) {
    Serial.println("Numara başarıyla yazıldı: " + yeni_numara);
  } else {
    Serial.println("Yazma hatası!");
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

bool kartaYaz(String veri) {
  byte buffer[18];
  veri.getBytes(buffer, 18);
  
  byte blok = 4;  
  byte trailerBlock = 7;

  MFRC522::StatusCode durum = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (durum != MFRC522::STATUS_OK) {
    return false;
  }

  durum = mfrc522.MIFARE_Write(blok, buffer, 16);
  if (durum != MFRC522::STATUS_OK) {
    return false;
  }

  return true;
}