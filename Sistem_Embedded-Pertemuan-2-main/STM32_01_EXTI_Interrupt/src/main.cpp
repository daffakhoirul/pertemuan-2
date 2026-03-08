#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

// === OLED 0.91" SSD1306 128x32 (I2C: PB6=SCK, PB7=SDA) ===
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// === Pin Definitions ===
#define BUTTON_PIN  PB0
#define LED_PIN     PC13

// === Mode Definitions ===
enum Mode { ACTIVE, IDLE };
volatile Mode currentMode = ACTIVE;
volatile bool buttonPressed = false;
bool ledState = false;

unsigned long modeStartTime = 0;
const unsigned long ACTIVE_DURATION = 10000;  // 10 detik
const unsigned long IDLE_DURATION   = 15000;  // 15 detik

// === ISR: hanya set flag dengan debounce ===
volatile unsigned long lastISRTime = 0;
void buttonISR() {
    unsigned long now = millis();
    if (now - lastISRTime > 200) {  // debounce 200ms
        buttonPressed = true;
        lastISRTime = now;
    }
}

// === Update OLED ===
void updateOLED(const char* mode, int sisa, const char* ledSt, const char* btn) {
    char line1[22];
    char line2[22];
    snprintf(line1, sizeof(line1), "%s  %ds", mode, sisa);
    snprintf(line2, sizeof(line2), "LED:%s %s", ledSt, btn);

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB12_tr);  // Font besar untuk baris 1
    u8g2.drawStr(0, 14, line1);
    u8g2.setFont(u8g2_font_6x10_tr);     // Font kecil untuk baris 2
    u8g2.drawStr(0, 30, line2);
    u8g2.sendBuffer();
}

void setup() {
    Serial.begin(115200);
    delay(500);

    // Init OLED
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(30, 20, "Starting...");
    u8g2.sendBuffer();
    delay(1000);

    Serial.println("Program: EXTI Interrupt - Active/Idle Mode\n");

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);

    // LED kedip 3x tanda program jalan
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, LOW);  // ON
        delay(200);
        digitalWrite(LED_PIN, HIGH); // OFF
        delay(200);
    }

    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

    // Mulai mode ACTIVE
    currentMode = ACTIVE;
    modeStartTime = millis();
    ledState = false;
    updateOLED("AKTIF", 10, "OFF", "Btn:ON");
}

void loop() {
    unsigned long elapsed = millis() - modeStartTime;

    if (currentMode == ACTIVE) {
        int sisa = (ACTIVE_DURATION - elapsed) / 1000;
        if (sisa < 0) sisa = 0;

        // Cek button: interrupt ATAU polling
        static bool lastBtnState = HIGH;
        bool currentBtn = digitalRead(BUTTON_PIN);
        if (buttonPressed || (currentBtn == LOW && lastBtnState == HIGH)) {
            buttonPressed = false;
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState ? LOW : HIGH);
            delay(50);  // debounce
        }
        lastBtnState = currentBtn;

        updateOLED("AKTIF", sisa, ledState ? "ON" : "OFF", "Btn:ON");

        if (elapsed >= ACTIVE_DURATION) {
            currentMode = IDLE;
            modeStartTime = millis();
            ledState = true;
            digitalWrite(LED_PIN, LOW);
            updateOLED("IDLE", 15, "ON", "Btn:OFF");
        }
    }
    else {
        int sisa = (IDLE_DURATION - elapsed) / 1000;
        if (sisa < 0) sisa = 0;

        if (buttonPressed) {
            buttonPressed = false;  // Diabaikan
        }

        digitalWrite(LED_PIN, LOW);
        updateOLED("IDLE", sisa, "ON", "Btn:OFF");

        if (elapsed >= IDLE_DURATION) {
            currentMode = ACTIVE;
            modeStartTime = millis();
            ledState = false;
            digitalWrite(LED_PIN, HIGH);
            updateOLED("AKTIF", 10, "OFF", "Btn:ON");
        }
    }

    delay(200);
}
