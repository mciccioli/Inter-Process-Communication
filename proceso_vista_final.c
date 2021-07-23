#include "application.h"

int main(int argc, char *argv[])
{


    //creating semaphores, both initialized in 0.
    // If O_CREAT is specified, and a semaphore with the given name already exists, then mode and value are ignored.
    //Therefore, assuming that App initialized first, mode and value are ignored.
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

  


    char shm_name[12] = {0};
    char *string;
    // ./vista <shm_name>
    if (argc > 1)
    {
        strcpy(shm_name, argv[1]);
    }
    else
    {

        read(STDIN, shm_name, 6);
        shm_name[5] = '\0';

    }


    //if application had been run before, I will get the already open shared memory
    // O_RDWR: Open the object for read-write access. (it was created in the app file - we don't wan to create it again)
    int fd_shm = shm_open(shm_name, O_RDWR, 0666);


    if (fd_shm == -1)
    {
        perror("vista shm");
        exit(2);
    }


    //point to the shared memory space
    void *map_ptr = mmap(0, SIZEOFSHMSEG, PROT_WRITE | PROT_READ, MAP_SHARED, fd_shm, 0);

    if (map_ptr == (void *)-1)
    {
        perror("mmap vista");
        exit(2);
    }

    //Deposit in the shared memory space the "handshake_a"
    strcpy((char *)map_ptr, handshake_a);





    /*  EXAMPLE: learned in class  
    
    //ATOMIC
    void sem_wait(int* sem){
        sleep_until_sem_greater_than_zero(sem);
        sem--;
    }

     */
    

    //wont be able to execute below this line until they make a sem_post(sem_read) on the App process.
    //Therefore, the code below this line wont be reached unless both App and Vista are connected to the Shared Memory
    sem_wait(sem_read);


   
    do
    {
        string = (char *)map_ptr;
        printf("%s\n", string); // i
        sem_post(sem_write);
        sem_wait(sem_read); // vista espera a que el proceso app le de el visto bueno a la lectura de datos
    } while (*string != EOF);

    sem_post(sem_write);
    return 0;
}


