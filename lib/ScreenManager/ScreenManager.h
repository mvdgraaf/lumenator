#ifndef LUMENATOR_SCREENMANAGER_H
#define LUMENATOR_SCREENMANAGER_H

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_STACK_DEPTH 4

#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;

enum class ScreenId {
    Start,
    Home,
    Message,
};

class ScreenManager;

struct ScreenState {
    ScreenId screen;
    uint32_t expireAt;      // millis() waarop dit scherm terug moet, 0 = geen timeout
    void (*callback)(ScreenManager*); // draw-functie voor dit scherm
};

class ScreenManager {
    public:
        void begin();
        void update();
        void showMessage(const char* title, const char* message, uint32_t durationMs = 3000);

        void pushScreen(ScreenId id, void (*callback)(ScreenManager*), uint32_t durationMs = 0);
        void popScreen();        // handmatig terug
    private:
        void draw();
        bool initialized = false;

        ScreenState stack[SCREEN_STACK_DEPTH] = {};
        uint8_t stackTop = 0;

        char msgTitle[32] = {};
        char msgBody[64] = {};
        const char* getMsgTitle() const { return msgTitle; }
        const char* getMsgBody() const { return msgBody; }
};




#endif //LUMENATOR_SCREENMANAGER_H
