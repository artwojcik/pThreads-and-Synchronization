/*
 *  raceTest.cpp
 *
 *  Artur Wojcik
 *  CS 361 UIC Fall 2017
 *  NetID: awojci5
 *
 *
 *  A simulation in which a number of threads access a group of
 *  shared buffers for both reading and writing purposes. User has an option
 *  to use a synchronization tools ( semaphores ) to illustrate the problems
 *  of race conditions, and then with thesynchronization tools to solve the
 *  problems. ( A separate mutex will be used to coordinate writing to the screen. )
 *
 *
*/


#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <limits>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

using namespace std;

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun {
    int val;
/* value for SETVAL */
    struct semid_ds *buf;
/* buffer for IPC_STAT, IPC_SET */
    unsigned short *array;
/* array for GETALL, SETALL */
/* Linux specific part: */
    struct seminfo *__buf;
/* buffer for IPC_INFO */
};
#endif


struct Worker {
    int nBuffers;
    int workerID;
    double sleepTime;
    int semID;
    int mutexID;
    int *buffers;
    int nReadErrors;
    int next;
    int nWorkers;
    int lock;
};

//prototyes
void info();
bool isPrime(int num);
void *worker(void *vargp);
void badBits(int nWorkers, int bit, int &error);




int main(int argc, char **argv) {
    info();

    //cmd line arguments
    int nBuffers, nWorkers;
    double sleepMin = 1.0, sleepMax = 5.0;
    int randSeed;
    int lock= 0;//=1, nolock=0;

    if (argc < 3 || argc > 7) {
        cerr << "Invalid Number of arguments. Correct arguments -> \n";
        cerr << "./raceTest nBuffers nWorkers [ sleepMin sleepMax ] [ randSeed ] [ -lock | -nolock ]";
        exit(-1);
    }
    nBuffers = atoi(argv[1]);
    if ((nBuffers <= 2 || nBuffers > 32) || !isPrime(nBuffers)) {
        cerr << "Not a prime number or out of range 2<nBuffers<32\n";
        exit(-2);
    }

    nWorkers = atoi(argv[2]);

    if (nWorkers >= nBuffers || nWorkers <= 0) {
        cerr << "Too many workers... Number of workers should be less then nBuffers\n";
        exit(-3);
    }

    if (argc >= 4) {
        sleepMin = atof(argv[3]);
        if (sleepMin < 0) {
            sleepMin = 1.0;
        }
    }
    if (argc >= 5) {
        sleepMax = atof(argv[4]);
        if (sleepMax < 0) {
            sleepMax = 5.0;
        }
    }
    //check if time is set correclty if not set to defult
    if (sleepMin >= sleepMax) {
        sleepMax = 5.0;
        sleepMin = 1.0;
        cout << "Invalid Time setup: \nTime set to default: minSleep= " << sleepMin;
        cout << " sleepMax = " << sleepMax;
        cout << endl;
    }
    if (argc >= 6) {
        randSeed = atoi(argv[5]);
    }
    if (argc == 7) {
        if (strstr(argv[6], "-lock") != NULL) {
            lock = 1;
        } else if ((strstr(argv[6], "-nolock") != NULL)) {
            lock = 0;
        } else {
            lock = 0;
        }

    }


    //check if randomSeed was provided if not time(null)
    if (randSeed > 0) {
        srand(randSeed);
    } else {
        srand(time(NULL));
    }


    //create single semaphore for screen
    int mutex =  semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (mutex < 0 ) {
        perror ( "mutexID semaphore ");
        exit (-9);
    }

    //s
    int semLock = semget(IPC_PRIVATE, nBuffers, IPC_CREAT | 0600);
    if(semLock < 0 ){
        cerr<< " semLock \n";
        exit(-10);
    }

    Worker workers[nWorkers];
    int buffer[nBuffers] = {0};
    pthread_t tid[nWorkers];
    union semun lockMutex;
    union semun lockArray;

    //initilize ale semaphores
    unsigned short numSemaphores [nBuffers];// = {1};
    lockArray.array = numSemaphores;
    for(int i=0; i< nBuffers; i++){
        numSemaphores[i]=1;
    }
    //lockArray.array = numSemaphores;
    if(semctl(semLock, 0, SETALL, lockArray) < 0 ) {
        cerr << "semctl -> semLock\n";
        exit(-11);
    }



    unsigned  short a[1];
    a[0] =1;

    lockMutex.array = a;
    if (semctl(mutex, 0, SETALL, lockMutex)<0) {
        perror("semctl -> mutex");
        exit(-10);
    }


    cout << "\n*** Running program with " << nWorkers << " workers and working on "<< nBuffers;
    if(lock == 1){
        cout<< " buffers with option [-lock]\n\n";
    }
    else {
        cout<< " buffers with option [-unlock]\n\n";
    }

    //initilize each struct in the array and create thread
    for (int i = 0; i < nWorkers; i++) {

        workers[i].buffers = buffer;
        workers[i].workerID = i + 1;
        workers[i].mutexID = mutex;
        workers[i].semID = semLock;
        workers[i].nBuffers = nBuffers;
        workers[i].nReadErrors = 0;
        workers[i].sleepTime = (sleepMin + (sleepMax - sleepMin) * rand() / RAND_MAX)*1000000;
        printf("  -Worker number %d has sleep time %lf\n", i+1, workers[i].sleepTime/1000000);
        workers[i].next = i + 1;
        workers[i].nWorkers = nWorkers;
        workers[i].lock = lock;


        if (pthread_create(&tid[i], NULL, worker, &workers[i])) {
            cerr << "Error creating thread\n";
            return 1;
        }

    }
    cout<< endl;

    //wait to join threads
    for (int i = 0; i < nWorkers; i++) {
        pthread_join(tid[i], NULL);
    }


    //count all read errors
    int rError = 0;
    for (int i = 0; i < nWorkers; i++) {
        rError += workers[i].nReadErrors;
    }



    cout << endl;

    //count all write errors
    int shouldContain = (1 << nWorkers) - 1;
    int wError = 0;
    for (int i = 0; i < nBuffers; i++) {
        if (buffer[i] != shouldContain) {

            cout << " ^ Error in buffer " << i << ". Bad bits = ";
            int bit = (shouldContain ^ buffer[i]);

            badBits(nWorkers, bit, wError);
            cout << endl;
        }


    }

    cout << "\n ** Value that should be in each buffer is: --> " << shouldContain << endl;
    cout << "\n ** Actual value in each buffer:\n\n";
    for (int i =0; i< nBuffers; i++){
        cout << " ** Value in buffer number [" << i << "] is ->>  " <<buffer[i] << endl;
    }

    cout << endl<<"  -= "<< rError << " Read errors and " << wError << " write errors encountered. =-\n\n";

    cout<<"\n  -= Exiting Program =-\n\n";

    semctl(mutex, 1, IPC_RMID);
    semctl(semLock, 1, IPC_RMID);


    return 0;
}



//thread function deals with semaphores and all operations
void *worker(void *vargp) {

    Worker *argv = (Worker *) vargp;

    int bSleep, aSleep, error = 0;

    struct sembuf lockMutex = {(unsigned short)0, -1, 0};
    struct sembuf unlockMutex = {(unsigned short)0, 1, 0};


    struct sembuf lockBuf;
    struct sembuf unlockBuf;

    for (int i = 0; i < argv->nBuffers; i++) {

        lockBuf = {(unsigned short)(argv->next), -1,0};
        unlockBuf = {(unsigned short)(argv->next), 1, 0};

        if(argv->lock == 1) {
            semop(argv->semID, &lockBuf, 1);
        }

        bSleep = argv->buffers[argv->next];
        usleep(argv->sleepTime);
        aSleep = argv->buffers[argv->next];

        semop(argv->semID, &unlockBuf, 1 );

        //next compare results
        //...

        if (aSleep != bSleep) {


            if(semop(argv->mutexID, &lockMutex, 1)< 0){
                exit(9);
            }
            cout << "  !! Child number " << argv->workerID << " reported change from " << bSleep << " to " << aSleep
                 << " in buffer " << argv->next << " Bad bits = " ;//endl;

            int bit = (bSleep ^ aSleep);
            badBits(argv->nWorkers, bit, error);
            cout << endl;
            semop(argv->mutexID, &unlockMutex, 1);


        }

        //update next```
        argv->next = (argv->next + argv->workerID) % argv->nBuffers;

        lockBuf.sem_num = argv->next;
        unlockBuf.sem_num = argv->next;

        if(argv->lock == 1) {
            semop(argv->semID, &lockBuf, 1);
        }
        //read in other buffer
        bSleep = argv->buffers[argv->next];
        usleep(argv->sleepTime);
        aSleep = argv->buffers[argv->next];
        semop(argv->semID, &unlockBuf, 1);

        //next compare results
        if (aSleep != bSleep) {

            semop(argv->mutexID, &lockMutex, 1);
            cout << "  !! Child number " << argv->workerID << " reported change from " << bSleep << " to " << aSleep
                 << " in buffer " << argv->next << " Bad bits = " ;//endl;

            int bit = (bSleep ^ aSleep);
            badBits(argv->nWorkers, bit, error);
            cout << endl;
            semop(argv->mutexID, &unlockMutex, 1);


        }

        //update next and update semaphore number
        argv->next = (argv->next + argv->workerID) % argv->nBuffers;
        lockBuf.sem_num = argv->next;
        unlockBuf.sem_num = argv->next;



        if(argv->lock == 1) {
            semop(argv->semID, &lockBuf, 1);
        }
        //write operation
        bSleep = argv->buffers[argv->next];;
        usleep(argv->sleepTime);
        bSleep += (1 << (argv->workerID - 1));
        argv->buffers[argv->next] = bSleep;

        semop(argv->semID, &unlockBuf, 1);


        argv->nReadErrors = error;

    }

    return NULL;
}

//info....
void info() {
    cout << endl;
    cout << "  ++++++++++++++++++++++++++++++\n";
    cout << "  +   Artur Wojcik             +\n";
    cout << "  +   NetID: awojci5           +\n";
    cout << "  +   CS 361 UIC Fall 2017     +\n";
    cout << "  +   11/25/2017               +\n";
    cout << "  ++++++++++++++++++++++++++++++\n\n";
}

//check if number is prime
bool isPrime(int n) {
    for (int i = 2; i < n; i++)
        if (n % i == 0)
            return false;

    return true;
}

//check bad bits in buffer
void badBits(int nWorkers, int bit, int &error) {
    for (int i = 0; i != nWorkers; i++) {
        if (bit & (1 << i)) {
            error++;
            cout << " " << i;
        }
    }
}
