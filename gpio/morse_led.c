/*
 * 학번(숫자 문자열)을 모스부호로 변환해 Jetson GPIO 핀 19의 LED 점멸로 출력
 *
 * - /dev/mem mmap으로 pinmux · GPIO 레지스터 직접 설정
 * - dash(-): 긴 점등, dot(.): 짧은 점등, 숫자 사이: 긴 소등
 *
 * 비고: 수업 제출본의 문자 비교(morse[i] == "-") · 반복 변수 재사용 오류를
 *       제출 이후 정리한 버전. 레지스터 주소 · 오프셋 · 지연 상수는 제출본과 동일.
 *
 * 빌드: gcc -O0 -o morse_led morse_led.c
 * 실행: sudo ./morse_led
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <stdint.h>

static const char *num[] = {
    "-----", /* 0 */
    ".----", /* 1 */
    "..---", /* 2 */
    "...--", /* 3 */
    "....-", /* 4 */
    ".....", /* 5 */
    "-....", /* 6 */
    "--...", /* 7 */
    "---..", /* 8 */
    "----.", /* 9 */
};

/* busy-wait 지연 (제출본과 동일한 반복 횟수 기반) */
static void delay(long count)
{
	volatile long d;
	for (d = 0; d < count; d++)
		;
}

int main(void)
{
	int fd;

	volatile void *cnf_naon;
	volatile void *pinmux_g2;

	volatile unsigned int *pinmux19;
	volatile unsigned int *pin19_cnf;
	volatile unsigned int *pin19_out;
	volatile unsigned int *pin19_out_value;

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) {
		printf("run this program as %% sudo ./morse_led\n");
		return -1;
	}

	cnf_naon = mmap(0, 5 * 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x02210000);
	if (cnf_naon == MAP_FAILED)
		return -3;

	pinmux_g2 = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x0243d000);
	if (pinmux_g2 == MAP_FAILED)
		return -3;

	pinmux19        = (volatile unsigned int *)((char *)pinmux_g2 + 0x40);
	pin19_cnf       = (volatile unsigned int *)((char *)cnf_naon + 0x014a0 + 0x00);
	pin19_out       = (volatile unsigned int *)((char *)cnf_naon + 0x014a0 + 0x0C);
	pin19_out_value = (volatile unsigned int *)((char *)cnf_naon + 0x014a0 + 0x10);

	*pinmux19 = 0x00000040;   /* pinmux: GPIO 기능 선택 */
	*pin19_cnf = 0x00000003;  /* GPIO 활성화 + 출력 방향 */
	*pin19_out &= ~(0x00000001);

	/* 학번 입력 → 모스부호 문자열 생성 (숫자 사이 공백) */
	char id[16];
	char morse[16 * 6 + 1] = "";

	printf("Enter your student ID number: ");
	if (scanf("%15s", id) != 1)
		return -2;

	for (size_t j = 0; j < strlen(id); j++) {
		if (id[j] < '0' || id[j] > '9') {
			printf("digits only\n");
			return -2;
		}
		strcat(morse, num[id[j] - '0']);
		strcat(morse, " ");
	}
	printf("[%s]\n", morse);

	/* 모스부호 반복 출력 */
	while (1) {
		for (size_t i = 0; i < strlen(morse); i++) {
			if (morse[i] == '-') {
				*pin19_out_value = 1;
				delay(200000000);
				*pin19_out_value = 0;
				delay(25000000);
			} else if (morse[i] == '.') {
				*pin19_out_value = 1;
				delay(40000000);
				*pin19_out_value = 0;
				delay(25000000);
			} else { /* 숫자 사이 구분 */
				*pin19_out_value = 0;
				delay(200000000);
			}
		}
	}

	return 0;
}
