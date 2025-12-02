#include "touch.h"
#include <Arduino.h>

TouchModule::TouchModule(int width, int height) 
    : ts(TOUCH_CS, TOUCH_IRQ), screenWidth(width), screenHeight(height) {}

void TouchModule::begin() {
    ts.begin();
}

bool TouchModule::isTouched() {
    return ts.touched();
}

TS_Point TouchModule::getRawPoint() {
    return ts.getPoint();
}

int TouchModule::mapX(int rawX) {
    // Raw X를 Screen X로 매핑 (역방향)
    return map(rawX, RAW_X_MAX, RAW_X_MIN, 0, screenWidth);
}

int TouchModule::mapY(int rawY) {
    // Raw Y를 Screen Y로 매핑 (정방향)
    return map(rawY, RAW_Y_MIN, RAW_Y_MAX, 0, screenHeight);
}

void TouchModule::getScreenCoordinates(int& screenX, int& screenY) {
    if (isTouched()) {
        TS_Point p = getRawPoint();
        // 축 교환: Raw X → Screen Y, Raw Y → Screen X
        screenX = mapY(p.y);
        screenY = mapX(p.x);
    }
}

void TouchModule::printRawCoordinates() {
    if (isTouched()) {
        TS_Point p = getRawPoint();
        Serial.print("Raw X = ");
        Serial.print(p.x);
        Serial.print(", Raw Y = ");
        Serial.println(p.y);
    }
}
