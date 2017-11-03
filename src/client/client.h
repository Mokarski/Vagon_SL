/***************************************************
 * client.h
 * Created on Mon, 23 Oct 2017 06:24:10 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#ifndef CLIENT_H_
#define CLIENT_H_

#define PORT 		8888

#include <pthread.h>
#include "common/ringbuffer.h"

// Sending command for main/network thread
struct rb_command_s {
  char c_cmd[4];
  int c_num_param;
  int c_param[10];
  char cs_name[64];
};

// Client execution context
struct execution_context_s {
  struct signal_s *signals;
  struct hash_s *hash;
  int event_socket;
  pthread_t worker_thread;
  int socket;
  volatile int running;
  struct ring_buffer_s *command_buffer;
  void *clientstate;
};

// Functions to be implemented in actual client:

// Callback for write event, if client subscribes to some
void event_write_signal(struct signal_s *signal, int value, struct execution_context_s *ctx);

// Callback for update event, if client subscribes to some
void event_update_signal(struct signal_s *signal, int value, struct execution_context_s *ctx);

// Client init (can create userdata and store in the context)
void client_init(struct execution_context_s *ctx, int argc, char **argv);

// Main client thread procedure
void client_thread_proc(struct execution_context_s *ctx);

#endif
