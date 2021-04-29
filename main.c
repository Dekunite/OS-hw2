#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

#define LSIZ 128 
#define RSIZ 10 

int main(void) {
    char line[RSIZ][LSIZ];
	
    FILE *fptr = NULL; 
    int i = 0;
    int M ;
    int n;
    int x;
    int y;
    int memorySize = sizeof(M) + sizeof(n);

    printf("\n\n Read the file and store the lines into an array :\n");
	printf("------------------------------------------------------\n"); 
	printf(" Input the filename to be opened : ");
    char fname[20] = "test.txt";
    //char fname[20];
	//scanf("%s",fname);	
    int counter = 0;

    fptr = fopen(fname, "r");
    int num;
    int* memoryPtr = (int*) malloc(memorySize);
    //first line
    fscanf(fptr, "%d", &num);
    *(memoryPtr + 4) = num;
    M = *(memoryPtr + 4);
    //second line
    fscanf(fptr, "%d", &num);
    *(memoryPtr) = num;
    n = *(memoryPtr);

    memorySize = sizeof(M) + sizeof(n) + sizeof(x) + sizeof(y) + (2 * n * sizeof(int));
    memoryPtr = realloc(memoryPtr, memorySize);
    //+(x*sizeof(int)) + (y*sizeof(int))

    int* A = (memoryPtr + 16);
    //third line & array
    for (int k = 0; k<n; k++) {
        fscanf(fptr, "%d", &num);
        A[k] = num;
    }

	printf("\n The content of the file %s  are : \n",fname);    
    for(i = 0; i < n; ++i)
    {
        //printf(" %s\n", line[i]);
        printf(" %d\n", A[i]);
    }
    printf("M: %d \n",M);
    printf("n: %d \n",n);
    printf("\n");

    int result;
    result = fork();

    printf("parent num: %d", num);
    if (result == 0) {
        printf("Child process %d. child pid: %d parent pid: %d \n",i, getpid(), getppid());
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
        *(memoryPtr + (2 * sizeof(int))) = xCounter;

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
        //wait(NULL);
    }
    else {
        //wait(NULL);

        //TO DO MAin process child dan sonra z i printleyebilsin
        printf("A: %p\n", A);
        printf("Parent process (i=%d). pid: %d \n",i, getpid());
        printf("x value: %d", *(memoryPtr+8));
    }



    return 0;
}
