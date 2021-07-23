#include "application.h"

void computeLPSArray(int *lps)
{
  int len = 0, i = 1;
  lps[0] = 0;
  while (i < PATT_LENGTH)
  {
    if (pattern[i] == pattern[len])
    {
      len++;
      lps[i] = len;
      i++;
    }
    else
    {
      if (len != 0)
      {
        len = lps[len - 1];
      }
      else
      {
        lps[i] = 0;
        i++;
      }
    }
  }
}

int howManyFilesReceived(char *slaveOutput, int *lps)
{
  //https://www.geeksforgeeks.org/kmp-algorithm-for-pattern-searching/
  int finds = 0, i = 0, j = 0;
  while (slaveOutput[i] != 0)
  {
    if (pattern[j] == slaveOutput[i])
    {
      j++;
      i++;
    }
    if (j == PATT_LENGTH)
    {
      j = lps[j - 1];
      finds++;
    }
    else if (slaveOutput[i] != 0 && pattern[j] != slaveOutput[i])
    {
      if (j != 0)
        j = lps[j - 1];
      else
        i++;
    }
  }

  return finds;
}

int main(int argc, char *argv[])
{
   //Verify that at least 1 file has been passed as an argument
  if( argc <= 1 ){
      perror("This program needs to receive .cnf files to work\n");
      exit(1);
  }


  char shm_name[12] = {0}; //will be used to allocate the name of the shared memory
  
  sprintf(shm_name, "/team"); //name of the shared memory inserted into array


 
  //create shared memory 
  // - shm_open()  creates  and opens a new, or opens an existing, POSIX shared memory object
  //  On success, shm_open() returns a nonnegative file descriptor.  On failure, shm_open() returns -1.

  //O_CREAT    Create  the  shared  memory  object  if it does not exist.
  //O_RDWR     Open the object for read-write access.
  int fd_shm = shm_open(shm_name, O_CREAT | O_RDWR, 0666);

  //validate if there was an error creating the shared mem
  if(fd_shm == -1){
    perror("Could not successfully create Shared mem\n");
    exit(1);
  }



  //A new shared memory object initially has zero length--the size of the object can be set using ftruncate.  The newly allocated bytes of a shared memory object are automatically initialized to 0.
  //ftruncate - truncate a file to a specified length
  //On success, zero is returned.  On error, -1 is returned
  if ( ftruncate(fd_shm, SIZEOFSHMSEG) == -1 ){
    perror("Could not truncate the Shared Mem to a specified length\n");
    exit(1);
  }


  //mmap - map files or devices into memory

  //  void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
  //  mmap() creates a new mapping in the virtual address space of the calling process.
  // The starting address for the new mapping is specified in addr. |||  If  addr  is  NULL, then the kernel chooses the address at which to create the mapping
  // The length argument specifies the length of the mapping.
  // The prot argument describes the desired memory protection of the mapping (and must not conflict with the open mode of the file)
  // The  flags  argument  determines whether updates to the mapping are visible to other processes mapping the same region 

  //On success, mmap() returns a pointer to the mapped area.  On error, the value MAP_FAILED is returned.
  void *map_ptr = mmap(0, SIZEOFSHMSEG, PROT_WRITE | PROT_READ, MAP_SHARED, fd_shm, 0);

  //validate the mapping of shared memory
  if(  map_ptr == MAP_FAILED){
    perror("Could not map files or devices into memory\n");
    exit(1);
  }

  

  //creating semaphores
  // sem_open - initialize and open a named semaphore
  // On success, sem_open() returns the address of the new semaphore; this address is used when calling other semaphore-related functions.  
  // On error, sem_open() returns SEM_FAILED, with errno set to indicate the error.
  sem_t *sem_write = sem_open(semWriteName, O_CREAT, 0666, 0);
  sem_t *sem_read = sem_open(semReadName, O_CREAT, 0666, 0);


  //check if the sempahores initialized correctly
  if (sem_write == SEM_FAILED)
  {
    perror("sem_open write app\n");
    exit(1);
  }
  if (sem_read == SEM_FAILED)
  {
    perror("sem_open read app\n");
    exit(1);
  }

  

  printf("%s\n", shm_name); //send to STDOUT the name of the shared memory
  fflush(stdout); //writes all buffered data 
  sleep(2); //It should wait 2 segs for a Vista Process to appear. 

    

  int sharing = 0; //flag that determines if both the App and Vista Process have accessed the shared memory space
  char *handshake_test; //will be used to compare the strings of the handshake test

  

  handshake_test = (char *)map_ptr; //also points to the start of Shared Mem

  //carry out a handshake test between the App Process and the Vista Process
  //checking if the Application is connected with the Vista through the shared memory
  //if both processes are not connected, the Application process will run without semaphores
  if (strcmp(handshake_test, handshake_a) == 0)
  { 
    sharing = 1; //are successfully connected to the shared memory, we will use semaphores
  }


  //if App and Vista are both connected to the Shared Memory
  if (sharing)
  {
    //sem_read++;
    //This will unlock certain part of the code in Vista
    sem_post(sem_read);
  }



  int lps[PATT_LENGTH];
  computeLPSArray(lps);

  int files_sent = 0;         // Amount of files sent to slaves.
  int files_received = 0;     //Amount of files received from Slaves.
  int total_files = argc - 1; // Amount of CNF Files (argv's first argument is the name of the program running)

  
  pid_t pid[N_SLAVES]; //Array of pid's
  int i, j; //Will be used for the for cycles

  fd_set readfds; //Struct to use the select() function
  int pipes_ready; //How many pipes are ready to be read 
  char eof = EOF; //EndOfFile 
  int index; //index for the arguments received. (we want to skip argv[0], which is the name of the program)



  //Creation of the file where we will save the results coming from slaves
  //open() returns the new file descriptor, or -1 if an error occurred
  //O_WRONLY requests opening the file write-only
  //O_CREAT  If the file does not exist, it will be created.
  //O_TRUNC  If the file already exists and is a regular file and the access mode allows writing it will be truncated to length 0.
  int output = open("results.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU); //S_IRWXU  00700 user (file owner) has read, write and execute permission


  //Verify that the file was created successfully
  if(  output == -1 ){
    perror("Failed while creating the results.txt file\n");
    exit(1);
  }



 //Do the pipe configurations for the communication between the App and Slave processes:
  //For each pair Parent-Child (App-Slave) one pair of pipes is created (because pipes are unidirectional)
  int pipe_application_slave[N_SLAVES][2]; //This is the pipe by which the Application process will send the file to the Slave process
  int pipe_slave_application[N_SLAVES][2]; //This is the pipe by which the Slave process will send the results back to the Application process
  



  /*  PIPE() FUNCTION:

   int pipe(int pipefd[2]);
  
    pipe() a unidirectional data channel that can be used for interprocess communication. 
    The array pipefd is used to return two file descriptors referring to the ends of the pipe.  
    pipefd[0] refers to the read end of the pipe.
    pipefd[1] refers to the write end of the pipe.  
    Data written to the write end of the pipe is buffered by the kernel until it is read from the read end of the pipe

    On success, zero is returned.  On error, -1 is returned, and errno is set appropriately.
  */

for (i = 0; i < N_SLAVES; i++)
  {
     //creating all the pipes while checking for errors  
    if ( pipe( pipe_application_slave[i] ) == -1 || pipe(pipe_slave_application[i]) == -1 ){
      perror("Error while creating pipes.\n");
      exit(1);
    }
  }



    /* Creation of Slaves */

 for (i = 0; i < N_SLAVES; i++)
  {

    //creatting a new process by duplicating the calling process. The new process, referred to as the child, is an exact duplicate of the calling process, but will have a different PID
    //fork(): On success, the PID of the child process is returned in the parent, and 0 is returned in the child.  
    //On failure, -1 is returned in the parent, no child process is created, and errno is set appropriately.
    pid[i] = fork(); //create a child process
    if (pid[i] < 0){
      perror("Error while usinf the fork() function.\n");
      exit(1);
    }else if (pid[i] == 0){
      //child (i.e the Slave!)

      //close fd, because it will be reused 
      close(STDIN);

      // duplicate a file descriptor, STDIN will now be a replicate of pipe_application_slave[i][0]
      dup2(pipe_application_slave[i][READ_END_PIPE], STDIN); //Now whem the Slave Process reads from STDIN, it will read from the pipe 


      //close fd, because it will be reused
      close(STDOUT);

      // duplicate a file descriptor, STDOUT will now be a replicate of pipe_application_slave[i][1]
      dup2(pipe_slave_application[i][WRITE_END_PIPE], STDOUT); //Now when the Slave Process Writes in STDOUT, instead of going to screen, it will go to the pipe 


      char *args[] = {NULL};

      //replace the current process image with a new process image. The file to be executed is "Slave"
      //the new process image will have the same PID 
      execv("slave", args); 

      // The execv() returns only if an error has occurred.  
      perror("Error while executing execv().\n");
      exit(1);
    }else{
      //parent (i.e the Application!)
    }

  }




  index = 1;

  //initial distribution of files to the Slaves
  for (i = 0; i < N_SLAVES; i++)
  {
    for (j = 0; j < FILES_PER_SLAVE && index < argc; j++)
    {
      write(pipe_application_slave[i][WRITE_END_PIPE], argv[index], strlen(argv[index])); //send the relative path of the file
      write(pipe_application_slave[i][WRITE_END_PIPE], "\n", 1);                          //a "\n" is sent because Slave is using getline() 
      index++;
      files_sent++;
    }
  }

  

  //Trato de implementar select para la lectura de los resultados y enviar nuevos archivos a procesar si es que hace falta

  

  j = 0;


  while (files_received < total_files)
  {

    //los hace aca, cada vez que comienza un loop
    FD_ZERO(&readfds);
    for (i = 0; i < N_SLAVES; i++)
    {
      FD_SET(pipe_slave_application[i][READ_END_PIPE], &readfds);
    }

    //Esperaremos hasta que por lo menos un pipe tenga informacion disponible para leer.
    pipes_ready = select(FD_SETSIZE, &readfds, NULL, NULL, NULL); //FD_SETSIZE = 1024 para asegurarnos que nuestro fd este dentro del rango

    if (pipes_ready == -1)
    {
      perror("Error in select():\n");
      exit(1);
    }

    //set = 0; && !set
    for (i = 0; i < N_SLAVES ; i++)
    {
      if (FD_ISSET(pipe_slave_application[i][READ_END_PIPE], &readfds))
      {

        char line[1024] = {0};
        ssize_t length;
        length = read(pipe_slave_application[i][READ_END_PIPE], line, 1024);
        if (length < 0)
        {
          perror("Error en la lectura del esclavo\n");
          return 1;
        }


        if (sharing)
        {
          sem_wait(sem_write); // espero el permiso a escribir en el shm

          strcpy((char *)map_ptr, line); // imprime esto en memoria compartida

          sem_post(sem_read); // le mando el visto bueno de lectura al proceso vista
        }


        write(output, line, strlen(line));

        write(output, "\n", 1);

        files_received += howManyFilesReceived(line, lps);

        //Chequeo si aun quedan archivos por procesar y se lo mando al slave que se acaba de desocupar
        if (files_sent < total_files)
        {
          write(pipe_application_slave[i][WRITE_END_PIPE], argv[index], strlen(argv[index]));
          write(pipe_application_slave[i][WRITE_END_PIPE], "\n", 1);
          index++;
        }

        files_sent++;
        //set = 1;
      }
    }

    //ahora si puedo cerrar el stream (por que fclose cierra los underlying file descriptors )
    if (files_received == total_files && sharing)
    {
      sem_wait(sem_write);
      *((char *)map_ptr) = eof;
      sem_post(sem_read);
    }
    //printf("f_r:%d t_f:%d\n", files_received, total_files);  for checking files-recieved vs. total-files
  }

  //Le aviso a los esclavos que terminen(cuando el esclavo lea el caracter ¡ sabe que tiene que terminar su ejecución)
  //Mando el caracter de salida y llamo a una nueva línea para que sea tomado por getline en el esclavo y este salga del while
  for (i = 0; i < N_SLAVES; i++)
  {
    write(pipe_application_slave[i][WRITE_END_PIPE], "e", 1);
    write(pipe_application_slave[i][WRITE_END_PIPE], "\n", 1);
  }

  //Espero a que todos los esclavos finalizen
  for (i = 0; i < N_SLAVES; i++)
  {
    waitpid(pid[i], NULL, 0);
  }

  close(output); //cierro el archivo creado

  //Cierro todos los pipes
  for (i = 0; i < N_SLAVES; i++)
  {
    close(pipe_application_slave[i][READ_END_PIPE]);
    close(pipe_application_slave[i][WRITE_END_PIPE]);
    close(pipe_slave_application[i][READ_END_PIPE]);
    close(pipe_slave_application[i][WRITE_END_PIPE]);
  }
  if(sharing){
    sem_wait(sem_write);
  }
  munmap(map_ptr, SIZEOFSHMSEG); //borra el mapeo del segmento de memoria compartida

  shm_unlink(shm_name); //dealocamos y destruimos todo en el shm

  close(fd_shm); //cerramos el file descp abierto al hacer shm

  sem_close(sem_read); //hacer cuando el proceso ya no usa el semaphororetorna -1 si error
  sem_close(sem_write);

  sem_unlink(semWriteName); //se remueve y destruye el semaforo, es lo ultimo que se debe hacer
  sem_unlink(semReadName);

  return 0;
}
