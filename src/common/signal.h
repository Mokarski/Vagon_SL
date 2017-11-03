/***************************************************
 * signal.h
 * Created on Thu, 19 Oct 2017 10:37:25 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#ifndef SIGNAL_H_
#define SIGNAL_H_

#include <sys/types.h>

struct device_s {
  int d_type;  //< Device type
  int d_mb_id; //< Device modbus id
};

struct device_reg_s {
  int dr_type; //< Signal register type: 'b' for bit value, 'i' for int value
  int dr_addr; //< Signal address
  int dr_bit;  //< If signal type is bit, the bit position of the signal
  struct device_s dr_device;
};

struct signal_s {
  int s_id;       //< ID number
  char *s_name;   //< Signal name
  int s_value;    //< Signal value
  int s_rw;       //< Read/write
  struct device_reg_s s_register; //< Signal HW register info
  struct signal_s *next;
};

// Decode signal from string
int str_to_signal(char *str, struct signal_s *signal);

// Encode signal to string
int signal_to_str(struct signal_s *signal, char *str, size_t buflen);

// Print out symbol debug info
void print_signal(struct signal_s *signal);

#endif
