/*
 * Sistem Monitoring Ketinggian Air + Buzzer + Traffic Light LED
 * + Telegram + Blynk
 * Sensor: JSN-SR04T | MCU: ESP32
 *
 * Virtual Pin Blynk:
 * V0 = Jarak sensor (Double, 0-600 cm)
 * V1 = Level siaga angka (Integer: 0=Normal, 1=Siaga1, 2=Siaga2, 3=Siaga3)
 * V2 = Status teks (String: "NORMAL" / "SIAGA 1" / dst)
 *
 * CATATAN:
 * Semua data rahasia (WiFi, token Blynk, token Telegram) TIDAK
 * ditulis langsung di file ini, tapi diambil dari "secrets.h".
 * Sebelum compile, pastikan kamu sudah:
 *   1. Copy "secrets.h.example" -> "secrets.h"
 *   2. Isi "secrets.h" dengan credential asli kamu
 * File "secrets.h" sudah otomatis diblokir dari Git lewat .gitignore.
 */

// ================= SECRETS (WAJIB ADA SEBELUM BLYNK CONFIG) =================
#include "secrets.h"

// ================= BLYNK CONFIG (HARUS DI PALING ATAS) =================
#define BLYNK_TEMPLATE_ID   SECRET_BLYNK_TEMPLATE_ID
#define BLYNK_TEMPLATE_NAME SECRET_BLYNK_TEMPLATE_NAME
#define BLYNK_AUTH_TOKEN    SECRET_BLYNK_AUTH_TOKEN
#define BLYNK_PRINT Serial

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <BlynkSimpleEsp32.h>

// ================= WiFi & Telegram =================
const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASSWORD;

#define BOT_TOKEN SECRET_BOT_TOKEN
#define CHAT_ID   SECRET_CHAT_ID

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= PIN =================
#define TRIG_PIN    5
#define ECHO_PIN    18
#define BUZZER_PIN  19

// Traffic light LED
#define LED_R 25
#define LED_Y 26
#define LED_G 27

// ================= LEVEL (cm) =================
#define SIAGA_1 35.0
#define SIAGA_2 28.0
#define SIAGA_3 24.0

// ================= VARIABEL =================
float distance;
float lastValid          = 0;
bool  firstRead          = true;
float lastStableDistance = -1;
String status     = "NORMAL";
String lastStatus = "";

unsigned long lastNotifTime = 0;
const unsigned long NOTIF_COOLDOWN = 60000;

BlynkTimer timer;

// ================= FORWARD DECLARATIONS =================
void allLedOff();
void buzzerSiaga1();
void buzzerSiaga2();
void buzzerSiaga3();
void checkWaterLevel(float dist);
void updateLCD(float dist, String stat);
void sendTelegramAlert(String lvl, float dist);
float readRaw();
float readStable();

// ================= KIRIM KE BLYNK (dipanggil tiap 1 detik) =================
void sendToBlynk() {
  int levelAngka = 0;
  if      (status == "SIAGA 1") levelAngka = 1;
  else if (status == "SIAGA 2") levelAngka = 2;
  else if (status == "SIAGA 3") levelAngka = 3;

  if (distance > 0) Blynk.virtualWrite(V0, distance);
  Blynk.virtualWrite(V1, levelAngka);
  Blynk.virtualWrite(V2, status);
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_Y, OUTPUT);
  pinMode(LED_G, OUTPUT);

  allLedOff();
  digitalWrite(LED_G, HIGH);
  noTone(BUZZER_PIN);

  // ---- Koneksi WiFi ----
  lcd.setCursor(0, 0); lcd.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi Terhubung!");
  client.setInsecure();

  // ---- Koneksi Blynk ----
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("WiFi + Blynk OK!");
  delay(1500);
  lcd.clear();

  bot.sendMessage(CHAT_ID,
    "✅ *Sistem Monitoring Air Online*\nESP32 terhubung dan siap memantau ketinggian air.",
    "Markdown");

  timer.setInterval(1000L, sendToBlynk);

  Serial.println("Jarak(cm) | Status");
  Serial.println("-------------------");
}

// ================= LOOP =================
void loop() {
  Blynk.run();
  timer.run();

  // --- Baca sensor dengan fallback SIAGA 3 saat blind spot ---
  float d = readStable();

  if (d > 0) {
    lastStableDistance = d;

    if (firstRead) {
      lastValid = d;
      firstRead = false;
    }

    if (abs(d - lastValid) >= 1) {
      lastValid = d;
    }

    distance = lastValid;
  }
  else {
    // ===== FALLBACK SIAGA 3 =====
    if (lastStableDistance > 0 &&
        lastStableDistance <= (SIAGA_3 + 2)) {
      // sensor masuk blind spot, anggap air sudah melewati batas SIAGA 3
      distance = SIAGA_3;
    }
    else {
      distance = -1;
    }
  }

  checkWaterLevel(distance);

  // --- Kirim notifikasi Telegram hanya saat status BERUBAH ---
  if (status != lastStatus) {
    if (status == "SIAGA 1" || status == "SIAGA 2" || status == "SIAGA 3") {
      sendTelegramAlert(status, distance);
    }
    else if (status == "NORMAL" && lastStatus != "") {
      bot.sendMessage(CHAT_ID,
        "✅ *Status NORMAL*\nKetinggian air kembali aman.\nJarak: " + String(distance, 1) + " cm",
        "Markdown");
    }
    lastStatus = status;
    lastNotifTime = millis();
  }

  if (distance > 0) {
    Serial.print(distance, 1);
    Serial.print(" cm | ");
    Serial.println(status);
  } else {
    Serial.println("ERROR | Sensor");
  }

  updateLCD(distance, status);
  delay(500);
}

// ================= TELEGRAM ALERT =================
void sendTelegramAlert(String lvl, float dist) {
  String emoji = "⚠️";
  String pesan = "";

  if (lvl == "SIAGA 1") {
    emoji = "🟡";
    pesan = "SIAGA 1 — Waspada!\nAir mulai naik. Perhatikan perkembangan.";
  } else if (lvl == "SIAGA 2") {
    emoji = "🟠";
    pesan = "SIAGA 2 — Siaga!\nKetinggian air berbahaya. Bersiap evakuasi.";
  } else if (lvl == "SIAGA 3") {
    emoji = "🔴";
    pesan = "SIAGA 3 — BAHAYA!\nKetinggian air sangat tinggi. SEGERA EVAKUASI!";
  }

  String msg = emoji + " *" + pesan.substring(0, pesan.indexOf('\n')) + "*\n"
             + pesan.substring(pesan.indexOf('\n') + 1) + "\n"
             + "📏 Jarak sensor: *" + String(dist, 1) + " cm*";

  bot.sendMessage(CHAT_ID, msg, "Markdown");
  Serial.println("Notifikasi Telegram terkirim: " + lvl);
}

// ================= LCD =================
void updateLCD(float dist, String stat) {
  char line1[17];
  char line2[17];

  if (dist > 0) {
    snprintf(line1, 17, "Jarak:%6.1fcm", dist);
  } else {
    snprintf(line1, 17, "Jarak: ERROR   ");
  }

  snprintf(line2, 17, "Status:%-7s", stat.c_str());

  lcd.setCursor(0, 0); lcd.print(line1);
  lcd.setCursor(0, 1); lcd.print(line2);
}

// ================= SENSOR =================
// Menggunakan micros() manual karena pulseIn() kurang presisi di ESP32
float readRaw() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Tunggu echo HIGH mulai (timeout 30ms)
  unsigned long tStart = micros();
  while (digitalRead(ECHO_PIN) == LOW) {
    if (micros() - tStart > 30000) return -1;
  }

  // Ukur durasi HIGH (timeout 40ms)
  unsigned long tHigh = micros();
  while (digitalRead(ECHO_PIN) == HIGH) {
    if (micros() - tHigh > 40000) return -1;
  }
  unsigned long duration = micros() - tHigh;

  return (duration * 0.0343 / 2.0);
}

float readStable() {
  float data[5];
  int valid = 0;
  for (int i = 0; i < 5; i++) {
    float d = readRaw();
    if (d > 20 && d < 200) {
      data[valid++] = d;
    }
    delay(120);
  }
  if (valid < 3) return -1;
  // sort bubble
  for (int i = 0; i < valid - 1; i++) {
    for (int j = i + 1; j < valid; j++) {
      if (data[i] > data[j]) {
        float t = data[i];
        data[i] = data[j];
        data[j] = t;
      }
    }
  }
  return data[valid / 2]; // median
}

// ================= LOGIC + BUZZER + LED =================
void checkWaterLevel(float dist) {
  if (dist <= 0) {
    // Jika sebelumnya sudah dekat SIAGA 3, jangan langsung ERROR
    if (lastStableDistance > 0 &&
        lastStableDistance <= (SIAGA_3 + 2)) {
      allLedOff();
      status = "SIAGA 3";
      digitalWrite(LED_R, HIGH);
      buzzerSiaga3();
    }
    else {
      allLedOff();
      noTone(BUZZER_PIN);
      status = "ERROR";
    }
    return;
  }

  if (dist <= SIAGA_3) {
    allLedOff();
    status = "SIAGA 3";
    digitalWrite(LED_R, HIGH);
    buzzerSiaga3();
  }
  else if (dist <= SIAGA_2) {
    allLedOff();
    status = "SIAGA 2";
    digitalWrite(LED_Y, HIGH);
    buzzerSiaga2();
  }
  else if (dist <= SIAGA_1) {
    allLedOff();
    status = "SIAGA 1";
    digitalWrite(LED_G, HIGH);
    buzzerSiaga1();
  }
  else {
    allLedOff();
    noTone(BUZZER_PIN);
    status = "NORMAL";
    digitalWrite(LED_G, HIGH);
  }
}

// ================= BUZZER PATTERN =================
void buzzerSiaga1() {
  tone(BUZZER_PIN, 3500); delay(120); noTone(BUZZER_PIN);
}

void buzzerSiaga2() {
  for (int i = 0; i < 2; i++) {
    tone(BUZZER_PIN, 3800); delay(120); noTone(BUZZER_PIN); delay(120);
  }
}

void buzzerSiaga3() {
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, 4000); delay(90); noTone(BUZZER_PIN); delay(90);
  }
}

// ================= LED HELPER =================
void allLedOff() {
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_Y, LOW);
  digitalWrite(LED_G, LOW);
}
