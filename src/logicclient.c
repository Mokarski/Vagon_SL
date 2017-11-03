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
#include "logic/keyboard.h"
#include "client/client.h"


void event_update_signal(struct signal_s *signal, int value, struct execution_context_s *ctx) {
	static int oil_pump_started = 0;
	printf("Updating signal [%s] value [%d => %d]\n", signal->s_name, signal->s_value, value);
	
	signal->s_value=value; //new value
	process_loop();
}

void event_write_signal(struct signal_s *signal, int value, struct execution_context_s *ctx) {
//  printf("*** EVENT HANDLER: Writing value %d to signal %s\n", value, signal->s_name);
//  post_update_command(ctx, "dev.485.rsrs2.sound2_ledms", value);
	//process_loop();
}

void client_init(struct execution_context_s *ctx, int argc, char **argv) {
  printf("Initializing virtual client\n");
	g_Ctx = ctx;
	get_and_subscribe(&ctx->signals, ctx->hash, "dev", ctx->socket, SUB_UPDATE);
  ctx->clientstate = NULL;
	Init_Worker();
  printf("Client initialized\n");
}

void client_thread_proc(struct execution_context_s *ctx) {
  printf("Started proc thread\n");
  while(ctx->running) {
		Worker(NULL);
  }
}
