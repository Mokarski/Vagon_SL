/***************************************************
 * signalrouter.cpp
 * Created on Fri, 20 Oct 2017 05:52:31 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include "common/proto.h"
#include "clientcommand.h"
#include "client.h"

void *thread_worker(void *arg);

int main(int argc, char **argv) {
  char line[128];
  char name[128], *prefix;
  int i = 0, cnt = 0, found, running = 1;
  struct signal_s **select_list;
  struct execution_context_s ctx;

  select_list = malloc(cnt * sizeof(void*));

  int client;
  struct sockaddr_in addr;
  pthread_t worker;
  int event[2];

  client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if(client < 0) {
    perror("Creating server socket failed");
    abort();
  }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(0);

  if(bind(client, (struct sockaddr *)&addr , sizeof(addr)) < 0) {
    perror("Bind failed");
    abort();
  }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(argv[1]);
  //addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  addr.sin_port = htons(PORT);

  printf("Connecting to the server\n");
  if(connect(client, (struct sockaddr *)&addr , sizeof(addr)) < 0) {
    perror("Connect failed");
    abort();
  }

  struct signal_s *signal = 0;
  struct cmd_entry_s command;
  char   buffer[8192], response_buffer[8192];
  struct response_packet_header_s *response = (struct response_packet_header_s *)response_buffer;
  struct execution_context_s context;
  fd_set socks;

  socketpair(AF_LOCAL, SOCK_STREAM, 0, event);

  context.event_socket = event[1];
  context.socket = client;
  context.signals = NULL;
  ring_buffer_init(&context.command_buffer);
  hash_create(&context.hash);

  client_init(&context, argc, argv);

  int maxfd = context.event_socket > context.socket ? event[0] : context.socket;

  pthread_create(&context.worker_thread, NULL, &thread_worker, &context);

  while(running) {
    FD_ZERO(&socks);
    FD_SET(event[0], &socks);
    FD_SET(context.socket, &socks);

    if(select(maxfd + 1, &socks, NULL, NULL, NULL) < 0) {
      continue;
    }

    if(FD_ISSET(event[0], &socks)) {
      char localbuf[127];
      read(event[0], localbuf, sizeof(localbuf));
    }

    if(FD_ISSET(context.socket, &socks)) {
      int bytes = packet_read(client, buffer, sizeof(buffer));

      if(bytes <= 0) {
        perror("Socket read error");
        abort();
      }

      int n = 0;

      do {
        cmd_next(&command, &n, buffer, bytes);

        if(!command.ce_command || !command.ce_signal) {
          response->rph_status = STATUS_ERR;
          break;
        }

        switch(GETCMD(command.ce_command->c_cmd)) {
				case CONST_WRITE:
					process_command_write(context.signals, context.hash, &command, &context);
					break;
				case CONST_UPDATE:
					process_command_update(context.signals, context.hash, &command, &context);
					break;
        }
      } while(n);

      struct response_packet_header_s *packet = (struct response_packet_header_s *)&buffer;
      packet->rph_num_signals = 0;
      packet->rph_status = STATUS_OK;
      packet->rph_size = htons(sizeof(struct response_packet_header_s));
      send(client, packet, sizeof(struct response_packet_header_s), 0);
    }

    struct cmd_packet_header_s *cmd = cmd_create_packet(buffer);
    struct cmd_entry_s ce;
    int commands = 0;

    while(ring_buffer_size(context.command_buffer) > 0) {
      struct rb_command_s *command = ring_buffer_get(context.command_buffer);

      if(!cmd_add(cmd, sizeof(buffer), &ce, command->c_num_param, command->cs_name)) {
				// Buffer overflow
				send_command(&context.signals, context.hash, cmd, context.socket);
				cmd = cmd_create_packet(buffer);
				commands = 0;
				continue;
			}

      for(i = 0; i < command->c_num_param; i ++) {
        ce.ce_command->c_param[i] = htons(command->c_param[i]);
      }

      memcpy(ce.ce_command->c_cmd, command->c_cmd, 3);
      ring_buffer_pop(context.command_buffer);
      free(command);
      commands ++;
    }

    if(commands) {
      send_command(&context.signals, context.hash, cmd, context.socket);
    }
  }
}

void *thread_worker(void *arg) {
  ((struct execution_context_s *)arg)->running = 1;
  client_thread_proc((struct execution_context_s *)arg);
  return NULL;
}
