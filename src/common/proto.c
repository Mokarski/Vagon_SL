/***************************************************
 * proto.c
 * Created on Fri, 20 Oct 2017 07:38:11 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "proto.h"

int cmd_next(struct cmd_entry_s *command, int *n, char *buffer, int bufferSize) {
  struct cmd_packet_header_s *cmd = (struct cmd_packet_header_s *)buffer;
  struct cmd_s *cmd_entry;
  struct cmd_signal_s *cmd_signal;
  char *start;
  
  if(*n == 0) {
    *n = ntohs(cmd->cph_num_cmd);
    start = &cmd->cph_command[0];
  } else {
    start = &command->ce_signal->cs_name[ntohs(command->ce_signal->cs_name_len)];
  }

  command->ce_command = NULL;
  command->ce_signal = NULL;

  if(*n == 0) {
    return 0;
  }

  (*n) --;
  cmd_entry = (struct cmd_s*)start;
  cmd_signal = (struct cmd_signal_s*)&cmd_entry->c_param[ntohs(cmd_entry->c_num_param)];

  command->ce_command = cmd_entry;
  command->ce_signal = cmd_signal;

  return 1;
}

int response_next(struct response_entry_s *response, int *n, char *buffer, int bufferSize) {
  struct response_packet_header_s *hdr = (struct response_packet_header_s *)buffer;
  char *start;

  if(*n == 0) {
    if(hdr->rph_status != STATUS_OK) {
      return 0;
    }
    *n = ntohs(hdr->rph_num_signals);
    start = &hdr->rph_signals[0];
  } else {
    start = (char*)&response->re_signal_desc[1];
  }

  if(*n == 0) {
    return 0;
  }

  (*n) --;
  response->re_signal = (struct cmd_signal_s *)start;
  response->re_signal_desc = (struct response_signal_description_s *)&response->re_signal->cs_name[ntohs(response->re_signal->cs_name_len)];
  return 1;
}

int cmd_add(struct cmd_packet_header_s *packet, int bufsize, struct cmd_entry_s *command, int num_param, const char *signal) {
  int ncmd = ntohs(packet->cph_num_cmd);
  int signal_len = strlen(signal);
  int size;

  if(ncmd == 0) {
    command->ce_command = (struct cmd_s *)&packet->cph_command[0];
  } else {
    command->ce_command = (struct cmd_s *)&command->ce_signal->cs_name[ntohs(command->ce_signal->cs_name_len)];
  }

  command->ce_signal = (struct cmd_signal_s*)&command->ce_command->c_param[num_param];
  size = ((char*)&command->ce_signal->cs_name[signal_len]) - ((char*)packet);
  if(size > bufsize) {
    return 0;
  }

  command->ce_command->c_num_param = htons(num_param);
  command->ce_signal->cs_name_len = htons(signal_len);
  memcpy(command->ce_signal->cs_name, signal, ntohs(command->ce_signal->cs_name_len));
  ncmd ++;
  packet->cph_num_cmd = htons(ncmd);
  packet->cph_size = htons(size);
  return 1;
}

int response_add(struct response_packet_header_s *packet, int bufsize, struct response_entry_s *response, const char *signal) {
  int ncmd = ntohs(packet->rph_num_signals);
  int signal_len = strlen(signal);
  int size;

  if(ncmd == 0) {
    response->re_signal = (struct cmd_signal_s *)&packet->rph_signals[0];
  } else {
    response->re_signal = (struct cmd_signal_s *)&response->re_signal_desc[1];
  }

  response->re_signal_desc = (struct response_signal_description_s *)&response->re_signal->cs_name[signal_len];
  size = ((char*)(&response->re_signal_desc[1])) - ((char*)packet);
  if(size > bufsize) {
    return 0;
  }

  response->re_signal->cs_name_len = htons(strlen(signal));
  memcpy(response->re_signal->cs_name, signal, signal_len);
  ncmd ++;
  packet->rph_num_signals = htons(ncmd);
  packet->rph_size = htons(size);
  return 1;
}

struct cmd_packet_header_s *cmd_create_packet(char *buffer) {
  struct cmd_packet_header_s *cmd = (struct cmd_packet_header_s *)buffer;
  int n = 0;

  cmd->cph_num_cmd = htons(0);
  cmd->cph_size = htons(sizeof(struct cmd_packet_header_s));
  return cmd;
}

int cmd_create_command(struct cmd_packet_header_s *pkt, int bufsize, char *signal, char *cmd, int nparam, ...) {
  int ncmd = ntohs(pkt->cph_num_cmd);
  int size = ntohs(pkt->cph_size);
  int len = strlen(signal), i;
  char *buffer = (char*)pkt;
  struct cmd_s *cmd_entry = (struct cmd_s*)&buffer[size];
  struct cmd_signal_s *cmd_signal = (struct cmd_signal_s *)&cmd_entry->c_param[nparam];
  int newSize = (char*)&cmd_signal->cs_name[len] - (char*)pkt;
  va_list args;

  if(newSize > bufsize) {
    return 0;
  }

  memmove(cmd_entry->c_cmd, cmd, 3);
  cmd_entry->c_num_param = htons(nparam);
  cmd_signal->cs_name_len = htons(len);
  memmove(cmd_signal->cs_name, signal, len);

  va_start(args, nparam);

  for(i = 0; i < nparam; i ++) {
    cmd_entry->c_param[i] = htons(va_arg(args, int));
  }

  va_end(args);

  ncmd ++;

  pkt->cph_size = htons(newSize);
  pkt->cph_num_cmd = htons(ncmd);

  return ncmd;
}

int packet_read(int socket, void *buffer, int bufferSize) {
  struct cmd_packet_header_s *hdr = (struct cmd_packet_header_s *)buffer;
  int size, bytes, dataread = 0;

  if(bufferSize < sizeof(struct cmd_packet_header_s)) {
		printf("Buffer size too small\n");
    return -1;
  }

  bytes = recv(socket, buffer, sizeof(struct cmd_packet_header_s), 0);

  if(bytes <= 0) {
		perror("Packet header read error");
    return bytes;
  }

  size = ntohs(hdr->cph_size);

  if(bufferSize < size) {
		printf("Packet too big (%d bytes) for the buffer provided (%d bytes)\n", size, bufferSize);
    return -1;
  }

  size -= sizeof(struct cmd_packet_header_s);

  while(size > dataread) {
    bytes = recv(socket, &hdr->cph_command[dataread], size - dataread, 0);
    if(bytes < 0) {
			perror("Packet read error");
      return -1;
    }
    dataread += bytes;
  }

  return size + sizeof(struct cmd_packet_header_s);
}

