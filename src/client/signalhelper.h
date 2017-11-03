/***************************************************
 * signalhelper.h
 * Created on Tue, 24 Oct 2017 03:54:26 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#ifndef SIGNALHELPER_H_
#define SIGNALHELPER_H_

#include "common/hash.h"
#include "client.h"

int  signal_get(struct execution_context_s *h, char *name);
void signal_set(struct execution_context_s *h, char *name, int value);

#endif
