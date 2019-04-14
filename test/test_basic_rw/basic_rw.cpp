#include <iostream>
#include <random>
#include <ctime>
#include <sys/time.h>
#include <unistd.h>
#include "ringbuffer.h"

using namespace std;

void basic_rw_test()
{
	uint32_t dataNum = 16;
	RingBuffer<uint8_t> rb(dataNum);
	uint8_t *bufW = new uint8_t[dataNum];
	uint8_t *bufR = new uint8_t[dataNum];
	default_random_engine r_engine;
	struct timeval now;

	gettimeofday(&now, nullptr);

	r_engine.seed(now.tv_usec);

	printf("\n\n>>>>>>>> %s %lu<<<<<<<<\n", __func__, r_engine());

	for (uint32_t i = 0; i < dataNum; i++) {
		bufW[i] = r_engine() % 256;
	}

	do {
		uint32_t start, count, actualCount;
		
		start = r_engine() % (dataNum - 1);
		count = r_engine() % (dataNum / 3);
		if (start + count > dataNum)
			count = dataNum - start;

		if (count > 0) {
			printf("\nWR: count = %d\n", count);
			rb.write(bufW + start, count, actualCount);
			printf("WR: actualCount = %d, total = %d\n", actualCount, rb.dataCount());

			if (rb.isFull())
				break;
		}

		if (r_engine() % 2 != 0)
			continue;

		count = r_engine() % (dataNum / 4);
		if (count > 0) {
			printf("\nRD: count = %d\n", count);
			rb.read(bufR, count, actualCount);
			printf("RD: actualCount = %d, total = %d\n", actualCount, rb.dataCount());
		}
	} while (!rb.isFull());
}



int main(int argc, char *argv[])
{
	for (int i = 0; i < 10; i++)
		basic_rw_test();


	return 0;
}
