/*
 *   C ECHO client example using sockets
 */
#include <stdio.h> //printf
#include <string.h>    //strlen
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <stdlib.h>

#define  DEBUG 0 //may be 0,1,2,3
#include <errno.h>
#include <unistd.h>
#include "process.h"
#include "keyboard.h"

#define Devices 40
#define SignalsPerDev 100

#define CONTROL_MASK		0x3
#define CONTROL_RADIO		0x1
#define	CONTROL_MANU		0x2
#define CONTROL_CABLE		0x3

#define MODE_MASK				(0x03 << 2)
#define MODE_DIAG				(0x01 << 2)
#define MODE_PUMP				(0x02 << 2)
#define MODE_NORM				(0x03 << 2)
#define MODE_STOP				128

//char SignalHash[MAX_Signals][MAX_Signals]; //array for store index of Signals

int GetVal(char *Signal ){
	return signal_get(g_Ctx, Signal);
}

int Get_State(){
	int state=0;
	int x = 0;
	struct Signal *s;
	int mode1 = 0, mode2 = 0, control1 = 0,control2 = 0,alarm_stop1 = 0,alarm_stop2 = 0,alarm_stop3 = 0;

	mode1 = GetVal("dev.485.kb.kei1.mode1");
	mode2 = GetVal("dev.485.kb.kei1.mode2");
	alarm_stop1 = Get_Signal( "dev.485.kb.kei1.stop_alarm"); //gribok stop
	alarm_stop2 = Get_Signal( "dev.485.rpdu485.kei.crit_stop"); //gribok stop
	alarm_stop3 = Get_Signal( "dev.485.kb.pukonv485c.stop_alarm"); //gribok stop

	control1 = GetVal("dev.485.kb.kei1.control1");
	control2 = GetVal("dev.485.kb.kei1.control2");

	state = control1 | (control2 << 1) | (mode1 << 2) | (mode2 << 3);

	//printf (">>>>>>>>>>>>>>>>>>>>>> MODE STATE %i | mode1 %i, mode2 %i, control1 %i,control2 %i,alarm_stop1 %i,alarm_stop2 %i,alarm_stop3 %i   \n\r",state, mode1, mode2, control1,control2,alarm_stop1,alarm_stop2,alarm_stop3);
	if ((alarm_stop1 | alarm_stop2 | alarm_stop3)) {
		printf(" *GRIBOK STOP!!!|\n");
		return state | 128;
	}

	//if ((control1== 0) && ( control2==1)) state =31; //Mestno
	//if ((control1== 1) && ( control2==1)) state =32; //Provod	        
	//if ((control1== 1) && ( control2==0)) state =33; //Radio	        

	return state;
}

int Init (){
  Pressure_Show();
	Oil_Show();
	Metan_Show();
	Voltage_Show();
  printf("Wago ready state: %d \n", Get_Signal("dev.wago.oc_ready.state"));
  if(!Get_Signal("dev.wago.oc_ready.state")) {
	  printf("Wago Ready error!\n");
		return 0;
	}

	if(Get_Signal("dev.wago.oc_mdi.err_phase")) {
		printf("Phase error!\n");
		return 0;
	}

	if(Get_Signal("dev.wago.bki_k1.M1") ||
		 Get_Signal("dev.wago.bki_k2.M2") ||
		 Get_Signal("dev.wago.bki_k3_k4.M3_M4") ||
		 Get_Signal("dev.wago.bki_k5.M5") ||
		 Get_Signal("dev.wago.bki_k7.M7"))
	{
		printf("BKI error!\n");
		return 0;
	}

	if(!Get_Signal("dev.wago.oc_mdi1.oc_w_qf1")) {
		WRITE_SIGNAL("dev.wago.oc_mdo1.ka7_1", 1);
		return 0;
	}

	printf("Initialization completed!\n");
	return 1;
}

int process_loop()
{
	int STATE=0;
	static int oldMode = 0;
	static int initialized = 0;

	//while (1){
	if(!initialized) {
		initialized = Init();
		return;
	}

	STATE = Get_State();

	if(STATE & 128) {
		Process_RED_BUTTON();
		STATE = 0;
	}

	if(oldMode != (STATE & MODE_MASK)) {
		Process_Mode_Change();
		oldMode = STATE & MODE_MASK;
	}

	switch(STATE & MODE_MASK) {
		case MODE_NORM:
			Process_Normal();
			switch(STATE & CONTROL_MASK){
				case CONTROL_MANU:  //INIT
					Process_Local_Kb();
					System_Mode(1);
					System_Radio();
					Exec_Dev_Show();
					Pultk_Mode();
					Pressure_Show();
					Oil_Show();
					Metan_Show();
					Voltage_Show();
					Water_Show();
					break;         

				case CONTROL_CABLE:  //INIT
					System_Mode(2);
					System_Radio();

					Exec_Dev_Show();

					Pultk_Mode();
					Process_Cable_Kb();
					break;

				case CONTROL_RADIO:  //RESET 
					System_Mode(3);
					System_Radio();

					Exec_Dev_Show();

					Pultk_Mode();
					Process_Radio_Kb();
					Water_Show();
					Pressure_Show();
					Oil_Show();
					Metan_Show();
					Voltage_Show();
					break;	               

				default:  //DEFAULT
					if ( DEBUG == 1 )    printf("default state \n\r");  
			}
			break;
		case MODE_PUMP:
				 Pressure_Show();
				 System_Mode(4);
					System_Radio();
				 Oil_Show();
				 Metan_Show();
				 Voltage_Show();
				 Water_Show();
				 Process_Pumping();
			break;

		case MODE_DIAG:
			Process_Diag();
			switch(STATE & CONTROL_MASK){
				case CONTROL_MANU:  //INIT
					Process_Local_Kb();


					Exec_Dev_Show();

					System_Mode(5);
					break;         

				case CONTROL_CABLE:  //INIT
					Process_Cable_Kb();
					break;

				case CONTROL_RADIO:  //RESET 
					Process_Radio_Kb();
					break;	               

				default:  //DEFAULT
					if ( DEBUG == 1 )    printf("default state \n\r");  
			}
			break;
	}
	//}
}
