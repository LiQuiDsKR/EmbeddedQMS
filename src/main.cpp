#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "touch.h"

// ST7789 디스플레이 핀 설정
#define TFT_CS    15
#define TFT_RST   4
#define TFT_DC    2
#define TFT_MOSI  23
#define TFT_SCLK  18
#define TFT_BL    32

// 화면 크기 (ST7789는 240x320)
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320

// 색상 정의
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F

// TFT 및 터치스크린 객체 생성
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
TouchModule touchModule(SCREEN_WIDTH, SCREEN_HEIGHT);

// 함수 선언
void drawButton(int x, int y, int w, int h, const char* label, uint16_t color);
bool isButtonPressed(int touchX, int touchY, int btnX, int btnY, int btnW, int btnH);
void flashButton(int x, int y, int w, int h, const char* label, uint16_t color);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=================================");
  Serial.println("ESP32 Touch Display Starting...");
  Serial.println("=================================");
  
  // SPI 초기화
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  
  // 백라이트 핀 설정
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  // TFT 디스플레이 초기화 (ST7789 240x320)
  tft.init(240, 320, SPI_MODE3);
  tft.setRotation(0);  // 회전 설정 (0=세로, 1=가로, 2=세로반전, 3=가로반전)
  tft.fillScreen(COLOR_BLACK);
  delay(100);  // 안정화 대기
  
  // 터치스크린 초기화
  touchModule.begin();
  
  // 초기 화면 그리기
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Touch Display");
  tft.println("Ready!");
  tft.setTextSize(1);
  tft.setCursor(10, 50);
  tft.println("Touch screen to test");
  
  // 간단한 UI 요소 그리기
  drawButton(20, 80, 200, 60, "Button 1", COLOR_BLUE);
  drawButton(20, 160, 200, 60, "Button 2", COLOR_GREEN);
  
  Serial.println("Setup complete!");
  Serial.println("Touch the screen...");
  Serial.println("---------------------------------");
}

void loop() {
  // 터치 감지
  if (touchModule.isTouched()) {
    // Raw 좌표 출력
    TS_Point p = touchModule.getRawPoint();
    Serial.print("Raw Touch: X=");
    Serial.print(p.x);
    Serial.print(", Y=");
    Serial.print(p.y);
    
    // 화면 좌표로 변환
    int screenX = touchModule.mapX(p.x);
    int screenY = touchModule.mapY(p.y);
    
    // 범위 제한
    screenX = constrain(screenX, 0, SCREEN_WIDTH);
    screenY = constrain(screenY, 0, SCREEN_HEIGHT);
    
    Serial.print(" | Screen: X=");
    Serial.print(screenX);
    Serial.print(", Y=");
    Serial.println(screenY);
    
    // 터치 위치에 점 그리기
    tft.fillCircle(screenX, screenY, 3, COLOR_RED);
    
    // 버튼 영역 체크
    if (isButtonPressed(screenX, screenY, 20, 80, 200, 60)) {
      Serial.println(">>> Button 1 PRESSED!");
      flashButton(20, 80, 200, 60, "Button 1", COLOR_BLUE);
    }
    else if (isButtonPressed(screenX, screenY, 20, 160, 200, 60)) {
      Serial.println(">>> Button 2 PRESSED!");
      flashButton(20, 160, 200, 60, "Button 2", COLOR_GREEN);
    }
    
    delay(100);  // 디바운스
  }
}

// 버튼 그리기 함수
void drawButton(int x, int y, int w, int h, const char* label, uint16_t color) {
  tft.fillRoundRect(x, y, w, h, 8, color);
  tft.drawRoundRect(x, y, w, h, 8, COLOR_WHITE);
  
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(2);
  
  // 텍스트 중앙 정렬 (수동 계산)
  int16_t labelLen = strlen(label) * 12;  // 대략적인 문자 폭
  int16_t textX = x + (w - labelLen) / 2;
  int16_t textY = y + (h - 16) / 2;  // 텍스트 높이 고려
  
  tft.setCursor(textX, textY);
  tft.print(label);
}

// 버튼 눌림 확인 함수
bool isButtonPressed(int touchX, int touchY, int btnX, int btnY, int btnW, int btnH) {
  return (touchX >= btnX && touchX <= (btnX + btnW) && 
          touchY >= btnY && touchY <= (btnY + btnH));
}

// 버튼 눌림 효과 함수
void flashButton(int x, int y, int w, int h, const char* label, uint16_t color) {
  // 밝게
  drawButton(x, y, w, h, label, COLOR_WHITE);
  delay(100);
  // 원래 색상으로
  drawButton(x, y, w, h, label, color);
}
