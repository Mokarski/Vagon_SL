/***************************************************
 * servercommand.c
 * Created on Sat, 21 Oct 2017 06:38:10 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "common/hash.h"
#include "servercommand.h"

void process_command_read(struct execution_context_s *context, struct cmd_entry_s *command,
                          struct response_packet_header_s *h, struct response_entry_s *r, size_t bufsize) {
  struct signal_s *signals[512];
  char signal_name[128] = {0};
  int i, snum;
  struct response_signal_description_s *desc;

  memcpy(signal_name, command->ce_signal->cs_name, ntohs(command->ce_signal->cs_name_len));
  snum = hash_find_all(context->hash, signal_name, (void**)signals, sizeof(signals) / sizeof(signals[0]));

  if(snum > sizeof(signals) / sizeof(signals[0])) {
    printf("Signals buffer overflow\n");
    return;
  }

  for(i = 0; i < snum; i ++) {
    if(!response_add(h, bufsize, r, signals[i]->s_name)) {
      printf("Failed to add response: buffer size too small\n");
      continue;
    }

    desc = r->re_signal_desc;
    desc->rsd_value     = signals[i]->s_value;
    desc->rsd_rw        = signals[i]->s_rw;
    desc->rsd_reg_type  = signals[i]->s_register.dr_type;
    desc->rsd_reg_addr  = signals[i]->s_register.dr_addr;
    desc->rsd_reg_bit   = signals[i]->s_register.dr_bit;
    desc->rsd_dev_type  = signals[i]->s_register.dr_device.d_type;
    desc->rsd_dev_mb_id = signals[i]->s_register.dr_device.d_mb_id;
  }
}

void process_command_update(struct execution_context_s *context, struct cmd_entry_s *command,
                            struct response_packet_header_s *h, struct response_entry_s *r, size_t bufsize) {
  struct signal_s *signals[512];
  char signal_name[128] = {0};
  int i, snum;
  struct response_signal_description_s *desc;

  memcpy(signal_name, command->ce_signal->cs_name, ntohs(command->ce_signal->cs_name_len));
	signal_name[ntohs(command->ce_signal->cs_name_len)] = 0;
  snum = hash_find_all(context->hash, signal_name, (void**)signals, sizeof(signals) / sizeof(signals[0]));

  if(snum > sizeof(signals) / sizeof(signals[0])) {
    printf("Signals buffer overflow\n");
    return;
  }

  if(ntohs(command->ce_command->c_num_param) != 1) {
    return;
  }

  int value = ntohs(command->ce_command->c_param[0]);

  for(i = 0; i < snum; i ++) {
    #ifdef DEBUG
    printf("Updating signal %s to value %d\n", signals[i]->s_name, value);
    #endif
    struct event_s *e = malloc(sizeof(struct event_s));
    e->e_type = SUB_UPDATE;
    e->e_signal = signals[i];
    e->e_value = value;
    e->e_next = context->events;
    context->events = e;
    signals[i]->s_value = value;
  }
}

void process_command_write(struct execution_context_s *context, struct cmd_entry_s *command,
                           struct response_packet_header_s *h, struct response_entry_s *r, size_t bufsize) {
  struct signal_s *signals[512];
  char signal_name[512] = {0};
  int i, snum;
  struct response_signal_description_s *desc;

  memcpy(signal_name, command->ce_signal->cs_name, ntohs(command->ce_signal->cs_name_len));
  snum = hash_find_all(context->hash, signal_name, (void**)signals, sizeof(signals) / sizeof(signals[0]));

  if(snum > sizeof(signals) / sizeof(signals[0])) {
    printf("Signals buffer overflow\n");
    return;
  }

  if(ntohs(command->ce_command->c_num_param) != 1) {
    return;
  }

  int value = ntohs(command->ce_command->c_param[0]);

  for(i = 0; i < snum; i ++) {
    #ifdef DEBUG
    printf("Writing signal %s value %d\n", signals[i]->s_name, value);
    #endif
    struct event_s *e = malloc(sizeof(struct event_s));
    e->e_type   = SUB_WRITE;
    e->e_signal = signals[i];
    e->e_value  = value; // Old value
    e->e_next   = context->events;
    context->events = e;
  }
}

void process_command_subscribe(struct execution_context_s *context, struct cmd_entry_s *command,
                               struct response_packet_header_s *h, struct response_entry_s *r, size_t bufsize) {
  struct signal_s *signals[512];
  char signal_name[512] = {0};
  int i, snum;
  struct response_signal_description_s *desc;

  memcpy(signal_name, command->ce_signal->cs_name, ntohs(command->ce_signal->cs_name_len));
  snum = hash_find_all(context->hash, signal_name, (void**)signals, sizeof(signals) / sizeof(signals[0]));

  if(snum > sizeof(signals) / sizeof(signals[0])) {
    printf("Signals buffer overflow\n");
    return;
  }

  if(ntohs(command->ce_command->c_num_param) != 1) {
    return;
  }

  int type = ntohs(command->ce_command->c_param[0]);

  if(type > 2) {
    return;
  }

  for(i = 0; i < snum; i ++) {
    struct subscription_s *s = subscription_list_add(&context->subscriptions[context->current_client], signals[i], 
                                                     type, context->current_client);
    hash_add(context->subscription_by_signal, signals[i]->s_name, s);
  }
}

void process_command_unsubscribe(struct execution_context_s *context, struct cmd_entry_s *command,
                                 struct response_packet_header_s *h, struct response_entry_s *r, size_t bufsize) {
  struct signal_s *signals[512];
  char signal_name[512] = {0};
  int i, snum;
  struct response_signal_description_s *desc;

  memcpy(signal_name, command->ce_signal->cs_name, ntohs(command->ce_signal->cs_name_len));
  snum = hash_find_all(context->hash, signal_name, (void**)signals, sizeof(signals) / sizeof(signals[0]));

  if(snum > sizeof(signals) / sizeof(signals[0])) {
    printf("Signals buffer overflow\n");
    return;
  }

  if(ntohs(command->ce_command->c_num_param) != 1) {
    return;
  }

  int type = ntohs(command->ce_command->c_param[0]);

  if(type > 2) {
    return;
  }

  struct subscription_s **prev = &context->subscriptions[context->current_client].sl_subscriptions[type];
  for(i = 0; i < snum; i ++) {
    struct subscription_s *s = *prev;
    while(s) {
      if(signals[i] == s->s_signal) {
        hash_remove_one(context->subscription_by_signal, signals[i]->s_name, s);
        *prev = s->s_next;
        free(s);
      } else {
        prev = &s->s_next;
      }
      s = *prev;
    }
  }
}

int process_command(struct execution_context_s *context, int socket) {
  char buffer[16384], *readstart = buffer;
  char response_buffer[20480], *writestart;
  int  n = 0, bytes, bufsize = 0;
  int  cmdname = 0, i;
  struct cmd_entry_s command;
  struct cmd_packet_header_s *hdr = (struct cmd_packet_header_s *)buffer;
  struct response_packet_header_s *response = (struct response_packet_header_s *)response_buffer;
  struct response_entry_s entry;

  response->rph_num_signals = htons(0);
  response->rph_status = STATUS_OK;
  response->rph_size = htons(sizeof(struct response_packet_header_s));

  bytes = packet_read(socket, buffer, sizeof(buffer));

  if(bytes <= 0) {
    printf("Socket %d (client #%d): cleaning up client subscriptions\n", socket, context->current_client);
    for(i = 0; i < SUB_MAX; i ++) {
      struct subscription_s *s = context->subscriptions[context->current_client].sl_subscriptions[i];
      while(s) {
        hash_remove_one(context->subscription_by_signal, s->s_signal->s_name, s);
        s = s->s_next;
      }
    }

    subscription_list_clear(&context->subscriptions[context->current_client]);
    return 0;
  }

  do {
    cmd_next(&command, &n, buffer, bufsize);

    if(!command.ce_command || !command.ce_signal) {
      response->rph_status = STATUS_ERR;
      break;
    }

    switch(GETCMD(command.ce_command->c_cmd)) {
    case CONST_READ:
      process_command_read(context, &command, response, &entry, sizeof(response_buffer));
      break;
    case CONST_WRITE:
      process_command_write(context, &command, response, &entry, sizeof(response_buffer));
      break;
    case CONST_UPDATE:
      process_command_update(context, &command, response, &entry, sizeof(response_buffer));
      break;
    case CONST_SUB:
      process_command_subscribe(context, &command, response, &entry, sizeof(response_buffer));
      break;
    case CONST_UNSUB:
      process_command_unsubscribe(context, &command, response, &entry, sizeof(response_buffer));
      break;
    }
  } while(n);

  int size = ntohs(response->rph_size);
  write(socket, response_buffer, size);

  return 1;
}
