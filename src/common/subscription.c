/***************************************************
 * subscription.c
 * Created on Fri, 20 Oct 2017 19:55:07 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <stdlib.h>
#include <string.h>
#include "subscription.h"

struct subscription_s *subscription_list_add(struct subscription_list_s *sl, struct signal_s *signal, int type, int socket) {
  struct subscription_s *new_sub = malloc(sizeof(struct subscription_s));

  new_sub->s_signal = signal;
  new_sub->s_socket = socket;
  new_sub->s_type = type;
  new_sub->s_next = sl->sl_subscriptions[type];
  sl->sl_subscriptions[type] = new_sub;
  return new_sub;
}

void subscription_list_clear(struct subscription_list_s *sl) {
  int type;
  for(type = 0; type < SUB_MAX; type ++) {
    while(sl->sl_subscriptions[type]) {
      struct subscription_s *sub = sl->sl_subscriptions[type];
      sl->sl_subscriptions[type] = sub->s_next;
      free(sub);
    }
  }
}
