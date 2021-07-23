#include <stdio.h> 
#include <stdlib.h> 
#include <sys/wait.h> 
#include <unistd.h> 
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/select.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/stat.h>       
#include <fcntl.h>  
#include <time.h>



#define N_SLAVES 5 /* Amount of slaves */
#define FILES_PER_SLAVE 2  /* Initial amount of files given to each Slave */

#define STDIN 0
#define STDOUT 1

#define READ_END_PIPE		0
#define WRITE_END_PIPE		1


#define SIZEOFSHMSEG 512      /* Size of the shared mem segment    */


#define LINE_SIZE 256

#define BUFFER_SIZE 300



const char *semWriteName = "/writeteam5"; /* name for our Semaphore */

const char *semReadName = "/readteam5"; /* name for our Semaphore */

const char *pattern = "SATISFIABLE";

#define PATT_LENGTH 11

const char *handshake_a = "LaGallinaCloquea"; /* Handshake Verification String */

