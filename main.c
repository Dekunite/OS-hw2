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

#define LSIZ 128 
#define RSIZ 10 

int KEYSEM;
int KEYSHM = 156;
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


int main(int argc, char *argv[]) {
    char line[RSIZ][LSIZ];
	
    FILE *fptr = NULL; 
    int shmid = 0;
    int i = 0;
    int M ;
    int n;
    int x;
    int y;
    
    //int* memoryPtr = NULL;
    int child[2];  //  child process ids

    initKeys(argv);

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
    printf("memory size: %d", memorySize);

    //  creating a shared memory area with the size of an int
    //shmid = shmget(KEYSHM, memorySize, 0700|IPC_CREAT);

    //  attaching the shared memory segment identified by shmid
    //to the address space of the calling process(parent)
    //memoryPtr = (int*)shmat(shmid, 0, 0);

    //  detaching the shared memory segment from the address
    //space of the calling process(parent)
    //shmdt(memoryPtr);

    //init x as -1
    int* xPtr = (memoryPtr + (2* sizeof(int)));
    //*xPtr = -1;

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

    int result;
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
    

    if (result == 0) {
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
        exit(0);
    }
    else {
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
        exit(0);
    }

    return 0;
}
