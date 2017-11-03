/***************************************************
 * servercommand.h
 * Created on Sat, 21 Oct 2017 06:37:36 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#ifndef SERVERCOMMAND_H_
#define SERVERCOMMAND_H_

#include <sys/types.h>
#include "signalrouter.h"
#include "common/proto.h"

int process_command(struct execution_context_s *context, int socket);

#endif
