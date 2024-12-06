#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         22
#define SS_PIN          5
#define BASLANGIC_BLOK  4   
#define SON_BLOK        62  

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

void setup() {
    Serial.begin(115200);
    SPI.begin();
    mfrc522.PCD_Init();
    
    // Varsayılan anahtarı ayarla
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    
    Serial.println("Kart sıfırlama sistemi hazır...");
    Serial.println("Sıfırlamak istediğiniz kartı okutun...");
}

void loop() {
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        delay(50);
        return;
    }
    
    Serial.println("\nKart algılandı. Sıfırlama başlıyor...");
    
    if (kartiSifirla()) {
        Serial.println("Kart başarıyla sıfırlandı!");
    } else {
        Serial.println("Sıfırlama işleminde hata!");
    }
    
    Serial.println("\nBaşka bir kart sıfırlamak için kartı okutun...");
    
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    delay(1000);
}

bool kartiSifirla() {
    byte sifirBuffer[16] = {0};  // Sıfırlardan oluşan buffer
    
    for (byte blok = BASLANGIC_BLOK; blok < SON_BLOK; blok++) {
        // Trailer bloklarını atla
        if ((blok + 1) % 4 == 0) {
            continue;
        }
        
        // Her sektör için kimlik doğrulama yap
        if (blok % 4 == 0) {
            byte trailerBlock = ((blok / 4) * 4) + 3;
            status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
            
            if (status != MFRC522::STATUS_OK) {
                Serial.print("Blok ");
                Serial.print(blok);
                Serial.println(" - Kimlik doğrulama hatası!");
                return false;
            }
        }
        
        // Bloğu sıfırla
        status = mfrc522.MIFARE_Write(blok, sifirBuffer, 16);
        
        if (status == MFRC522::STATUS_OK) {
            Serial.print("Blok ");
            Serial.print(blok);
            Serial.println(" sıfırlandı.");
        } else {
            Serial.print("Blok ");
            Serial.print(blok);
            Serial.println(" sıfırlama hatası!");
            return false;
        }
    }
    
    return true;
}