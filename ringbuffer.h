/*
 * empty condition: rear == front
 * full  condition: (rear + 1) % size = front
 * data len = (rear + size - front) % size
 */
#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <cstdint>
#include <cstring>
#include <cassert>
#include <string>
#include <mutex>
#include <chrono>
#include <condition_variable>

//#define RINGBUF_DEBUG

#ifdef RINGBUF_DEBUG
#define LOG_I(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define LOG_I(fmt, ...)
#endif

template <class T> class RingBuffer
{
public:
	/* initial ringbuffer data count */
	explicit RingBuffer(uint32_t dataNum);
	~RingBuffer();

	enum {
		ERR_BASECODE = 5000,
		ERR_TIMEOUT,
	};

	int write(T *buf, uint32_t dataNum,
			uint32_t &actualDataNum, int timeoutMs = 3000);
	int read(T *buf, uint32_t dataNum,
			uint32_t &actualDataNum, int timeoutMs = 3000);

	bool isEmpty() {
		std::unique_lock<std::mutex> locker(m_lock);
		return empty_nolock();
	}

	bool isFull() {
		std::unique_lock<std::mutex> locker(m_lock);
		return full_nolock();
	}

	/* RingBuffer capacity:  Max data num */
	uint32_t capacity() {
		return m_dataNum;
	}

	/* How many data in the RingBuffer ready to read */
	uint32_t dataCount() {
		std::unique_lock<std::mutex> locker(m_lock);
		return dataCount_nolock();
	}

private:
	std::mutex m_lock;

	std::condition_variable m_condR; /* condition for read */
	std::condition_variable m_condW; /* condition for write */

	uint32_t m_dataNum;
	uint32_t m_front; /* point to the ready to read position */
	uint32_t m_rear;  /* point to the next available posotion to write */
	T *m_data;

	inline bool empty_nolock();
	inline bool full_nolock();
	inline uint32_t dataCount_nolock();
	inline uint32_t freeCount_nolock();
};

template <class T>
RingBuffer<T>::RingBuffer(uint32_t dataNum)
		: m_dataNum (dataNum)
		, m_front (0)
		, m_rear (0)
{
		m_data = new T[dataNum];
}

template <class T>
RingBuffer<T>::~RingBuffer()
{
	delete m_data;
}

template <class T>
inline uint32_t RingBuffer<T>::dataCount_nolock()
{
	return (m_rear + m_dataNum - m_front) % m_dataNum;
}

template <class T>
inline uint32_t RingBuffer<T>::freeCount_nolock()
{
	return (m_dataNum - 1 - dataCount_nolock());
}


template <class T>
inline bool RingBuffer<T>::empty_nolock()
{
	return (m_front == m_rear);
}

template <class T>
inline bool RingBuffer<T>::full_nolock()
{
	return (m_rear + 1) % m_dataNum == m_front;

}

template <class T>
int RingBuffer<T>::write(T *buf, uint32_t dataNum,
		uint32_t &actualDataNum, int timeoutMs)
{
	uint32_t freeCount;
	uint32_t totalCount;
	uint32_t countsToWrite;
	uint32_t countsWritten = 0;

	if (dataNum == 0) {
		actualDataNum = 0;
		return 0;
	}

	std::unique_lock<std::mutex> locker(m_lock);

	LOG_I("front:%d, rear:%d\n", m_front, m_rear);
	/*
	 * 1. wait will check the condition variable
	 * 2. unlock
	 * 2. schedule out and wait
	 * 3. lock
	 */
	std::chrono::milliseconds timeout(timeoutMs);
	while (full_nolock()) {
		if (m_condW.wait_for(locker, timeout) == std::cv_status::timeout)
			return ERR_TIMEOUT;
	}

	freeCount = freeCount_nolock();
	totalCount = std::min(freeCount, dataNum); /* max write count */

	countsToWrite = std::min(totalCount, (m_dataNum - m_rear));
	memcpy(m_data + m_rear, buf, countsToWrite * sizeof(T));
	countsWritten += countsToWrite;
	m_rear = (m_rear + countsToWrite) % m_dataNum; /* update m_rear */

	countsToWrite = totalCount - countsWritten;
	assert(countsToWrite <= freeCount_nolock());
	if (countsToWrite > 0) {
		memcpy(m_data + m_rear, buf + countsWritten, countsToWrite * sizeof(T));
		countsWritten += countsToWrite;
		m_rear = (m_rear + countsToWrite) % m_dataNum;
	}

	assert(totalCount == countsWritten);

	actualDataNum = countsWritten;

	m_condR.notify_all();

	LOG_I("front:%d, rear:%d\n", m_front, m_rear);

	return 0;
}

template <class T>
int RingBuffer<T>::read(T *buf, uint32_t dataNum,
		uint32_t &actualDataNum, int timeoutMs)
{
	uint32_t availableCount;
	uint32_t totalCount;
	uint32_t countsToRead;
	uint32_t countsRead = 0;

	if (dataNum == 0) {
		actualDataNum = 0;
		return 0;
	}

	std::unique_lock<std::mutex> locker(m_lock);

	LOG_I("front:%d, rear:%d\n", m_front, m_rear);

	std::chrono::milliseconds timeout(timeoutMs);
	while (empty_nolock()) {
		if (m_condR.wait_for(locker, timeout) == std::cv_status::timeout)
			return ERR_TIMEOUT;
	}

	/* read count must <= available dataCount */
	availableCount = dataCount_nolock();
	totalCount = std::min(availableCount, dataNum); /* max read count */

	countsToRead = std::min(totalCount, (m_dataNum - m_front));
	memcpy(buf, m_data + m_front, countsToRead * sizeof(T));
	countsRead += countsToRead;
	m_front = (m_front + countsToRead) % m_dataNum;

	countsToRead = totalCount - countsRead;
	assert(countsToRead <= dataCount_nolock());
	if (countsToRead > 0) {
		memcpy(buf + countsRead, m_data + m_front, countsToRead * sizeof(T));
		countsRead += countsToRead;
		m_front = (m_front + countsToRead) % m_dataNum;
	}

	assert(countsRead == totalCount);

	actualDataNum = countsRead;

	m_condW.notify_all();

	LOG_I("front:%d, rear:%d\n", m_front, m_rear);

	return 0;
}

#endif
