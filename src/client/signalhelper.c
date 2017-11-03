/***************************************************
 * signalhelper.c
 * Created on Tue, 24 Oct 2017 03:57:57 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include "client.h"
#include "common/signal.h"
#include "common/hash.h"
#include "signalhelper.h"

int  signal_get(struct execution_context_s *h, char *name) {
  struct signal_s *s = (struct signal_s *)hash_find_first(h->hash, name);
  if(s) {
    return s->s_value;
  }
  return 0;
}

void signal_set(struct execution_context_s *h, char *name, int value) {
  struct signal_s *s = hash_find_first(h->hash, name);
  if(s) {
    s->s_value = value;
  }
}
