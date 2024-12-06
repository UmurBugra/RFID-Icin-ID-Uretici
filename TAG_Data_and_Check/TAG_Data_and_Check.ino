#include <SPI.h>
#include <MFRC522.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RST_PIN         22
#define SS_PIN          5
#define BASLANGIC_BLOK  4   
#define SON_BLOK        62  

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

TaskHandle_t TaskHandle_Read;
TaskHandle_t TaskHandle_Write;
SemaphoreHandle_t xMutex = NULL;

// Görsel blok numarasını gerçek blok numarasına çevir
int gorseldenGercekBloka(int gorselBlok) {
    int gercekBlok = gorselBlok + 3; // 1->4, 2->5, 3->6
    int sektorOffset = (gorselBlok - 1) / 3; // Her sektör için +1 offset ekle
    return gercekBlok + sektorOffset;
}

// Gerçek blok numarasını görsel blok numarasına çevir
int gercektenGorselBloka(int gercekBlok) {
    int sektorNum = gercekBlok / 4;
    int blokOffset = gercekBlok % 4;
    if (blokOffset == 3) return -1; // Trailer blok
    return (sektorNum * 3) + blokOffset - 3 + 1; // +1 ile 1'den başlat
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // Serial port bağlantısını bekle
    }
    
    SPI.begin();
    mfrc522.PCD_Init();
    
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    
    xMutex = xSemaphoreCreateMutex();
    
    xTaskCreatePinnedToCore(readTask, "ReadTask", 10000, NULL, 1, &TaskHandle_Read, 0);
    xTaskCreatePinnedToCore(writeTask, "WriteTask", 10000, NULL, 1, &TaskHandle_Write, 1);
    
    Serial.println("Sistem hazır...");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(100));
}

// İlk boş bloğu bul (görsel numarayla döndür)
int ilkBosBlokBul() {
    byte buffer[18];
    byte size = sizeof(buffer);
    
    for (byte blok = BASLANGIC_BLOK; blok < SON_BLOK; blok++) {
        if ((blok + 1) % 4 == 0) continue; // Trailer blokları atla
        
        byte trailerBlock = ((blok / 4) * 4) + 3;
        status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
        if (status != MFRC522::STATUS_OK) continue;
        
        status = mfrc522.MIFARE_Read(blok, buffer, &size);
        if (status == MFRC522::STATUS_OK) {
            bool bosBlok = true;
            for (uint8_t i = 0; i < 16; i++) {
                if (buffer[i] != 0) {
                    bosBlok = false;
                    break;
                }
            }
            if (bosBlok) {
                return gercektenGorselBloka(blok);
            }
        }
    }
    return 48; // 47'den büyük bir değer döndür (kart dolu)
}

void readTask(void * parameter) {
    while(true) {
        bool mutexAlindi = false;
        try {
            if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
                mutexAlindi = true;
                if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
                    Serial.println("\n--- Kart Okuma Başladı ---");
                    tumBloklariOku();
                    Serial.println("--- Kart Okuma Tamamlandı ---\n");
                    mfrc522.PICC_HaltA();
                    mfrc522.PCD_StopCrypto1();
                }
            }
        } catch (...) {
            Serial.println("Okuma görevi hatası!");
        }
        
        if (mutexAlindi) {
            xSemaphoreGive(xMutex);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void writeTask(void * parameter) {
    while(true) {
        bool mutexAlindi = false;
        try {
            if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
                mutexAlindi = true;
                if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
                    int gorselBosBlok = ilkBosBlokBul();
                    
                    if (gorselBosBlok > 47) {
                        Serial.println("KART_DOLU");
                    } else {
                        Serial.println("YENI_KART");
                        Serial.println("Veri bekleniyor...");
                        
                        unsigned long baslangicZamani = millis();
                        while (!Serial.available()) {
                            vTaskDelay(pdMS_TO_TICKS(10));
                            if (millis() - baslangicZamani > 5000) {
                                Serial.println("Veri bekleme zaman aşımı!");
                                break;
                            }
                        }
                        
                        if (Serial.available()) {
                            String yeni_numara = Serial.readStringUntil('\n');
                            yeni_numara.trim();
                            
                            int gercekBlok = gorseldenGercekBloka(gorselBosBlok);
                            if (kartaYaz(yeni_numara, gercekBlok)) {
                                Serial.println("YAZMA_OK");
                                Serial.println("Blok " + String(gorselBosBlok) + "'e yazıldı: " + yeni_numara);
                            } else {
                                Serial.println("YAZMA_HATA");
                            }
                        }
                    }
                    
                    mfrc522.PICC_HaltA();
                    mfrc522.PCD_StopCrypto1();
                }
            }
        } catch (...) {
            Serial.println("Yazma görevi hatası!");
        }
        
        if (mutexAlindi) {
            xSemaphoreGive(xMutex);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void tumBloklariOku() {
    byte buffer[18];
    byte size = sizeof(buffer);
    bool veriVar = false;
    
    for (byte blok = BASLANGIC_BLOK; blok < SON_BLOK; blok++) {
        if ((blok + 1) % 4 == 0) continue;
        
        byte trailerBlock = ((blok / 4) * 4) + 3;
        status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
        if (status != MFRC522::STATUS_OK) continue;
        
        status = mfrc522.MIFARE_Read(blok, buffer, &size);
        if (status == MFRC522::STATUS_OK) {
            String veri = "";
            for (uint8_t i = 0; i < 16 && buffer[i] != 0; i++) {
                if (buffer[i] >= 32 && buffer[i] <= 126) {
                    veri += (char)buffer[i];
                }
            }
            if (veri.length() > 0) {
                int gorselBlok = gercektenGorselBloka(blok);
                Serial.println("Blok " + String(gorselBlok) + ": " + veri);
                veriVar = true;
            }
        }
    }
    
    if (!veriVar) {
        Serial.println("Kartta kayıtlı veri bulunamadı.");
    }
}

bool kartaYaz(String veri, int gercekBlok) {
    if (veri.length() == 0 || gercekBlok > SON_BLOK) return false;
    
    byte buffer[18];
    memset(buffer, 0, 18);
    veri.getBytes(buffer, veri.length() + 1);
    
    byte trailerBlock = ((gercekBlok / 4) * 4) + 3;
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        return false;
    }
    
    status = mfrc522.MIFARE_Write(gercekBlok, buffer, 16);
    return (status == MFRC522::STATUS_OK);
}