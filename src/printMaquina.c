#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

#include "types.h"
#include "defines.h"

void red () {
  printf("\033[1;31m");
}
void yellow () {
  printf("\033[0;33m");
}
void green () {
  printf("\033[0;32m");
}
void reset () {
  printf("\033[0m");
}
void clear() {
  printf("\033[2J");
}

void printMachine() {
    clear();

    int i, j, k, w1, w2;
    float uso;
    int threadsXcpu = maquina.info.cores * maquina.info.threads;

    reset();
    printf(" ========================================= \n");
    printf(" ======="); green(); printf(" Informacion del hardware "); reset(); printf(" ======= \n");
    printf(" ========================================= \n\n");

    printf(" Nº de procesadores:    "); green(); printf(" %d\n", maquina.info.cpus); reset();
    printf(" Nº de cores por CPU:   "); green(); printf(" %d\n", maquina.info.cores); reset();
    printf(" Nº de hilos por CORE:  "); green(); printf(" %d\n", maquina.info.threads); reset();
    printf(" Nº de threads totales: "); green(); printf(" %d\n", maquina.info.cpus * maquina.info.cores * maquina.info.threads); reset();
    printf(" ------------------------------\n");
    printf(" Velocidad de Reloj:    "); green(); printf(" %d\n", maquina.info.frec); reset();
    printf(" Ciclos para Scheduler: "); green(); printf(" %d\n", maquina.info.calls_SchedulerTick); reset();
    printf(" Ciclos para Loader:    "); green(); printf(" %d\n\n", maquina.info.calls_LoaderTick); reset();

    printf(" ========================================= \n");
    printf(" ========="); green(); printf(" Estado del sistema "); reset(); printf(" =========== \n");
    printf(" ========================================= \n\n");


    if (flag_schedulerNoted == 1) {
        printf("%c[1m",27); red(); printf(" >> Scheduler | Un programa ha finalizado su ejecucion, y se ha sacado del thread\n"); reset(); printf("%c[0m",27);
        flag_schedulerNoted = 0;
    } else if (flag_schedulerNoted == 2) {
        printf("%c[1m",27); red(); printf(" >> Scheduler | Un programa ha sido cargado en un thread\n"); reset(); printf("%c[0m",27);
        flag_schedulerNoted = 0;
    } else {
        printf("%c[1m",27); printf(" >> Scheduler\n"); printf("%c[0m",27);
    }


    if (flag_loaderLoaded) {
        printf("%c[1m",27); red(); printf(" >> Loader | programa cargado: "); 
        for (i = 0; i < 11; i++) 
            printf("%c", nombreProg[i]);
        printf("\n\n");
        reset(); printf("%c[0m",27);
        flag_loaderLoaded = 0;
    } else {
        printf("%c[1m",27); printf(" >> Loader\n\n"); printf("%c[0m",27);
    }
    
    printf(" >> Cola de preparados: ");
    node_t* sig;
    if (listas.preparados.size != 0) {
        sig = listas.preparados.first;
        while(!cmpnode(sig, nullNode)) {
            printf("PID: %d  ", sig->data->pid);
            sig = sig->next;
            if (!cmpnode(sig, nullNode)) 
                printf("⟼   ");
        }
    } else {
        printf("cola vacia");
    }
    printf("\n >> Cola de terminados: ");
    if (listas.terminated.size != 0) {
        sig = listas.terminated.first;
        while(!cmpnode(sig, nullNode)) {
            printf("PID: %d  ", sig->data->pid);
            sig = sig->next;
            if (!cmpnode(sig, nullNode)) 
                printf("⟼   ");
        }
    } else {
        printf("cola vacia");
    }

    printf("\n\n ========================================= \n");
    printf(" ========"); green(); printf(" Estado de la maquina "); reset(); printf(" ========== \n");
    printf(" ========================================= \n\n");
    
    for (i = 0; i < maquina.info.cpus; i++) {
        uso = 0;
        for (w1 = 0; w1 < maquina.info.cores; w1++) {
            for (w2 = 0; w2 < maquina.info.threads; w2++) {
                if (!maquina.cpus[i].cores[w1].hilos[w2].flag_ocioso)
                    uso++;
            }
        }
        printf(" >> CPU %d | Informacion General | Uso: %% ", i);
        printf("%d\n", (int) ((uso/(float)threadsXcpu) * 100));
        printf(" -----------------------------------------\n");
        for (j = 0; j < maquina.info.cores; j++) {
            printf("     >> CORE %d\n", j);
            for (k = 0; k < maquina.info.threads; k++) {
                printf("        <> Thread %d: ", k); 
                if (maquina.cpus[i].cores[j].hilos[k].flag_ocioso) {
                    printf("vacio\n");
                } else {
                    red(); printf("ejecutando el PCB %d | IR: %x ", maquina.cpus[i].cores[j].hilos[k].executing->data->pid, maquina.cpus[i].cores[j].hilos[k].ir);
                    int c0 = maquina.cpus[i].cores[j].hilos[k].ir >> 28;
                    switch (c0) {
                        case 0: printf(" | Instruccion de tipo LD\n"); break;
                        case 1: printf(" | Instruccion de tipo ST\n"); break;
                        case 2: printf(" | Instruccion de tipo ADD\n"); break;
                        case 0xF: printf(" | Instruccion de tipo EXIT\n"); break;
                        default: printf(" | Instruccion no compatible con la arquitectura\n"); break;
                    }
                    reset();
                }
            }
        }
        printf("\n");
    }
}

void printStats() {
    printf("   --- Estadisticas Generales ---\n\n");
    printf("       > Programas ejecutados:                "); green(); printf("%d\n", maquina.info.progExec); reset();
    printf("       > Instrucciones ejecutadas:            "); green(); printf("%d\n", maquina.info.instrExec); reset();
    printf("       > Tiempo total:                        "); green(); printf("%1.3f ms\n", maquina.info.tej*1000); reset();
    printf("       > Max de hilos usados al mismo tiempo: "); green(); printf("%d\n", maquina.info.maxUsage); reset();
    printf("       > Frecuencia minima de reloj:          "); green(); printf("%d\n", maquina.info.minFrec); reset();
    printf("       > Frecuencia maxima de reloj:          "); green(); printf("%d\n", maquina.info.maxFrec); reset();
}