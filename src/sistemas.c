#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "types.h"
#include "defines.h"

int contTimers = 0;

void* hclock() {
    int i, j, k;
    while(1) {
        pthread_mutex_lock(&mutexTimers);   

        for (i = 0; i < maquina.info.cpus; i++) {
            for (j = 0; j < maquina.info.cores; j++) {
                for (k = 0; k < maquina.info.threads; k++) {
                    if ((maquina.cpus[i].cores[j].hilos[k].executing->data->vida > 0) && 
                        (!cmpnode(maquina.cpus[i].cores[j].hilos[k].executing, nullNode))) {
                        maquina.cpus[i].cores[j].hilos[k].executing->data->vida--;
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

void* hTimerProcessGenerator() {
    usleep(50);
    pthread_mutex_lock(&mutexTimers);   
    int contLocalTimer = 0;
    while(1) {
        pthread_mutex_lock(&mutexGenerator);
        contLocalTimer++;
        if (contLocalTimer >= maquina.info.calls_ProcessGeneratorTick) {
            pthread_cond_signal(&condGenerator);
            pthread_cond_wait(&condGenerator, &mutexGenerator);
            contLocalTimer = 0;
        }
        pthread_mutex_unlock(&mutexGenerator);

        contTimers++;
        pthread_cond_signal(&condTimers);
        pthread_cond_wait(&condAB, &mutexTimers);
        
    }

    pthread_exit(NULL);
}

void* hTimerScheduler() {
    usleep(100);
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
    }

    pthread_exit(NULL);
}

void* hProcessGenerator() {
    usleep(200);             
    pthread_mutex_lock(&mutexGenerator);
    int contID = 0;
    while(1) {
        pcb_t* p;
        node_t* n;
        crear_pcb(&p, contID++);
        crear_node(&n, p);
        encolar(n, preparados);
        printf("[Process Generator] PCB creado ···");
        printNode(n);
        //Aqui se acaba todo lo que tengo que hacer, y signaleo que ya he acabado mi tarea/iteracion
        pthread_cond_signal(&condGenerator);
        pthread_cond_wait(&condGenerator, &mutexGenerator);
    }

    pthread_exit(NULL);
}

void* hScheduler() {
    usleep(250);               
    pthread_mutex_lock(&mutexScheduler);
    int i, j, k;
    while(1) {
        // Sacar pcb's acabados de los hilos y meter nullProcs mediante nullNodes
        for (i = 0; i < maquina.info.cpus; i++) {
            for (j = 0; j < maquina.info.cores; j++) {
                for (k = 0; k < maquina.info.threads; k++) {
                    if ((maquina.cpus[i].cores[j].hilos[k].executing->data->vida <= 0) &&
                        !cmpnode(maquina.cpus[i].cores[j].hilos[k].executing, nullNode)) {
                        encolar(maquina.cpus[i].cores[j].hilos[k].executing, terminated);
                        printf("[Scheduler] Se ha liberado el hilo[%d][%d][%d] del PCB %d\n", i, j, k, maquina.cpus[i].cores[j].hilos[k].executing->data->pid);
                        maquina.cpus[i].cores[j].hilos[k].executing = nullNode;
                    }
                }
            }
        }

        // Meter pcb's en hilos vacios
        for (i = 0; i < maquina.info.cpus; i++) {
            for (j = 0; j < maquina.info.cores; j++) {
                for (k = 0; k < maquina.info.threads; k++) {
                    if (cmpnode(maquina.cpus[i].cores[j].hilos[k].executing, nullNode)) {
                        node_t* sugerencia = desencolar();
                        if (cmpnode(sugerencia, nullNode)) {
                            //printf("\n[Scheduler] Se ha intentado cargar el nodo nulo en un hilo de ejecucion\n");
                        } else {
                            maquina.cpus[i].cores[j].hilos[k].executing = sugerencia; 
                            printf("[Scheduler] PCB cargado en hilo[%d][%d][%d]: PCB PID: %d\n", i, j, k, maquina.cpus[i].cores[j].hilos[k].executing->data->pid);
                        }
                    }
                }
            }
        }
    
        printlnTodasListas();
        //Aqui se acaba todo lo que tengo que hacer, y signaleo que ya he acabado mi tarea/iteracion
        pthread_cond_signal(&condScheduler);
        pthread_cond_wait(&condScheduler, &mutexScheduler);
    }
    pthread_exit(NULL);
}