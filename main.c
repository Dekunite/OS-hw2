/*
<Name Surname: Muhammet Derviş Kopuz>
<Student Number: 504201531>
*/

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

key_t PARENTSEM = 363;
key_t KEYSEM = 364;
key_t KEYSHM = 365;

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


int main(int argc, char* argv[]) {
    FILE *fptr = NULL; 
    FILE *outputPtr = NULL;
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

    char* fname = "input.txt";
    if(argc > 1) {
        fname = argv[1];
    }
    //char fname[20];
	//scanf("%s",fname);	

    fptr = fopen(fname, "r");
    int num;
    //first line
    fscanf(fptr, "%d", &num);
    M = num;
    //second line
    fscanf(fptr, "%d", &num);
    n = num;

    int memorySize = sizeof(M) + sizeof(n) + sizeof(x) + sizeof(y) + (2 * n * sizeof(int));
    int* memoryPtr = NULL;

    //  creating a shared memory area with the defined memory size
    shmid = shmget(KEYSHM, memorySize, IPC_CREAT | 0700);

    //  attaching the shared memory segment identified by shmid
    //to the address space of the calling process(parent)
    memoryPtr = (int *) shmat(shmid, 0, 0);
    *memoryPtr = 0;
    
    //assign n value
    *(memoryPtr) = n; 
    //assign m value
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
    fclose(fptr);

    /*
	printf("\n The content of the file %s  are : \n",fname);    
    for(i = 0; i < n; ++i)
    {
        printf(" %d\n", A[i]);
    }
    */

    //  detaching the shared memory segment from the address
    //space of the calling process(parent)
    shmdt(memoryPtr);

    int result;
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

        shmdt(memoryPtr);

        for (i =0; i<2; i++) {
            sleep(2);
            kill(child[i],12);
        }

        sem_wait(parentSem, 2);

        //print output
        shmid = shmget(KEYSHM, memorySize, 0);
        memoryPtr = (int*) shmat(shmid,0,0);

        const char* outputFileName = "output.txt";
        if(argc > 2) {
        outputFileName = argv[2];
        }
        outputPtr = fopen(outputFileName,"w");

        //print M
        M = *(memoryPtr + sizeof(int));
        fprintf(outputPtr,"%d\n",M);
        //print n
        n = *(memoryPtr);
        fprintf(outputPtr,"%d\n",n);
        //print A
        A = (memoryPtr + (4 * sizeof(int)));
        int i = 0;
        for (i = 0; i<n; i++) {
            if(i == n-1) {
                fprintf(outputPtr,"%d\n",A[i]);
                break;
            }
            fprintf(outputPtr,"%d ",A[i]);
        }
        //print x
        x = *(memoryPtr + (2*sizeof(int)));
        fprintf(outputPtr,"%d\n",x);
        //print B
        B = (memoryPtr + (4*sizeof(int) + (n*sizeof(int))));
        for (i = 0; i<x; i++) {
            if(i == x-1) {
                fprintf(outputPtr,"%d\n",B[i]);
                break;
            }
            fprintf(outputPtr,"%d ",B[i]);
        }
        //print y
        y = *(memoryPtr + (3*sizeof(int)));
        fprintf(outputPtr,"%d\n",y);
        //print C
        int* C = (memoryPtr + (4*sizeof(int)) + (n*sizeof(int)) + (x*sizeof(int)) );
        for (i = 0; i<y; i++) {
            if(i == y-1) {
                fprintf(outputPtr,"%d\n",C[i]);
                break;
            }
            fprintf(outputPtr,"%d ",C[i]);
        }
        fclose(outputPtr);

        
        //print M
        M = *(memoryPtr + sizeof(int));
        printf("M: %d\n",M);
        //print n
        n = *(memoryPtr);
        printf("n: %d\n",n);
        //print A
        A = (memoryPtr + (4 * sizeof(int)));
        i = 0;
        printf("A: ");
        for (i = 0; i<n; i++) {
            if(i == n-1) {
                printf("%d\n",A[i]);
                break;
            }
            printf("%d ",A[i]);
        }
        //print x
        x = *(memoryPtr + (2*sizeof(int)));
        printf("x: %d\n",x);
        //print B
        B = (memoryPtr + (4*sizeof(int) + (n*sizeof(int))));
        printf("B: ");
        for (i = 0; i<x; i++) {
            if(i == x-1) {
                printf("%d\n",B[i]);
                break;
            }
            printf("%d ",B[i]);
        }
        //print y
        y = *(memoryPtr + (3*sizeof(int)));
        printf("y: %d\n",y);
        //print C
        C = (memoryPtr + (4*sizeof(int)) + (n*sizeof(int)) + (x*sizeof(int)) );
        printf("C: ");
        for (i = 0; i<y; i++) {
            if(i == y-1) {
                printf("%d\n",C[i]);
                break;
            }
            printf("%d ",C[i]);
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
        pause();

        if (myOrder == 0) {
            //child process 1
            printf("process 1 starting \n");
            syncSem = semget(KEYSEM, 1, 0);
            parentSem = semget(PARENTSEM, 1, 0);
            shmid = shmget(KEYSHM, memorySize, 0);
            memoryPtr = (int*) shmat(shmid, 0, 0);
            
            //child 1
            int xCounter = 0;
            int l;
            n = *(memoryPtr);
            M = *(memoryPtr + sizeof(int));
            for (l = 0 ; l<n; l++) {
                if (A[l] <= M) {
                    xCounter++;
                }
            }
            //write x value
            *xPtr = xCounter;
            
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
                    bCounter++;
                }
            }

            shmdt(memoryPtr);

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
            l=0;
            for (l = 0 ; l<n; l++) {
                if (A[l] > M) {
                    C[cCounter] = A[l];
                    cCounter++;
                }
            }

            sem_signal(parentSem, 1);
            shmdt(memoryPtr);
        }

        exit(0);
    }

    return 0;
}
