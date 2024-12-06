import serial
from datetime import datetime
import json
import random
import time

SERIAL_PORT = 'COM7'  # Port numaranız
BAUD_RATE = 115200 # Ayarladığınız Baud rate
VERITABANI = "uretilen_numaralar.json"


def numara_uret():
    numara = str(random.randint(1, 9))
    saniye = datetime.now().second
    numara += str(saniye // 10) + str(saniye % 10)

    for i in range(3):
        numara += str(random.randint(0, 9))

    tek_toplam = (int(numara[0]) + int(numara[2]) + int(numara[4])) * 8
    cift_toplam = (int(numara[1]) + int(numara[3]) + int(numara[5])) * 8

    yedinci_hane = str((tek_toplam + cift_toplam) % 10)
    numara += yedinci_hane

    toplam = sum(int(rakam) for rakam in numara)
    sekizinci_hane = str(toplam % 7)
    numara += sekizinci_hane

    return numara


def veritabani_yukle():
    try:
        with open(VERITABANI, 'r') as dosya:
            return json.load(dosya)
    except FileNotFoundError:
        return []


def veritabanina_kaydet(numaralar):
    with open(VERITABANI, 'w') as dosya:
        json.dump(numaralar, dosya)


def benzersiz_numara_uret():
    numaralar = veritabani_yukle()
    while True:
        yeni_numara = numara_uret()
        if yeni_numara not in numaralar:
            numaralar.append(yeni_numara)
            veritabanina_kaydet(numaralar)
            return yeni_numara


def main():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print("Sistem hazır...")

        while True:
            if ser.in_waiting:
                gelen_veri = ser.readline().decode('utf-8').strip()

                if gelen_veri == "YENI_KART":
                    yeni_numara = benzersiz_numara_uret()
                    print(f"Üretilen numara: {yeni_numara}")
                    ser.write(f"{yeni_numara}\n".encode('utf-8'))
                    ser.flush()  # Buffer'ı temizle
                    time.sleep(0.1)  # Kısa bir bekleme ekle

                elif gelen_veri == "KART_DOLU":
                    print("Kart depolama alanı dolu!")

                elif gelen_veri.startswith("YAZMA_OK"):
                    print("Numara başarıyla yazıldı.")

                elif gelen_veri.startswith("YAZMA_HATA"):
                    print("Yazma işlemi başarısız oldu!")

            time.sleep(0.1)

    except serial.SerialException as e:
        print(f"Seri port hatası: {e}")
    except Exception as e:
        print(f"Beklenmeyen hata: {e}")
    finally:
        if 'ser' in locals():
            ser.close()
        print("Program sonlandırılıyor...")


if __name__ == "__main__":
    main()