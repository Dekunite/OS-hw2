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

key_t PARENTSEM = 363;
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
    int parentSem = 0;
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
    //int* memoryPtr = (int*) malloc(memorySize);
    int* memoryPtr = NULL;
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
    int k;
    for (k = 0; k<n; k++) {
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
    for (i = 0; i < 2; i++)
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

        //parent sem
        parentSem = semget(PARENTSEM, 1, 0700|IPC_CREAT);
        semctl(parentSem, 0, SETVAL, 0);

        int* B = &A[n+1];
        //printf("B value in parent: %d\n", B[0]);
        //printf("x address in parent: %p\n", xPtr);
        //printf("x value in parent: %d\n", *xPtr);

        //TO DO MAin process child dan sonra z i printleyebilsin
        //printf("A: %p\n", A);
        //printf("asdasdxzczxczcyeeeeeeeeeeeeeeeet");
        //printf("x value: %d \n", *(memoryPtr+8));

        shmdt(memoryPtr);

        //start child 1
        //kill(child[0],12);
        //start child 2
        //kill(child[1],12);

        for (i =0; i<2; i++) {
            sleep(2);
            kill(child[i],12);
        }

        sem_wait(parentSem, 2);
        //sleep(2);
        printf("geri dmdms\n");

        //print outputt
        shmid = shmget(KEYSHM, memorySize, 0);
        memoryPtr = (int*) shmat(shmid,0,0);
        //print M
        M = *(memoryPtr + sizeof(int));
        printf("%d\n",M);
        //print n
        n = *(memoryPtr);
        printf("%d\n",n);
        //print A
        A = (memoryPtr + (4 * sizeof(int)));
        int i = 0;
        for (i = 0; i<n; i++) {
            printf("%d ",A[i]);
        }
        printf("\n");
        //print x
        x = *(memoryPtr + (2*sizeof(int)));
        printf("X = %d\n",x);
        //print B
        B = (memoryPtr + (4*sizeof(int) + (n*sizeof(int))));
        for (i = 0; i<x; i++) {
            printf("%d\n",B[i]);
        }
        //print y
        y = *(memoryPtr + (3*sizeof(int)));
        printf("%d\n",y);
        //print C
        int* C = (memoryPtr + (4*sizeof(int)) + (n*sizeof(int)) + (x*sizeof(int)) );
        for (i = 0; i<y; i++) {
            printf("%d\n",C[i]);
        }

        //remove semaphores and created memory
        semctl(syncSem,0,IPC_RMID,0);
        semctl(parentSem,0,IPC_RMID,0);
        shmctl(shmid,IPC_RMID,0);

        exit(0);
        
    }
    //childs
    else {
        myOrder = i;
        printf("myorder: %d \n", myOrder);
        pause();
        printf("myorder: %d is starting \n", myOrder);

        if (myOrder == 0) {
            //child process 1
            printf("process 1 starting \n");
            syncSem = semget(KEYSEM, 1, 0);
            parentSem = semget(PARENTSEM, 1, 0);
            shmid = shmget(KEYSHM, memorySize, 0);
            memoryPtr = (int*) shmat(shmid, 0, 0);
            
            //printf("child M: %d\n", *(memoryPtr+4));
            //printf("child n: %d\n", *(memoryPtr));
            //printf("child num: %d\n", num);
            //printf("A: %p\n", A);

            //child 1
            int xCounter = 0;
            int l;
            n = *(memoryPtr);
            M = *(memoryPtr + sizeof(int));
            printf("n = %d", n);
            printf("M = %d", M);
            for (l = 0 ; l<n; l++) {
                if (A[l] <= M) {
                    xCounter++;
                }
            }
            //write x value
            *xPtr = xCounter;
            printf("xcounter = %d", xCounter);
            //printf("x address in child: %p\n", xPtr);
            printf("x value in child: %d\n", *xPtr);
            
            //increase sem by 1 so child 2 can start
            sem_signal(syncSem,1);
            sem_signal(parentSem,1);

            //B start address
            int* B = (memoryPtr + (4*sizeof(int) + (n * sizeof(int))));

            //copy into B
            int bCounter = 0;
            for (l = 0 ; l<n; l++) {
                if (A[l] <= M) {
                    B[bCounter] = A[l];
                    printf("B, bCounter: %d \n", B[bCounter] );
                    bCounter++;
                }
            }

            shmdt(memoryPtr);
            //semctl(syncSem,0,IPC_RMID,0);
            //semctl(parentSem,0,IPC_RMID,0);
            //sem_signal(syncSem, 1);

        } else {
            //child process 2
            printf("process 2 starting \n");
            syncSem = semget(KEYSEM, 1, 0);
            parentSem = semget(PARENTSEM, 1, 0);
            shmid = shmget(KEYSHM, memorySize, 0);
            memoryPtr = (int*) shmat(shmid, 0, 0);

            sem_wait(syncSem, 1);

            //child 1
            int yCounter = 0;
            int l;
            for (l = 0 ; l<n; l++) {
                if (A[l] > M) {
                    yCounter++;
                }
            }
            //write x value
            *yPtr = yCounter;

            //C start address
            x = *(memoryPtr + (2*sizeof(int)));
            n = *(memoryPtr);
            int* C = (memoryPtr + (4*sizeof(int)) + (n*sizeof(int)) + (x*sizeof(int)) );

            //copy into C
            int cCounter = 0;
            M = *(memoryPtr + sizeof(int));
            A = (memoryPtr + (4 * sizeof(int)));
            printf("n = %d", n);
            printf("M = %d", M);
            printf("C, cCounter: %d \n", C[cCounter] );
            l=0;
            for (l = 0 ; l<n; l++) {
                if (A[l] > M) {
                    C[cCounter] = A[l];
                    printf("C, cCounter: %d \n", C[cCounter] );
                    cCounter++;
                }
            }
            
            

            //sem_signal(syncSem, 1);
            sem_signal(parentSem, 1);
            //semctl(syncSem,0,IPC_RMID,0);
            //semctl(parentSem,0,IPC_RMID,0);
            shmdt(memoryPtr);
        }


        exit(0);
    }




    return 0;
}
