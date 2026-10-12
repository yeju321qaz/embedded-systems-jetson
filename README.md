# Embedded Systems on NVIDIA Jetson

이화여자대학교 임베디드시스템 교과목 실습 (2024.11 – 2024.12) · 개인 실습

NVIDIA Jetson 보드에서 **GPIO 레지스터 직접 제어**와 **CUDA 기반 영상 처리 → LCD 출력**을 다룬 실습 코드.
수업 제공 템플릿 · 보드 라이브러리 위에 과제 요구사항을 직접 구현.

| 파일 | 내용 | 직접 구현 |
| --- | --- | --- |
| [`gpio/led_blink.c`](gpio/led_blink.c) | GPIO 핀 19 LED 점멸 | `/dev/mem` mmap, pinmux · GPIO 레지스터 설정 |
| [`gpio/morse_led.c`](gpio/morse_led.c) | 학번 → 모스부호 LED 출력 | 숫자 → 모스부호 변환, dot · dash 점멸 타이밍 |
| [`cuda/grayscale_lcd.cu`](cuda/grayscale_lcd.cu) | CUDA grayscale 변환 → LCD 출력 | 2D grid / block 커널, RGB565 변환, 버튼 상태 머신 |
| [`cuda/grayscale_flip_lcd.cu`](cuda/grayscale_flip_lcd.cu) | 위 실습 + 좌우 반전 출력 | 출력 좌표 반전 처리 |

<br/>

## 1. GPIO 레지스터 직접 제어

라이브러리 없이 물리 메모리를 사용자 공간에 매핑해 레지스터 직접 설정.

```text
/dev/mem open (O_SYNC)
      ↓
mmap  0x0243D000  pinmux 영역   →  +0x40           : pin 19 GPIO 기능 선택
mmap  0x02210000  GPIO 영역     →  +0x14A0 + 0x00  : CNF  (GPIO 활성화 · 출력 방향)
                                 →  +0x14A0 + 0x0C  : OUT  (출력 enable)
                                 →  +0x14A0 + 0x10  : OUT_VALUE (1 = 점등, 0 = 소등)
```

- `volatile` 포인터로 레지스터 접근, 컴파일러 최적화에 의한 쓰기 생략 방지
- **LED 점멸**: OUT_VALUE 1 / 0 반복, busy-wait 지연
- **모스부호 출력**: 입력 학번의 각 숫자를 모스부호로 변환, dash는 긴 점등 · dot은 짧은 점등 · 숫자 사이는 긴 소등으로 무한 반복

## 2. CUDA 영상 처리 → LCD 출력

```text
PPM 이미지 로드 (host)
      ↓
원본을 RGB888 → RGB565 변환 후 LCD(1.3") 출력
      ↓
KEY1 버튼 입력 대기 (RELEASED → PRESSED → RELEASED 상태 머신)
      ↓
cudaMalloc · cudaMemcpy (host → device)
      ↓
grayscale 커널  ·  block 16 × 16, grid ⌈w/16⌉ × ⌈h/16⌉
Y = 0.299 R + 0.587 G + 0.114 B  (픽셀 1개 = 스레드 1개)
      ↓
cudaMemcpy (device → host) → RGB565 변환 → LCD 출력
```

- 스레드 인덱스로 픽셀 좌표 계산, 이미지 경계 밖 스레드는 `row < h && col < w` 조건으로 제외
- 버튼은 눌림 → 뗌을 모두 확인한 뒤 전환해 한 번의 입력으로 처리
- `grayscale_flip_lcd.cu`: 출력 열 좌표를 `w - 1 - j`로 바꿔 원본 · 처리 결과를 좌우 반전 출력

<br/>

## 빌드 · 실행 환경

- NVIDIA Jetson (L4T), GPIO 핀 19 LED 연결, Waveshare 1.3inch LCD HAT
- GPIO: `gcc -O0 -o led_blink gpio/led_blink.c` → `sudo ./led_blink` (`/dev/mem` 접근에 root 권한 필요)
- CUDA: `nvcc`로 빌드, 수업 제공 `ppm.h` · `clockMeasure.h` · LCD 드라이버(`DEV_Config`, `LCD_1in3`, `GUI_Paint`) 필요

## 비고

- 수업 제공 템플릿 · LCD 드라이버 · 이미지 파일은 저장소에 포함하지 않음. `.cu` 파일 상단 헤더는 교과목 템플릿의 원 작성자 표기
- `gpio/morse_led.c`는 제출본의 문자 비교 · 반복 변수 재사용 오류를 제출 이후 정리한 버전 (레지스터 설정 · 지연 상수는 제출본과 동일, 정리 후 보드 재검증은 미실시)
- `.cu`의 `BLUR_SIZE` 인자는 템플릿 시그니처를 유지한 것으로, grayscale 커널에서는 사용하지 않음
