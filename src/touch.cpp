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
    return map(rawX, RAW_X_MAX, RAW_X_MIN, 0, screenWidth);
}

int TouchModule::mapY(int rawY) {
    return map(rawY, RAW_Y_MAX, RAW_Y_MIN, 0, screenHeight);
}

void TouchModule::getScreenCoordinates(int& screenX, int& screenY) {
    if (isTouched()) {
        TS_Point p = getRawPoint();
        screenX = mapX(p.x);
        screenY = mapY(p.y);
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
