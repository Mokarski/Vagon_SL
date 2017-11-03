#ifndef JOURNAL_H_
#define JOURNAL_H_


#define _FILE_SIZE  10000 //limit to file size in bytes
//#define ALL_IN_ONE  //write ALL in one file
//#define ROTATION  //rotate log file

//code states
#define NORMAL   0 //normal level
#define ALARM    1 //alarm level
#define WARNING  2 //warning level
#define CRITICAL 3 //critical level

long MAX_FILE_SIZE = _FILE_SIZE;
int write_journal (int state_code, int err_code); // write to file cyrrent system state and error code

#endif
