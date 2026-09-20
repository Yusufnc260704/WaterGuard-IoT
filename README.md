# ESP32-Based IoT Water Level Monitoring & Early Warning System
An IoT-based water level monitoring system using ESP32 and JSN-SR04T ultrasonic sensor, integrated with Blynk, Telegram, LCD, LED indicators, and buzzer alerts.

# Sistem Monitoring Ketinggian Air (Early Warning System)

Alat pemantau ketinggian air otomatis berbasis **ESP32**, dilengkapi indikator LED traffic light, buzzer peringatan, LCD, serta notifikasi jarak jauh lewat **Telegram** dan monitoring real-time lewat **Blynk IoT**. Dibuat untuk membantu deteksi dini banjir/kenaikan air di sungai, selokan, atau area rawan genangan.

<table>
  <tr>
    <td align="center"><img src="images/images1.webp" width="260" alt="Foto Alat 1"/></td>
    <td align="center"><img src="images/images2.webp" width="260" alt="Foto Alat 2"/></td>
    <td align="center"><img src="images/images3.png" width="260" alt="Foto Alat 3"/></td>
  </tr>
</table>

<p align="center">
  <img src="images/circuit.png" width="600" alt="Wiring Diagram"/>
  <br/>
  <em>Wiring Diagram</em>
</p>

---

## ✨ Fitur

- Pengukuran jarak air ke sensor menggunakan sensor ultrasonik **JSN-SR04T** (tahan pancaran/waterproof, cocok untuk aplikasi outdoor)
- Filter median 5 sampel supaya pembacaan jarak stabil, tidak gampang salah baca (noise reduction)
- Fallback otomatis ke status **SIAGA 3** saat sensor masuk *blind spot* (supaya sistem tidak salah anggap NORMAL saat air sudah sangat tinggi)
- 3 level siaga (SIAGA 1 / SIAGA 2 / SIAGA 3) dengan indikator berbeda di LED, buzzer, dan LCD
- Notifikasi otomatis ke **Telegram** setiap kali status berubah (naik atau kembali normal)
- Live monitoring jarak & status lewat **Blynk App**
- Tampilan LCD 16x2 menampilkan jarak air & status real-time
- Kredensial (WiFi, token Blynk, token Telegram) dipisah ke file `secrets.h` supaya aman saat diupload ke GitHub

---

## 🧰 Komponen yang Dipakai

### Komponen Utama

1. **ESP32** — sebagai mikrokontroler utama
2. **Sensor ultrasonik JSN-SR04T** — untuk mengukur jarak/ketinggian air
3. **LCD 16×2 I2C** — untuk menampilkan jarak dan status
4. **Buzzer** — sebagai alarm peringatan
5. **LED Merah** — indikator SIAGA 3
6. **LED Kuning** — indikator SIAGA 2
7. **LED Hijau** — indikator NORMAL dan SIAGA 1
8. **Resistor** — pembatas arus untuk LED
9. **Kabel jumper** — penghubung antar-komponen
10. **Sumber daya 5V** — untuk ESP32/sistem

### Komponen/Layanan Software

Selain hardware, kode juga menggunakan:

- **Blynk** — monitoring melalui aplikasi
- **Telegram Bot** — notifikasi status ketinggian air
- **Wi-Fi** — komunikasi ESP32 dengan internet

---

## 🔌 Wiring ESP32

| Komponen       | Pin Komponen  | ESP32                |
|-----------------|---------------|------------------------|
| JSN-SR04T        | VCC           | 5V/VIN                  |
|                  | GND           | GND                     |
|                  | TRIG          | GPIO 5                  |
|                  | ECHO          | GPIO 18                 |
| Buzzer            | (+)           | GPIO 19                 |
|                  | (-)           | GND                     |
| LED Merah         | Anoda (+)     | GPIO 25                 |
|                  | Katoda (-)    | GND melalui resistor     |
| LED Kuning        | Anoda (+)     | GPIO 26                 |
|                  | Katoda (-)    | GND melalui resistor     |
| LED Hijau         | Anoda (+)     | GPIO 27                 |
|                  | Katoda (-)    | GND melalui resistor     |
| LCD I2C 16×2      | VCC           | 5V/VIN                  |
|                  | GND           | GND                     |
|                  | SDA           | GPIO 21                 |
|                  | SCL           | GPIO 22                 |

> Lihat gambar wiring lengkap di folder `images/` untuk detail penyambungan kabel.

### Penjelasan Wiring

Wiring sistem dilakukan dengan menghubungkan sensor ultrasonik JSN-SR04T ke ESP32. Pin TRIG sensor dihubungkan ke GPIO 5, sedangkan pin ECHO dihubungkan ke GPIO 18. Pin VCC sensor dihubungkan ke sumber tegangan 5V dan GND dihubungkan ke GND ESP32. Buzzer dihubungkan pada GPIO 19 sebagai keluaran suara peringatan.

Indikator traffic light menggunakan tiga LED, yaitu LED merah pada GPIO 25, LED kuning pada GPIO 26, dan LED hijau pada GPIO 27. Masing-masing LED dihubungkan ke GND melalui resistor pembatas arus. LCD 16×2 dengan modul I2C menggunakan GPIO 21 sebagai SDA dan GPIO 22 sebagai SCL, sedangkan VCC dan GND masing-masing dihubungkan ke 5V dan GND ESP32.

---

## 🚦 Level Siaga

Status ditentukan dari jarak sensor ke permukaan air (semakin kecil jarak = air semakin tinggi):

| Status    | Jarak Sensor    | LED     | Buzzer                          |
|-----------|-----------------|---------|----------------------------------|
| NORMAL    | > 35 cm          | Hijau   | Mati                              |
| SIAGA 1   | ≤ 35 cm           | Hijau   | Bunyi pendek 1x                    |
| SIAGA 2   | ≤ 28 cm           | Kuning  | Bunyi 2x berturut                  |
| SIAGA 3   | ≤ 24 cm (atau sensor blind spot) | Merah   | Bunyi cepat 4x — BAHAYA, evakuasi |

Nilai ambang batas ini diatur lewat konstanta `SIAGA_1`, `SIAGA_2`, `SIAGA_3` di kode, dan bisa disesuaikan dengan kondisi lokasi pemasangan.

---

## 📡 Virtual Pin Blynk

| Virtual Pin | Tipe    | Keterangan                                             |
|-------------|---------|----------------------------------------------------------|
| V0          | Double  | Jarak sensor (cm), range 0–600                             |
| V1          | Integer | Level siaga angka: 0=Normal, 1=Siaga1, 2=Siaga2, 3=Siaga3 |
| V2          | String  | Status teks: "NORMAL" / "SIAGA 1" / "SIAGA 2" / "SIAGA 3"   |

---

## 📚 Library yang Dibutuhkan

Install lewat Arduino IDE Library Manager:

- `LiquidCrystal_I2C`
- `WiFi` (bawaan board ESP32)
- `WiFiClientSecure` (bawaan board ESP32)
- `UniversalTelegramBot`
- `Blynk` (`BlynkSimpleEsp32`)

Pastikan board **ESP32** sudah terpasang di Arduino IDE (via Boards Manager).

---

## ⚙️ Instalasi & Konfigurasi

1. Clone atau download repo ini
2. Duplikat file `secrets.h.example` menjadi `secrets.h`
3. Isi `secrets.h` dengan data asli kamu:
   ```cpp
   #define SECRET_SSID       "wifi_kamu"
   #define SECRET_PASSWORD   "password_wifi_kamu"
   #define SECRET_BLYNK_TEMPLATE_ID   "template_id_blynk"
   #define SECRET_BLYNK_TEMPLATE_NAME "template_name_blynk"
   #define SECRET_BLYNK_AUTH_TOKEN    "auth_token_blynk"
   #define SECRET_BOT_TOKEN  "token_bot_telegram"
   #define SECRET_CHAT_ID    "chat_id_telegram"
   ```
4. Buka `water-monitor.ino` di Arduino IDE
5. Pilih board **ESP32 Dev Module**, pilih port yang sesuai
6. Upload ke board

> ⚠️ File `secrets.h` sudah otomatis diabaikan git lewat `.gitignore`, jangan pernah upload file itu ke GitHub karena berisi data pribadi/kredensial.

---

## 🗂️ Struktur File

```
water-monitor/
├── water-monitor.ino     # Kode utama
├── secrets.h.example       # Template kredensial
├── .gitignore
├── README.md
└── images/
    ├── images1.webp
    ├── images2.webp
    ├── images3.png
    └── circuit.png
```

---

## 🧠 Cara Kerja Singkat

1. Sensor JSN-SR04T mengukur jarak ke permukaan air setiap loop, diambil 5 sampel lalu difilter median untuk hasil stabil
2. Jarak dibandingkan dengan ambang batas SIAGA untuk menentukan status
3. Jika sensor tidak mendapat pembacaan valid (blind spot) tapi jarak terakhir sudah dekat SIAGA 3, sistem tetap menganggap status SIAGA 3 (fail-safe, tidak asal NORMAL)
4. LED, buzzer, dan LCD update sesuai status terkini
5. Setiap kali status berubah, bot Telegram mengirim notifikasi ke chat yang terdaftar
6. Data jarak & status juga dikirim ke Blynk setiap 1 detik untuk dashboard monitoring jarak jauh

---

## 📄 Lisensi

Bebas digunakan dan dimodifikasi untuk keperluan edukasi maupun pengembangan lebih lanjut.

