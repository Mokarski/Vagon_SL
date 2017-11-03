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
#ifdef MODBUS_ENABLE
#include <modbus.h>
#endif
#include "common/signal.h"
#include "common/subscription.h"
#include "common/ringbuffer.h"
#include "client/client.h"
#include "client/signalhelper.h"
#include "mbdev.h"

int modbus_read(struct mb_device_list_s *ctx, int mbid, int max_regs) {
#ifdef MODBUS_ENABLE
  modbus_t *mb_ctx = ctx->mb_context;
  uint16_t regs[MAX_REG];
  int i;
  
  if(mb_ctx != NULL) {
    modbus_set_slave(mb_ctx, mbid);

    int rc = modbus_read_registers(mb_ctx, 0, max_regs, regs);

    modbus_flush(mb_ctx);

    if(rc == -1) {
			perror("Modbus read error");
      return -1;
    }

    for(i = 0; i < max_regs; i ++) {
      ctx->device[mbid].reg[i].value = regs[i];
    }

    return 0;
  } else {
    //printf("Read: No modbus device opened\n");
    return -1;
  }
#else // Testing without modbus library 
  usleep(10000);
  return 0;
#endif
}

int modbus_write(struct mb_device_list_s *ctx, int mbid, int reg, int value) {
#ifdef MODBUS_ENABLE
  void *mb_ctx = ctx->mb_context;
  uint16_t regs[MAX_REG];

  if(mb_ctx != NULL) {
    modbus_set_slave(mb_ctx, mbid);

		uint16_t registers[1] = { value };
		int rc = modbus_write_registers(mb_ctx, reg, 1, registers); //write in device by register

    modbus_flush(mb_ctx);

    if(rc == -1) {
			printf("Writing modbus register failed: write failed\n");
      return -1;
    }

    return 0;
  } else {
    printf("Write: No modbus device opened\n");
    return -1;
  }
#else // Testing without modbus library 
  usleep(10000);
  return 0;
#endif
}

void event_update_signal(struct signal_s *signal, int value, struct execution_context_s *ctx) {
  // Should never get here
  printf("*** EVENT HANDLER: Signal %s updating from value %d to value %d\n", signal->s_name, signal->s_value, value);
}

void event_write_signal(struct signal_s *signal, int value, struct execution_context_s *ctx) {
  mb_dev_add_write_request(ctx->clientstate, signal, value);
}

void *create_mb_context() {
#ifdef MODBUS_ENABLE
  modbus_t *ctx = modbus_new_tcp ("192.168.1.150", 502);

  if(!ctx) {
    perror("Couldn't open TCP modbus\n");
  }

	modbus_set_error_recovery(ctx, 1);
	modbus_set_debug(ctx, 0);

	modbus_set_slave(ctx, 1);
	while(modbus_connect(ctx));

  struct timeval byte_timeout;
	struct timeval response_timeout;

	return ctx;
#else
  return NULL;
#endif
}

void client_init(struct execution_context_s *ctx, int argc, char **argv) {
  struct signal_s *s;
  struct mb_device_list_s *dlist;

  dlist = malloc(sizeof(struct mb_device_list_s));
  mb_dev_list_init(dlist);

  dlist->mb_read_device  = &modbus_read;
  dlist->mb_write_device = &modbus_write;
  dlist->mb_context = create_mb_context();

  ctx->clientstate = dlist;
  get_and_subscribe(&ctx->signals, ctx->hash, "dev.wago", ctx->socket, SUB_WRITE);
  s = ctx->signals;
  while(s) {
    mb_dev_add_signal(ctx->clientstate, s);
		printf("Signal %s added\n", s->s_name);
    s = s->next;
  }
}

// Polling modbus devices
void client_thread_proc(struct execution_context_s *ctx) {
  printf("Started TCPmodbus wago proc thread\n");
  while(ctx->running) {
    struct signal_s *s;
		int i = 0;
    mb_dev_update(ctx->clientstate);
    s = ctx->signals;
    while(s) {
			i ++;
      if(mb_dev_check_signal(ctx->clientstate, s) == 1) {
				printf("Signal %s updated; posting update\n", s->s_name);
        post_update_command(ctx, s->s_name, s->s_value);
      }
      s = s->next;
    }
    post_process(ctx);
  }
}
