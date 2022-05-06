#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "types.h"
#include "defines.h"

void initializeStructures() {
    int i, j, k, w;

    pthread_mutex_init(&mutexTimers, NULL);
    pthread_mutex_init(&mutexLoader, NULL);
    pthread_mutex_init(&mutexScheduler, NULL);
    pthread_cond_init(&condTimers, NULL);
    pthread_cond_init(&condAB, NULL);
    pthread_cond_init(&condLoader, NULL);
    pthread_cond_init(&condScheduler, NULL);

    masterThreads = malloc(sizeof(pthread_t) * 5);

    nullTlb = malloc(sizeof(tlb_t));
    nullTlb->virtualPage = malloc(sizeof(unsigned int) * TAM_TLB);
    nullTlb->physicalPage = malloc(sizeof(unsigned int) * TAM_TLB);
    nullTlb->score =  malloc(sizeof(int) * TAM_TLB);

    nullStatus = malloc(sizeof(status_t));
    nullStatus->regs = malloc(sizeof(int) * NUM_RGs);
    nullStatus->pc = NULL;
    nullStatus->ir = -1;
    nullStatus->ptbr = NULL;
    nullStatus->tlb = nullTlb;

    nullMm = malloc(sizeof(mm_t));
    nullMm->code = NULL;
    nullMm->data = NULL;
    nullMm->pgb = NULL;
    nullMm->psize = 0;

    nullProc = malloc(sizeof(pcb_t));
    nullProc->pid = -1;
    nullProc->status = nullStatus;
    nullProc->mm = nullMm;

    nullNode = malloc(sizeof(node_t));
    nullNode->data = nullProc;
    nullNode->next = NULL;

    listas.preparados.first = nullNode;
    listas.preparados.last = nullNode;
    listas.preparados.size = 0;
    listas.terminated.first = nullNode;
    listas.terminated.last = nullNode;
    listas.terminated.size = 0;

    maquina.cpus = malloc(sizeof(cpu_t) * maquina.info.cpus);
    for (i = 0; i < maquina.info.cpus; i++) {
        maquina.cpus[i].cores = malloc(sizeof(core_t) * maquina.info.cores);
        for (j = 0; j < maquina.info.cores; j++) {
            maquina.cpus[i].cores[j].hilos = malloc(sizeof(hilo_t) * maquina.info.threads);
            for (k = 0; k < maquina.info.threads; k++) {
                maquina.cpus[i].cores[j].hilos[k].executing = nullNode;
                maquina.cpus[i].cores[j].hilos[k].rgs = malloc(sizeof(int) * NUM_RGs);
                maquina.cpus[i].cores[j].hilos[k].pc = NULL;
                maquina.cpus[i].cores[j].hilos[k].ir = -1;
                maquina.cpus[i].cores[j].hilos[k].ptbr = NULL;
                maquina.cpus[i].cores[j].hilos[k].tlb = malloc(sizeof(tlb_t));
                maquina.cpus[i].cores[j].hilos[k].tlb->virtualPage = malloc(sizeof(unsigned int) * TAM_TLB);
                maquina.cpus[i].cores[j].hilos[k].tlb->physicalPage = malloc(sizeof(unsigned int) * TAM_TLB);
                maquina.cpus[i].cores[j].hilos[k].tlb->score = malloc(sizeof(int) * TAM_TLB);
                for (w = 0; w < TAM_TLB; w++) {
                    maquina.cpus[i].cores[j].hilos[k].tlb->virtualPage[w] = -1;
                    maquina.cpus[i].cores[j].hilos[k].tlb->physicalPage[w] = -1;
                    maquina.cpus[i].cores[j].hilos[k].tlb->score[w] = -1;
                }
                maquina.cpus[i].cores[j].hilos[k].flag_ocioso = 1;
            }
        }
    }

    initializePageTable();

    nombreProg = malloc(sizeof(char) * 11);
    nombreProg[0] = 'p'; nombreProg[1] = 'r'; nombreProg[2] = 'o'; nombreProg[3] = 'g';
    nombreProg[4] = 'X'; nombreProg[5] = 'Y'; nombreProg[6] = 'Z';
    nombreProg[7] = '.'; nombreProg[8] = 'e'; nombreProg[9] = 'l'; nombreProg[10] = 'f';

    printf("\n");
}

void printHelp(char* nombre) {
    printf("\nUso incorrecto. Uso: %s <cpus> <cores> <threads> <ciclosLoader> <ciclosScheduler> <frecuencia>\n", nombre);
    printf("<cpus>              Nº CPUs del sistema\n");
    printf("<cores>             Nº Cores por cada CPU\n");
    printf("<threads>           Nº Threads por cada Core\n");
    printf("<ciclosLoader>      Nº ciclos para generar un tick del Loader\n");
    printf("<ciclosScheduler>   Nº ciclos para generar un tick del Scheduler\n");
    printf("<frecuencia>        Velocidad del sistema, cada cuanto ocurre un ciclo\n");
    printf("                        ⟼   Cuanto mas alto, mas lento\n");
    printf("                        ⟼   Cuanto mas bajo, mas rapido\n");
    printf("                        ⟼   Se recomienda como minimo 300000\n");
}

int main(int argc, char* argv[]) {
    
    if (argc != 7) {
        printHelp(argv[0]);
        return 1;
    }

    int local_cpus = atoi(argv[1]);
    int local_cores = atoi(argv[2]);
    int local_threads = atoi(argv[3]);
    int local_callsLoader = atoi(argv[4]);
    int local_callsScheduler = atoi(argv[5]);
    int local_frec = atoi(argv[6]);

    if (local_cpus <= 0 || local_cores <= 0 || local_threads <= 0 || 
        local_callsLoader <= 0 || local_callsScheduler <= 0 || 
        local_frec <= 0) {
            printHelp(argv[0]);
            return 2;
    }

    srand(time(NULL));
    maquina.info.cpus = local_cpus;
    maquina.info.cores = local_cores;
    maquina.info.threads = local_threads;
    maquina.info.calls_LoaderTick = local_callsLoader;
    maquina.info.calls_SchedulerTick = local_callsScheduler;
    maquina.info.frec = local_frec;

    initializeStructures();

    pthread_create(&masterThreads[0], NULL, hclock,                 NULL);
    pthread_create(&masterThreads[1], NULL, hTimerLoader,           NULL);
    pthread_create(&masterThreads[2], NULL, hTimerScheduler,        NULL);
    pthread_create(&masterThreads[3], NULL, hLoader,                NULL);
    pthread_create(&masterThreads[4], NULL, hScheduler,             NULL);
    
    pthread_join(masterThreads[0], NULL);
    pthread_join(masterThreads[1], NULL);
    pthread_join(masterThreads[2], NULL);
    pthread_join(masterThreads[3], NULL);
    pthread_join(masterThreads[4], NULL);

    return 0;
}
