import random
from datetime import datetime


def numara_uret():
    # 1. Kural: İlk hane 1-9 arası rastgele
    numara = str(random.randint(1, 9))

    # 2. Kural: Saniye bilgisi 2. ve 3. hanelere
    saniye = datetime.now().second
    numara += str(saniye // 10) + str(saniye % 10)

    # 4-6. haneler için rastgele sayılar (0-9 arası)
    for i in range(3):
        rastgele_sayi = str(random.randint(0, 9))
        numara += rastgele_sayi

    # 3. Kural: 1, 3 ve 5. hanelerin toplamı * 8
    tek_toplam = (int(numara[0]) + int(numara[2]) + int(numara[4])) * 8

    # 4. Kural: 2, 4 ve 6. hanelerin toplamı * 8
    cift_toplam = (int(numara[1]) + int(numara[3]) + int(numara[5])) * 8

    # 5. Kural: 7. hane hesaplama
    yedinci_hane = str((tek_toplam + cift_toplam) % 10)
    numara += yedinci_hane

    # 6. Kural: 8. hane hesaplama
    toplam = 0
    for i in numara:
        toplam += int(i)
    sekizinci_hane = str(toplam % 7)
    numara += sekizinci_hane

    return numara

def test():
    uretilen_numara = numara_uret()
    print(f"Üretilen numara: {uretilen_numara}")
    return uretilen_numara

if __name__ == "__main__":
    test()