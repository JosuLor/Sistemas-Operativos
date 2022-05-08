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

    printf("%c[1m",27); red(); printf(" >> Scheduler\n"); reset(); printf("%c[0m",27);

    printf("%c[1m",27); red(); printf(" >> Loader\n\n"); reset(); printf("%c[0m",27);


    printf(" ========================================= \n");
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
                    red(); printf("ejecutando el PCB %d | IR: %x \n", maquina.cpus[i].cores[j].hilos[k].executing->data->pid, maquina.cpus[i].cores[j].hilos[k].ir); reset();
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