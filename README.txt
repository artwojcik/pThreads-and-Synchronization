
  raceTest.cpp
 
  Artur Wojcik
  CS 361 UIC Fall 2017
  NetID: awojci5

A simulation in which a number of threads access a group of
shared buffers for both reading and writing purposes. User has an option
to use a synchronization tools ( semaphores ) to illustrate the problems
of race conditions, and then with thesynchronization tools to solve the
problems. ( A separate mutex will be used to coordinate writing to the screen. )

To compile the program just type in command line: make 
This will build a program and will create executable file: raceTest

Two run program after was compiled command: make run
can be used However on some sytems doesnt work 
Program requiers following arguments to run if make run will fail


./raceTest nBuffers nWorkers [ sleepMin sleepMax ] [ randSeed ] [ -lock | -nolock ]

Paranetera in []  are optional not requiered

Files included in folder 

raceTest-awojci5.cpp      ->  actual code for program 
README.txt                ->  this file 
nolock.txt                ->  sample output with nolock on semaphores 
lock.txt                  ->  sample output with lock on semaphores 
makefile                  ->  to compile program 



