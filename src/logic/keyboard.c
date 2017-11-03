#include "keyboard.h"
#include "../client/client.h"
#include "../client/clientcommand.h"
#include "../client/signalhelper.h"

#define __USE_UNIX98
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <inttypes.h>

#define MODE_IDLE			0
#define MODE_PUMPING	1
#define MODE_DIAG			2
#define MODE_NORM			3

#define B_OVERLOAD_START	0
#define B_OVERLOAD_STOP		1
#define B_CONV_START	2
#define B_CONV_STOP		3
#define B_ORGAN_START	4
#define B_ORGAN_STOP	5
#define B_OIL_START		6
#define B_OIL_STOP		7
#define B_HYDRA_START	8
#define B_HYDRA_STOP	9
#define B_STARS_START	10
#define B_STARS_STOP	11
#define B_CHECK_START	12
#define B_CHECK_STOP	13
#define B_SOUND_ALARM	14
#define B_START_ALL		15
#define B_STOP_ALL		16
#define B_STARS_REVERSE	17

#define	J_CONVEYOR		0
#define	J_ORGAN				1
#define	J_LEFT_T			2
#define	J_RIGHT_T			3
#define	J_ACCEL				4
#define	J_TELESCOPE		5
#define	J_RESERVE			6
#define	J_SUPPORT			7
#define	J_SOURCER			8

#define J_BIT_UP			0
#define J_BIT_DOWN		1
#define J_BIT_LEFT		2
#define J_BIT_RIGHT		3
#define JOYVAL_UP			0x1
#define JOYVAL_DOWN		0x2
#define JOYVAL_LEFT		0x4
#define JOYVAL_RIGHT	0x8

#define IS_UP(j)	((joystick[j] & JOYVAL_UP))
#define IS_DOWN(j)	((joystick[j] & JOYVAL_DOWN))
#define IS_LEFT(j)	((joystick[j] & JOYVAL_LEFT))
#define IS_RIGHT(j)	((joystick[j] & JOYVAL_RIGHT))

pthread_mutex_t g_waitMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_waitCond = PTHREAD_COND_INITIALIZER;
pthread_t g_worker;
static volatile int g_mode = 0, g_workStarted = 0, g_diag = 0;
volatile int buttons[32] = {0};
volatile int joystick[32] = {0};

int Wait_For_Feedback(char *name, int expect, int timeout, volatile int *what) {
	int oc  = signal_get(g_Ctx, name);       
	struct timespec start, now;
	clock_gettime(CLOCK_REALTIME, &start);

	printf("Waiting for feedback %d\n", expect);

	while((oc != expect) && (*what)) {
		int exState;
		oc  = signal_get(g_Ctx, name);
		if(oc == expect) continue;
		usleep(10000);
		clock_gettime(CLOCK_REALTIME, &now);
		if((now.tv_sec > start.tv_sec + timeout) || (now.tv_sec == start.tv_sec + timeout) && (now.tv_nsec >= start.tv_nsec)) {
			printf("Result: %d\n", oc);
			return oc == expect;
		}
	}

	printf("Result: %d\n", oc);
	return oc == expect;
}

int Get_Signal(char *name) {

	return signal_get(g_Ctx,name);
}

void Worker_Set_Mode(int mode) {
	if(g_mode == mode) {
		return;
	}
	if(g_mode == MODE_IDLE) {
		while(g_workStarted != 0 && pthread_self() != g_worker)
			pthread_yield();
		pthread_mutex_lock(&g_waitMutex);
		g_mode = mode;
		pthread_mutex_unlock(&g_waitMutex);
		pthread_cond_signal(&g_waitCond);
		return;
	} else {
		if(mode != MODE_IDLE) {
			Worker_Set_Mode(MODE_IDLE);
			Worker_Set_Mode(mode);
			return;
		}
		//pthread_mutex_lock(&g_waitMutex);
		g_mode = mode;
		//pthread_mutex_unlock(&g_waitMutex);
	}
}

void Check_Stop_Buttons() {
	buttons[B_CHECK_STOP]			|= Get_Signal("dev.485.kb.key.stop_check");
	buttons[B_STARS_STOP]			|= Get_Signal("dev.485.kb.key.stop_stars");
	buttons[B_HYDRA_STOP]			|= Get_Signal("dev.485.kb.key.stop_hydratation");
	buttons[B_OIL_STOP] 			|= Get_Signal("dev.485.kb.key.stop_oil_station");
	buttons[B_OVERLOAD_STOP]  |= Get_Signal("dev.485.kb.key.stop_reloader");
	buttons[B_CONV_STOP] 			|= Get_Signal("dev.485.kb.key.stop_conveyor");
	buttons[B_ORGAN_STOP] 		|= Get_Signal("dev.485.kb.key.stop_exec_dev");

	buttons[B_OVERLOAD_STOP]  |= Get_Signal("dev.485.rpdu485.kei.reloader_down");
	buttons[B_CONV_STOP] 			|= Get_Signal("dev.485.rpdu485.kei.conveyor_down");
	buttons[B_ORGAN_STOP]			|= Get_Signal("dev.485.rpdu485.kei.exec_dev_down");
	buttons[B_OIL_STOP] 			|= Get_Signal("dev.485.rpdu485.kei.oil_station_down");

	buttons[B_OVERLOAD_STOP]  |= Get_Signal("dev.485.kb.pukonv485c.stop_loader");
	buttons[B_CONV_STOP] 			|= Get_Signal("dev.485.kb.pukonv485c.stop_loader");
	buttons[B_STARS_STOP]			|= Get_Signal("dev.485.kb.pukonv485c.stop_loader");

	buttons[B_OVERLOAD_STOP]  |= Get_Signal("dev.485.rpdu485.kei.stop_loader");
	buttons[B_CONV_STOP] 			|= Get_Signal("dev.485.rpdu485.kei.stop_loader");
	buttons[B_STARS_STOP]			|= Get_Signal("dev.485.rpdu485.kei.stop_loader");

	buttons[B_SOUND_ALARM]		 = 0;
	buttons[B_SOUND_ALARM]		|= Get_Signal("dev.485.kb.kei1.sound_alarm");
	buttons[B_SOUND_ALARM]    |= Get_Signal("dev.485.kb.pukonv485c.beep");
	buttons[B_SOUND_ALARM]    |= Get_Signal("dev.485.rpdu485.sound_beepl");
}

void Process_RED_BUTTON() {
	Worker_Set_Mode(MODE_IDLE);
}

void Process_Mode_Change() {
	Worker_Set_Mode(MODE_IDLE);
}

 int Pukonv_Conv_Joy_Animation () {
 int left = Get_Signal("dev.485.kb.pukonv485c.joy_left_conv");
 int right = Get_Signal("dev.485.kb.pukonv485c.joy_right_conv");
 int up = Get_Signal("dev.485.kb.pukonv485c.joy_up_conv");
 int down = Get_Signal("dev.485.kb.pukonv485c.joy_down_conv");
 WRITE_SIGNAL("panel10.kb.kei1.conveyor_left",left);
 WRITE_SIGNAL("panel10.kb.kei2.conveyor_right",right);
 WRITE_SIGNAL("panel10.kb.kei2.conveyor_up",up);
 WRITE_SIGNAL("panel10.kb.kei2.conveyor_down",down);
}



int Radio_Execdev_Joy_Anim() {
	static int left = 0, right = 0, up = 0, down = 0;

	if(up != IS_UP(J_ORGAN)) {
		WRITE_SIGNAL("panel10.kb.kei3.exec_dev_up", up = IS_UP(J_ORGAN));
	}

	if(down != IS_DOWN(J_ORGAN)) {
		WRITE_SIGNAL("panel10.kb.kei2.exec_dev_down", down = IS_DOWN(J_ORGAN));
	}

	if(left != IS_LEFT(J_ORGAN)) {
		WRITE_SIGNAL("panel10.kb.kei2.exec_dev_left", left = IS_LEFT(J_ORGAN));
	}

	if(left != IS_RIGHT(J_ORGAN)) {
		WRITE_SIGNAL("panel10.kb.kei2.exec_dev_right", right = IS_RIGHT(J_ORGAN));
	}
}

int Radio_Conv_Joy_Anim() {
	static int left = 0, right = 0, up = 0, down = 0;

	if(up != IS_UP(J_CONVEYOR)) {
		WRITE_SIGNAL("panel10.kb.kei1.conveyor_up", up = IS_UP(J_CONVEYOR));
	}

	if(down != IS_DOWN(J_CONVEYOR)) {
		WRITE_SIGNAL("panel10.kb.kei1.conveyor_down", down = IS_DOWN(J_CONVEYOR));
	}

	if(left != IS_LEFT(J_CONVEYOR)) {
		WRITE_SIGNAL("panel10.kb.kei1.conveyor_left", left = IS_LEFT(J_CONVEYOR));
	}

	if(right != IS_RIGHT(J_CONVEYOR)) {
		WRITE_SIGNAL("panel10.kb.kei1.conveyor_right", right = IS_RIGHT(J_CONVEYOR));
	}
}

int Process_Pu_Conv() {
	if(Get_Signal("dev.485.kb.kei1.post_conveyor")) {
		buttons[B_SOUND_ALARM]			= Get_Signal("dev.485.kb.pukonv485c.beep");
		joystick[J_CONVEYOR] = (Get_Signal("dev.485.kb.pukonv485c.joy_left_conv") << J_BIT_LEFT) | (Get_Signal("dev.485.kb.pukonv485c.joy_down_conv") << J_BIT_DOWN) |
													(Get_Signal("dev.485.kb.pukonv485c.joy_up_conv") << J_BIT_UP) | (Get_Signal("dev.485.kb.pukonv485c.joy_right_conv") << J_BIT_RIGHT);

//  Pukonv_Conv_Joy_Animation ();
		return 1;
	}

	return 0;
}

void Process_Local_Kb() {
	buttons[B_OVERLOAD_START] |= Get_Signal("dev.485.kb.key.start_reloader");
	buttons[B_ORGAN_START]		|= Get_Signal("dev.485.kb.key.start_exec_dev");
	buttons[B_OIL_START] 			|= Get_Signal("dev.485.kb.key.start_oil_station");
	buttons[B_HYDRA_START]		|= Get_Signal("dev.485.kb.key.start_hydratation");
	buttons[B_CHECK_START]		|= Get_Signal("dev.485.kb.key.start_check");
	buttons[B_CONV_START] 	  |= Get_Signal("dev.485.kb.key.start_conveyor");
	buttons[B_STARS_START]		|= Get_Signal("dev.485.kb.key.start_stars");
	buttons[B_START_ALL]			|= Get_Signal("dev.485.kb.kei1.start_all");
	buttons[B_STOP_ALL]				|= Get_Signal("dev.485.kb.kei1.stop_all");

	Check_Stop_Buttons();

	if(!Process_Pu_Conv()) {
		joystick[J_CONVEYOR] = (Get_Signal("dev.485.kb.kei1.conveyor_left") << J_BIT_LEFT) | (Get_Signal("dev.485.kb.kei1.conveyor_right") << J_BIT_RIGHT) |
													 (Get_Signal("dev.485.kb.kei1.conveyor_up") << J_BIT_UP) | (Get_Signal("dev.485.kb.kei1.conveyor_down") << J_BIT_DOWN);
	}

	joystick[J_ORGAN] = (Get_Signal("dev.485.kb.kei1.exec_dev_left") << J_BIT_LEFT) | (Get_Signal("dev.485.kb.kei1.exec_dev_down") << J_BIT_DOWN) | 
											(Get_Signal("dev.485.kb.kei1.exec_dev_right") << J_BIT_RIGHT) | (Get_Signal("dev.485.kb.kei1.exec_dev_up") << J_BIT_UP);
	joystick[J_LEFT_T] = (Get_Signal("dev.485.kb.kei1.left_truck_back") << J_BIT_DOWN) | (Get_Signal("dev.485.kb.kei1.left_truck_forward") << J_BIT_UP);
	joystick[J_RIGHT_T] = (Get_Signal("dev.485.kb.kei1.right_truck_back") << J_BIT_DOWN) | (Get_Signal("dev.485.kb.kei1.right_truck_forward") << J_BIT_UP);
	joystick[J_ACCEL] = (Get_Signal("dev.485.kb.kei1.acceleration") << J_BIT_UP);
	joystick[J_TELESCOPE] = (Get_Signal("dev.485.kb.kei1.telescope_up") << J_BIT_UP) | (Get_Signal("dev.485.kb.kei1.telescope_down") << J_BIT_DOWN);
	joystick[J_RESERVE] = (Get_Signal("dev.485.kb.kei1.reserve_down") << J_BIT_DOWN) | (Get_Signal("dev.485.kb.kei1.reserve_up") << J_BIT_UP);
	joystick[J_SUPPORT] = (Get_Signal("dev.485.kb.kei1.combain_support_down") << J_BIT_DOWN) | (Get_Signal("dev.485.kb.kei1.combain_support_up") << J_BIT_UP);
	joystick[J_SOURCER] = (Get_Signal("dev.485.kb.kei1.sourcer_down") << J_BIT_DOWN) | (Get_Signal("dev.485.kb.kei1.sourcer_up") << J_BIT_UP);
}

void Process_Cable_Kb() {
	Check_Stop_Buttons();
}

void Process_Radio_Kb() {
	char lt = '0', rt = '0';
	static int stars_started = 0;

	joystick[J_SUPPORT] = (Get_Signal("dev.485.rpdu485.kei.support_down") << J_BIT_DOWN) | (Get_Signal("dev.485.rpdu485.kei.support_up") << J_BIT_UP);
	joystick[J_SOURCER] = (Get_Signal("dev.485.rpdu485.kei.sourcer_down") << J_BIT_DOWN) | (Get_Signal("dev.485.rpdu485.kei.sourcer_up") << J_BIT_UP);
	joystick[J_ACCEL] = (Get_Signal("dev.485.rpdu485.kei.acceleration_up") << J_BIT_UP);
	joystick[J_TELESCOPE] = (Get_Signal("dev.485.rpdu485.kei.telescope_up") << J_BIT_UP) | (Get_Signal("dev.485.rpdu485.kei.telescope_down") << J_BIT_DOWN);
	joystick[J_ORGAN] = (Get_Signal("dev.485.rpdu485.kei.joy_exec_dev_left") << J_BIT_LEFT) | (Get_Signal("dev.485.rpdu485.kei.joy_exec_dev_down") << J_BIT_DOWN) | 
											(Get_Signal("dev.485.rpdu485.kei.joy_exec_dev_right") << J_BIT_RIGHT) | (Get_Signal("dev.485.rpdu485.kei.joy_exec_dev_up") << J_BIT_UP);
	if(!Process_Pu_Conv()) {
		joystick[J_CONVEYOR] = (Get_Signal("dev.485.rpdu485.kei.joy_conv_up") << J_BIT_UP) | (Get_Signal("dev.485.rpdu485.kei.joy_conv_down") << J_BIT_DOWN) |
													 (Get_Signal("dev.485.rpdu485.kei.joy_conv_left") << J_BIT_LEFT) | (Get_Signal("dev.485.rpdu485.kei.joy_conv_right") << J_BIT_RIGHT);
	}
  Radio_Conv_Joy_Anim();
	int jleft, jright, jup, jdown;
	jleft = Get_Signal("dev.485.rpdu485.kei.joy_forward");
	jright = Get_Signal("dev.485.rpdu485.kei.joy_back");
	jup = Get_Signal("dev.485.rpdu485.kei.joy_left");
	jdown = Get_Signal("dev.485.rpdu485.kei.joy_right");
  /*
	int support_down = Get_Signal("485.rpdudev.485.kei.support_down");
	int support_up =  Get_Signal("485.rpdu485.kei.support_up");
	WRITE_SIGNAL("panel10.kb.kei1.combain_support_down",support_down);
	WRITE_SIGNAL("panel10.kb.kei1.combain_support_up",support_up);

	int sourcer_dwon = Get_Signal("485.rpdu485с.kei.sourcer_down");
	int sourcer_up = Get_Signal("485.rpdu485с.kei.sourcer_up");
	WRITE_SIGNAL("panel10.kb.kei1.sourcer_down",sourcer_dwon);
	WRITE_SIGNAL("panel10.kb.kei1.sourcer_up",sourcer_up);
  */
	if(jup) {
		joystick[J_LEFT_T] = (jright << J_BIT_UP) | (!jleft << J_BIT_UP);
		joystick[J_RIGHT_T] = (jleft << J_BIT_UP) | (!jright << J_BIT_UP);
	} else if(jdown) {
		joystick[J_LEFT_T] = (jright << J_BIT_DOWN) | (!jleft << J_BIT_DOWN);
		joystick[J_RIGHT_T] = (jleft << J_BIT_DOWN) | (!jright << J_BIT_DOWN);
	} else {
		joystick[J_LEFT_T] = jright << J_BIT_UP | jleft << J_BIT_DOWN;
		joystick[J_RIGHT_T] = jright << J_BIT_DOWN | jleft << J_BIT_UP;
	}

	buttons[B_OVERLOAD_START] |= Get_Signal("dev.485.rpdu485.kei.reloader_up");
	buttons[B_CONV_START] 	  |= Get_Signal("dev.485.rpdu485.kei.conveyor_up");
	buttons[B_ORGAN_START]		|= Get_Signal("dev.485.rpdu485.kei.exec_dev_up");
	buttons[B_OIL_START] 			|= Get_Signal("dev.485.rpdu485.kei.oil_station_up");
	buttons[B_START_ALL]			|= Get_Signal("dev.485.rpdu485.kei.start_all");
	buttons[B_STOP_ALL]				|= Get_Signal("dev.485.rpdu485.kei.stop_all");
	
	if(Get_Signal("dev.485.rpdu485.kei.loader_up")) {
		if(stars_started == B_STARS_REVERSE) {
			buttons[B_STARS_REVERSE] = 0;
		}
		if(stars_started == 0) {
			buttons[B_STARS_START]	|= 1;
			stars_started = B_STARS_START;
		}
	} else if(Get_Signal("dev.485.rpdu485.kei.loader_down")) {
		if(stars_started == B_STARS_START) {
			buttons[B_STARS_START] = 0;
		}
		if(stars_started == 0) {
			buttons[B_STARS_REVERSE]	|= 1;
			stars_started = B_STARS_REVERSE;
		}
	} else {
		if(stars_started != 0) {
			buttons[B_STARS_STOP] = 1;
			stars_started = 0;
		}
	}
	
	Check_Stop_Buttons();
	//buttons[B_STARS_START]		|= Get_Signal("");
	//buttons[B_STARS_STOP]			|= Get_Signal("");
	//buttons[B_CHECK_START]		|= Get_Signal("");
	//buttons[B_CHECK_STOP]			|= Get_Signal("");
}

void Process_Normal() {
	if(g_diag != 0) {
		set_Diagnostic(0);
		g_diag = 0;
	}
	Worker_Set_Mode(MODE_NORM);
}

void Process_Pumping() {
#define PUMPING_START		"dev.485.kb.key.start_oil_pump"
#define PUMPING_STOP		"dev.485.kb.key.stop_oil_pump"
	int start = Get_Signal(PUMPING_START), stop = Get_Signal(PUMPING_STOP);

	if(stop != 0) {
		printf("Stop button pressed\n");		
		printf("Setting idle mode\n");
		Worker_Set_Mode(MODE_IDLE);
	} else if(start != 0) {
		printf("Start button pressed\n");
		if(g_mode != MODE_PUMPING) {
			printf("Starting\n");
			printf("Lighting the button contrast\n");
			WRITE_SIGNAL("dev.485.kb.kbl.led_contrast", 50);
			WRITE_SIGNAL("dev.panel10.system_state_code", 22);
			printf("Lighting the start oil pump button\n");
			WRITE_SIGNAL("dev.485.kb.kbl.start_oil_pump", 1);
			
			//post_read_command(g_Ctx,"dev.wago.oc_bki.M7");			
			//post_read_command(g_Ctx,"dev.wago.oc_mdi1.oc_w_qf1");
			//post_read_command(g_Ctx,"dev.wago.oc_mdo1.ka7_1");			
			printf("Setting mode\n");
			Worker_Set_Mode(MODE_PUMPING);
		}
	}
}

void Process_Diag() {
	if(g_diag != 1) {
		set_Diagnostic(1);
		g_diag = 1;
	}
	Worker_Set_Mode(MODE_NORM);
}

///////////////////////////////////////////////////////////////////////////////////////////

void Process_Timeout() {
	struct timespec start, now;
	printf("Processing timeout for mode %d\n", g_mode);

	WRITE_SIGNAL( "dev.485.rsrs2.state_sound1_on", 1);
	WRITE_SIGNAL( "dev.485.rsrs2.state_sound1_led", 1);
	WRITE_SIGNAL( "dev.485.rsrs2.state_sound2_on", 1);
	WRITE_SIGNAL( "dev.485.rsrs2.state_sound2_led", 1);
	
	printf("Getting time\n", g_mode);
	clock_gettime(CLOCK_REALTIME, &start);
	printf("Got time\n");
	while(g_mode != 0) {
		clock_gettime(CLOCK_REALTIME, &now);
		if((now.tv_sec > (start.tv_sec + 5)) || (now.tv_sec == (start.tv_sec + 5)) && (now.tv_nsec >= start.tv_nsec)) {
			printf("Exiting by timeout\n");
			break;
		}
		usleep(1000);
	}

	if(g_mode == 0) {
		printf("Mode changed!!!!!!!!!!!!!!!!!!!!!!\n");
	}

	WRITE_SIGNAL("dev.485.rsrs2.state_sound1_on", 0);
	WRITE_SIGNAL("dev.485.rsrs2.state_sound1_led", 0);
	WRITE_SIGNAL("dev.485.rsrs2.state_sound2_on", 0);
	WRITE_SIGNAL("dev.485.rsrs2.state_sound2_led", 0);
}

///////////////////////////////////////////////////////////////////////////////////////////
#define LEFT_TRACK_FW	0
#define LEFT_TRACK_BW	1
#define RIGHT_TRACK_FW	2
#define RIGHT_TRACK_BW	3
#define	ASSOCIATE_CONTROL(what, value, signal, panel_signal)	\
if(joystick[what] & value) { \
		if(!(controls[what] & value)) { \
			controls[what] |= value; \
			WRITE_SIGNAL(signal, 1); \
			if(panel_signal) WRITE_SIGNAL(panel_signal, 1); \
		} \
} else { \
		if(controls[what] & value) { \
			controls[what] &= ~value; \
			WRITE_SIGNAL(signal, 0); \
			if(panel_signal) WRITE_SIGNAL(panel_signal, 0); \
		} \
}
#define	DISABLE(what, value, signal, panel_signal)	\
		if(controls[what] & value) { \
			controls[what] &= ~value; \
			WRITE_SIGNAL(signal, 0); \
			if(panel_signal) WRITE_SIGNAL(panel_signal, 0); \
		}
struct timespec last_moving;

void Process_Joysticks() {
	static int controls[16] = {0};

	if(g_diag || enabled_Oil()) {
		if(joystick[J_LEFT_T] && !controls[J_LEFT_T] && !controls[J_RIGHT_T] || 
			 joystick[J_RIGHT_T] && !controls[J_RIGHT_T] && !controls[J_LEFT_T]) {
			struct timespec now;
			clock_gettime(CLOCK_REALTIME, &now);
			if(now.tv_sec - last_moving.tv_sec > 6) {
				Process_Timeout();
			}
		}

		ASSOCIATE_CONTROL(J_LEFT_T, JOYVAL_UP, "dev.485.rsrs.rm_u2_on10", "panel10.kb.kei2.left_truck_forward");
		ASSOCIATE_CONTROL(J_LEFT_T, JOYVAL_DOWN, "dev.485.rsrs.rm_u2_on11", "panel10.kb.kei2.left_truck_back");
		ASSOCIATE_CONTROL(J_RIGHT_T, JOYVAL_UP, "dev.485.rsrs.rm_u2_on0", "panel10.kb.kei2.right_truck_forward");
		ASSOCIATE_CONTROL(J_RIGHT_T, JOYVAL_DOWN, "dev.485.rsrs.rm_u2_on1", "panel10.kb.kei2.right_truck_back");

		ASSOCIATE_CONTROL(J_ORGAN, JOYVAL_UP, "dev.485.rsrs.rm_u1_on6", "panel10.kb.kei3.exec_dev_up");
		ASSOCIATE_CONTROL(J_ORGAN, JOYVAL_DOWN, "dev.485.rsrs.rm_u1_on7", "panel10.kb.kei2.exec_dev_down");
		ASSOCIATE_CONTROL(J_ORGAN, JOYVAL_LEFT, "dev.485.rsrs.rm_u1_on3", "panel10.kb.kei2.exec_dev_left");
		ASSOCIATE_CONTROL(J_ORGAN, JOYVAL_RIGHT, "dev.485.rsrs.rm_u1_on2", "panel10.kb.kei2.exec_dev_right");

		ASSOCIATE_CONTROL(J_ACCEL, JOYVAL_UP, "dev.485.rsrs.rm_u1_on0", NULL);

		ASSOCIATE_CONTROL(J_TELESCOPE, JOYVAL_UP, "dev.485.rsrs.rm_u1_on4", "panel10.kb.kei2.telescope_up");
		ASSOCIATE_CONTROL(J_TELESCOPE, JOYVAL_DOWN, "dev.485.rsrs.rm_u1_on5", "panel10.kb.kei3.telescope_down");

		ASSOCIATE_CONTROL(J_SUPPORT, JOYVAL_UP, "dev.485.rsrs.rm_u2_on8", "panel10.kb.kei2.combain_support_up");
		ASSOCIATE_CONTROL(J_SUPPORT, JOYVAL_DOWN, "dev.485.rsrs.rm_u2_on9", "panel10.kb.kei1.combain_support_down");

		ASSOCIATE_CONTROL(J_SOURCER, JOYVAL_UP, "dev.485.rsrs.rm_u1_on8", "panel10.kb.kei1.sourcer_up");
		ASSOCIATE_CONTROL(J_SOURCER, JOYVAL_DOWN, "dev.485.rsrs.rm_u1_on9", "panel10.kb.kei1.sourcer_down");

		ASSOCIATE_CONTROL(J_CONVEYOR, JOYVAL_UP, "dev.485.rsrs.rm_u2_on2", "panel10.kb.kei1.conveyor_up");
		ASSOCIATE_CONTROL(J_CONVEYOR, JOYVAL_DOWN, "dev.485.rsrs.rm_u2_on3", "panel10.kb.kei1.conveyor_down");
		ASSOCIATE_CONTROL(J_CONVEYOR, JOYVAL_LEFT, "dev.485.rsrs.rm_u2_on5", "panel10.kb.kei1.conveyor_left");
		ASSOCIATE_CONTROL(J_CONVEYOR, JOYVAL_RIGHT, "dev.485.rsrs.rm_u2_on4", "panel10.kb.kei1.conveyor_right");

		if(controls[J_LEFT_T] || controls[J_RIGHT_T]) {
			clock_gettime(CLOCK_REALTIME, &last_moving);
		}
	} else {
		DISABLE(J_LEFT_T, JOYVAL_UP, "dev.485.rsrs.rm_u2_on10", NULL);
		DISABLE(J_LEFT_T, JOYVAL_DOWN, "dev.485.rsrs.rm_u2_on11", NULL);
		DISABLE(J_RIGHT_T, JOYVAL_UP, "dev.485.rsrs.rm_u2_on0", NULL);
		DISABLE(J_RIGHT_T, JOYVAL_DOWN, "dev.485.rsrs.rm_u2_on1", NULL);

		DISABLE(J_ORGAN, JOYVAL_UP, "dev.485.rsrs.rm_u1_on6", NULL);
		DISABLE(J_ORGAN, JOYVAL_DOWN, "dev.485.rsrs.rm_u1_on7", NULL);
		DISABLE(J_ORGAN, JOYVAL_LEFT, "dev.485.rsrs.rm_u1_on3", NULL);
		DISABLE(J_ORGAN, JOYVAL_RIGHT, "dev.485.rsrs.rm_u1_on2", NULL);

		DISABLE(J_ACCEL, JOYVAL_UP, "dev.485.rsrs.rm_u1_on0", NULL);

		DISABLE(J_TELESCOPE, JOYVAL_UP, "dev.485.rsrs.rm_u1_on4", NULL);
		DISABLE(J_TELESCOPE, JOYVAL_DOWN, "dev.485.rsrs.rm_u1_on5", NULL);

		DISABLE(J_SUPPORT, JOYVAL_UP, "dev.485.rsrs.rm_u2_on8", NULL);
		DISABLE(J_SUPPORT, JOYVAL_DOWN, "dev.485.rsrs.rm_u2_on9", NULL);

		DISABLE(J_SOURCER, JOYVAL_UP, "dev.485.rsrs.rm_u1_on8", NULL);
		DISABLE(J_SOURCER, JOYVAL_DOWN, "dev.485.rsrs.rm_u1_on9", NULL);

		DISABLE(J_CONVEYOR, JOYVAL_UP, "dev.485.rsrs.rm_u2_on2", NULL);
		DISABLE(J_CONVEYOR, JOYVAL_DOWN, "dev.485.rsrs.rm_u2_on3", NULL);
		DISABLE(J_CONVEYOR, JOYVAL_LEFT, "dev.485.rsrs.rm_u2_on5", NULL);
		DISABLE(J_CONVEYOR, JOYVAL_RIGHT, "dev.485.rsrs.rm_u2_on4", NULL);
	}
}

void Work_Norm(){
	int alarm_enabled = 0;
	printf("Working in normal mode\n");
	while(g_mode == MODE_NORM) {
		if(buttons[B_SOUND_ALARM] && !alarm_enabled) {
			WRITE_SIGNAL( "dev.485.rsrs2.state_sound1_on", 1);
			WRITE_SIGNAL( "dev.485.rsrs2.state_sound1_led", 1);
			WRITE_SIGNAL( "dev.485.rsrs2.state_sound2_on", 1);
			WRITE_SIGNAL( "dev.485.rsrs2.state_sound2_led", 1);
			alarm_enabled = 1;
		} else if(!buttons[B_SOUND_ALARM] && alarm_enabled) {
			WRITE_SIGNAL( "dev.485.rsrs2.state_sound1_on", 0);
			WRITE_SIGNAL( "dev.485.rsrs2.state_sound1_led", 0);
			WRITE_SIGNAL( "dev.485.rsrs2.state_sound2_on", 0);
			WRITE_SIGNAL( "dev.485.rsrs2.state_sound2_led", 0);
			alarm_enabled = 0;
		}
		if(buttons[B_START_ALL]) {
			int stop = 0;
			int step = 0;
			for(step = 0; (step < 6) && !stop; step ++) {
				switch(step) {
				case 0:
					start_Oil();
					break;
				case 1:
					start_Overloading();
					break;
				case 2:
					start_Conveyor();
					break;
				case 3:
					start_Stars();
					usleep(50000);
					break;
				case 4:
					start_Hydratation();
					usleep(10000);
					break;
				case 5:
					start_Organ();
					break;
				default:
					break;
				}
				stop = buttons[B_STOP_ALL];
			}
			buttons[B_START_ALL] = 0;
			if(stop) {
				printf("Stopping all\n");
				stop_all();
				buttons[B_STOP_ALL] = 0;
			}
		}
		if(buttons[B_STOP_ALL]) {
			stop_all();
			buttons[B_STOP_ALL] = 0;
		}
		if(buttons[B_OVERLOAD_STOP]) {
			stop_Overloading();
			buttons[B_OVERLOAD_START] = buttons[B_OVERLOAD_STOP] = 0;
		} else if(buttons[B_OVERLOAD_START]) {
			start_Overloading();
			buttons[B_OVERLOAD_START] = 0;
		}
		if(buttons[B_CONV_STOP]) {
			printf("Stop conveyor\n");
			stop_Conveyor();
			buttons[B_CONV_START] = buttons[B_CONV_STOP] = 0;
		} else if(buttons[B_CONV_START]) {
			start_Conveyor();
			buttons[B_CONV_START] = 0;
		}
		if(buttons[B_ORGAN_STOP]) {
			stop_Organ();
			buttons[B_ORGAN_STOP] = 0;
			buttons[B_ORGAN_START] = 0;
		} else if(buttons[B_ORGAN_START]) {
			start_Organ();
			buttons[B_ORGAN_START] = 0;
		}
		if(buttons[B_OIL_STOP]) {
			stop_Oil();
			buttons[B_OIL_STOP] = 0;
			buttons[B_OIL_START] = 0;
		} else if(buttons[B_OIL_START]) {
			start_Oil();
			buttons[B_OIL_START] = 0;
		}
		if(buttons[B_HYDRA_STOP]) {
			stop_Hydratation();
			buttons[B_HYDRA_STOP] = 0;
			buttons[B_HYDRA_START] = 0;
		} else if(buttons[B_HYDRA_START]) {
			start_Hydratation();
			buttons[B_HYDRA_START] = 0;
		}
		if(buttons[B_STARS_STOP]) {
			stop_Stars();
			buttons[B_STARS_STOP] = 0;
			buttons[B_STARS_START] = 0;
			buttons[B_STARS_REVERSE] = 0;
		} else if(buttons[B_STARS_START]) {
			start_Stars(0);
			buttons[B_STARS_START] = 0;
			buttons[B_STARS_REVERSE] = 0;
		} else if(buttons[B_STARS_REVERSE]) {
			start_Stars(1);
			buttons[B_STARS_START] = 0;
			buttons[B_STARS_REVERSE] = 0;
		}
		Process_Joysticks();
		control_all();
		usleep(10000);
	}

	printf("Exiting normal mode\n");
	stop_all();
	WRITE_SIGNAL("dev.485.rsrs.rm_u2_on10", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u2_on11", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u2_on0", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u2_on1", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u1_on6", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u1_on7", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u1_on3", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u1_on2", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u1_on0", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u1_on4", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u1_on5", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u2_on8", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u2_on9", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u1_on8", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u1_on9", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u2_on2", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u2_on3", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u2_on5", 0);
	WRITE_SIGNAL("dev.485.rsrs.rm_u2_on4", 0);
}

void Work_Pumping() {
#define CHECK_MODE_PUMPING() if(g_mode != MODE_PUMPING) { return; }
	int exState;

	Process_Timeout();
	CHECK_MODE_PUMPING();

	int tMotor = Get_Signal("wago.oc_temp.pt100_m7");
	int bki = Get_Signal("wago.bki_k7.M7");

	if(bki != 0) { // || tMotor >= 70) {
		printf("tMotor: %d; bki: %d\n", tMotor, bki);
		Worker_Set_Mode(MODE_IDLE);
		return;
	}
	printf("Waiting feedback for 3 sec\n");
	WRITE_SIGNAL("wago.oc_mdo1.ka6_1", 1);
	RB_FLUSH();

	int oc = Wait_For_Feedback("wago.oc_mdi1.oc_w_k6", 1, 3, &g_mode);
	if(!oc) {
		printf("No Feedback!\n");
		printf("Feedback signal: %d\n", Get_Signal("wago.oc_mdi1.oc_w_k6"));
		printf("Feedback register: %d\n", Get_Signal("wago.oc_mdi1.oc"));
		Worker_Set_Mode(MODE_IDLE);
	}

  Pressure_Show(); //new 
  Water_Show(); //new

	printf("Pumping started\n");
	while(g_mode == MODE_PUMPING) {
		int m7a = Get_Signal("wago.oc_mui8.current_m7a");
		int m7b = Get_Signal("wago.oc_mui8.current_m7b");
		int m7c = Get_Signal("wago.oc_mui8.current_m7c");
		usleep(10000);
		// Check currents
	}
	int forever = 1;
	printf("Pumping stopped\n");
	do {
		printf("Turning off wago\n");
		WRITE_SIGNAL("wago.oc_mdo1.ka6_1", 0);
	} while(!Wait_For_Feedback("wago.oc_mdi1.oc_w_k6", 0, 3, &forever));
	printf("Dimming the button contrast\n");
	WRITE_SIGNAL("dev.485.kb.kbl.led_contrast", 1);
	printf("Dimming the start oil pump button\n");
	WRITE_SIGNAL( "dev.485.kb.kbl.start_oil_pump", 0);
}

void *Worker(void* arg) {
#define MODE_CHANGED (oldMode != g_mode)
#define CHECK_MODE()	if(MODE_CHANGED) {  }
	int oldMode = g_mode;
	while(1) {
		if(!g_mode) {
			oldMode = 0;
			pthread_mutex_lock(&g_waitMutex);
			g_workStarted = 0;
			pthread_cond_wait(&g_waitCond, &g_waitMutex);
			pthread_mutex_unlock(&g_waitMutex);
		}

		g_workStarted = 1;
		printf("Mode switched to %d\n", g_mode);

		switch(g_mode) {
			case MODE_PUMPING:
				Work_Pumping();
				break;

			case MODE_NORM:
				Work_Norm();
				break;
		}
	}
}

void Init_Worker() {
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&g_waitMutex, &attr);
	//pthread_create(&g_worker, NULL, &Worker, NULL);
}
