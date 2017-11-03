/***************************************************
 * virtualclient.c
 * Created on Sun, 22 Oct 2017 19:31:13 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/signal.h"
#include "common/subscription.h"
#include "common/ringbuffer.h"
#include "client/client.h"
#include "client/signalhelper.h"

void event_update_signal(struct signal_s *signal, int value, struct execution_context_s *ctx) {
	static int oil_pump_started = 0;
	printf("Updating %s\n", signal->s_name);
	if(!strcmp(signal->s_name, "dev.485.kb.key.start_oil_pump")) {
		if(!oil_pump_started) {
			post_write_command(ctx, "dev.485.kb.kbl.start_oil_pump", 1);
			post_write_command(ctx, "dev.485.kb.kbl.led_contrast", 50);
			post_process(ctx);
		}
	}
	if(!strcmp(signal->s_name, "dev.485.kb.key.stop_oil_pump")) {
		if(!oil_pump_started) {
			post_write_command(ctx, "dev.485.kb.kbl.start_oil_pump", 0);
			post_write_command(ctx, "dev.485.kb.kbl.led_contrast", 0);
			post_process(ctx);
		}
	}
}

void event_write_signal(struct signal_s *signal, int value, struct execution_context_s *ctx) {
//  printf("*** EVENT HANDLER: Writing value %d to signal %s\n", value, signal->s_name);
//  post_update_command(ctx, "dev.485.rsrs2.sound2_ledms", value);
}

void client_init(struct execution_context_s *ctx) {
  printf("Initializing virtual client\n");
  get_signals(&ctx->signals, ctx->hash, "dev.485", ctx->socket);
  subscribe(&ctx->signals, ctx->hash, "dev.485", ctx->socket, SUB_UPDATE);
  ctx->clientstate = NULL;
  printf("Client initialized\n");
}

void client_thread_proc(struct execution_context_s *ctx) {
  printf("Started proc thread\n");
  while(ctx->running) {
    post_process(ctx);
    usleep(50000);
  }
}
