/***************************************************
 * proto.h
 * Created on Fri, 20 Oct 2017 05:53:34 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#ifndef PROTO_H_
#define PROTO_H_

#include <stdint.h>

#define GETCMD(cmd) ((int)((cmd)[0] | ((cmd)[1]<<8) | ((cmd)[2]<<16)))
#define CONST_READ   ('R' | 'D' << 8 | ' ' << 16)
#define CONST_WRITE  ('W' | 'R' << 8 | ' ' << 16)
#define CONST_UPDATE ('U' | 'P' << 8 | 'D' << 16)
#define CONST_SUB    ('S' | 'U' << 8 | 'B' << 16)
#define CONST_UNSUB  ('B' | 'U' << 8 | 'S' << 16)
#define STATUS_OK    ('O' | 'K' << 8 )
#define STATUS_ERR   ('E' | 'R' << 8 )
#define CMD_READ    "RD "
#define CMD_WRITE   "WR "
#define CMD_UPDATE  "UPD"
#define CMD_SUB     "SUB"
#define CMD_UNSUB   "BUS"

struct cmd_s {
  char c_cmd[3];
  uint16_t c_num_param;
  uint16_t c_param[0];
}__attribute__((__packed__));

struct cmd_signal_s {
  uint16_t cs_name_len;
  char cs_name[0];
}__attribute__((__packed__));

struct cmd_entry_s {
  struct cmd_s *ce_command;
  struct cmd_signal_s *ce_signal;
};

struct cmd_packet_header_s {
  uint16_t cph_size;
  uint16_t cph_num_cmd;
  char cph_command[0];
}__attribute__((__packed__));

// *************** Response ****************

struct response_signal_description_s {
  uint16_t rsd_value;
  uint16_t rsd_rw;
  uint16_t rsd_reg_type;
  uint16_t rsd_reg_addr;
  uint16_t rsd_reg_bit;
  uint16_t rsd_dev_type;
  uint16_t rsd_dev_mb_id;
}__attribute__((__packed__));

struct response_entry_s {
  struct cmd_signal_s *re_signal;
  struct response_signal_description_s *re_signal_desc;
};

struct response_packet_header_s {
  uint16_t rph_size;
  uint16_t rph_status;
  uint16_t rph_num_signals;
  char rph_signals[0];
}__attribute__((__packed__));

int cmd_next(struct cmd_entry_s *command, int *n, char *buffer, int bufferSize);
int response_next(struct response_entry_s *command, int *n, char *buffer, int bufferSize);

struct cmd_packet_header_s *cmd_create_packet(char *buffer);
int cmd_create_command(struct cmd_packet_header_s *, int bufsize, char *signal, char *cmd, int nparam, ...);

int cmd_add(struct cmd_packet_header_s *packet, int bufsize, struct cmd_entry_s *command, int num_param, const char *signal);
int response_add(struct response_packet_header_s *packet, int bufsize, struct response_entry_s *response, const char *signal);

int packet_read(int socket, void *buffer, int bufferSize);

#endif
