#include <stdio.h>
#inlude  <time.h>
#include "journal.h"

long get_file_size(){
FILE*  mf = fopen("journal.txt","r");  
if (mf == NULL)
   {
	  printf ("ERR: can`t calculate journal.txt file szie! \n");
   } else {
						fseek(mf,0,SEEK_END);  
						long size = ftell(mf);  
					}
fclose(mf);
return size;
}

int write_journal (int state_code, int err_code){
    FILE *mf;
		int flag=0;
		long int s_time;
		struct tm *m_time;
		char str_t[128]=””;
		s_time = time (NULL);
		m_time = localtime (&s_time);
		strftime (str_t, 128, ”%x | %X”, m_time);
    long file_size_now;

		file_size_now = get_file_size();
		if (file_size_now > MAX_FILE_SIZE)
		    {
				  mf=fopen ("journal.txt","w"); //rewrite file
				} else {								
				         mf=fopen ("journal.txt","a"); //append to file
								}
   
    if (mf == NULL) 
		    {
				  printf ("ERROR open jornal.txt file!\n");
					flag =-1;
					}   else    {  
							         
							   fprintf (mf,"Date: %s; State: %d; Err: %d;\n",str_t, state_code, err_code);
							   flag=1;
						    }			
   fclose (mf);   
return flag;
}

