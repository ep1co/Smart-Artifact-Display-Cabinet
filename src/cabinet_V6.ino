#define BLYNK_TEMPLATE_NAME "SmartCarbinet"
#define BLYNK_AUTH_TOKEN "Gs61NxHhMqRZ9NgjSQElXhh3xNra198O"
#define BLYNK_TEMPLATE_ID "TMPL6Fgj-IuTd"
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <DFRobotDFPlayerMini.h>

// ──────────────── CẤU HÌNH PHẦN CỨNG (GPIO) ────────────────
#define PIN_SG90        13  
#define PIN_TRIG        26  
#define PIN_ECHO        27  
#define PIN_IR          25  
#define PIN_MG996R      14  
#define PIN_BUZZER      33  
#define PIN_BUTTON      19  
#define PIN_ESP32_RX2   16  
#define PIN_ESP32_TX2   17  

// ──────────────── RADAR SPEED CONTROL ────────────────
// Tăng RADAR_STEP → quét nhanh hơn | Giảm → quét chậm hơn
// Tốc độ (°/s) = RADAR_STEP / RADAR_INTERVAL_MS * 1000
// Ví dụ: STEP=2, INTERVAL=20ms → 100°/s
#define RADAR_STEP         5
#define RADAR_INTERVAL_MS  30

// ──────────────── THÔNG SỐ CẤU HÌNH MẠNG ────────────────
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Wifi_4";     
char pass[] = "22345678"; 

Servo sg90;
Servo mg996r;
DFRobotDFPlayerMini dfPlayer;
HardwareSerial dfSerial(2); 

// ──────────────── BIẾN TOÀN CỤC HỆ THỐNG ────────────────
unsigned long lastRadarMove = 0;
int currentAngle = 30;
int angleDirection = RADAR_STEP;
float currentDistance = 0;

unsigned long lastWatchTime = 0;
unsigned long watchAccumulatedTime = 0;
int targetAngle = -1;

int currentPriority = 5;      
unsigned long audioEndTime = 0;
int watchSequenceState = 0;   

enum CabinetState { STOPPED, OPENING, CLOSING };
enum CabinetPos { UNKNOWN, OPEN, CLOSED };
CabinetState cabState = STOPPED;
CabinetPos cabPos = UNKNOWN;  
unsigned long cabActionStartTime = 0;

bool dangerActive = false;
bool buzzerMuted = true;
unsigned long lastBuzzerToggle = 0;
bool buzzerState = false;

int blynkMusicToggle = 0;
int physicalMusicToggle = 0;
bool lastBtnState = HIGH;
unsigned long lastDebounceTime = 0;

unsigned long lastTelemetry = 0;

bool dangerLogged = false;
bool irDangerLogged = false;
bool watchLogged = false;

// ──────────────── PHƯƠNG THỨC QUẢN LÝ ÂM THANH ƯU TIÊN ────────────────
void playAudio(int track, int priority, unsigned long durationMs) {
  if (millis() > audioEndTime) {
    currentPriority = 5;
  }
  if (priority <= currentPriority) {
    dfPlayer.play(track);
    currentPriority = priority;
    audioEndTime = millis() + durationMs;
  }
}

// ──────────────── ĐIỀU KHIỂN TỦ (NON-BLOCKING) ────────────────
void startOpening() {
  if (cabPos != OPEN) {
    cabState = OPENING;
    cabActionStartTime = millis();
    mg996r.writeMicroseconds(1300); 
    Blynk.virtualWrite(V1, "Cabinet Status: OPENING...");
  }
}

void startClosing() {
  if (cabPos != CLOSED) {
    cabState = CLOSING;
    cabActionStartTime = millis();
    mg996r.writeMicroseconds(1700); 
    Blynk.virtualWrite(V1, "Cabinet Status: CLOSING...");
  }
}

void runCabinetStateMachine() {
  if (cabState != STOPPED) {
    if (millis() - cabActionStartTime >= 3500) { 
      mg996r.writeMicroseconds(1500); 
      if (cabState == OPENING) {
        cabPos = OPEN;
        Blynk.virtualWrite(V1, "Cabinet Status: OPEN");
      } else if (cabState == CLOSING) {
        cabPos = CLOSED;
        Blynk.virtualWrite(V1, "Cabinet Status: CLOSED");
      }
      cabState = STOPPED;
    }
  }
}

// ──────────────── CẢM BIẾN HỒNG NGOẠI IR (BẪY TRÊN CAO) ────────────────
void checkIRSensor() {
  int irState = digitalRead(PIN_IR);
  
  if (irState == LOW) {
    if (!dangerActive) {
      dangerActive = true;
      buzzerMuted = false; // Reset mute khi có nguy hiểm mới
      startClosing();        
      
      if (!irDangerLogged) {
        Blynk.logEvent("intrusion_ir");
        Blynk.virtualWrite(V1, "ALERT: IR TOP DANGER!");
        irDangerLogged = true;
      }
    }
  }
}

// ──────────────── XỬ LÝ CÁC PHÂN VÙNG RADAR (ZONES) ────────────────
void processZones() {
  // --- ZONE DANGER (< 10cm) ---
  if (currentDistance < 10 && currentDistance > 0) {
    if (!dangerActive) {
      dangerActive = true;
      buzzerMuted = false; // Reset mute khi có nguy hiểm mới
      startClosing(); 
      
      if (!dangerLogged) {
        Blynk.logEvent("intrusion_radar");
        Blynk.virtualWrite(V1, "ALERT: RADAR DANGER!");
        dangerLogged = true;
      }
    }
    return; 
  }

  // --- ZONE WARN (10–15cm) ---
  if (currentDistance >= 10 && currentDistance <= 15) {
    if (currentPriority > 1 || (currentPriority == 1 && millis() > audioEndTime)) {
      playAudio(1, 1, 7000); 
      Blynk.virtualWrite(V1, "Zone: WARN - Radar Object near");
    }
    lastWatchTime = 0;
    return;
  }

  // --- ZONE WATCH (15–20cm) ---
  if (currentDistance > 15 && currentDistance <= 20) {
    if (lastWatchTime == 0) {
      lastWatchTime = millis();
      targetAngle = currentAngle;
    }
    
    unsigned long dt = millis() - lastWatchTime;
    lastWatchTime = millis();

    if (abs(currentAngle - targetAngle) <= 15) {
      watchAccumulatedTime += dt;
    } else {
      targetAngle = currentAngle; 
      watchAccumulatedTime += dt;
    }

    if (watchAccumulatedTime >= 10000) { 
      watchSequenceState = 1; 
      playAudio(2, 4, 16000); 
      Blynk.virtualWrite(V1, "Zone: WATCH - Triggering Intro");
      
      if (!watchLogged) {
        Blynk.logEvent("watch_alert");
        watchLogged = true;
      }
      watchAccumulatedTime = 0; 
    }
    return;
  }

  // --- ZONE CLEAR (> 20cm) ---
  if (currentDistance > 20 && digitalRead(PIN_IR) == HIGH) {
    if (dangerActive) {
      dangerActive = false;
      // KHÔNG reset buzzerMuted ở đây
      // Buzzer tiếp tục toggle cho đến khi staff bấm V4
      Blynk.virtualWrite(V1, "Cabinet Status: Safe");
      
      dangerLogged = false;
      irDangerLogged = false;
      watchLogged = false;
    }
  }
}

// ──────────────── QUÉT RADAR MỖI 20MS (NON-BLOCKING) ────────────────
void runRadarSweep() {
  if (millis() - lastRadarMove >= RADAR_INTERVAL_MS) {
    lastRadarMove = millis();

    currentAngle += angleDirection;
    if (currentAngle >= 170) {
      currentAngle = 170;
      angleDirection = -RADAR_STEP;
    } else if (currentAngle <= 30) {
      currentAngle = 30;
      angleDirection = RADAR_STEP;
    }
    int pulseWidth = map(currentAngle, 30, 170, 817, 2294);
    sg90.writeMicroseconds(pulseWidth);

    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);

    long duration = pulseIn(PIN_ECHO, HIGH, 25000); 
    if (duration == 0) {
      currentDistance = 999; 
    } else {
      currentDistance = duration * 0.034 / 2;
    }

    Serial.print("Goc: "); Serial.print(currentAngle);
    Serial.print(" | Khoang cach: "); Serial.println(currentDistance);

    processZones();
  }
}

// ──────────────── ĐIỀU KHIỂN CHUỖI NHẠC TỰ ĐỘNG & BUZZER ────────────────
void runSystemFeedback() {
  if (millis() > audioEndTime) {
    currentPriority = 5; 
    if (watchSequenceState == 1) {
      watchSequenceState = 2;
      playAudio(3, 4, 16000); 
    } else if (watchSequenceState == 2) {
      watchSequenceState = 0; 
    }
  }

  // Buzzer toggle liên tục khi chưa bị mute
  // Không phụ thuộc vào dangerActive — chỉ dừng khi staff bấm V4
  if (!buzzerMuted) {
    if (millis() - lastBuzzerToggle >= 150) {
      lastBuzzerToggle = millis();
      buzzerState = !buzzerState;
      digitalWrite(PIN_BUZZER, buzzerState ? HIGH : LOW);
    }
  } else {
    digitalWrite(PIN_BUZZER, LOW);
    buzzerState = false;
  }
}

// ──────────────── QUẢN LÝ NÚT NHẤN VẬT LÝ ────────────────
void runPhysicalButton() {
  int reading = digitalRead(PIN_BUTTON);

  if (reading != lastBtnState) {
    lastDebounceTime = millis();
    lastBtnState = reading; // cập nhật ngay khi có thay đổi
  }

  if ((millis() - lastDebounceTime) > 50) {
    if (reading == LOW) { // chỉ cần LOW là đủ, không cần check lastBtnState
      Serial.println("Button pressed!");
      physicalMusicToggle++;
      if (physicalMusicToggle % 2 != 0) {
        playAudio(2, 3, 16000);
      } else {
        playAudio(3, 3, 17000);
      }
      Blynk.virtualWrite(V1, "Guest pressed physical button");
      lastDebounceTime = millis() + 500; // chặn spam nhấn liên tục
    }
  }
}
// ──────────────── ĐẨY DỮ LIỆU ĐỊNH KỲ LÊN BLYNK ────────────────
void sendBlynkTelemetry() {
  if (millis() - lastTelemetry >= 200) { 
    lastTelemetry = millis();
    if(Blynk.connected()) {
      Blynk.virtualWrite(V0, currentDistance);
    }
  }
}

// ──────────────── GIAO TIẾP VỚI CÁC NÚT NHẤN APP BLYNK ────────────────
BLYNK_WRITE(V2) { 
  if (param.asInt() == 1) {
    dangerActive = false; 
    dangerLogged = false;
    irDangerLogged = false;
    buzzerMuted = false;
    startOpening();       
    Blynk.virtualWrite(V1, "Staff Override: Opening Cabinet");
  }
}

BLYNK_WRITE(V3) { 
  if (param.asInt() == 1) {
    blynkMusicToggle++;
    if (blynkMusicToggle % 2 != 0) {
      playAudio(2, 2, 16000); 
    } else {
      playAudio(3, 2, 17000); 
    }
    Blynk.virtualWrite(V1, "Employee triggered music");
  }
}

BLYNK_WRITE(V4) { 
  if (param.asInt() == 1) {
    buzzerMuted = true;
    digitalWrite(PIN_BUZZER, LOW); // Tắt ngay lập tức
    Blynk.virtualWrite(V1, "Buzzer Muted by Staff");
  }
}

BLYNK_WRITE(V5) { 
  if (param.asInt() == 1) {
    startClosing();
  }
}

// ──────────────── KHỞI TẠO HỆ THỐNG BAN ĐẦU ────────────────
void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_IR, INPUT); 
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  digitalWrite(PIN_BUZZER, LOW);

  sg90.attach(PIN_SG90);
  mg996r.attach(PIN_MG996R);
  
  sg90.writeMicroseconds(817);    
  mg996r.writeMicroseconds(1500); 

  dfSerial.begin(9600, SERIAL_8N1, PIN_ESP32_RX2, PIN_ESP32_TX2);
  if (dfPlayer.begin(dfSerial)) {
    dfPlayer.volume(30); 
  }

  Blynk.begin(auth, ssid, pass);
  Blynk.virtualWrite(V1, "System Ready - Safe");
}

// ──────────────── VÒNG LẶP CHÍNH ────────────────
void loop() {
  Blynk.run();
  
  checkIRSensor();          
  runRadarSweep();          
  runCabinetStateMachine(); 
  runSystemFeedback();      
  runPhysicalButton();      
  sendBlynkTelemetry();     
}
