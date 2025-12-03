#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "touch.h"
#include "goadminbtn_32x32.h"
#include "gouserbtn_32x32.h"

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
#define PADDING 20  // 터치 오차를 위한 외곽 패딩

// 색상 정의 (반전 적용)
#define COLOR_BLACK   0xFFFF
#define COLOR_WHITE   0x0000
#define COLOR_RED     0x07FF
#define COLOR_GREEN   0xF81F
#define COLOR_BLUE    0xFFE0

// 사용자 화면 색상
#define COLOR_USER_BG 0xdf1e        // 사용자 화면 배경색
#define COLOR_USER_TEXT 0x32b2      // 사용자 글자색 & 버튼 테두리 색

// 관리자 화면 색상
#define COLOR_ADMIN_BG 0xfdb6       // 관리자 배경색
#define COLOR_ADMIN_TEXT 0xb800     // 관리자 글자색 & 버튼 테두리 색
#define COLOR_ADMIN_BTN 0xfe99      // 관리자 버튼 배경색

// RGB565 색상 반전 함수
inline uint16_t invertColor(uint16_t color) {
  return ~color;
}

// 화면 상태
enum ScreenState {
  USER_MODE,          // 사용자 모드 메인
  ADMIN_LOGIN,        // 관리자 로그인
  ADMIN_MODE,         // 관리자 모드
  TICKET_ISSUED,      // 대기번호 발행 화면
  QUEUE_FULL,         // 대기열 꿽 차서 예약 불가
  CALL_MODAL,         // 호출 모달
  QUEUE_LIST,         // 대기열 현황 조회
  QUEUE_DELETE_CONFIRM, // 대기열 삭제 확인
  TIME_SETTING,       // 사용자당 처리 시간 설정
  PASSWORD_CHANGE     // 관리자 비밀번호 변경
};

// 전역 변수
ScreenState currentScreen = USER_MODE;
int currentTicket = 0;            // 현재 발행된 마지막 번호
int waitingCount = 0;             // 현재 대기 인원
int waitingTimeSec = 0;           // 예상 대기시간(초)
int issuedTicketWaitTime = 0;     // 발행된 티켓의 대기시간(초)
int issuedTicket = 0;             // 방금 발행된 번호
String adminPassword = "";        // 관리자 비밀번호 입력 버퍼
String correctPassword = "5678";  // 관리자 실제 비밀번호
unsigned long ticketIssueTime = 0; // 번호 발행 시간
int callWaitPosition = 1;         // 대기열 내 본인 순번

// 대기열 관리
int queueList[20];   // 대기열 번호 목록 (최대 20명)
int queueCount = 0;  // 현재 대기열 개수
int selectedQueueIndex = -1;  // 삭제 선택된 queue 인덱스
int userProcessTimeSec = 60;  // 1명당 처리 시간(초)
String newPassword = "";  // 변경할 새 비밀번호 입력 버퍼

// 터치 디버깅용
int lastTouchX = -1;
int lastTouchY = -1;
unsigned long touchDisplayTime = 0;

// 대기열 자동 처리 타이머
unsigned long lastProcessTime = 0;

// 동적 대기시간 표시용
int lastDisplayedWaitMin = -1;
int lastDisplayedWaitSec = -1;
unsigned long lastWaitTimeUpdate = 0;

// 터치 안정화 (물리적 손상 대비)
int touchStabilizeCount = 0;
int touchStabilizeX = -1;
int touchStabilizeY = -1;
const int TOUCH_STABILIZE_THRESHOLD = 50;  // 50회 연속 동일 지점 확인
const int TOUCH_STABILIZE_AREA = 10;       // 동일 영역 오차 범위(px)

// TFT 및 터치스크린 객체 생성
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
TouchModule touchModule(SCREEN_HEIGHT, SCREEN_WIDTH);

// 함수 선언
void drawUserMode();
void drawAdminLogin();
void drawAdminMode();
void drawTicketIssued();
void drawQueueFull();
void drawCallModal();
void drawQueueList();
void drawQueueDeleteConfirm();
void drawTimeSetting();
void drawPasswordChange();
void handleTouch(int x, int y);
void handleAdminLoginTouch(int x, int y);
void handlePasswordChangeTouch(int x, int y);
void addToQueue(int ticketNum);
void removeFromQueue(int index);
void drawInvertedRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[], int16_t w, int16_t h);

// ===== 유틸리티 함수 구현 =====

// RGB565 색상 반전 이미지 그리기
void drawInvertedRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[], int16_t w, int16_t h) {
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      uint16_t color = pgm_read_word(&bitmap[j * w + i]);
      tft.drawPixel(x + i, y + j, ~color);  // 색상 반전
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=================================");
  Serial.println("Embedded QMS Starting...");
  Serial.println("=================================");
  
  // SPI 초기화
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  
  // 백라이트 핀 설정
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  // TFT 디스플레이 초기화
  tft.init(240, 320, SPI_MODE3);
  tft.setRotation(0);
  
  // 터치스크린 초기화
  touchModule.begin();
  
  // 초기 화면 그리기
  drawUserMode();
  
  Serial.println("QMS System Ready!");
}

void loop() {
  // 터치 감지
  if (touchModule.isTouched()) {
    int screenX, screenY;
    touchModule.getScreenCoordinates(screenX, screenY);
    
    screenX = constrain(screenX, 0, SCREEN_WIDTH - 1);
    screenY = constrain(screenY, 0, SCREEN_HEIGHT - 1);
    
    // 터치 안정화: 10x10 내에서 3회 연속 터치 시 인정
    if (touchStabilizeCount == 0) {
      // 첫 터치: 기준점 설정
      touchStabilizeX = screenX;
      touchStabilizeY = screenY;
      touchStabilizeCount = 1;
      Serial.print("Touch stabilize [1/3]: X=");
      Serial.print(screenX);
      Serial.print(", Y=");
      Serial.println(screenY);
    } else {
      // 이전 터치와 거리 비교
      int deltaX = abs(screenX - touchStabilizeX);
      int deltaY = abs(screenY - touchStabilizeY);
      
      if (deltaX <= TOUCH_STABILIZE_AREA && deltaY <= TOUCH_STABILIZE_AREA) {
        // 같은 영역 내 터치
        touchStabilizeCount++;
        Serial.print("Touch stabilize [");
        Serial.print(touchStabilizeCount);
        Serial.print("/3]: X=");
        Serial.print(screenX);
        Serial.print(", Y=");
        Serial.println(screenY);
        
        if (touchStabilizeCount >= TOUCH_STABILIZE_THRESHOLD) {
          // 3회 연속 성공 → 실제 터치로 처리
          int avgX = (touchStabilizeX + screenX) / 2;
          int avgY = (touchStabilizeY + screenY) / 2;
          
          Serial.print("Touch CONFIRMED: X=");
          Serial.print(avgX);
          Serial.print(", Y=");
          Serial.println(avgY);
          
          // 터치 처리
          handleTouch(avgX, avgY);
          
          // 안정화 리셋
          touchStabilizeCount = 0;
          
          delay(200);  // 디바운스
        }
      } else {
        // 기준점 벗어남 → 초기화
        touchStabilizeX = screenX;
        touchStabilizeY = screenY;
        touchStabilizeCount = 1;
        Serial.println("Touch area changed, reset counter");
      }
    }
  } else {
    // 터치 없음: 카운터 초기화
    if (touchStabilizeCount > 0) {
      touchStabilizeCount = 0;
    }
  }
  
  // 사용자 화면에서 매 초마다 대기시간 동적 업데이트
  if (currentScreen == USER_MODE && queueCount > 0) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastWaitTimeUpdate >= 1000) {
      lastWaitTimeUpdate = currentMillis;
      
      // 남은 시간 계산 (현재 처리 중인 사람의 남은 시간 포함)
      unsigned long elapsedSinceLastProcess = (currentMillis - lastProcessTime) / 1000;
      int remainingForCurrent = userProcessTimeSec - elapsedSinceLastProcess;
      if (remainingForCurrent < 0) remainingForCurrent = 0;
      
      int totalRemainingSec = remainingForCurrent + (queueCount - 1) * userProcessTimeSec;
      int mins = totalRemainingSec / 60;
      int secs = totalRemainingSec % 60;
      
      // 숫자가 변경된 경우만 부분 업데이트
      if (mins != lastDisplayedWaitMin || secs != lastDisplayedWaitSec) {
        lastDisplayedWaitMin = mins;
        lastDisplayedWaitSec = secs;
        
        // 시간 표시 영역만 지우고 다시 그리기
        tft.fillRect(PADDING + 84, PADDING + 75, 60, 10, invertColor(COLOR_USER_BG));
        tft.setTextColor(invertColor(COLOR_USER_TEXT));
        tft.setTextSize(1);
        tft.setCursor(PADDING + 84, PADDING + 75);
        if (mins < 10) tft.print("0");
        tft.print(mins);
        tft.print(":");
        if (secs < 10) tft.print("0");
        tft.print(secs);
      }
    }
  }
  
  // 티켓 발행 화면 → 10초 뒤 자동 복귀
  if (currentScreen == TICKET_ISSUED && millis() - ticketIssueTime > 10000) {
    currentScreen = USER_MODE;
    drawUserMode();
  }
  
  // 대기열 꾽 차서 화면 → 10초 뒤 자동 복귀
  if (currentScreen == QUEUE_FULL && millis() - ticketIssueTime > 10000) {
    currentScreen = USER_MODE;
    drawUserMode();
  }

  // 대기열 자동 처리
  if (queueCount > 0 && millis() - lastProcessTime >= userProcessTimeSec * 1000) {
    removeFromQueue(0);
    lastProcessTime = millis();
    if (currentScreen == USER_MODE) {
      drawUserMode();
    }
  }
}

// ===== UI 그리기 함수 =====

void drawUserMode() {
  tft.fillScreen(invertColor(COLOR_USER_BG));
  
  // 상단 제목
  tft.setTextColor(invertColor(COLOR_USER_TEXT));
  tft.setTextSize(2);
  tft.setCursor(PADDING, PADDING);
  tft.print("Embedded QMS");
  
  // 관리자 버튼 아이콘 (우측 상단 32x32) - 색상 반전
  drawInvertedRGBBitmap(SCREEN_WIDTH-PADDING-32, PADDING, goadminbtn_32x32, 32, 32);
  
  // 사용자 모드 표시
  tft.setTextSize(1);
  tft.setCursor(PADDING, PADDING+25);
  tft.print("User Mode");
  
  // 대기 정보 표시
  tft.setTextSize(1);
  tft.setCursor(PADDING, PADDING+55);
  tft.print("Current Waiting: ");
  tft.print(waitingCount);
  tft.print(" people");
  
  tft.setCursor(PADDING, PADDING+75);
  tft.print("Expected Wait: ");
  
  // 남은 시간 계산
  unsigned long elapsedSinceLastProcess = (millis() - lastProcessTime) / 1000;
  int remainingForCurrent = userProcessTimeSec - elapsedSinceLastProcess;
  if (remainingForCurrent < 0) remainingForCurrent = 0;
  int totalRemainingSec = (queueCount > 0) ? (remainingForCurrent + (queueCount - 1) * userProcessTimeSec) : 0;
  
  int mins = totalRemainingSec / 60;
  int secs = totalRemainingSec % 60;
  lastDisplayedWaitMin = mins;
  lastDisplayedWaitSec = secs;
  
  if (mins < 10) tft.print("0");
  tft.print(mins);
  tft.print(":");
  if (secs < 10) tft.print("0");
  tft.print(secs);
  
  // 대기표 발행 버튼 (중앙 큰 버튼)
  int btnW = SCREEN_WIDTH - PADDING*2;
  int btnH = 70;
  int btnX = PADDING;
  int btnY = SCREEN_HEIGHT - PADDING - btnH - 30;
  tft.fillRect(btnX, btnY, btnW, btnH, invertColor(0xfb4d));  // 버튼 배경색
  tft.drawRect(btnX, btnY, btnW, btnH, invertColor(0xb800));  // 버튼 테두리
  tft.setTextColor(invertColor(0xffff));  // 버튼 글씨
  tft.setTextSize(2);
  tft.setCursor(btnX+40, btnY+25);
  tft.print("Join Queue");
}

void drawAdminLogin() {
  tft.fillScreen(invertColor(COLOR_ADMIN_BG));
  
  // 상단 제목
  tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
  tft.setTextSize(2);
  tft.setCursor(PADDING, PADDING);
  tft.print("Embedded QMS");
  
  // 서브 타이틀
  tft.setTextSize(1);
  tft.setCursor(PADDING, PADDING+25);
  tft.print("Admin Login - Hex");
  
  // 비밀번호 표시 영역
  tft.fillRect(PADDING+10, PADDING+50, 160, 35, invertColor(COLOR_ADMIN_TEXT));
  tft.setTextColor(invertColor(COLOR_GREEN));
  tft.setTextSize(3);
  tft.setCursor(PADDING+20, PADDING+57);
  tft.print(adminPassword);
  
  // 16진수 키패드 (4x4)
  const char* keys[16] = {"1", "2", "3", "4", "5", "6", "7", "8", 
                          "9", "0", "A", "B", "C", "D", "E", "F"};
  
  int keySize = 38;  // 축소 사이즈
  int keyGap = 4;
  int startX = PADDING;
  int startY = PADDING + 105;
  
  for (int i = 0; i < 16; i++) {
    int row = i / 4;
    int col = i % 4;
    int x = startX + col * (keySize + keyGap);
    int y = startY + row * (keySize + keyGap);
    
    tft.fillRect(x, y, keySize, keySize, invertColor(COLOR_ADMIN_BTN));
    tft.drawRect(x, y, keySize, keySize, invertColor(COLOR_ADMIN_TEXT));
    tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
    tft.setTextSize(2);
    
    tft.setCursor(x + 12, y + 12);
    tft.print(keys[i]);
  }
  
  // 우측 기능 버튼 (BACK, CLEAR, DEL, ENTER)
  int funcX = SCREEN_WIDTH - PADDING - 30;
  for (int i = 0; i < 4; i++) {
    int y = startY + i * (keySize + keyGap);
    tft.fillRect(funcX, y, 30, keySize, invertColor(COLOR_ADMIN_BTN));
    tft.drawRect(funcX, y, 30, keySize, invertColor(COLOR_ADMIN_TEXT));
    tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
    tft.setTextSize(1);
    
    tft.setCursor(funcX + 2, y + 16);
    if (i == 0) tft.print("BCK");
    else if (i == 1) tft.print("CLR");
    else if (i == 2) tft.print("DEL");
    else tft.print("ENT");
  }
}

void drawAdminMode() {
  tft.fillScreen(invertColor(COLOR_ADMIN_BG));
  
  // 상단 제목
  tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
  tft.setTextSize(2);
  tft.setCursor(PADDING, PADDING);
  tft.print("Embedded QMS");
  
  // 사용자 모드 복귀 버튼 (우측 상단 32x32) - 색상 반전
  drawInvertedRGBBitmap(SCREEN_WIDTH-PADDING-32, PADDING, gouserbtn_32x32, 32, 32);
  
  // 관리자 모드 표시
  tft.setTextSize(1);
  tft.setCursor(PADDING, PADDING+25);
  tft.print("Admin Mode");
  
  // 메뉴 3개
  const char* menus[3] = {"Waiting Call", "User Time Setting", "Admin Password"};
  int btnY = PADDING + 70;
  int btnHeight = 50;
  int btnGap = 20;
  int btnW = SCREEN_WIDTH - PADDING*2;
  
  for (int i = 0; i < 3; i++) {
    int y = btnY + i * (btnHeight + btnGap);
    tft.fillRect(PADDING, y, btnW, btnHeight, invertColor(COLOR_ADMIN_BTN));
    tft.drawRect(PADDING, y, btnW, btnHeight, invertColor(COLOR_ADMIN_TEXT));
    tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
    tft.setTextSize(1);
    tft.setCursor(PADDING+10, y + 20);
    tft.print(menus[i]);
  }
}

void drawTicketIssued() {
  tft.fillScreen(invertColor(COLOR_USER_BG));
  
  // 상단 제목
  tft.setTextColor(invertColor(COLOR_USER_TEXT));
  tft.setTextSize(2);
  tft.setCursor(PADDING, PADDING);
  tft.print("Embedded QMS");
  
  int startY = PADDING + 60;
  
  // Waiting Number (크기 1)
  tft.setTextColor(invertColor(COLOR_USER_TEXT));
  tft.setTextSize(1);
  tft.setCursor(PADDING, startY);
  tft.print("Waiting Number");
  
  // 대기번호 (크기 3)
  tft.setTextColor(COLOR_BLUE);
  tft.setTextSize(3);
  tft.setCursor(PADDING, startY + 15);
  if (issuedTicket < 100) tft.print("0");
  if (issuedTicket < 10) tft.print("0");
  tft.print(issuedTicket);
  
  // Your Position (크기 1)
  tft.setTextColor(invertColor(COLOR_USER_TEXT));
  tft.setTextSize(1);
  tft.setCursor(PADDING, startY + 60);
  tft.print("Your Position");
  
  // 순번 (크기 2)
  tft.setTextSize(2);
  tft.setCursor(PADDING, startY + 75);
  tft.print(callWaitPosition);
  
  // Waiting Time (크기 1)
  tft.setTextSize(1);
  tft.setCursor(PADDING, startY + 115);
  tft.print("Waiting Time");
  
  // 시간 (크기 2) - 티켓 발행 시점의 대기시간 사용
  tft.setTextSize(2);
  tft.setCursor(PADDING, startY + 130);
  int mins = issuedTicketWaitTime / 60;
  int secs = issuedTicketWaitTime % 60;
  if (mins < 10) tft.print("0");
  tft.print(mins);
  tft.print(":");
  if (secs < 10) tft.print("0");
  tft.print(secs);
  
  // 확인 버튼
  int btnW = 100;
  int btnH = 30;
  int btnX = (SCREEN_WIDTH - btnW) / 2;
  int btnY = SCREEN_HEIGHT - PADDING - btnH - 10;
  tft.fillRect(btnX, btnY, btnW, btnH, invertColor(0xfb4d));  // 버튼 배경색
  tft.drawRect(btnX, btnY, btnW, btnH, invertColor(0xb800));  // 버튼 테두리
  tft.setTextColor(invertColor(0xffff));  // 버튼 글씨
  tft.setTextSize(1);
  tft.setCursor(btnX+38, btnY+10);
  tft.print("OK");
}

void drawQueueFull() {
  tft.fillScreen(invertColor(COLOR_USER_BG));
  
  // 상단 제목
  tft.setTextColor(invertColor(COLOR_USER_TEXT));
  tft.setTextSize(2);
  tft.setCursor(PADDING, PADDING);
  tft.print("Embedded QMS");
  
  int startY = PADDING + 50;
  
  // FAIL TO JOIN (크기 3)
  tft.setTextColor(COLOR_RED);
  tft.setTextSize(3);
  tft.setCursor(PADDING, startY);
  tft.print("FAIL TO JOIN");
  
  // Queue is already full. (크기 2)
  tft.setTextColor(invertColor(COLOR_USER_TEXT));
  tft.setTextSize(2);
  tft.setCursor(PADDING, startY + 30);
  tft.print("Queue is Full.");
  
  // Please try again in a moment. (크기 1)
  tft.setTextSize(1);
  tft.setCursor(PADDING, startY + 75);
  tft.print("Please try again in a moment.");
  
  // Thank you. (크기 1)
  tft.setCursor(PADDING, startY + 120);
  tft.print("Thank you.");
  
  // 확인 버튼
  int btnW = 100;
  int btnH = 30;
  int btnX = (SCREEN_WIDTH - btnW) / 2;
  int btnY = SCREEN_HEIGHT - PADDING - btnH - 10;
  tft.fillRect(btnX, btnY, btnW, btnH, invertColor(COLOR_ADMIN_TEXT));
  tft.drawRect(btnX, btnY, btnW, btnH, invertColor(COLOR_USER_TEXT));
  tft.setTextColor(invertColor(COLOR_WHITE));
  tft.setTextSize(1);
  tft.setCursor(btnX+25, btnY+10);
  tft.print("Confirm");
}

void drawCallModal() {
  // 기존 화면 위에 반투명처럼 표현
  tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, invertColor(COLOR_ADMIN_BG));
  
  // 상단 제목
  tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
  tft.setTextSize(2);
  tft.setCursor(PADDING, PADDING);
  tft.print("Embedded QMS");
  
  // X 버튼
  tft.fillRect(SCREEN_WIDTH-PADDING-32, PADDING, 32, 32, invertColor(COLOR_ADMIN_TEXT));
  tft.drawRect(SCREEN_WIDTH-PADDING-32, PADDING, 32, 32, invertColor(COLOR_ADMIN_TEXT));
  tft.setTextColor(invertColor(COLOR_WHITE));
  tft.setTextSize(2);
  tft.setCursor(SCREEN_WIDTH-PADDING-25, PADDING+10);
  tft.print("X");
  
  // Call 제목
  tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
  tft.setTextSize(2);
  tft.setCursor(PADDING+60, PADDING+40);
  tft.print("Call");
  
  // 입력 박스
  int boxW = SCREEN_WIDTH - PADDING*2;
  int boxH = 200;
  tft.fillRect(PADDING, PADDING+80, boxW, boxH, invertColor(COLOR_WHITE));
  tft.drawRect(PADDING, PADDING+80, boxW, boxH, invertColor(COLOR_ADMIN_TEXT));
}

void drawQueueList() {
  tft.fillScreen(invertColor(COLOR_ADMIN_BG));
  
  // 상단 제목
  tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
  tft.setTextSize(2);
  tft.setCursor(PADDING, PADDING);
  tft.print("Embedded QMS");
  
  // X 버튼
  tft.fillRect(SCREEN_WIDTH-PADDING-32, PADDING, 32, 32, invertColor(COLOR_ADMIN_BG));
  tft.drawRect(SCREEN_WIDTH-PADDING-32, PADDING, 32, 32, invertColor(COLOR_ADMIN_TEXT));
  tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
  tft.setTextSize(2);
  tft.setCursor(SCREEN_WIDTH-PADDING-25, PADDING+10);
  tft.print("X");
  
  // 제목
  tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
  tft.setTextSize(1);
  tft.setCursor(PADDING, PADDING+45);
  tft.print("Manage Queue");
  
  // 대기열 버튼 (4x5 = 최대 20개)
  int btnSize = 45;
  int btnHeight = 30;  // 높이 줄이기 (70%)
  int btnGap = 6;
  int startX = PADDING + 5;
  int startY = PADDING + 70;
  int cols = 4;
  
  for (int i = 0; i < 20 && i < queueCount; i++) {
    int row = i / cols;
    int col = i % cols;
    int x = startX + col * (btnSize + btnGap);
    int y = startY + row * (btnHeight + btnGap);
    
    tft.fillRect(x, y, btnSize, btnHeight, invertColor(COLOR_ADMIN_BG));
    tft.drawRect(x, y, btnSize, btnHeight, invertColor(COLOR_ADMIN_TEXT));
    tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
    tft.setTextSize(2);
    
    String numStr = String(queueList[i]);
    int textX = x + (btnSize - numStr.length() * 12) / 2;
    int textY = y + 8;
    tft.setCursor(textX, textY);
    tft.print(numStr);
  }
}

void drawQueueDeleteConfirm() {
  tft.fillScreen(invertColor(COLOR_ADMIN_BG));
  
  // 상단 제목
  tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
  tft.setTextSize(2);
  tft.setCursor(PADDING, PADDING);
  tft.print("Embedded QMS");
  
  // X 버튼
  tft.fillRect(SCREEN_WIDTH-PADDING-32, PADDING, 32, 32, invertColor(COLOR_ADMIN_BG));
  tft.drawRect(SCREEN_WIDTH-PADDING-32, PADDING, 32, 32, invertColor(COLOR_ADMIN_TEXT));
  tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
  tft.setTextSize(2);
  tft.setCursor(SCREEN_WIDTH-PADDING-25, PADDING+10);
  tft.print("X");
  
  // 삭제 메시지
  tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
  tft.setTextSize(1);
  tft.setCursor(PADDING+20, PADDING+80);
  tft.print("Remove this element?");
  
  // 선택된 번호 표시
  if (selectedQueueIndex >= 0 && selectedQueueIndex < queueCount) {
    int ticketNum = queueList[selectedQueueIndex];
    tft.fillRect(85, 140, 70, 70, invertColor(COLOR_ADMIN_BG));
    tft.drawRect(85, 140, 70, 70, invertColor(COLOR_ADMIN_TEXT));
    tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
    tft.setTextSize(3);
    
    String numStr = String(ticketNum);
    int textX = 85 + (70 - numStr.length() * 18) / 2;
    tft.setCursor(textX, 165);
    tft.print(numStr);
  }
  
  // YES 버튼
  tft.fillRect(50, 230, 60, 40, invertColor(COLOR_GREEN));
  tft.drawRect(50, 230, 60, 40, invertColor(COLOR_ADMIN_TEXT));
  tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
  tft.setTextSize(2);
  tft.setCursor(58, 242);
  tft.print("YES");
  
  // NO 버튼
  tft.fillRect(130, 230, 60, 40, invertColor(COLOR_ADMIN_BG));
  tft.drawRect(130, 230, 60, 40, invertColor(COLOR_ADMIN_TEXT));
  tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
  tft.setTextSize(2);
  tft.setCursor(145, 242);
  tft.print("NO");
}

void drawTimeSetting() {
  tft.fillScreen(invertColor(COLOR_ADMIN_BG));
  
  // 상단 제목
  tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
  tft.setTextSize(2);
  tft.setCursor(PADDING, PADDING);
  tft.print("Embedded QMS");
  
  // 라벨 - 25px 높게, 한 줄에 표시
  tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
  tft.setTextSize(1);
  tft.setCursor(PADDING, PADDING+45);
  tft.print("user process time duration");
  
  // 증가 삼각형
  tft.fillTriangle(90, 110, 120, 80, 150, 110, invertColor(COLOR_ADMIN_BG));
  tft.drawTriangle(90, 110, 120, 80, 150, 110, invertColor(COLOR_ADMIN_TEXT));
  
  // 숫자 박스
  tft.fillRect(70, 115, 100, 70, invertColor(COLOR_ADMIN_BG));
  tft.drawRect(70, 115, 100, 70, invertColor(COLOR_ADMIN_TEXT));
  tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
  tft.setTextSize(4);
  tft.setCursor(85, 135);
  tft.print(userProcessTimeSec);
  
  tft.setTextSize(1);
  tft.setCursor(145, 160);
  tft.print("sec");
  
  // 감소 삼각형
  tft.fillTriangle(90, 195, 120, 225, 150, 195, invertColor(COLOR_ADMIN_BG));
  tft.drawTriangle(90, 195, 120, 225, 150, 195, invertColor(COLOR_ADMIN_TEXT));
  
  // YES 버튼 - 관리자 버튼 색상
  tft.fillRect(90, 240, 60, 40, invertColor(COLOR_ADMIN_BTN));
  tft.drawRect(90, 240, 60, 40, invertColor(COLOR_ADMIN_TEXT));
  tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
  tft.setTextSize(2);
  tft.setCursor(98, 252);
  tft.print("OK");
}

void drawPasswordChange() {
  tft.fillScreen(invertColor(COLOR_ADMIN_BG));
  
  // 상단 제목
  tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
  tft.setTextSize(2);
  tft.setCursor(PADDING, PADDING);
  tft.print("Embedded QMS");
  
  // 서브 타이틀
  tft.setTextSize(1);
  tft.setCursor(PADDING, PADDING+25);
  tft.print("Change Password");
  
  // 입력 비밀번호 표시 박스
  tft.fillRect(PADDING+10, PADDING+50, 160, 35, invertColor(COLOR_ADMIN_TEXT));
  tft.setTextColor(invertColor(COLOR_GREEN));
  tft.setTextSize(3);
  tft.setCursor(PADDING+20, PADDING+57);
  tft.print(newPassword);
  
  // 16진수 키패드 (4x4)
  const char* keys[16] = {"1", "2", "3", "4", "5", "6", "7", "8", 
                          "9", "0", "A", "B", "C", "D", "E", "F"};
  
  int keySize = 38;
  int keyGap = 4;
  int startX = PADDING;
  int startY = PADDING + 105;
  
  for (int i = 0; i < 16; i++) {
    int row = i / 4;
    int col = i % 4;
    int x = startX + col * (keySize + keyGap);
    int y = startY + row * (keySize + keyGap);
    
    tft.fillRect(x, y, keySize, keySize, invertColor(COLOR_ADMIN_BG));
    tft.drawRect(x, y, keySize, keySize, invertColor(COLOR_ADMIN_TEXT));
    tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
    tft.setTextSize(2);
    
    tft.setCursor(x + 12, y + 12);
    tft.print(keys[i]);
  }
  
  // 기능 버튼 (BACK, CLEAR, DEL, ENTER)
  int funcX = SCREEN_WIDTH - PADDING - 30;
  for (int i = 0; i < 4; i++) {
    int y = startY + i * (keySize + keyGap);
    tft.fillRect(funcX, y, 30, keySize, invertColor(COLOR_ADMIN_BG));
    tft.drawRect(funcX, y, 30, keySize, invertColor(COLOR_ADMIN_TEXT));
    tft.setTextColor(invertColor(COLOR_ADMIN_TEXT));
    tft.setTextSize(1);
    
    tft.setCursor(funcX + 2, y + 16);
    if (i == 0) tft.print("BCK");
    else if (i == 1) tft.print("CLR");
    else if (i == 2) tft.print("DEL");
    else tft.print("ENT");
  }
}

// ===== 터치 처리 =====

void handleTouch(int x, int y) {
  
  switch (currentScreen) {
    case USER_MODE:
      // 설정 아이콘 영역
      if (x >= SCREEN_WIDTH-PADDING-32 && x <= SCREEN_WIDTH-PADDING && y >= PADDING && y <= PADDING+32) {
        currentScreen = ADMIN_LOGIN;
        adminPassword = "";
        drawAdminLogin();
      }
      // 대기표 발행 버튼
      else if (x >= PADDING && x <= SCREEN_WIDTH-PADDING && y >= SCREEN_HEIGHT-PADDING-100 && y <= SCREEN_HEIGHT-PADDING-30) {
        if (queueCount >= 20) {
          // 대기열 꾽 차서 예약 불가
          ticketIssueTime = millis();
          currentScreen = QUEUE_FULL;
          drawQueueFull();
        } else {
          // 정상 발행
          currentTicket++;
          issuedTicket = currentTicket;
          
          // 티켓 발행 시점의 대기시간 계산 및 저장
          unsigned long elapsedSinceLastProcess = (millis() - lastProcessTime) / 1000;
          int remainingForCurrent = userProcessTimeSec - elapsedSinceLastProcess;
          if (remainingForCurrent < 0) remainingForCurrent = 0;
          int totalRemainingSec = (queueCount > 0) ? (remainingForCurrent + queueCount * userProcessTimeSec) : 0;
          issuedTicketWaitTime = totalRemainingSec;
          
          addToQueue(currentTicket);  // 대기열에 번호 추가
          callWaitPosition = queueCount;
          ticketIssueTime = millis();
          currentScreen = TICKET_ISSUED;
          drawTicketIssued();
        }
      }
      break;
      
    case ADMIN_LOGIN:
      handleAdminLoginTouch(x, y);
      break;
      
    case ADMIN_MODE:
      // 뒤로가기
      if (x >= SCREEN_WIDTH-PADDING-32 && x <= SCREEN_WIDTH-PADDING && y >= PADDING && y <= PADDING+32) {
        currentScreen = USER_MODE;
        drawUserMode();
      }
      // 대기열 보기
      else if (x >= PADDING && x <= SCREEN_WIDTH-PADDING && y >= PADDING+70 && y <= PADDING+120) {
        currentScreen = QUEUE_LIST;
        drawQueueList();
      }
      // 시간 설정
      else if (x >= PADDING && x <= SCREEN_WIDTH-PADDING && y >= PADDING+140 && y <= PADDING+190) {
        currentScreen = TIME_SETTING;
        drawTimeSetting();
      }
      // 비밀번호 변경
      else if (x >= PADDING && x <= SCREEN_WIDTH-PADDING && y >= PADDING+210 && y <= PADDING+260) {
        currentScreen = PASSWORD_CHANGE;
        newPassword = "";
        drawPasswordChange();
      }
      break;
      
    case TICKET_ISSUED:
      // 확인 버튼
      if (x >= (SCREEN_WIDTH-100)/2 && x <= (SCREEN_WIDTH+100)/2 && y >= SCREEN_HEIGHT-PADDING-40 && y <= SCREEN_HEIGHT-PADDING-10) {
        currentScreen = USER_MODE;
        drawUserMode();
      }
      break;
      
    case QUEUE_FULL:
      // 확인 버튼
      if (x >= (SCREEN_WIDTH-100)/2 && x <= (SCREEN_WIDTH+100)/2 && y >= SCREEN_HEIGHT-PADDING-40 && y <= SCREEN_HEIGHT-PADDING-10) {
        currentScreen = USER_MODE;
        drawUserMode();
      }
      break;
      
    case QUEUE_LIST:
      // X 버튼
      if (x >= SCREEN_WIDTH-PADDING-32 && x <= SCREEN_WIDTH-PADDING && y >= PADDING && y <= PADDING+32) {
        currentScreen = ADMIN_MODE;
        drawAdminMode();
      }
      // 번호 클릭
      else {
        int btnSize = 45;
        int btnHeight = 30;
        int btnGap = 6;
        int startX = PADDING + 5;
        int startY = PADDING + 70;
        int cols = 4;
        
        for (int i = 0; i < 20 && i < queueCount; i++) {
          int row = i / cols;
          int col = i % cols;
          int btnX = startX + col * (btnSize + btnGap);
          int btnY = startY + row * (btnHeight + btnGap);
          
          if (x >= btnX && x <= btnX + btnSize && y >= btnY && y <= btnY + btnHeight) {
            selectedQueueIndex = i;
            currentScreen = QUEUE_DELETE_CONFIRM;
            drawQueueDeleteConfirm();
            break;
          }
        }
      }
      break;
      
    case QUEUE_DELETE_CONFIRM:
      // X 버튼
      if (x >= SCREEN_WIDTH-PADDING-32 && x <= SCREEN_WIDTH-PADDING && y >= PADDING && y <= PADDING+32) {
        currentScreen = QUEUE_LIST;
        drawQueueList();
      }
      // YES
      else if (x >= 50 && x <= 110 && y >= 230 && y <= 270) {
        removeFromQueue(selectedQueueIndex);
        selectedQueueIndex = -1;
        currentScreen = QUEUE_LIST;
        drawQueueList();
      }
      // NO
      else if (x >= 130 && x <= 190 && y >= 230 && y <= 270) {
        currentScreen = QUEUE_LIST;
        drawQueueList();
      }
      break;
      
    case TIME_SETTING:
      // 증가
      if (x >= 90 && x <= 150 && y >= 80 && y <= 110) {
        if (userProcessTimeSec < 99) {
          userProcessTimeSec++;
          waitingTimeSec = queueCount * userProcessTimeSec;
          drawTimeSetting();
        }
      }
      // 감소
      else if (x >= 90 && x <= 150 && y >= 195 && y <= 225) {
        if (userProcessTimeSec > 1) {
          userProcessTimeSec--;
          waitingTimeSec = queueCount * userProcessTimeSec;
          drawTimeSetting();
        }
      }
      // YES 버튼 (중앙으로 이동)
      else if (x >= 90 && x <= 150 && y >= 240 && y <= 280) {
        currentScreen = ADMIN_MODE;
        drawAdminMode();
      }
      break;
      
    case PASSWORD_CHANGE:
      handlePasswordChangeTouch(x, y);
      break;
      
    case CALL_MODAL:
      // X 버튼
      if (x >= SCREEN_WIDTH-PADDING-32 && x <= SCREEN_WIDTH-PADDING && y >= PADDING && y <= PADDING+32) {
        currentScreen = ADMIN_MODE;
        drawAdminMode();
      }
      break;
  }
}

void handleAdminLoginTouch(int x, int y) {
  int keySize = 38;
  int keyGap = 4;
  int startX = PADDING;
  int startY = PADDING + 105;
  
  Serial.println("--- Admin Login Touch Debug ---");
  Serial.print("Touch pos: (");
  Serial.print(x);
  Serial.print(", ");
  Serial.print(y);
  Serial.println(")");
  
  // 16진수 키패드
  const char* keys[16] = {"1", "2", "3", "4", "5", "6", "7", "8", 
                          "9", "0", "A", "B", "C", "D", "E", "F"};
  
  for (int i = 0; i < 16; i++) {
    int row = i / 4;
    int col = i % 4;
    int btnX = startX + col * (keySize + keyGap);
    int btnY = startY + row * (keySize + keyGap);
    
    // debug
    Serial.print("Key ");
    Serial.print(keys[i]);
    Serial.print(": X[");
    Serial.print(btnX);
    Serial.print("-");
    Serial.print(btnX + keySize);
    Serial.print("], Y[");
    Serial.print(btnY);
    Serial.print("-");
    Serial.print(btnY + keySize);
    Serial.print("] ");
    
    if (x >= btnX && x <= btnX + keySize && y >= btnY && y <= btnY + keySize) {
      Serial.println("<-- HIT!");
      if (adminPassword.length() < 4) {
        adminPassword += keys[i];
        drawAdminLogin();
      }
      return;
    } else {
      Serial.println("");
    }
  }
  
  // 기능 버튼
  int funcX = SCREEN_WIDTH - PADDING - 30;
  for (int i = 0; i < 4; i++) {
    int btnY = startY + i * (keySize + keyGap);
    if (x >= funcX && x <= funcX + 30 && y >= btnY && y <= btnY + keySize) {
      if (i == 0) {  // BACK
        currentScreen = USER_MODE;
        drawUserMode();
      } else if (i == 1) {  // CLEAR
        adminPassword = "";
        drawAdminLogin();
      } else if (i == 2) {  // DEL
        if (adminPassword.length() > 0) {
          adminPassword.remove(adminPassword.length() - 1);
          drawAdminLogin();
        }
      } else if (i == 3) {  // ENTER
        if (adminPassword == correctPassword) {
          currentScreen = ADMIN_MODE;
          drawAdminMode();
        } else {
          adminPassword = "";
          drawAdminLogin();
          Serial.println("Wrong password!");
        }
      }
      return;
    }
  }
}

// ===== 대기열 관리 =====

void addToQueue(int ticketNum) {
  if (queueCount < 20) {
    queueList[queueCount] = ticketNum;
    queueCount++;
    waitingCount = queueCount;
    waitingTimeSec = queueCount * userProcessTimeSec;
  }
}

void removeFromQueue(int index) {
  if (index >= 0 && index < queueCount) {
    // 배열에서 요소 삭제 (뒤 요소들을 당겨옴)
    for (int i = index; i < queueCount - 1; i++) {
      queueList[i] = queueList[i + 1];
    }
    queueCount--;
    waitingCount = queueCount;
    waitingTimeSec = queueCount * userProcessTimeSec;
  }
}

void handlePasswordChangeTouch(int x, int y) {
  int keySize = 38;
  int keyGap = 4;
  int startX = PADDING;
  int startY = PADDING + 105;
  
  // 키패드
  const char* keys[16] = {"1", "2", "3", "4", "5", "6", "7", "8", 
                          "9", "0", "A", "B", "C", "D", "E", "F"};
  
  for (int i = 0; i < 16; i++) {
    int row = i / 4;
    int col = i % 4;
    int btnX = startX + col * (keySize + keyGap);
    int btnY = startY + row * (keySize + keyGap);
    
    if (x >= btnX && x <= btnX + keySize && y >= btnY && y <= btnY + keySize) {
      if (newPassword.length() < 4) {
        newPassword += keys[i];
        drawPasswordChange();
      }
      return;
    }
  }
  
  // 기능 버튼
  int funcX = SCREEN_WIDTH - PADDING - 30;
  for (int i = 0; i < 4; i++) {
    int btnY = startY + i * (keySize + keyGap);
    if (x >= funcX && x <= funcX + 30 && y >= btnY && y <= btnY + keySize) {
      if (i == 0) {  // BACK
        currentScreen = ADMIN_MODE;
        drawAdminMode();
      } else if (i == 1) {  // CLEAR
        newPassword = "";
        drawPasswordChange();
      } else if (i == 2) {  // DEL
        if (newPassword.length() > 0) {
          newPassword.remove(newPassword.length() - 1);
          drawPasswordChange();
        }
      } else if (i == 3) {  // ENTER
        if (newPassword.length() == 4) {
          correctPassword = newPassword;
          newPassword = "";
          currentScreen = ADMIN_MODE;
          drawAdminMode();
          Serial.println("Password changed!");
        }
      }
      return;
    }
  }
}
