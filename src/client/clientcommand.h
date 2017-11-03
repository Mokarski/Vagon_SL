/***************************************************
 * clientcommand.h
 * Created on Sat, 21 Oct 2017 05:25:09 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#ifndef CLIENTCOMMAND_H_
#define CLIENTCOMMAND_H_

#include "common/proto.h"
#include "common/hash.h"
#include "common/signal.h"
#include "client.h"

void subscribe(struct signal_s **signal, struct hash_s *hash, char *mask, int socket, int type);
void write_signal(struct signal_s **signal, struct hash_s *hash, char *mask, int socket, int type);
void update_signal(struct signal_s **signal, struct hash_s *hash, char *mask, int socket, int type);
void get_signals(struct signal_s **signal, struct hash_s *hash, char *mask, int socket);
void get_and_subscribe(struct signal_s **signal, struct hash_s *hash, char *mask, int socket, int type);
void process_command_write(struct signal_s *signal, struct hash_s *hash, struct cmd_entry_s *command, struct execution_context_s *ctx);
void process_command_update(struct signal_s *signal, struct hash_s *hash, struct cmd_entry_s *command, struct execution_context_s *ctx);

void post_process(struct execution_context_s *ctx);
void post_read_command(struct execution_context_s *ctx, char *name);
void post_write_command(struct execution_context_s *ctx, char *name, int value);
void post_update_command(struct execution_context_s *ctx, char *name, int value);
void post_subscribe_command(struct execution_context_s *ctx, char *name, int value);
void post_unsubscribe_command(struct execution_context_s *ctx, char *name, int value);

#endif
