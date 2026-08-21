#include "ScreenManager.h"
#include "config.h"
#include <Wire.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void startScreen(ScreenManager*) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // Bovenste decoratieve lijn
    display.drawLine(8, 6, 119, 6, SSD1306_WHITE);

    // Titel
    display.setTextSize(2);
    display.setCursor(10, 13);
    display.println(F("LUMENATOR"));

    // Ondertitel
    display.setTextSize(1);
    display.setCursor(27, 34);
    display.println(F("LED CONTROLLER"));

    // Onderste lijn
    display.drawLine(8, 45, 119, 45, SSD1306_WHITE);

    display.display();
    delay(900);

    // Voortgangsbalk
    display.drawRect(14, 53, 100, 8, SSD1306_WHITE);
    display.display();

    for (int width = 0; width <= 96; width += 4) {
        display.fillRect(16, 55, width, 4, SSD1306_WHITE);
        display.display();
        delay(35);
    }
}

void mainMenu(ScreenManager*) {
    // Bovenste decoratieve lijn
    display.clearDisplay();
    display.drawLine(8, 6, 119, 6, SSD1306_WHITE);

    // Titel
    display.setTextSize(2);
    display.setCursor(10, 13);
    display.println(F("Home"));

    // Ondertitel
    display.setTextSize(1);
    display.setCursor(0, 34);
    display.println(F("dit word een menu"));

    // Onderste lijn
    display.drawLine(8, 45, 119, 45, SSD1306_WHITE);

    display.display();
}

void ScreenManager::begin() {
    stackTop = 0;
    stack[0] = { .screen = ScreenId::Start, .expireAt = 0, .callback = startScreen };
    Wire.begin(I2C_SDA, I2C_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("[SM] Display not found"));
        return;
    }

    draw();

    stack[0] = { .screen = ScreenId::Home, .expireAt = 0, .callback = mainMenu };
    draw();
}

void ScreenManager::pushScreen(ScreenId id, void (*callback)(ScreenManager*), uint32_t durationMs) {
    if (stackTop + 1 >= SCREEN_STACK_DEPTH) {
        popScreen(); // simpele overflow-bescherming: oudste tijdelijke screen eruit
    }
    stackTop++;
    stack[stackTop] = {
        id,
        durationMs > 0 ? millis() + durationMs : 0,
        callback
    };
    draw();
}

void ScreenManager::popScreen() {
    if (stackTop > 0) {
        stackTop--;
        draw();
    }
}

void ScreenManager::showMessage(const char* title, const char* message, uint32_t durationMs) {
    strncpy(msgTitle, title, sizeof(msgTitle) - 1);
    msgTitle[sizeof(msgTitle) - 1] = '\0';
    strncpy(msgBody, message, sizeof(msgBody) - 1);
    msgBody[sizeof(msgBody) - 1] = '\0';

    pushScreen(ScreenId::Message, [](ScreenManager* mgr) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1);
        display.println(mgr->getMsgTitle());
        display.println(mgr->getMsgBody());
        display.display();
    }, durationMs);
}

void ScreenManager::update() {
    if (stackTop > 0 && stack[stackTop].expireAt != 0 && millis() >= stack[stackTop].expireAt) {
        popScreen();
    }
}

void ScreenManager::draw() {
    if (stack[stackTop].callback) {
        stack[stackTop].callback(this);
    }
}