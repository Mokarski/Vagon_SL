#pragma once

#include <common/ringbuffer.h>

#define RB_FLUSH() do { post_process(g_Ctx); while(ring_buffer_size(g_Ctx->command_buffer) > 0) pthread_yield(); } while(0);

struct execution_context_s *g_Ctx;

#define WRITE_SIGNAL(signal, value) do { 	post_write_command(g_Ctx,signal, value); post_process(g_Ctx); } while (0);
#define READ_SIGNAL(signal)		post_read_command(g_Ctx,signal);
#define CHECK(what) 	if(!inProgress[what]) return;

int Wait_For_Feedback(char *name, int expect, int timeout, volatile int *what);
void Process_Timeout();
int  Get_Signal(char *name);
void Init_Worker();
void Worker_Set_Mode(int mode);
void Process_Mode_Change();
void Process_RED_BUTTON();
void Process_Local_Kb();
void Process_Cable_Kb();
void Process_Radio_Kb();
void Process_Pumping();
void Process_Normal();
void Process_Diag();
