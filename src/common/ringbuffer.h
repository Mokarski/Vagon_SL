#pragma once
#include <pthread.h>

#define RB_EMPTY() (ring_buffer_size(Signal_Mod_Buffer) == 0)

#define RINGBUFFER_SIZE	512

struct ring_buffer_s {
	pthread_mutex_t mutex;
	int start, end, size;
	void *buffer[RINGBUFFER_SIZE];
};

struct ring_buffer_s *ring_buffer_init(struct ring_buffer_s **buf);
void ring_buffer_destroy(struct ring_buffer_s **buf);
int ring_buffer_push(struct ring_buffer_s *buf, void *);
void *ring_buffer_get(struct ring_buffer_s *buf);
int ring_buffer_pop(struct ring_buffer_s *buf);
int ring_buffer_size(struct ring_buffer_s *buf);
