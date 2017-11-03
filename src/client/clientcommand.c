/***************************************************
 * clientcommand.c
 * Created on Sat, 21 Oct 2017 05:25:16 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "common/proto.h"
#include "common/subscription.h"
#include "clientcommand.h"

void parse_response(struct signal_s **signal, struct hash_s *hash, char *buffer, int bufSize) {
  struct response_entry_s response;
  char signal_name[256];
  int n = 0;
  struct signal_s *current;

  do {
    if(!response_next(&response, &n, buffer, bufSize)) {
      return;
    }

    int len = ntohs(response.re_signal->cs_name_len);
    memcpy(signal_name, response.re_signal->cs_name, len);
    signal_name[len] = 0;

    if((current = hash_find_first(hash, signal_name)) == NULL) {
      current = malloc(sizeof(struct signal_s));
      current->s_name = strdup(signal_name);
      current->next = *signal;
      *signal = current;
      hash_add(hash, signal_name, current);
    }

    current->s_value = response.re_signal_desc->rsd_value;
    current->s_rw = response.re_signal_desc->rsd_rw;
    current->s_register.dr_type = response.re_signal_desc->rsd_reg_type;
    current->s_register.dr_addr = response.re_signal_desc->rsd_reg_addr;
    current->s_register.dr_bit = response.re_signal_desc->rsd_reg_bit;
    current->s_register.dr_device.d_type = response.re_signal_desc->rsd_dev_type;
    current->s_register.dr_device.d_mb_id = response.re_signal_desc->rsd_dev_mb_id;
  } while(n);
}

void send_command(struct signal_s **signal, struct hash_s *hash, struct cmd_packet_header_s *cmd, int socket) {
  char buffer[20480];
  int size;
  size = ntohs(cmd->cph_size);
  send(socket, cmd, size, 0);
  size = packet_read(socket, buffer, sizeof(buffer));
  parse_response(signal, hash, buffer, size);
}

void subscribe(struct signal_s **signal, struct hash_s *hash, char *mask, int socket, int type) {
  char buffer[1024];
  struct cmd_packet_header_s *cmd = cmd_create_packet(buffer);

  if(!cmd_create_command(cmd, sizeof(buffer), mask, CMD_SUB, 1, type)) {
    return;
  }

  send_command(signal, hash, cmd, socket);
}

void update_signal(struct signal_s **signal, struct hash_s *hash, char *mask, int socket, int value) {
  char buffer[1024];
  struct cmd_packet_header_s *cmd = cmd_create_packet(buffer);

  if(!cmd_create_command(cmd, sizeof(buffer), mask, CMD_UPDATE, 1, value)) {
    return;
  }

  send_command(signal, hash, cmd, socket);
}

void write_signal(struct signal_s **signal, struct hash_s *hash, char *mask, int socket, int value) {
  char buffer[1024];
  struct cmd_packet_header_s *cmd = cmd_create_packet(buffer);

  if(!cmd_create_command(cmd, sizeof(buffer), mask, CMD_WRITE, 1, value)) {
    return;
  }

  send_command(signal, hash, cmd, socket);
}

void get_signals(struct signal_s **signal, struct hash_s *hash, char *mask, int socket) {
  char buffer[1024];
  struct cmd_packet_header_s *cmd = cmd_create_packet(buffer);

  if(!cmd_create_command(cmd, sizeof(buffer), mask, CMD_READ, 0)) {
    return;
  }

  send_command(signal, hash, cmd, socket);
}

void get_and_subscribe(struct signal_s **signal, struct hash_s *hash, char *mask, int socket, int type) {
  char buffer[1024];
  struct cmd_packet_header_s *cmd = cmd_create_packet(buffer);

  if(!cmd_create_command(cmd, sizeof(buffer), mask, CMD_SUB, 1, type)) {
    return;
  }

  if(!cmd_create_command(cmd, sizeof(buffer), mask, CMD_READ, 0)) {
    return;
  }

  send_command(signal, hash, cmd, socket);
}

void process_command_write(struct signal_s *signal, struct hash_s *hash, struct cmd_entry_s *command, struct execution_context_s *ctx) {
  struct signal_s *signals[512];
  char signal_name[512] = {0};
  int i, snum;

  memcpy(signal_name, command->ce_signal->cs_name, ntohs(command->ce_signal->cs_name_len));
  snum = hash_find_all(hash, signal_name, (void**)signals, sizeof(signals) / sizeof(signals[0]));

  if(snum > sizeof(signals) / sizeof(signals[0])) {
    printf("Signals buffer overflow\n");
    return;
  }

  int value = ntohs(command->ce_command->c_param[0]);
  for(i = 0; i < snum; i ++) {
    event_write_signal(signals[i], value, ctx);
  }
}

void process_command_update(struct signal_s *signal, struct hash_s *hash, struct cmd_entry_s *command, struct execution_context_s *ctx) {
  struct signal_s *signals[512];
  char signal_name[512] = {0};
  int i, snum;

  memcpy(signal_name, command->ce_signal->cs_name, ntohs(command->ce_signal->cs_name_len));
  snum = hash_find_all(hash, signal_name, (void**)signals, sizeof(signals) / sizeof(signals[0]));

  if(snum > sizeof(signals) / sizeof(signals[0])) {
    printf("Signals buffer overflow\n");
    return;
  }

  for(i = 0; i < snum; i ++) {
    event_update_signal(signals[i], ntohs(command->ce_command->c_param[0]), ctx);
    signals[i]->s_value = ntohs(command->ce_command->c_param[0]);
  }
}

void post_process(struct execution_context_s *ctx) {
  write(ctx->event_socket, "w", 1);
}

void post_read_command(struct execution_context_s *ctx, char *name) {
  struct rb_command_s *cmd = malloc(sizeof(struct rb_command_s));
  strcpy(cmd->c_cmd, "RD ");
  cmd->c_num_param = 0;
  strcpy(cmd->cs_name, name);
  ring_buffer_push(ctx->command_buffer, cmd);
}

void post_write_command(struct execution_context_s *ctx, char *name, int value) {
  struct rb_command_s *cmd = malloc(sizeof(struct rb_command_s));
  strcpy(cmd->c_cmd, "WR ");
  cmd->c_num_param = 1;
  cmd->c_param[0] = value;
  strcpy(cmd->cs_name, name);
  ring_buffer_push(ctx->command_buffer, cmd);
}

void post_update_command(struct execution_context_s *ctx, char *name, int value) {
  struct rb_command_s *cmd = malloc(sizeof(struct rb_command_s));
  strcpy(cmd->c_cmd, "UPD");
  cmd->c_num_param = 1;
  cmd->c_param[0] = value;
  strcpy(cmd->cs_name, name);
  ring_buffer_push(ctx->command_buffer, cmd);
}

void post_subscribe_command(struct execution_context_s *ctx, char *name, int value) {
  struct rb_command_s *cmd = malloc(sizeof(struct rb_command_s));
  strcpy(cmd->c_cmd, "SUB");
  cmd->c_num_param = 1;
  cmd->c_param[0] = value;
  strcpy(cmd->cs_name, name);
  ring_buffer_push(ctx->command_buffer, cmd);
}

void post_unsubscribe_command(struct execution_context_s *ctx, char *name, int value) {
  struct rb_command_s *cmd = malloc(sizeof(struct rb_command_s));
  strcpy(cmd->c_cmd, "BUS");
  cmd->c_num_param = 1;
  cmd->c_param[0] = value;
  strcpy(cmd->cs_name, name);
  ring_buffer_push(ctx->command_buffer, cmd);
}
