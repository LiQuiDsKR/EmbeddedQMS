# ESP32 Touch Display Project

ESP32 DevKit C 기반의 터치 디스플레이 모듈 프로젝트입니다.

## 하드웨어

- **MCU**: ESP32 DevKit C
- **디스플레이**: ST7789 TFT LCD (240x240)
- **터치스크린**: XPT2046 (일체형)

## 핀 연결

### ST7789 TFT Display
| TFT 핀 | ESP32 핀 | 설명 |
|--------|----------|------|
| MOSI   | GPIO 23  | SPI MOSI |
| SCLK   | GPIO 18  | SPI Clock |
| CS     | GPIO 15  | Chip Select |
| DC     | GPIO 2   | Data/Command |
| RST    | GPIO 4   | Reset |
| BL     | GPIO 32  | Backlight |

### XPT2046 Touch Controller
| Touch 핀 | ESP32 핀 | 설명 |
|----------|----------|------|
| T_CS     | GPIO 5   | Touch Chip Select |
| T_IRQ    | GPIO 17  | Touch Interrupt (선택사항) |
| MOSI     | GPIO 23  | 공유 (TFT와 동일) |
| MISO     | GPIO 19  | SPI MISO |
| SCLK     | GPIO 18  | 공유 (TFT와 동일) |

## 기능

- ST7789 TFT 디스플레이 제어 (240x240 해상도)
- XPT2046 터치 입력 감지
- 터치 좌표 표시
- 간단한 버튼 UI 및 터치 이벤트 처리

## 빌드 및 업로드

```bash
# 프로젝트 빌드
pio run

# ESP32에 업로드
pio run --target upload

# 시리얼 모니터
pio device monitor
```

## 라이브러리

- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library) - 그래픽 라이브러리
- [Adafruit ST7735 and ST7789 Library](https://github.com/adafruit/Adafruit-ST7735-Library) - ST7789 디스플레이 드라이버
- [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen) - 터치 입력 처리
- [U8g2](https://github.com/olikraus/u8g2) - 유니코드 폰트 라이브러리
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) - JSON 처리

## 사용 방법

1. 하드웨어를 위의 핀 연결 테이블대로 연결합니다.
2. PlatformIO로 프로젝트를 빌드하고 ESP32에 업로드합니다.
3. 디스플레이에 버튼이 표시되며, 터치하면 반응합니다.
4. 터치한 위치에 빨간 점이 표시됩니다.
5. 시리얼 모니터(115200 baud)에서 터치 좌표를 확인할 수 있습니다.

## 터치 보정

터치 좌표가 정확하지 않을 경우 `main.cpp`의 다음 값들을 조정하세요:

```cpp
#define TOUCH_X_MIN 200
#define TOUCH_X_MAX 3800
#define TOUCH_Y_MIN 200
#define TOUCH_Y_MAX 3800
```

시리얼 모니터에서 Raw 좌표 값을 확인하여 최소/최대 값을 설정합니다.

## 커스터마이징

- 디스플레이 회전: `tft.setRotation(0-3)` 변경
- 터치 회전: `touch.setRotation(0-3)` 변경
- 버튼 추가: `drawButton()` 함수 사용
- UI 커스터마이징: TFT_eSPI의 다양한 그래픽 함수 활용
