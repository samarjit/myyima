/*******************************************************************************

                          Yima Personal Edition v1.0
                               January 10, 2003

                            Streaming Media System
                        http://streamingmedia.usc.edu

                        Department of Computer Science
                        Integrated Media Systems Center
                       University of Southern California
                             Los Angeles, CA 90089

    Developed by:

    Roger Zimmermann
    Cyrus Shahabi
    Kun Fu
    Sahitya Gupta
    Mehrdad Jahangiri
    Farnoush Banaei-Kashani
    Hong Zhu


Copyright 2003, University of Southern California. All Rights Reserved.

"This software is experimental in nature and is provided on an AS-IS basis only.
The University SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS AND IMPLIED,
INCLUDING WITHOUT LIMITATION ANY WARRANTY AS TO MERCHANTIBILITY OR FITNESS FOR
A PARTICULAR PURPOSE.

This software may be reproduced and used for non-commercial purposes only,
so long as this copyright notice is reproduced with each such copy made."

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/time.h>


extern int SCHEDULER_main(void);
extern int RTSPS_main(void);

/*###########################*/
/*# Main program            #*/
/*###########################*/
int main(void)
{
  pid_t rtsps_pid;
  pid_t sched_pid;
  int stat;

  /* start scheduler process */
  if((sched_pid = fork()) < 0)
      printf("<FATAL Error>: creation scheduler process\n");
  else if( sched_pid == 0)  /* child process */
  {
	 printf("<YimaPE 1.1> begin scheduler \n");
         SCHEDULER_main();
         printf("<YimaPE 1.1> end scheduler \n");
         exit(0);
  }

  /* start rtsps process */
  if((rtsps_pid = fork()) < 0)
      printf("<FATAL Error>: creation rtsps process\n");
  else if( rtsps_pid == 0)  /* child process */
  {
         sleep(2);
	 printf("<YimaPE 1.1> begin rtsps\n");
         RTSPS_main();
         printf("<YimaPE 1.1> end rtsps\n");
         exit(0);
  }
 
  /* waiting for the end of rtsps process */
  if (rtsps_pid > 0)
          waitpid(rtsps_pid, &stat, 0); 

  /* waiting for the end of s process */
  if (sched_pid > 0)
          waitpid(sched_pid, &stat, 0); 

  printf("end of Main!!\n");
}
