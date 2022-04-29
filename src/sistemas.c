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

int contTimers = 0;

void* hclock() {
    int i, j, k;
    int kills = 0;

    while(1) {
        pthread_mutex_lock(&mutexTimers);   

        if (sigkill && (kills == (maquina.info.cpus * maquina.info.cores * maquina.info.threads))) {
            printf("\n\n\n=========================================\n\n");
            printf("   >> >> Se han ejecutado todos los programas de ./progs/ << <<\n\n");
            printf(" Se ha terminado la ejecucion del simulador");
            printf("\n\n\n=========================================\n\n");
            pthread_exit(NULL);
            // Hay que hacer tambien los pthread_exits de los otros hilos maestros
        }


        for (i = 0; i < maquina.info.cpus; i++) {
            for (j = 0; j < maquina.info.cores; j++) {
                for (k = 0; k < maquina.info.threads; k++) {
                    if ((maquina.cpus[i].cores[j].hilos[k].flag_ocioso == 1) && sigkill) {
                        printf("\nSe ha acabado con el hilo [%d][%d][%d]\n", i, j, k);
                        kills++;
                    }
                    if ((maquina.cpus[i].cores[j].hilos[k].flag_ocioso == 0) && (!cmpnode(maquina.cpus[i].cores[j].hilos[k].executing, nullNode))) {
                        executeProgram(i, j, k);
                    }
                }
            }
        }

        while(contTimers<2) {
            pthread_cond_wait(&condTimers, &mutexTimers);
        }
        contTimers = 0;
        usleep(maquina.info.frec);
    
        pthread_cond_broadcast(&condAB);
        pthread_mutex_unlock(&mutexTimers);
    }

    pthread_exit(NULL);
}

void* hTimerLoader() {
    usleep(50000);
    pthread_mutex_lock(&mutexTimers);   
    int contLocalTimer = 0;
    while(1) {
        pthread_mutex_lock(&mutexLoader);
        contLocalTimer++;
        if (contLocalTimer >= maquina.info.calls_LoaderTick) {
            pthread_cond_signal(&condLoader);
            pthread_cond_wait(&condLoader, &mutexLoader);
            contLocalTimer = 0;
        }
        pthread_mutex_unlock(&mutexLoader);

        contTimers++;
        pthread_cond_signal(&condTimers);
        pthread_cond_wait(&condAB, &mutexTimers);
        //printf("\n\nLEON TROTSKY\n\n");
        //printf("\n                         ");
        int *basura = malloc(sizeof(int) * 30);
    }

    pthread_exit(NULL);
}

void* hTimerScheduler() {
    usleep(100000);
    pthread_mutex_lock(&mutexTimers);
    int contLocalTimer = 0;
    while(1) {
        pthread_mutex_lock(&mutexScheduler);
        contLocalTimer++;
        if (contLocalTimer >= maquina.info.calls_SchedulerTick) {
            pthread_cond_signal(&condScheduler);
            pthread_cond_wait(&condScheduler, &mutexScheduler);
            contLocalTimer = 0;
        }
        pthread_mutex_unlock(&mutexScheduler);

        contTimers++;
        pthread_cond_signal(&condTimers);
        pthread_cond_wait(&condAB, &mutexTimers);
        //printf("\n\nIOSIF STALIN\n\n");
        //printf("\n                          ");
        int *basura = malloc(sizeof(int) * 30);
    }

    pthread_exit(NULL);
}

void* hLoader() {
    usleep(200000);             
    pthread_mutex_lock(&mutexLoader);
    int pidActual = -1;
    while(1) {
        if (sigkill == 0) {
            buscarSiguienteElf();
            actualizarNombreProgALeer();
            loadProgram(++pidActual);
            printf("[ Loader ] Programa cargado (%d.elf) y PCB creado (pid %d) ···\n", elfActual, pidActual);
        }
        //Aqui se acaba todo lo que tengo que hacer, y signaleo que ya he acabado mi tarea/iteracion
        pthread_cond_signal(&condLoader);
        pthread_cond_wait(&condLoader, &mutexLoader);
        //printf("\n[ Loader ] He acabado mi iteracion, ahora a esperar...\n");
    }

    pthread_exit(NULL);
}

void* hScheduler() {
    usleep(250000);               
    pthread_mutex_lock(&mutexScheduler);
    int i, j, k;
    while(1) {
        // Sacar pcb's acabados de los hilos y meter nullProcs mediante nullNodes
        for (i = 0; i < maquina.info.cpus; i++) {
            for (j = 0; j < maquina.info.cores; j++) {
                for (k = 0; k < maquina.info.threads; k++) {
                    if ((maquina.cpus[i].cores[j].hilos[k].flag_ocioso == 1) && !cmpnode(maquina.cpus[i].cores[j].hilos[k].executing, nullNode)) {
                        terminateStatus(i, j, k);                                               // Hacer una "foto finish" del estado del hilo cuando el programa termina
                        encolar(maquina.cpus[i].cores[j].hilos[k].executing, terminated);       // Encolar nodo en la cola de terminados
                        printf("[Scheduler] Se ha liberado el hilo[%d][%d][%d] del PCB %d\n", i, j, k, maquina.cpus[i].cores[j].hilos[k].executing->data->pid);
                        freeThread(i, j, k);      // Limpiar estructuras del hilo
                        freeProgram(maquina.cpus[i].cores[j].hilos[k].executing);
                    }
                }
            }
        }

        // Meter pcb's en hilos vacios
        for (i = 0; i < maquina.info.cpus; i++) {
            for (j = 0; j < maquina.info.cores; j++) {
                for (k = 0; k < maquina.info.threads; k++) {
                    if (cmpnode(maquina.cpus[i].cores[j].hilos[k].executing, nullNode) && maquina.cpus[i].cores[j].hilos[k].flag_ocioso == 1 && sigkill == 0) {
                        node_t* sugerencia = desencolar();
                        if (cmpnode(sugerencia, nullNode)) {
                            printf("\n[Scheduler Warning] Se ha intentado cargar el nodo nulo en un hilo de ejecucion\n");
                        } else {
                            // Cargar informacion del pcb y programa en el hilo
                            loadThread(sugerencia, i, j, k);
                            printf("[Scheduler] PCB cargado en hilo[%d][%d][%d]: PCB PID: %d\n", i, j, k, maquina.cpus[i].cores[j].hilos[k].executing->data->pid);
                        }
                    }
                }
            }
        }
    
        printlnTodasListas();
        //Aqui se acaba todo lo que tengo que hacer, y signaleo que ya he acabado mi tarea/iteracion
        pthread_cond_signal(&condScheduler);
        printf("\n[ Scheduler ] Iteracion del Scheduler terminada...\n");
        pthread_cond_wait(&condScheduler, &mutexScheduler);
        printf("\n[ Scheduler ] Iteracion del Scheduler terminada...\n");
    }
    pthread_exit(NULL);
}