#include <stdlib.h>
#include <string.h>
#include "ringbuffer.h"
#define WRAP(x) ((x) % RINGBUFFER_SIZE)

struct ring_buffer_s *Signal_Mod_Buffer = NULL;

struct ring_buffer_s *ring_buffer_init(struct ring_buffer_s **buf) {
	*buf = malloc(sizeof(struct ring_buffer_s));
	pthread_mutex_init(&(*buf)->mutex, NULL);
	(*buf)->start = (*buf)->end = 0;
	return *buf;
}

void ring_buffer_destroy(struct ring_buffer_s **buf) {
	pthread_mutex_destroy(&(*buf)->mutex);
	if(*buf)
		free(*buf);
	*buf = NULL;
}

int ring_buffer_push(struct ring_buffer_s *buf, void *value) {
	pthread_mutex_lock(&buf->mutex);
	if(WRAP(buf->end + 1) == buf->start) {
		pthread_mutex_unlock(&buf->mutex);
		return 0;
	}

	buf->buffer[buf->end] = value;
	buf->end = WRAP(buf->end + 1);
	pthread_mutex_unlock(&buf->mutex);
	return 1;
}

void *ring_buffer_get(struct ring_buffer_s *buf) {
  void *result;
	pthread_mutex_lock(&buf->mutex);
	if(buf->end == buf->start) {
		pthread_mutex_unlock(&buf->mutex);
		return 0;
	}

	result = buf->buffer[buf->start];
	pthread_mutex_unlock(&buf->mutex);
	return result;
}

int ring_buffer_pop(struct ring_buffer_s *buf) {
	pthread_mutex_lock(&buf->mutex);
	if(buf->end == buf->start) {
		pthread_mutex_unlock(&buf->mutex);
		return 0;
	}

	buf->start = WRAP(buf->start + 1);
	pthread_mutex_unlock(&buf->mutex);
	return 1;
}

int ring_buffer_size(struct ring_buffer_s *buf) {
	pthread_mutex_lock(&buf->mutex);
	int size = buf->end - buf->start;
	pthread_mutex_unlock(&buf->mutex);
	if(size < 0) {
		return size + RINGBUFFER_SIZE;
	}
	return size;
}
