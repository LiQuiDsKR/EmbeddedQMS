#ifndef TOUCH_H
#define TOUCH_H

#include <XPT2046_Touchscreen.h>

// 터치스크린 핀 설정 (main.cpp와 동일하게)
#define TOUCH_CS 5
#define TOUCH_IRQ 17

// 터치 보정값 (기존 프로젝트 값 유지)
#define RAW_X_MIN 205
#define RAW_X_MAX 3835
#define RAW_Y_MIN 435
#define RAW_Y_MAX 3715

class TouchModule {
private:
    XPT2046_Touchscreen ts;
    int screenWidth;
    int screenHeight;

public:
    TouchModule(int width, int height);
    void begin();
    bool isTouched();
    TS_Point getRawPoint();
    int mapX(int rawX);
    int mapY(int rawY);
    void getScreenCoordinates(int& screenX, int& screenY);
    void printRawCoordinates();
};

#endif
