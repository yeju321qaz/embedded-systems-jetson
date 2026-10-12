#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <stdint.h>

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
		printf("run this program as %% sudo lab1\n");
		return -1;
	}

	cnf_naon = mmap(0, 5 * 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x02210000);
	if (cnf_naon == MAP_FAILED) {
		return -3;
	}

	pinmux_g2 = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x0243d000);
	if (pinmux_g2 == MAP_FAILED) {
		return -3;
	}

	pinmux19 = (volatile unsigned int *)((char *)pinmux_g2 + 0x40);
	pin19_cnf = (volatile unsigned int *)((char *)cnf_naon + 0x014a0 + 0x00);
	pin19_out = (volatile unsigned int *)((char *)cnf_naon + 0x014a0 + 0x0C);
	pin19_out_value = (volatile unsigned int *)((char *)cnf_naon + 0x014a0 + 0x10);


	*pinmux19 = 0x00000040;
	*pin19_cnf = 0x00000003;
	*pin19_out &= ~(0x00000001);

	while (1) {
		int i;
		*pin19_out_value = 1;
		for (i=0;i<100000000;i++);
		*pin19_out_value = 0;
		for (i=0;i<100000000;i++);
	}

	munmap((void *)pinmux_g2, 4096);
	munmap((void *)cnf_naon, 5 * 4096);

	close(fd);

	return 0;
}
	

