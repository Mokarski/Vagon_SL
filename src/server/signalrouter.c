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
#include "signalrouter.h"
#include "servercommand.h"

int main(int argc, char **argv) {
  FILE *s = fopen("signals.cfg", "r");
  char line[128];
  char name[128], *prefix;
  int i = 0, cnt = 0, found, running = 1;
  struct signal_s *signals = NULL, *current;
  struct hash_s *hash = NULL;
  struct execution_context_s ctx;

  hash_create(&hash);

  while(!feof(s)) {
    if(fgets(line, sizeof(line), s)) {
      cnt ++;
      current = malloc(sizeof(struct signal_s));
      str_to_signal(line, current);
      strcpy(name, current->s_name);
      prefix = name + strlen(name);

      do {
        *prefix = '\0';
        hash_add(hash, name, current);
      } while(prefix = strrchr(name, '.'));

      current->next = signals;
      signals = current;
    }
  }

	fclose(s);
  current = signals;

  while(current) {
    //hash_find_first(hash, current->s_name);
    current->s_id = i ++;
    current = current->next;
  }

  int listening;
  struct sockaddr_in addr;
  pthread_t worker;
  int reuse = 1, addrlen = sizeof(addr);
	int event[2];

	socketpair(AF_LOCAL, SOCK_STREAM, 0, event);

  ctx.event_socket = event[1];
  ctx.signals = signals;
  ctx.events = NULL;
  ctx.hash = hash;
  bzero(ctx.sockets, sizeof(ctx.sockets));
  bzero(ctx.subscriptions, sizeof(ctx.subscriptions));
  hash_create(&ctx.subscription_by_signal);

  if(pthread_create(&worker, NULL, &connection_worker, &ctx) != 0) {
    perror("Error! Couldn't spawn worker thread");
    abort();
  }

  listening = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if(listening < 0) {
    perror("Creating server socket failed");
    abort();
  }

	if(setsockopt(listening, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    perror("Setsockopt failed");
    abort();
  }

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	if(bind(listening, (struct sockaddr *)&addr , sizeof(addr)) < 0) {
		perror("Bind failed");
		abort();
	}

  if(listen(listening, 10) < 0) {
		perror("Listen failed");
		abort();
  }

  printf("Accepting connections\n");
  while(running) {
    char *evbuf = "w";
    int sc;
    int client_sock = accept(listening, (struct sockaddr*)&addr, &sc);

    if(client_sock < 0) {
      perror("Accept error");
      abort();
    }

		for(sc = 0; sc < MAX_CONN; sc ++) {
			if(ctx.sockets[sc] == 0) {
				ctx.sockets[sc] = client_sock;
				write(event[0], evbuf, 1);
				break;
			}
		}
  }
}

// ****************** Connection worker ***********************

void *connection_worker(void *arg) {
  struct execution_context_s *ctx = arg;
	fd_set socks;
  signal(SIGPIPE, SIG_IGN);

	while(1) {
		int n=0, i, maxfd = ctx->event_socket;

		FD_ZERO(&socks);
		FD_SET(ctx->event_socket, &socks);

		for(i = 0; i < MAX_CONN; i ++) {
			if(ctx->sockets[i] != 0) {
				maxfd = maxfd > ctx->sockets[i] ? maxfd : ctx->sockets[i];
				FD_SET(ctx->sockets[i], &socks);
			}
		}

		if(select(maxfd + 1, &socks, NULL, NULL, NULL) < 0) {
			continue;
		}

		if(FD_ISSET(ctx->event_socket, &socks)) {
			char localbuf[127];
			read(ctx->event_socket, localbuf, sizeof(localbuf));
		}

		for(i = 0; i < MAX_CONN; i ++) {
      int socket = ctx->sockets[i];

      if(!socket || !FD_ISSET(socket, &socks)) {
        continue;
      }

      ctx->current_client = i;

      if(!process_command(ctx, socket)) {
        close(socket);
        ctx->sockets[i] = 0;
      }

      process_events(ctx);
    }
  }
  printf("Stopping server");
}
