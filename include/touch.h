#ifndef TOUCH_H
#define TOUCH_H

#include <XPT2046_Touchscreen.h>

// 터치스크린 핀 설정 (main.cpp와 동일하게)
#define TOUCH_CS 5
#define TOUCH_IRQ 17

// 터치 보정값 (실측 좌표계 기준 - 90도 회전)
// Raw X: 3800(LT) ~ 250(RB)
// Raw Y: 400(LT) ~ 3800(RB)
#define RAW_X_MIN 280
#define RAW_X_MAX 3800
#define RAW_Y_MIN 350
#define RAW_Y_MAX 3720

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
