/***************************************************
 * signalrouter.h
 * Created on Fri, 20 Oct 2017 20:08:57 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#ifndef SIGNALROUTER_H_
#define SIGNALROUTER_H_

#include "common/signal.h"
#include "common/event.h"
#include "common/hash.h"
#include "common/subscription.h"

#define PORT 		8888
#define MAX_CONN  10

struct execution_context_s {
  struct signal_s *signals;
  struct hash_s *hash;
  int event_socket;
  int sockets[MAX_CONN];
  int current_client;
  struct event_s *events;
  struct subscription_list_s subscriptions[MAX_CONN];
  struct hash_s *subscription_by_signal;
};

void *connection_worker(void *ctx);

#endif
