#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "types.h"
#include "defines.h"

void initializeStructures() {
    int i, j, k, w;

    pthread_mutex_init(&mutexTimers, NULL);
    pthread_mutex_init(&mutexGenerator, NULL);
    pthread_mutex_init(&mutexScheduler, NULL);
    pthread_cond_init(&condTimers, NULL);
    pthread_cond_init(&condAB, NULL);
    pthread_cond_init(&condGenerator, NULL);
    pthread_cond_init(&condScheduler, NULL);

    masterThreads = malloc(sizeof(pthread_t) * 5);

    nullProc = malloc(sizeof(pcb_t));
    nullProc->pid = -1;
    nullProc->vida = -1;

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
            }
        }
    }
    printf("\n");
}

void printHelp(char* nombre) {
    printf("\nUso incorrecto. Uso: %s <cpus> <cores> <threads> <ciclosGenerator> <ciclosScheduler> <frecuencia>\n", nombre);
    printf("<cpus>              Nº CPUs del sistema\n");
    printf("<cores>             Nº Cores por cada CPU\n");
    printf("<threads>           Nº Threads por cada Core\n");
    printf("<ciclosGenerator>   Nº ciclos para generar un tick del Process Generator\n");
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
    int local_callsProcessGenerator = atoi(argv[4]);
    int local_callsScheduler = atoi(argv[5]);
    int local_frec = atoi(argv[6]);

    if (local_cpus <= 0 || local_cores <= 0 || local_threads <= 0 || 
        local_callsProcessGenerator <= 0 || local_callsScheduler <= 0 || 
        local_frec <= 0) {
            printHelp(argv[0]);
            return 2;
    }

    srand(time(NULL));
    maquina.info.cpus = local_cpus;
    maquina.info.cores = local_cores;
    maquina.info.threads = local_threads;
    maquina.info.calls_ProcessGeneratorTick = local_callsProcessGenerator;
    maquina.info.calls_SchedulerTick = local_callsScheduler;
    maquina.info.frec = local_frec;

    initializeStructures();

    pthread_create(&masterThreads[0], NULL, hclock,                 NULL);
    pthread_create(&masterThreads[1], NULL, hTimerProcessGenerator, NULL);
    pthread_create(&masterThreads[2], NULL, hTimerScheduler,        NULL);
    pthread_create(&masterThreads[3], NULL, hProcessGenerator,      NULL);
    pthread_create(&masterThreads[4], NULL, hScheduler,             NULL);
    
    pthread_join(masterThreads[0], NULL);
    pthread_join(masterThreads[1], NULL);
    pthread_join(masterThreads[2], NULL);
    pthread_join(masterThreads[3], NULL);
    pthread_join(masterThreads[4], NULL);

    //sleep(30);

    return 0;
}
