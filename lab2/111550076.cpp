/*
Student No.: 111550076
Student Name: 楊子賝
Email: zichen55zz.cs11@nycu.edu.tw
SE tag: xnxcxtxuxoxsx
Statement: I am fully aware that this program is not
supposed to be posted to a public server, such as a
public GitHub repository or a public web page.
*/
#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <sys/time.h>

using namespace std;

int main() {
    int dimension;
    cout << "Input the matrix dimension:";
    cin >> dimension;

    int shmidA = shmget(IPC_PRIVATE, sizeof(int) * dimension * dimension, IPC_CREAT | 0600);
    int shmidB = shmget(IPC_PRIVATE, sizeof(int) * dimension * dimension, IPC_CREAT | 0600);
    int shmidC = shmget(IPC_PRIVATE, sizeof(int) * dimension * dimension, IPC_CREAT | 0600);

    int *A = (int*)shmat(shmidA, NULL, 0);
    int *B = (int*)shmat(shmidB, NULL, 0);
    int *C = (int*)shmat(shmidC, NULL, 0);

    for (int i = 0; i < dimension * dimension; ++i) {
        A[i] = i;
        B[i] = i;
    }

    struct timeval start, end;
    for (int processes = 1; processes <= 16; ++processes) {
        gettimeofday(&start, NULL); 

        int rows_per_process = dimension / processes;
        for (int i = 0; i < processes; ++i) {
            pid_t pid = fork();
            if (pid == 0) {  
                int l = i * rows_per_process;
                int r = (i == processes - 1) ? dimension : l + rows_per_process;
                
                for (int i = l; i < r; ++i) {
                    for (int j = 0; j < dimension; ++j) {
                        int idx = i * dimension + j;
                        C[idx] = 0;
                        for (int k = 0; k < dimension; k++) {
                            C[idx] += A[i * dimension + k] * B[k * dimension + j];
                        }
                    }
                }
                shmdt(A);
                shmdt(B);
                shmdt(C);
                exit(0);
            }
        }

        for (int i = 0; i < processes; i++) {
            wait(NULL);
        }

        gettimeofday(&end, NULL);  

        double time_taken = (end.tv_sec - start.tv_sec) +  (end.tv_usec - start.tv_usec) * 1e-6;

        unsigned int checksum = 0;
        for (int i = 0; i < dimension * dimension; i++) {
            checksum += C[i];
        }
        cout << "Multiplying matrices using " << processes<<
        ((processes == 1) ? (" process") : (" processes"))
                  << endl << "Elapsed time: " << time_taken
                  << " sec, Checksum: " << checksum << endl;
    }

    shmdt(A);
    shmdt(B);
    shmdt(C);
    shmctl(shmidA, IPC_RMID, 0);
    shmctl(shmidB, IPC_RMID, 0);
    shmctl(shmidC, IPC_RMID, 0);

    return 0;
}
