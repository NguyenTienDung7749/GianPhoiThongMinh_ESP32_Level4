#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <ESP32Servo.h>
#include <DHT.h>
#include <time.h>

// ================= WIFI + FIREBASE =================
#include <WiFi.h>
#include <WiFiManager.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Firebase config (LẤY TỪ FIREBASE.JS) (Đã giấu)
#include "secrets.h"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool firebaseReady = false;

// =============== OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1

#define OLED_SDA 8
#define OLED_SCL 9

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// =============== SERVO =================
Servo mg996;

const int SERVO_PIN   = 13;
const int STOP_PWM    = 1500;
const int FORWARD_PWM = 1300;
const int REVERSE_PWM = 1700;
const int MOVE_TIME   = 3000;

bool isMovingServo = false;

// =============== SENSORS ===============
// LDR_DO: HIGH = Tối, LOW = Sáng
const int LDR_DO = 3;
const int RAIN_AO = 6;
int RAIN_THRESHOLD = 2000;

const int DHT_PIN = 2;
#define DHTTYPE DHT22
DHT dht(DHT_PIN, DHTTYPE);

unsigned long lastDhtRead = 0;
const unsigned long DHT_INTERVAL = 5000;

float lastTemp = NAN;
float lastHum  = NAN;

const int LED_THU  = 4; // đỏ
const int LED_PHOI = 5; // xanh

// ======== NTP TIME (UTC+7 cho Viet Nam) =========
const long gmtOffset_sec     = 7 * 3600;  // +7 giờ
const int  daylightOffset_sec = 0;        // không dùng DST

// =============== SYSTEM STATE ===============
bool isOut = false;

bool lastIsBright  = false;
bool lastIsRaining = false;

bool lastDebugBright = false;
bool lastDebugRain   = false;
bool lastDebugOut    = false;

enum MoveReason {
  REASON_BRIGHT,
  REASON_DARK,
  REASON_RAIN,
  REASON_RAIN_CLEARED,
  REASON_MANUAL_OUT,
  REASON_MANUAL_IN
};

// ----- MODE & COMMAND TỪ WEB -----
// systemMode: "auto" hoặc "manual"
String systemMode = "auto";
String lastWebCommand = "";
unsigned long lastCmdCheck = 0;
const unsigned long CMD_CHECK_INTERVAL = 1000; // ms

// Khi mới chuyển sang Auto -> show "Mode / Auto" 5s
unsigned long autoModeMsgUntil = 0;

// =============== OLED MODE ===============
enum DisplayMode {
  DISPLAY_STATUS,
  DISPLAY_TEMP,
  DISPLAY_HUM
};

DisplayMode displayMode = DISPLAY_STATUS;
unsigned long lastDisplayToggle = 0;
const unsigned long DISPLAY_INTERVAL = 10000;

// ================= OLED FUNCTIONS =================

void showCenteredBig(const char *text) {
  display.clearDisplay();
  display.setRotation(0);
  display.setFont(&FreeSansBold12pt7b);
  display.setTextColor(SSD1306_WHITE);

  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  int16_t x = (SCREEN_WIDTH - w) / 2;
  int16_t y = (SCREEN_HEIGHT + h) / 2;

  display.setCursor(x, y);
  display.println(text);
  display.display();
}

void showTwoLineBig(const char *line1, const char *line2) {
  display.clearDisplay();
  display.setRotation(0);
  display.setFont(&FreeSansBold12pt7b);
  display.setTextColor(SSD1306_WHITE);

  int16_t x1, y1; uint16_t w1, h1;
  display.getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
  int16_t xLine1 = (SCREEN_WIDTH - w1) / 2;
  int16_t yLine1 = 22;

  int16_t x2, y2; uint16_t w2, h2;
  display.getTextBounds(line2, 0, 0, &x2, &y2, &w2, &h2);
  int16_t xLine2 = (SCREEN_WIDTH - w2) / 2;
  int16_t yLine2 = 50;

  display.setCursor(xLine1, yLine1);
  display.println(line1);

  display.setCursor(xLine2, yLine2);
  display.println(line2);

  display.display();
}

// hiển thị "Mode / Auto" hoặc "Mode / Manual"
void showModeScreen(const char *modeText) {
  showTwoLineBig("Mode", modeText);
}

void showStatusStable() {
  if (isOut) showCenteredBig("Dang Phoi");
  else       showCenteredBig("Thu Xong");
}

void showTempBig() {
  if (isnan(lastTemp)) { showCenteredBig("DHT ERR"); return; }

  char value[16];
  snprintf(value, sizeof(value), "%.0f*C", lastTemp);

  showTwoLineBig("Nhiet Do", value);
}

void showHumBig() {
  if (isnan(lastHum)) { showCenteredBig("DHT ERR"); return; }

  char value[16];
  snprintf(value, sizeof(value), "%.0f%%", lastHum);

  showTwoLineBig("Do Am", value);
}

// ================= SERVO =================

// quay servo, có nháy LED nếu đang AUTO
void moveServoWithBlink(int pwm, int ledBlinkPin) {
  unsigned long start = millis();
  bool ledState = false;

  isMovingServo = true;

  // Nếu Manual: KHÔNG nháy LED, 2 LED luôn bật
  if (systemMode == "manual") {
    mg996.writeMicroseconds(pwm);
    delay(MOVE_TIME);
    mg996.writeMicroseconds(STOP_PWM);
    isMovingServo = false;

    digitalWrite(LED_THU, HIGH);
    digitalWrite(LED_PHOI, HIGH);
    return;
  }

  // AUTO: giữ logic cũ
  if (ledBlinkPin == LED_THU) digitalWrite(LED_PHOI, LOW);
  else digitalWrite(LED_THU, LOW);

  mg996.writeMicroseconds(pwm);

  while (millis() - start < MOVE_TIME) {
    digitalWrite(ledBlinkPin, ledState);
    ledState = !ledState;
    delay(200);
  }

  mg996.writeMicroseconds(STOP_PWM);
  isMovingServo = false;
}

void moveOut(MoveReason reason) {
  // Chỉ hiện chữ giải thích khi đang AUTO
  if (systemMode == "auto") {
    if (reason == REASON_RAIN_CLEARED) showCenteredBig("Tanh");
    else if (reason == REASON_BRIGHT) showCenteredBig("Sang");
    delay(700);

    showCenteredBig("Phoi Ra");
  }

  moveServoWithBlink(FORWARD_PWM, LED_PHOI);

  if (systemMode == "auto") {
    digitalWrite(LED_THU, LOW);
    digitalWrite(LED_PHOI, HIGH);
    showStatusStable();
    displayMode = DISPLAY_STATUS;
    lastDisplayToggle = millis();
  } else {
    // Manual: vẫn giữ màn Mode Manual
    digitalWrite(LED_THU, HIGH);
    digitalWrite(LED_PHOI, HIGH);
    showModeScreen("Manual");
  }
}

void moveIn(MoveReason reason) {
  if (systemMode == "auto") {
    if (reason == REASON_RAIN) showCenteredBig("Mua");
    else if (reason == REASON_DARK) showCenteredBig("Toi");
    delay(700);

    showCenteredBig("Thu Vao");
  }

  moveServoWithBlink(REVERSE_PWM, LED_THU);

  if (systemMode == "auto") {
    digitalWrite(LED_PHOI, LOW);
    digitalWrite(LED_THU, HIGH);
    showStatusStable();
    displayMode = DISPLAY_STATUS;
    lastDisplayToggle = millis();
  } else {
    digitalWrite(LED_THU, HIGH);
    digitalWrite(LED_PHOI, HIGH);
    showModeScreen("Manual");
  }
}

// ================== UPLOAD STATE + LOG HISTORY ==================

void uploadState(const String &reason) {
  if (!firebaseReady) return;

  // trạng thái hiện tại
  String st = isOut ? "out" : "in";

  // 1) Ghi vào /system/state
  Firebase.RTDB.setString(&fbdo, "/system/state", st);

  // 2) Ghi log vào /logs/{ts} với Unix time thực
  time_t now = time(nullptr);         // số giây từ 1/1/1970
  uint32_t ts = (uint32_t)now;

  // Nếu NTP chưa sync thì bỏ qua để đỡ log 1970
  if (ts < 1600000000) {              // ~ năm 2019
    Serial.println("Time chua sync, bo qua log");
    return;
  }

  String base = "/logs/" + String(ts); // ví dụ /logs/1732470100

  if (!Firebase.RTDB.setString(&fbdo, base + "/state", st)) {
    Serial.print("Log state err: ");
    Serial.println(fbdo.errorReason());
  }

  Firebase.RTDB.setString(&fbdo, base + "/mode",   systemMode);
  Firebase.RTDB.setInt(&fbdo,    base + "/ts",     ts);
  Firebase.RTDB.setString(&fbdo, base + "/reason", reason);
}

// ================== MODE CHANGE HANDLER ==================

void onModeChanged(const String &newMode) {
  Serial.print("MODE changed to: ");
  Serial.println(newMode);

  if (newMode == "manual") {
    // 2 LED luôn sáng
    digitalWrite(LED_THU, HIGH);
    digitalWrite(LED_PHOI, HIGH);

    // OLED chỉ hiện "Mode / Manual"
    showModeScreen("Manual");

    // Không xoay màn hình nữa
    autoModeMsgUntil = 0;
  } else { // auto
    // LED theo trạng thái OUT/IN
    if (isOut) {
      digitalWrite(LED_PHOI, HIGH);
      digitalWrite(LED_THU, LOW);
    } else {
      digitalWrite(LED_THU, HIGH);
      digitalWrite(LED_PHOI, LOW);
    }

    // OLED hiện "Mode / Auto" trong 5s
    showModeScreen("Auto");
    autoModeMsgUntil = millis() + 5000;

    // Sau đó sẽ quay lại xoay STATUS/TEMP/HUM như cũ
    displayMode = DISPLAY_STATUS;
    lastDisplayToggle = millis();
  }
}

// ================== SETUP ==================

void setup() {
  Serial.begin(115200);
  delay(1000);

  // ---------- WIFI MANAGER ----------
  WiFi.mode(WIFI_STA);
  WiFiManager wm;

  Serial.println("Dang kiem tra WiFi...");

  if (!wm.autoConnect("SmartDrying-Setup")) {
    Serial.println("Khong ket noi duoc WiFi. Reset...");
    delay(3000);
    ESP.restart();
  }

  Serial.print("WiFi connected: ");
  Serial.println(WiFi.localIP());

  // ---------- NTP TIME ----------  
  Serial.println("Sync NTP time...");
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time, log sẽ dùng 1970 :(");
  } else {
    Serial.print("Current time: ");
    Serial.println(&timeinfo, "%d/%m/%Y %H:%M:%S");
  }

  // ---------- FIREBASE ----------
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Sign in anonymously
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase Anonymous Auth OK");
    firebaseReady = true;
  } else {
    Serial.printf("Firebase Auth Error: %s\n", config.signer.signupError.message.c_str());
    firebaseReady = false;
  }

  Serial.println("Firebase ready.");

  // ----- TEST GHI THỬ LÊN FIREBASE -----
  Serial.println("Test ghi Firebase: system/state = \"booted\"");

  if (Firebase.RTDB.setString(&fbdo, "/system/state", "booted")) {
    Serial.println("Ghi thu thanh cong!");
  } else {
    Serial.print("Ghi thu THAT BAI: ");
    Serial.println(fbdo.errorReason());
  }

  // khởi tạo mode/command trên DB
  Firebase.RTDB.setString(&fbdo, "/system/mode", systemMode);
  Firebase.RTDB.setString(&fbdo, "/system/command", "stop");

  // ---------- OLED ----------
  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // ---------- SERVO ----------
  mg996.setPeriodHertz(50);
  mg996.attach(SERVO_PIN, 500, 2500);
  mg996.writeMicroseconds(STOP_PWM);

  // ---------- IO ----------
  pinMode(LDR_DO, INPUT);
  pinMode(RAIN_AO, INPUT);
  pinMode(LED_THU, OUTPUT);
  pinMode(LED_PHOI, OUTPUT);

  dht.begin();

  // Auto mặc định ban đầu: bật LED theo trạng thái
  if (isOut) {
    digitalWrite(LED_PHOI, HIGH);
    digitalWrite(LED_THU, LOW);
  } else {
    digitalWrite(LED_THU, HIGH);
    digitalWrite(LED_PHOI, LOW);
  }

  showStatusStable();
  lastDisplayToggle = millis();

  Serial.println("He thong gian phoi start.");
}

// ================== LOOP ==================

void loop() {

  unsigned long now = millis();

  // ----- ĐỌC CẢM BIẾN ÁNH SÁNG / MƯA -----
  bool isDark   = (digitalRead(LDR_DO) == HIGH);
  bool isBright = !isDark;
  int  rainVal   = analogRead(RAIN_AO);
  bool isRaining = (rainVal < RAIN_THRESHOLD);

  // ===== ĐỌC MODE + COMMAND TỪ WEB (1s/lần) =====
  if (firebaseReady && Firebase.ready() &&
      (millis() - lastCmdCheck > CMD_CHECK_INTERVAL)) {

    lastCmdCheck = millis();

    // 1) Đọc mode: /system/mode
    if (Firebase.RTDB.getString(&fbdo, "/system/mode")) {
      String m = fbdo.stringData();
      if (m == "auto" || m == "manual") {
        if (m != systemMode) {
          systemMode = m;
          onModeChanged(systemMode);
        }
      }
    }

    // 2) Đọc command: /system/command
    if (Firebase.RTDB.getString(&fbdo, "/system/command")) {
      String cmd = fbdo.stringData();

      if (cmd != lastWebCommand) {
        lastWebCommand = cmd;
        Serial.print("[WEB CMD] ");
        Serial.println(cmd);

        if (systemMode == "manual") {
          // CHỈ xử lý lệnh tay khi đang MANUAL
          if (cmd == "out" && !isOut) {
            Serial.println("-> Manual: PHOI RA");
            moveOut(REASON_MANUAL_OUT);
            isOut = true;
            uploadState("manual_out");
          } else if (cmd == "in" && isOut) {
            Serial.println("-> Manual: THU VAO");
            moveIn(REASON_MANUAL_IN);
            isOut = false;
            uploadState("manual_in");
          } else if (cmd == "stop") {
            Serial.println("-> Manual: DUNG");
            mg996.writeMicroseconds(STOP_PWM);
            isMovingServo = false;
            showModeScreen("Manual");
            // Nếu muốn log stop:
            // uploadState("manual_stop");
          }
        } else {
          // ĐANG AUTO -> bỏ qua lệnh tay
          Serial.println("-> Dang AUTO, bo qua command");
        }
      }
    }
  }

  // ===== LOGIC TỰ ĐỘNG CHỈ KHI MODE = AUTO =====
  if (systemMode == "auto") {
    bool shouldOut = (isBright && !isRaining);

    if (shouldOut && !isOut) {
      MoveReason reason = (lastIsRaining && !isRaining
                           ? REASON_RAIN_CLEARED
                           : REASON_BRIGHT);
      moveOut(reason);
      isOut = true;

      String reasonStr = (reason == REASON_RAIN_CLEARED)
                         ? "auto_rain_cleared"
                         : "auto_bright";
      uploadState(reasonStr);
    }
    else if (!shouldOut && isOut) {
      MoveReason reason = (!lastIsRaining && isRaining
                           ? REASON_RAIN
                           : REASON_DARK);
      moveIn(reason);
      isOut = false;

      String reasonStr = (reason == REASON_RAIN)
                         ? "auto_rain"
                         : "auto_dark";
      uploadState(reasonStr);
    }
  }

  // lưu lại trạng thái mưa / sáng để biết “vừa thay đổi”
  lastIsBright  = isBright;
  lastIsRaining = isRaining;

  // ===== ĐỌC DHT =====
  if (now - lastDhtRead >= DHT_INTERVAL) {
    lastDhtRead = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
      lastHum = h;
      lastTemp = t;
      // Sensor data is uploaded in periodic Firebase push section
    }
  }

  // ===== XOAY MÀN HÌNH OLED =====
  if (systemMode == "manual") {
    // Manual: luôn giữ "Mode / Manual", không xoay gì hết
    // (đã vẽ sẵn khi vào chế độ, nên ở đây không cần làm gì)
  } else {
    // AUTO: nếu vừa chuyển sang Auto thì giữ "Mode / Auto" 5s
    if (autoModeMsgUntil != 0 && now < autoModeMsgUntil) {
      // giữ nguyên màn Mode Auto
    } else {
      // quay lại xoay STATUS / TEMP / HUM như cũ
      if (!isMovingServo &&
          now - lastDisplayToggle >= DISPLAY_INTERVAL) {

        lastDisplayToggle = now;

        if (displayMode == DISPLAY_STATUS) displayMode = DISPLAY_TEMP;
        else if (displayMode == DISPLAY_TEMP) displayMode = DISPLAY_HUM;
        else displayMode = DISPLAY_STATUS;

        if (displayMode == DISPLAY_STATUS) showStatusStable();
        else if (displayMode == DISPLAY_TEMP) showTempBig();
        else showHumBig();
      }
    }
  }

  // ===== ĐẨY DỮ LIỆU LÊN FIREBASE ĐỊNH KỲ =====
  static unsigned long lastFbPush = 0;
  const unsigned long FB_INTERVAL = 2000; // 2 giây / lần

  if (Firebase.ready() && millis() - lastFbPush > FB_INTERVAL) {
    lastFbPush = millis();

    // Gửi sensor
    if (!isnan(lastTemp)) {
      Firebase.RTDB.setFloat(&fbdo, "/system/sensor/temperature", lastTemp);
    }
    if (!isnan(lastHum)) {
      Firebase.RTDB.setFloat(&fbdo, "/system/sensor/humidity", lastHum);
    }

    Firebase.RTDB.setInt(&fbdo, "/system/sensor/light",  isBright   ? 1 : 0);
    Firebase.RTDB.setInt(&fbdo, "/system/sensor/rain",   isRaining  ? 1 : 0);

    // Gửi trạng thái + mode
    Firebase.RTDB.setString(&fbdo, "/system/state", isOut ? "out" : "in");
    Firebase.RTDB.setString(&fbdo, "/system/mode", systemMode);

    // log lỗi nếu có
    if (fbdo.httpCode() != 200 && fbdo.httpCode() != 204) {
      Serial.print("Firebase error: ");
      Serial.println(fbdo.errorReason());
    }
  }

  delay(200);
}
