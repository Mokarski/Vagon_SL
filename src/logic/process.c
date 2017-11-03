#include "process.h"
#include "keyboard.h"
#include <time.h>
#include <stdio.h>
#include <inttypes.h>

#define IDLE				0
#define STARTING		1
#define RUNNING			2

#define OVERLOADING	0
#define CONVEYOR		1
#define STARS				2
#define OIL					3
#define HYDRATATION	4
#define ORGAN				5

volatile int inProgress[6] = {0};
volatile int debugging = 0;

int waitForFeedback(char *name, int timeout, volatile int *what) {
	int oc  = 0;       
	struct timespec start, now;
	clock_gettime(CLOCK_REALTIME, &start);

	while(!oc && (*what)) {
		int exState;
		oc  = Get_Signal(name);
		if(oc) continue;
		usleep(10000);
		clock_gettime(CLOCK_REALTIME, &now);
		if((now.tv_sec > start.tv_sec + timeout) || (now.tv_sec == start.tv_sec + timeout) && (now.tv_nsec >= start.tv_nsec)) {
			printf("Feedback waiting timeout\n");
			return oc;
		}
	}

	return oc;
}

void set_Diagnostic(int val) {
	debugging = val;
}
void Pressure_Show() {
	//READ_SIGNAL("485.ad2.adc1_phys_value");
	//READ_SIGNAL("485.ad2.adc2_phys_value");
	//READ_SIGNAL("485.ad2.adc3_phys_value");
	//READ_SIGNAL("485.ad2.adc4_phys_value");
	//READ_SIGNAL("485.ad3.adc1_phys_value");
  
	int H1= Get_Signal("485.ad2.adc1_phys_value");
	int H2= Get_Signal("485.ad2.adc2_phys_value");
	int H3= Get_Signal("485.ad2.adc3_phys_value");
	int H4= Get_Signal("485.ad2.adc4_phys_value");
	int H5= Get_Signal("485.ad3.adc1_phys_value");
	if (H1 >0) H1 = (H1/40);
	if (H2 >0) H2 = (H2/40);
	if (H3 >0) H3 = (H3/25);
	if (H4 >0) H4 = (H4/40);
	if (H5 >0) H5 = (H5/40);
	WRITE_SIGNAL("panel10.system_pressure1",H1);
	WRITE_SIGNAL("panel10.system_pressure2",H2);
	WRITE_SIGNAL("panel10.system_pressure3",H3);
	WRITE_SIGNAL("panel10.system_pressure4",H4);
	WRITE_SIGNAL("panel10.system_pressure5",H5);
}

void Oil_Show(){
	//READ_SIGNAL ("485.ad1.adc1_phys_value");
	//READ_SIGNAL ("485.ad1.adc2_phys_value");
	int Oil_level =Get_Signal ("485.ad1.adc1_phys_value");
	int Oil_temp  =Get_Signal ("485.ad1.adc2_phys_value");
	if (Oil_level >0) Oil_level=Oil_level/10;
	if (Oil_temp >0) Oil_temp=Oil_temp/10;
	WRITE_SIGNAL("panel10.system_oil_level",Oil_level);
	WRITE_SIGNAL("panel10.system_oil_temp",Oil_temp);
}

void Water_Show() {
	//READ_SIGNAL("485.ad1.adc3_phys_value");
	//READ_SIGNAL("485.ad1.adc4_phys_value");
	int water_flow = Get_Signal("485.ad1.adc3_phys_value");
	int water_pressure = Get_Signal("485.ad1.adc4_phys_value");
	if (water_pressure > 0) water_pressure=(water_pressure/25);
	if (water_flow > 0) water_flow= (water_flow/4);
	WRITE_SIGNAL("panel10.system_water_flow",water_flow);
	WRITE_SIGNAL("panel10.system_water_pressure",water_pressure);
}


void Exec_Dev_Show() {
	//READ_SIGNAL("wago.oc_mui2.current_m1a");
	//READ_SIGNAL("wago.oc_mui2.current_m1b");
	//READ_SIGNAL("wago.oc_mui2.current_m1c");

	int m1_Ia = Get_Signal("wago.oc_mui2.current_m1a");
	int m1_Ib = Get_Signal("wago.oc_mui2.current_m1b");
	int m1_Ic = Get_Signal("wago.oc_mui2.current_m1c");
	int I_all=0;

//	int Volt = Get_Signal("wago.oc_mui1.Uin_PhaseA");

  if ((m1_Ia+m1_Ib+m1_Ic) > 0){
      I_all=(m1_Ia+m1_Ib+m1_Ic)/3;
	   }
	int P_m1=(I_all*100)/150;	
	int rot=35;
	WRITE_SIGNAL("panel10.system_execdev_load", P_m1);
	WRITE_SIGNAL("panel10.system_execdev_rotation",rot);
}

void Metan_Show() {

	//READ_SIGNAL("485.ad3.adc3_phys_value");
	int Metan = Get_Signal("485.ad3.adc3_phys_value");
	if (Metan > 0) Metan =1;
	WRITE_SIGNAL ("panel10.system_metan",Metan);
}

void Radio_Mode (int n) {
	WRITE_SIGNAL("panel10.system_radio",n);
}

void Mestno_Mode (int n) {
	WRITE_SIGNAL("panel10.system_mestno",n);
}
void System_Mode(int n) {
	WRITE_SIGNAL("panel10.system_mode",n);
}
void System_Radio() {
      //READ_SIGNAL("485.rpdu485.connect");
	int radio_connect = Get_Signal("485.rpdu485.connect");
	WRITE_SIGNAL("panel10.system_radio",radio_connect);
 }

void Pultk_Mode(){
	//READ_SIGNAL("485.kb.kei1.post_conveyor");
	int puk = Get_Signal("485.kb.kei1.post_conveyor");
	WRITE_SIGNAL("panel10.system_pultk",puk);
}
void Voltage_Show() {
	int Volt = Get_Signal("wago.oc_mui1.Uin_PhaseA");
	WRITE_SIGNAL("panel10.system_voltage",Volt);
}

void start_Overloading() {
	if(inProgress[OVERLOADING]) return;
	inProgress[OVERLOADING] = STARTING;
	printf("Starting overloading\n");
	WRITE_SIGNAL("485.kb.kbl.led_contrast", 50);
	WRITE_SIGNAL("485.kb.kbl.start_reloader", 1);
	WRITE_SIGNAL("485.rpdu485.kbl.reloader_green", 1);

	WRITE_SIGNAL("panel10.system_state_code",20);
	Process_Timeout();
	CHECK(OVERLOADING);

	int bki = Get_Signal("wago.bki_k6.M6");
	if(bki) {
		printf("BKI error!\n");
		stop_Overloading();
	}
	control_Overloading(); // Check temp
	CHECK(OVERLOADING);

	WRITE_SIGNAL("wago.oc_mdo1.ka5_1", 1);
//	WRITE_SIGNAL("panel10.system_state_code",20);
	WRITE_SIGNAL("panel10.kb.key.reloader",1);
	if(!waitForFeedback("wago.oc_mdi1.oc_w_k5", 3, &inProgress[OVERLOADING])) {
		stop_Overloading();
		return;
	}

	inProgress[OVERLOADING] = RUNNING;
	control_Overloading();
}

void start_Conveyor() {
	if(inProgress[CONVEYOR]) return;
	printf("Starting conveyor\n");
	inProgress[CONVEYOR] = STARTING;
	WRITE_SIGNAL("485.kb.kbl.led_contrast", 50);
	WRITE_SIGNAL("485.rpdu485.kbl.conveyor_green", 1);
	WRITE_SIGNAL("485.kb.kbl.start_conveyor", 1);

	WRITE_SIGNAL("panel10.system_state_code",16);
	Process_Timeout();
	CHECK(CONVEYOR);

	int bki = Get_Signal("wago.bki_k3_k4.M3_M4");
	if(bki) {
		printf("BKI error!\n");
		stop_Conveyor();
	}
	CHECK(CONVEYOR);

	WRITE_SIGNAL("wago.oc_mdo1.ka3_1", 1);
	WRITE_SIGNAL("panel10.system_state_code",2);
	WRITE_SIGNAL("panel10.kb.key.conveyor",1);
	if(!waitForFeedback("wago.oc_mdi1.oc_w_k3", 3, &inProgress[CONVEYOR])) {
		stop_Conveyor();
		return;
	}

	inProgress[CONVEYOR] = RUNNING;
	control_Conveyor();
}

void start_Stars(int reverse) {
	if(inProgress[STARS]) return;
	inProgress[STARS] = STARTING;
	control_Stars();
	CHECK(STARS);
	printf("Starting stars%s\n", reverse ? " reverse" : "");
	WRITE_SIGNAL("485.kb.kbl.led_contrast", 50);
	WRITE_SIGNAL("485.rpdu485.kbl.loader_green", 1);
	WRITE_SIGNAL("485.kb.kbl.start_stars", 1);

	WRITE_SIGNAL("panel10.system_state_code",40);
	Process_Timeout();

	CHECK(STARS);
	if(!reverse) {
		WRITE_SIGNAL("485.rsrs.rm_u2_on7", 0);
		WRITE_SIGNAL("485.rsrs.rm_u2_on6", 1);
	} else {
		WRITE_SIGNAL("485.rsrs.rm_u2_on6", 0);
		WRITE_SIGNAL("485.rsrs.rm_u2_on7", 1);
	}
	WRITE_SIGNAL("panel10.kb.key.stars",1);
	WRITE_SIGNAL("panel10.system_state_code",3);
	inProgress[STARS] = RUNNING;
	control_Stars();
}

int enabled_Oil(){
	return inProgress[OIL] == RUNNING;
}

void start_Oil() {
	if(inProgress[OIL]) return;
	printf("Starting oil station\n");
	WRITE_SIGNAL("485.kb.kbl.led_contrast", 50);
	WRITE_SIGNAL("485.rpdu485.kbl.oil_station_green", 1);
	WRITE_SIGNAL("485.kb.kbl.start_oil_station", 1);
	inProgress[OIL] = STARTING;

	WRITE_SIGNAL("panel10.system_state_code",14);
	Process_Timeout();
	CHECK(OIL);

	int bki = Get_Signal("wago.bki_k2.M2");
	if(bki) {
		printf("BKI error!\n");
		stop_Oil();
	}
	control_Oil(); // Check temp
	CHECK(OIL);

	WRITE_SIGNAL("wago.oc_mdo1.ka2_1", 1);
	WRITE_SIGNAL("panel10.system_state_code",4);
	WRITE_SIGNAL ("panel10.kb.key.oil_station",1);
	if(!waitForFeedback("wago.oc_mdi1.oc_w_k2", 3, &inProgress[OIL])) {
		printf("Feedback error, stopping oil station\n");
		stop_Oil();
		return;
	}

	inProgress[OIL] = RUNNING;
	control_Oil();
}

void start_Hydratation() {
	if(inProgress[HYDRATATION]) return;
	printf("Starting hydratation\n");
	inProgress[HYDRATATION] = STARTING;

	WRITE_SIGNAL("485.kb.kbl.led_contrast", 50);
	WRITE_SIGNAL("485.kb.kbl.start_hydratation", 1);

	WRITE_SIGNAL("panel10.system_state_code",18);
	Process_Timeout();
	CHECK(HYDRATATION);

	int bki = Get_Signal("wago.bki_k5.M5");
	if(bki) {
		printf("BKI error!\n");
		stop_Hydratation();
	}
	control_Hydratation(); // Check temp
	CHECK(HYDRATATION);

	WRITE_SIGNAL("wago.oc_mdo1.ka4_1", 1);
	WRITE_SIGNAL("wago.oc_mdo1.water1", 1);
	WRITE_SIGNAL("panel10.system_state_code",5);
	WRITE_SIGNAL("panel10.kb.key.hydratation",1);
	if(!waitForFeedback("wago.oc_mdi1.oc_w_k4", 3, &inProgress[HYDRATATION])) {
		printf("Feedback error, stopping hydratation\n");
		stop_Hydratation();
		return;
	}

	inProgress[HYDRATATION] = RUNNING;
	control_Hydratation();
}

void start_Organ() {
	if(inProgress[HYDRATATION] != RUNNING && !debugging) return;
	if(inProgress[ORGAN]) return;
	printf("Starting organ\n");
	inProgress[ORGAN] = STARTING;

	WRITE_SIGNAL("485.kb.kbl.led_contrast", 50);
	WRITE_SIGNAL("485.kb.kbl.start_exec_dev", 1);
	WRITE_SIGNAL("485.rpdu485.kbl.exec_dev_green", 1);


	WRITE_SIGNAL("panel10.system_state_code",12);
	Process_Timeout();
	CHECK(ORGAN);

	int bki = Get_Signal("wago.bki_k1.M1");
	if(bki) {
		printf("BKI error!\n");
		stop_Organ();
	}
	control_Organ(); // Check temp
	CHECK(ORGAN);

	WRITE_SIGNAL("wago.oc_mdo1.ka1_1", 1);
	WRITE_SIGNAL("panel10.system_state_code",6);
	WRITE_SIGNAL("panel10.kb.key.exec_dev",1);
	if(!waitForFeedback("wago.oc_mdi1.oc_w_k1", 3, &inProgress[ORGAN])) {
		printf("Feedback error, stopping organ\n");
		stop_Organ();
		return;
	}

	inProgress[ORGAN] = RUNNING;
	control_Organ();
}


void control_Overloading() {
	if(!inProgress[OVERLOADING]) return;
	int temp = Get_Signal("wago.oc_temp.pt100_m6");
	int tempRelay = Get_Signal("wago.ts_m1.rele_T_m6");

	if(tempRelay) {
		printf("Overloading temp relay error!\n");
		stop_Overloading();
	}
}

void control_Conveyor() {
	if(!inProgress[CONVEYOR]) return;
	int temp1 = Get_Signal("wago.oc_temp.pt100_m3");
	int temp2 = Get_Signal("wago.oc_temp.pt100_m4");
	int tempRelay1 = Get_Signal("wago.ts_m1.rele_T_m3");
	int tempRelay2 = Get_Signal("wago.ts_m1.rele_T_m4");

	if(tempRelay1 || tempRelay2) {
		printf("Conveyor temp relay error!\n");
		stop_Conveyor();
	}
}

void control_Stars() {
	if(!inProgress[STARS]) return;
	if(inProgress[OIL] != RUNNING && !debugging) {
		printf("Oil station is not running!");
		stop_Stars();
	}
}

void control_Oil() {
	if(!inProgress[OIL]) return;
	int temp = Get_Signal("wago.oc_temp.pt100_m2");
	int tempRelay = Get_Signal("wago.ts_m1.rele_T_m2");

	if(tempRelay) {
		printf("Oil station temp relay error!\n");
		stop_Oil();
	}
}

void control_Hydratation() {
	if(!inProgress[HYDRATATION]) return;
	int temp = Get_Signal("wago.oc_temp.pt100_m5");
	int tempRelay = Get_Signal("wago.ts_m1.rele_T_m5");

	if(tempRelay) {
		printf("Organ temp relay error!\n");
		stop_Hydratation();
	}
}

void control_Organ() {
	if(!inProgress[ORGAN]) return;
	if(inProgress[HYDRATATION] != RUNNING && !debugging) stop_Organ();
	int temp = Get_Signal("wago.oc_temp.pt100_m1");
	int tempRelay = Get_Signal("wago.ts_m1.rele_T_m1");
	int waterFlow = Get_Signal("485.ad1.adc3.flow");
	//READ_SIGNAL("485.ad1.adc3.flow");

	if(waterFlow) {
	}

	if(tempRelay) {
		printf("Organ temp relay error!\n");
		stop_Organ();
	}
}


void stop_Overloading() {
	//if(!inProgress[OVERLOADING]) return;
	printf("Stopping overloading\n");
	WRITE_SIGNAL("485.rpdu485.kbl.reloader_green", 0);
	WRITE_SIGNAL("485.kb.kbl.start_reloader", 0);
	WRITE_SIGNAL("wago.oc_mdo1.ka5_1", 0);	
	WRITE_SIGNAL("panel10.kb.key.reloader",0);
	inProgress[OVERLOADING] = 0;
}

void stop_Conveyor() {
	//if(!inProgress[CONVEYOR]) return;
	printf("Stopping conveyor\n");
	WRITE_SIGNAL("485.rpdu485.kbl.conveyor_green", 0);
	WRITE_SIGNAL("485.kb.kbl.start_conveyor", 0);
	WRITE_SIGNAL("wago.oc_mdo1.ka3_1", 0);
	WRITE_SIGNAL("panel10.kb.key.conveyor",0);
	inProgress[CONVEYOR] = 0;
}

void stop_Stars() {
	//if(!inProgress[STARS]) return;
	printf("Stopping stars\n");
	WRITE_SIGNAL("485.rpdu485.kbl.loader_green", 0);
	WRITE_SIGNAL("485.kb.kbl.start_stars", 0);
	WRITE_SIGNAL("485.rsrs.rm_u2_on6", 0);
	WRITE_SIGNAL("485.rsrs.rm_u2_on7", 0);

	WRITE_SIGNAL("panel10.kb.key.stars",0);
	inProgress[STARS] = 0;
}

void stop_Oil() {
	if(!debugging) {
		stop_Stars();
	}
	//if(!inProgress[OIL]) return;
	printf("Stopping oil station\n");
	WRITE_SIGNAL("485.rpdu485.kbl.oil_station_green", 0);
	WRITE_SIGNAL("485.kb.kbl.start_oil_station", 0);
	WRITE_SIGNAL("wago.oc_mdo1.ka2_1", 0);

	WRITE_SIGNAL("panel10.kb.key.oil_station",0);
	inProgress[OIL] = 0;
}

void stop_Hydratation() {
	if(!debugging) {
		stop_Organ();
//		stop_Oil();
	}
	//if(!inProgress[HYDRATATION]) return;
	WRITE_SIGNAL("485.kb.kbl.start_hydratation", 0);
	WRITE_SIGNAL("wago.oc_mdo1.ka4_1", 0);
	WRITE_SIGNAL("wago.oc_mdo1.water1", 0);

	WRITE_SIGNAL("panel10.kb.key.hydratation",0);
	inProgress[HYDRATATION] = 0;
}

void stop_Organ() {
	WRITE_SIGNAL("485.rpdu485.kbl.exec_dev_green", 0);
	WRITE_SIGNAL("485.kb.kbl.start_exec_dev", 0);
	WRITE_SIGNAL("wago.oc_mdo1.ka1_1", 0);

	WRITE_SIGNAL("panel10.kb.key.exec_dev",0);
	inProgress[ORGAN] = 0;
}

void control_all() {
	control_Overloading();
	control_Conveyor();
	control_Stars();
	control_Oil();
	control_Hydratation();
	control_Organ();
}

void stop_all() {
	stop_Overloading();
	stop_Conveyor();
	stop_Stars();
	stop_Oil();
	stop_Hydratation();
	stop_Organ();
}
