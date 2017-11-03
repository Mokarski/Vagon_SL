/***************************************************
 * mbdev.c
 * Created on Tue, 24 Oct 2017 07:29:06 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "mbdev.h"

void mb_dev_list_init(struct mb_device_list_s *dlist) {
  // Set everything to 0/NULL and init the mutex
  memset(dlist, 0, sizeof(struct mb_device_list_s));
  pthread_mutex_init(&dlist->mutex, NULL);
}

int  mb_dev_add_signal(struct mb_device_list_s *dlist, struct signal_s *signal) {
  int mb_id = signal->s_register.dr_device.d_mb_id;
  int addr  = signal->s_register.dr_addr;

  // Check boundaries
  if(mb_id > MAX_DEVS) {
    return -1;
  }

  if(addr > MAX_REG) {
    return -1;
  }

  // Update max device number if needed
  if(mb_id > dlist->dev_max) {
    dlist->dev_max = mb_id;
  }

  // Lookup signal's register
  struct mb_device_s *device = &dlist->device[mb_id];
  struct mb_device_reg_s *reg = &device->reg[addr];
  struct mb_signal_list_s *signal_entry = malloc(sizeof(struct mb_signal_list_s));

  // Update max register number
  if(addr + 1 > device->mb_reg_max) {
    device->mb_reg_max = addr + 1;
  }

  // Attach signal to the register
  signal_entry->signal = signal;
  signal_entry->next = reg->signals;
  reg->signals = signal_entry;

  return 0;
}

void mb_dev_add_write_request(struct mb_device_list_s *dlist, struct signal_s *signal, int value) {
  int mb_id = signal->s_register.dr_device.d_mb_id;
  int addr = signal->s_register.dr_addr;

  // Check boundaries
  if(mb_id > MAX_DEVS) {
    return;
  }

  if(addr > MAX_REG) {
    return;
  }

  // Create write request
  struct mb_device_s *device = &dlist->device[mb_id];
  struct mb_device_reg_s *reg = &device->reg[addr];
  struct mb_reg_write_request_s *req = malloc(sizeof(struct mb_reg_write_request_s));

  req->dev_id = mb_id;
  req->reg_id = addr;
  req->reg    = reg;

  // Prepare value and mask for the signal
  req->write_mask   = 0xffff;
  req->write_value = value;

  if(signal->s_register.dr_type == 'b') {
    req->write_mask = 1 << signal->s_register.dr_bit;
    req->write_value = value ? (1 << signal->s_register.dr_bit) : 0;
  }

  // Be safe, as we can read out write requests in another thread
  pthread_mutex_lock(&dlist->mutex);
  req->next     = dlist->writes;
  dlist->writes = req;
  pthread_mutex_unlock(&dlist->mutex);
}

int mb_dev_update(struct mb_device_list_s *dlist) {
  int i = 0, j;
  int regvalue;
  struct mb_reg_write_request_s *req, *reglist = NULL;

  // Be safe, as we can add write requests in another thread
  pthread_mutex_lock(&dlist->mutex);
  req = dlist->writes;
  dlist->writes = NULL;
  pthread_mutex_unlock(&dlist->mutex);

  // Read out all write requests, leaving 1 per register, so we could write all signals at once
  // if they write to a single register
  while(req) {
    int regvalue, regmask;
    struct mb_reg_write_request_s *wr = req;
    struct mb_device_reg_s *reg = wr->reg;
    req = req->next;
    regmask = wr->reg->write_mask | wr->write_mask;
    regvalue = (wr->reg->write_value & ~(wr->write_mask)) | (wr->write_value & wr->write_mask);

    if(!wr->reg->write_mask) {
      wr->next = reglist;
      reglist = wr;
    } else {
      free(wr);
    }

    reg->write_value = regvalue;
    reg->write_mask = regmask;
  }

  // Read out the list of modified registers and perform writing
  while(reglist) {
    struct mb_reg_write_request_s *wr = reglist;
    reglist = reglist->next;
    int regvalue = wr->reg->value;
    regvalue = regvalue & ~(wr->write_mask);
    regvalue = regvalue | (wr->write_value);
    dlist->mb_write_device(dlist, wr->dev_id, wr->reg_id, regvalue);
    wr->reg->write_value = 0;
    wr->reg->write_mask  = 0;
    free(wr);
  }

  // Update registers values
  for(i = 0; i < dlist->dev_max + 1; i ++) {
    if(dlist->device[i].mb_reg_max > 0) {
      dlist->mb_read_device(dlist, i, dlist->device[i].mb_reg_max);
    }
  }
}

int mb_dev_check_signal(struct mb_device_list_s *dlist, struct signal_s *signal) {
  int mb_id = signal->s_register.dr_device.d_mb_id;
  int addr = signal->s_register.dr_addr;
  int i, result = 0;

  // Check bondaries
  if(mb_id > MAX_DEVS) {
    return -1;
  }

  if(addr > MAX_REG) {
    return -1;
  }

  // Select device/register the signal belongs to
  struct mb_device_s *device = &dlist->device[mb_id];
  struct mb_device_reg_s *reg = &device->reg[addr];
  int value = 0;

  // Check the signal value and compare to the one stored in the register
  switch(signal->s_register.dr_type) {
  case 'i':
    if(reg->value != signal->s_value) {
      result = 1;
      signal->s_value = reg->value;
    }
    break;
  case 'b':
    value = (reg->value & (1 << signal->s_register.dr_bit)) >> signal->s_register.dr_bit;
    signal->s_value = signal->s_value ? 1 : 0;
    if(value != signal->s_value) {
      result = 1;
      signal->s_value = value;
    }
    break;
  }

  return result;
}
