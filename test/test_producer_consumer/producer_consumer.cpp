/*
 * File Name: test_producer_consumer.cpp
 * Description:
 * Author: Dan Cao <caodan@linuxtoy.cn>
 * Created Time: 2019-04-13 19:17:52
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <iostream>
#include <random>
#include <ctime>
#include <thread>
#include <sys/time.h>
#include <unistd.h>
#include "ringbuffer.h"

using namespace std;


extern uint32_t calcCRC32(const uint8_t* buf, uint32_t length);

void genRandomData(uint8_t *buf, uint32_t length)
{
	default_random_engine r_engine;
	struct timeval now;

	gettimeofday(&now, nullptr);
	r_engine.seed(now.tv_usec);

	for (uint32_t i = 0; i < length; i++)
		buf[i] = (r_engine() >> (i % 8)) & 0xff;
}

void *thread_producer(void *arg)
{
	uint32_t test_data_len = 5 * 1024 * 1024; /* total bytes */
	uint8_t *bufW;
	uint32_t head[2];
	uint32_t actualCount;
	struct timeval now;
	default_random_engine r_engine;
	int reti;
	RingBuffer<uint8_t> *rb = reinterpret_cast<RingBuffer<uint8_t> *>(arg);

	gettimeofday(&now, nullptr);
	r_engine.seed(now.tv_usec);

	printf("%s\n", __func__);

	bufW = new uint8_t[test_data_len];
	genRandomData(bufW, test_data_len);

	/*
	 * head[0] total size
	 * head[1] crc32 for all the data in bufW
	 */
	head[0] = test_data_len;
	head[1] = calcCRC32(bufW, test_data_len);

	printf("producer: 0x%x, 0x%x\n", head[0], head[1]);

	rb->write(reinterpret_cast<uint8_t *>(head), sizeof(head), actualCount);
	uint32_t leftCount = test_data_len;
	uint32_t writtenCount = 0;
	while (leftCount) {
		uint32_t count = r_engine() % 1024;
		count = std::min(count, leftCount);

		reti = rb->write(bufW + writtenCount, count, actualCount);
		if (reti) {
			if (reti == RingBuffer<uint8_t>::ERR_TIMEOUT)
				printf("producer: timeout!\n");

			continue;
		}

		//printf("producer: %d\n", rb->dataCount());

		writtenCount += actualCount;
		leftCount -= actualCount;
	}

	delete bufW;
		
	return nullptr;
}

void *thread_consumer(void *arg)
{
	uint32_t head[2];
	uint32_t actualCount;
	uint8_t *bufR;
	int reti;
	default_random_engine r_engine;
	struct timeval now;
	RingBuffer<uint8_t> *rb = reinterpret_cast<RingBuffer<uint8_t> *>(arg);

	gettimeofday(&now, nullptr);
	r_engine.seed(now.tv_usec);

	printf("%s\n", __func__);

	actualCount = 0;
	reti = rb->read(reinterpret_cast<uint8_t *>(head), sizeof(head), actualCount);
	if (reti) {
		printf("consumer: wait data timeout\n");
		return nullptr;
	}
	if (actualCount != sizeof(head)) {
		printf("consumer: head error, actualCount=%d\n", actualCount);
		return nullptr;
	}

	printf("consumer: 0x%x, 0x%x, actualCount: %d\n", head[0], head[1], actualCount);

	uint32_t totalDataCount = head[0];
	bufR = new uint8_t[totalDataCount];

	uint32_t leftCount = totalDataCount;
	uint32_t receivedCount = 0;
	while (leftCount) {
		uint32_t count = r_engine() % 1024;

		count = std::min(count, leftCount);

		reti = rb->read(bufR + receivedCount, count, actualCount);
		if (reti) {
			if (reti == RingBuffer<uint8_t>::ERR_TIMEOUT)
				printf("consumer: timeout!\n");

			continue;
		}

		//printf("consumer: %d\n", rb->dataCount());
		
		receivedCount += actualCount;
		leftCount -= actualCount;
	}
	printf("consumer: received finished!\n");

	uint32_t crc32 = calcCRC32(bufR, totalDataCount);
	if (crc32 == head[1])
		printf("\nconsumer: PASS: check received data success!\n");
	else
		printf("\nconsumer: ERROR: check received data fail!\n");

	delete bufR;

	return nullptr;
}

void producer_consumer_test(RingBuffer<uint8_t> &rb)
{
	pthread_t tid_producer;
	pthread_t tid_consumer;
	pthread_attr_t attr;
	void *retp;

	pthread_attr_init(&attr);
	pthread_create(&tid_consumer, &attr, thread_consumer, &rb);
	pthread_create(&tid_producer, &attr, thread_producer, &rb);

	pthread_join(tid_producer, &retp);
	printf("%s %d\n", __func__, __LINE__);

	pthread_join(tid_consumer, &retp);
	printf("%s %d\n", __func__, __LINE__);
}

void get_time(std::chrono::steady_clock::time_point &now)
{
	now = std::chrono::steady_clock::now();
}

double time_delta(std::chrono::steady_clock::time_point &start,
		std::chrono::steady_clock::time_point &end)
{
	std::chrono::duration<double> time_span;
	
	time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

	return time_span.count();
}

int main(int argc, char *argv[])
{
	RingBuffer<uint8_t> rb1K(1024);
	RingBuffer<uint8_t> rb10K(10 * 1024);
	RingBuffer<uint8_t> rb128K(128 * 1024);
	RingBuffer<uint8_t> rb512K(512 * 1024);
	RingBuffer<uint8_t> rb1M(1 * 1024 * 1024);
	RingBuffer<uint8_t> rb2M(2 * 1024 * 1024);
	RingBuffer<uint8_t> rb5M(2 * 1024 * 1024);
	RingBuffer<uint8_t> rb8M(2 * 1024 * 1024);

	std::vector<RingBuffer<uint8_t> *> rbList;
	rbList.push_back(&rb1K);
	rbList.push_back(&rb10K);
	rbList.push_back(&rb128K);
	rbList.push_back(&rb512K);
	rbList.push_back(&rb1M);
	rbList.push_back(&rb2M);
	rbList.push_back(&rb5M);
	rbList.push_back(&rb8M);


	std::vector< RingBuffer<uint8_t>* >::iterator it;
	for (it = rbList.begin(); it != rbList.end(); it++) {
		RingBuffer<uint8_t> *rb;
		std::chrono::steady_clock::time_point t1, t2;

		rb = *it;

		printf("\n********* RingBuffer test, dataCount = %d\n", rb->capacity());

		get_time(t1);
		producer_consumer_test(*rb);
		get_time(t2);
		printf("Cost Time: %lf s\n", time_delta(t1, t2));
	}

	return 0;
}
