#include <pthread.h>
#include "types.h"

unsigned char physical[0xFFFFFF];
char* nombreProg;
int sigkill = 0;
int elfActual = -1;

machine_t maquina;
pthread_t* masterThreads;
coleccion_listas_t listas;

pthread_mutex_t mutexTimers, mutexLoader, mutexScheduler;
pthread_cond_t condTimers, condAB, condLoader, condScheduler;

pcb_t* nullProc;
node_t* nullNode;
tlb_t* nullTlb;
mm_t* nullMm;
status_t* nullStatus;