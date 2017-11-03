/***************************************************
 * event.h
 * Created on Sun, 22 Oct 2017 12:22:47 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#ifndef EVENT_H_
#define EVENT_H_

#include <signal.h>

struct event_s {
  char e_type;
  struct signal_s *e_signal;
  int16_t e_value;
  struct event_s *e_next;
};

#endif
