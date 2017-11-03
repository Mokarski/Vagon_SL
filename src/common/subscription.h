/***************************************************
 * subscription.h
 * Created on Fri, 20 Oct 2017 18:24:55 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#ifndef SUBSCRIPTION_H_
#define SUBSCRIPTION_H_

#include "signal.h"
#include "hash.h"

#define SUB_UPDATE  0
#define SUB_WRITE   1
#define SUB_MAX     2

// Event subscription: a client can subscribe to one of 2 events:
// UPDATE or WRITE

struct subscription_s {
  struct signal_s *s_signal;
  int s_socket;
  int s_type;
  struct subscription_s *s_next;
};

struct subscription_list_s {
  struct subscription_s *sl_subscriptions[SUB_MAX];
};

struct subscription_s *subscription_list_add(struct subscription_list_s *sl, struct signal_s *signal, int type, int socket);
void subscription_list_clear(struct subscription_list_s *sl);

#endif
