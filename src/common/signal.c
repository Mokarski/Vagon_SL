/***************************************************
 * signal.c
 * Created on Thu, 19 Oct 2017 11:23:25 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <string.h>
#include <stdio.h>
#include "signal.h"

int str_to_signal(char *str, struct signal_s *signal) {
  char *f, *s = str;
  int error;

  if((f = strtok_r(str, ",", &s)) == NULL) {
    return -1;
  }
  signal->s_name = strdup(f);

  if((f = strtok_r(NULL, ",", &s)) == NULL) {
    return -1;
  }
  signal->s_value = atoi(f);

  if((f = strtok_r(NULL, ",", &s)) == NULL) {
    return -1;
  }
  signal->s_rw = atoi(f);

  if((f = strtok_r(NULL, ",", &s)) == NULL) {
    return -1;
  }
  signal->s_register.dr_type = *f;

  if((f = strtok_r(NULL, ",", &s)) == NULL) {
    return -1;
  }
  signal->s_register.dr_device.d_mb_id = atoi(f);

  if((f = strtok_r(NULL, ",", &s)) == NULL) {
    return -1;
  }
  signal->s_register.dr_addr = atoi(f);

  if(signal->s_register.dr_type == 'b') {
    if((f = strtok_r(NULL, ",", &s)) == NULL) {
      return -1;
    }
    signal->s_register.dr_bit = atoi(f);
  }

  if((f = strtok_r(NULL, ",", &s)) == NULL) {
    return -1;
  }
  signal->s_register.dr_device.d_type = *f;
  return 0;
}

int signal_to_str(struct signal_s *signal, char *str, size_t buflen) {
  return snprintf(str, buflen, "%s,%d,%d,%c,%d,%d,%c,%d", 
          signal->s_name, signal->s_value, signal->s_rw, signal->s_register.dr_type, signal->s_register.dr_addr, 
          signal->s_register.dr_bit, signal->s_register.dr_device.d_type, signal->s_register.dr_device.d_mb_id);
}

void print_signal(struct signal_s *signal) {
  printf("#%d %s = %d (%s) %d:%d-%d", signal->s_id, signal->s_name, signal->s_value,
         signal->s_rw == 1 ? "ro" : "rw", signal->s_register.dr_addr, 
         signal->s_register.dr_type == 'i' ? 0 : signal->s_register.dr_bit, 
         signal->s_register.dr_type == 'i' ? 16 : signal->s_register.dr_bit);
}
