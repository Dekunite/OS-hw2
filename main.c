#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

//  for shared memory and semaphores
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>

//  for handling signals
#include <signal.h>

#include <sys/stat.h> 

#define LSIZ 128 
#define RSIZ 10 

key_t KEYSEM = 364;
key_t KEYSHM = 365;
void initKeys(char* argv[])
{
    char cwd[256];
    char* keyString;
    
    //  get current working directory
    getcwd(cwd, 256);
    
    //  form keystring
    keyString = malloc(strlen(cwd) + strlen(argv[0]) + 1);
    strcpy(keyString, cwd);
    strcat(keyString, argv[0]);
    
    KEYSEM = ftok(keyString, 1);
    KEYSHM = ftok(keyString, 2);
    
    //  deallocate keystring
    free(keyString);
    
    //  if you use get_current_dir_name, result of that call must be deallocated by caller
    //because it creates its own buffer to store path.
    //but in this example we use getcwd and our own buffer.
}

void sem_signal(int semid, int val)
{
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = val;  //  relative:  add sem_op to value
    semaphore.sem_flg = 0;   
    semop(semid, &semaphore, 1);
}


//  decrement operation
void sem_wait(int semid, int val)
{
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = (-1*val);  //  relative:  add sem_op to value
    semaphore.sem_flg = 0;  
    semop(semid, &semaphore, 1);
}


//  signal handling function
void mysignal(int signum)
{
    printf("Received signal with num=%d\n", signum);
}

void mysigset(int num)
{
    struct sigaction mysigaction;
    mysigaction.sa_handler = (void*) mysignal;
    
    //  using the signal-catching function identified by sa_handler
    mysigaction.sa_flags = 0;

    //  sigaction() system call is used to change the action taken by
    //a process on receipt of a specific signal(specified with num);
    sigaction(num, &mysigaction, NULL);
}


int main(int argc, char *argv[]) {
    char line[RSIZ][LSIZ];
	
    FILE *fptr = NULL; 
    int shmid = 0;
    int i = 0;
    int M ;
    int n;
    int x;
    int y;
    int child[2];  //  child process ids
    //semaphore for sync
    int syncSem = 0; 
    mysigset(12);
    int myOrder;

    //initKeys(argv);

    printf("keyshm: %d", KEYSHM);

    printf("\n\n Read the file and store the lines into an array :\n");
	printf("---------------------------------------------------\n"); 
    char fname[20] = "test.txt";
    //char fname[20];
	//scanf("%s",fname);	
    int counter = 0;

    fptr = fopen(fname, "r");
    int num;
    //first line
    fscanf(fptr, "%d", &num);
    //*(memoryPtr + sizeof(n)) = num;
    //M = *(memoryPtr + sizeof(n));
    M = num;
    //second line
    fscanf(fptr, "%d", &num);
    //*(memoryPtr) = num;
    //n = *(memoryPtr);
    n = num;

    int memorySize = sizeof(M) + sizeof(n) + sizeof(x) + sizeof(y) + (2 * n * sizeof(int));
    int* memoryPtr = (int*) malloc(memorySize);
    //int* memoryPtr = NULL;
    printf("memory size: %d", memorySize);

    //  creating a shared memory area with the size of an int
    shmid = shmget(KEYSHM, memorySize, IPC_CREAT | 0666);
    printf("memory size: %d", shmid);

    //  attaching the shared memory segment identified by shmid
    //to the address space of the calling process(parent)
    memoryPtr = (int *) shmat(shmid, 0, 0);
    *memoryPtr = 0;
    
    //n değerini ata
    *(memoryPtr) = n; 
    //m değerini ata
    *(memoryPtr + sizeof(n)) = M;

    //init x pointer
    int* xPtr = (memoryPtr + (2* sizeof(int)));
    //init y pointer
    int* yPtr = (memoryPtr + (3* sizeof(int)));

    int* A = (memoryPtr + (4 * sizeof(int)));
    //third line & array
    for (int k = 0; k<n; k++) {
        fscanf(fptr, "%d", &num);
        A[k] = num;
    }

	printf("\n The content of the file %s  are : \n",fname);    
    for(i = 0; i < n; ++i)
    {
        printf(" %d\n", A[i]);
    }
    printf("M: %d \n",M);
    printf("n: %d \n",n);

    //  detaching the shared memory segment from the address
    //space of the calling process(parent)
    shmdt(memoryPtr);

    


    // nullamayı unutma
    int result = 1;
    //  create 2 child processes
    for (i = 0; i < 2; ++i)
    {
        result = fork();
        if (result < 0)
        {
            printf("FORK error...\n");
            exit(1);
        }
        if (result == 0)
            break;
        child[i] = result;
    }

    //parent
    if (result != 0) {
        shmid = shmget(KEYSHM, memorySize, 0);
        memoryPtr = (int*) shmat(shmid,0,0);

        //sync
        syncSem = semget(KEYSEM, 1, 0700|IPC_CREAT);
        semctl(syncSem, 0, SETVAL, 0);

        int* B = &A[n+1];
        printf("B value in parent: %d\n", B[0]);
        printf("x address in parent: %p\n", xPtr);
        printf("x value in parent: %d\n", *xPtr);
        while (*xPtr == -1) {
            printf("x value in parent: %d\n", *xPtr);
        }

        //TO DO MAin process child dan sonra z i printleyebilsin
        printf("A: %p\n", A);
        printf("x value: %d", *(memoryPtr+8));

        shmdt(memoryPtr);

        printf("start child processes 1 \n");
        //start child 1
        kill(child[0],12);

        sem_wait(syncSem, 1);
        //start child 2
        kill(child[1],12);

        sem_wait(syncSem, 2);

        //remove semaphores and created memory
        semctl(syncSem,0,IPC_RMID,0);
        shmctl(shmid,IPC_RMID,0);

        exit(0);
        
    }
    //childs
    else {
        myOrder = i;
        printf("myorder: %d", myOrder);
        pause();

        if (myOrder == 0) {
            //child process 1
            syncSem = semget(KEYSEM, 1, 0);
            shmid = shmget(KEYSHM, memorySize, 0);
            memoryPtr = (int*) shmat(shmid, 0, 0);
            
            printf("child M: %d\n", *(memoryPtr+4));
            printf("child n: %d\n", *(memoryPtr));
            printf("child num: %d\n", num);
            printf("A: %p\n", A);

            //child 1
            int xCounter = 0;
            for (int l = 0 ; l<n; l++) {
                if (A[l] <= M) {
                    xCounter++;
                }
            }
            //write x value
            *xPtr = xCounter;
            printf("x address in child: %p\n", xPtr);
            printf("x value in child: %d\n", *xPtr);
            
            //increase sem by 1 so child 2 can start
            sem_signal(syncSem,1);

            //B start address
            int* B = &A[n+1];

            //copy into B
            int bCounter = 0;
            for (int l = 0 ; l<n; l++) {
                if (A[l] <= M) {
                    B[bCounter] = A[l];
                    bCounter++;
                }
            }

            shmdt(memoryPtr);
            //sem_signal(syncSem, 1);

        } else if (myOrder == 1) {
            //child process 2
            syncSem = semget(KEYSEM, 1, 0);
            shmid = shmget(KEYSHM, memorySize, 0);
            memoryPtr = (int*) shmat(shmid, 0, 0);
            
            printf("child M: %d\n", *(memoryPtr+4));
            printf("child n: %d\n", *(memoryPtr));
            printf("child num: %d\n", num);
            printf("A: %p\n", A);

            //child 1
            int yCounter = 0;
            for (int l = 0 ; l<n; l++) {
                if (A[l] > M) {
                    yCounter++;
                }
            }
            //write x value
            *yPtr = yCounter;
            printf("y address in child: %p\n", yPtr);
            printf("y value in child: %d\n", *yPtr);

            //C start address
            int* C = &A[n+x];

            //copy into B
            int cCounter = 0;
            for (int l = 0 ; l<n; l++) {
                if (A[l] > M) {
                    C[cCounter] = A[l];
                    cCounter++;
                }
            }

            shmdt(memoryPtr);
            sem_signal(syncSem, 1);
        }


        exit(0);
    }


    return 0;
}
