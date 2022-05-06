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

int contTimers  = 0;
int sigKill     = 0;
int killedThreads = 0;



void* hclock() {
    int i, j, k;
    int kills = 0;
    int cont = 0;

    while(1) {
        pthread_mutex_lock(&mutexTimers);   
        if (killedThreads == 4) 
            break;
            
        if (kills == (maquina.info.cpus * maquina.info.cores * maquina.info.threads)) {
            sigKill = 1;
            //printf("\n<<<<< <<<<< <<<<<<<<<<<< SIGKILL >>>>>>>>>>>>> >>>>>> >>>>>\n");
            if (killedThreads == 4)
                break;
        } else {
            for (i = 0; i < maquina.info.cpus; i++) {
                for (j = 0; j < maquina.info.cores; j++) {
                    for (k = 0; k < maquina.info.threads; k++) {
                        if ((maquina.cpus[i].cores[j].hilos[k].flag_ocioso == 1) && (loaderKill == 1) && (listas.preparados.size == 0)) {
                            printf("\nSe ha acabado con el hilo [%d][%d][%d]\n", i, j, k);
                            kills++;
                        }
                        if ((maquina.cpus[i].cores[j].hilos[k].flag_ocioso == 0) && (!cmpnode(maquina.cpus[i].cores[j].hilos[k].executing, nullNode))) {
                            executeProgram(i, j, k);
                        }
                    }
                }
            }
        }

        cont++;
        printf("\n%d | Alemania del Este | kills: %d\n", cont, killedThreads);
        if (killedThreads == 4)
                break;
         
        while(contTimers<2) {
            if (killedThreads == 4)
                break;
            pthread_cond_wait(&condTimers, &mutexTimers);
        }
        
        if (killedThreads == 4) 
            break;

        printf("\n%d | Alemania Occidental | kills: %d\n", cont, killedThreads);
        contTimers = 0;
        usleep(maquina.info.frec);
        if (killedThreads == 4) 
            break;

        pthread_cond_broadcast(&condAB);
        if (killedThreads == 4) 
            break;
        pthread_mutex_unlock(&mutexTimers);
        if (killedThreads == 4) 
            break;
    }

    printf("\n\n\n==================================================================================\n\n");
    printf("   >> >> Se han cargado y ejecutado todos los programas de ./progs/ << <<\n\n");
    printf("         >> Se ha detenido el reloj y las unidades de ejecucion <<\n\n");
    printf("              >> Se ha terminado la ejecucion del simulador <<");
    printf("\n\n\n==================================================================================\n\n");
    pthread_exit(NULL);
}

void* hTimerLoader() {
    usleep(50000);
    pthread_mutex_lock(&mutexTimers);   
    int contLocalTimer = 0;
    int *contProc = malloc(sizeof(int) * 60);
    while(1) {
        pthread_mutex_lock(&mutexLoader);

        contLocalTimer++;
        if ((killedThreads == 0) && (contLocalTimer >= maquina.info.calls_LoaderTick)) {
            pthread_cond_signal(&condLoader);
            pthread_cond_wait(&condLoader, &mutexLoader);
            contLocalTimer = 0;
        }

        pthread_mutex_unlock(&mutexLoader);

        if (sigKill)
            if (killedThreads >= 2) break;

        //if (sigKill == 1) {
        //    if (killedThreads == 2 && (maquina.info.calls_LoaderTick <= maquina.info.calls_SchedulerTick)) break;
        //    if (killedThreads == 3) break;
        //}

        //if ((killedThreads == 3) || (killedThreads == 2 && maquina.info.calls_LoaderTick >= maquina.info.calls_SchedulerTick))
        //    break;
        
        contTimers++;
        pthread_cond_signal(&condTimers);
        pthread_cond_wait(&condAB, &mutexTimers);
        
    }

    contTimers++;
    killedThreads++;
    pthread_cond_signal(&condTimers);
    pthread_mutex_unlock(&mutexTimers);
    printf("\nkilledThreads: %d\n", killedThreads);
    printf("\nhTimerLoader KILLED ---------------------------------------------------------------->\n");
    pthread_exit(NULL);
}

void* hTimerScheduler() {
    usleep(100000);
    pthread_mutex_lock(&mutexTimers);
    int contLocalTimer = 0;
    while(1) {
        pthread_mutex_lock(&mutexScheduler);
        
        contLocalTimer++;
        if ((killedThreads < 2) && (contLocalTimer >= maquina.info.calls_SchedulerTick)) {
            pthread_cond_signal(&condScheduler);
            pthread_cond_wait(&condScheduler, &mutexScheduler);
            contLocalTimer = 0;
        }

        pthread_mutex_unlock(&mutexScheduler);

        if (sigKill)
            if (killedThreads >= 2) break;

        //if (sigKill == 1) {
        //    if (killedThreads == 2 && (maquina.info.calls_SchedulerTick <= maquina.info.calls_LoaderTick)) break;
        //    if (killedThreads == 3) break;
        //}

        //if ((killedThreads == 3) || (killedThreads == 2 && maquina.info.calls_LoaderTick <= maquina.info.calls_SchedulerTick))
        //    break;

        contTimers++;
        pthread_cond_signal(&condTimers);
        pthread_cond_wait(&condAB, &mutexTimers);
        
    }

    contTimers++;
    killedThreads++;
    pthread_cond_signal(&condTimers);
    pthread_mutex_unlock(&mutexTimers);
    printf("\nkilledThreads: %d\n", killedThreads);
    printf("\nhTimerScheduler KILLED ---------------------------------------------------------------->\n");
    pthread_exit(NULL);
}

void* hLoader() {
    usleep(200000);             
    pthread_mutex_lock(&mutexLoader);
    int pidActual = -1;
    while(1) {
        if (loaderKill == 0) {
            buscarSiguienteElf();
            if (loaderKill == 0) {
                actualizarNombreProgALeer();
                loadProgram(++pidActual);
                printf("[ Loader ] Programa cargado en memoria (prog%d.elf) y PCB creado (pid %d) ···\n", elfActual, pidActual);
            }
        }

        //if (sigKill) {
        //    if (killedThreads >= 0) break;
        //}

        
        if (sigKill == 1) {
            if (killedThreads == 0 && (maquina.info.calls_LoaderTick >= maquina.info.calls_SchedulerTick)) break;
            if (killedThreads == 0 && (maquina.info.calls_LoaderTick <= maquina.info.calls_SchedulerTick)) break;
            if (killedThreads == 1) break;
        }
        
        //if ((killedThreads == 1) || (sigKill == 1 && maquina.info.calls_SchedulerTick >= maquina.info.calls_LoaderTick))
        //    break;

        //Aqui se acaba todo lo que tengo que hacer, y signaleo que ya he acabado mi tarea/iteracion
        pthread_cond_signal(&condLoader);
        pthread_cond_wait(&condLoader, &mutexLoader);
    }

    killedThreads++;
    pthread_cond_signal(&condLoader);
    pthread_mutex_unlock(&mutexLoader);
    printf("\nkilledThreads: %d\n", killedThreads);
    printf("\nhLoader KILLED ---------------------------------------------------------------->\n");
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
                    if (cmpnode(maquina.cpus[i].cores[j].hilos[k].executing, nullNode) && maquina.cpus[i].cores[j].hilos[k].flag_ocioso == 1 && listas.preparados.size > 0) {
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
    
        //if (sigKill) {
        //    if (killedThreads >= 0) break;
        //}

        
        if (sigKill == 1) {
            if (killedThreads == 0 && (maquina.info.calls_SchedulerTick >= maquina.info.calls_LoaderTick)) break;
            if (killedThreads == 0 && (maquina.info.calls_SchedulerTick <= maquina.info.calls_LoaderTick)) break;
            if (killedThreads == 1) break;
        }
        
        //if ((killedThreads == 1) || (sigKill == 1 && maquina.info.calls_SchedulerTick <= maquina.info.calls_LoaderTick))
        //    break;

        //printlnTodasListas();
        //Aqui se acaba todo lo que tengo que hacer, y signaleo que ya he acabado mi tarea/iteracion
        pthread_cond_signal(&condScheduler);
        pthread_cond_wait(&condScheduler, &mutexScheduler);

    }
    
    killedThreads++;
    pthread_cond_signal(&condScheduler);
    pthread_mutex_unlock(&mutexScheduler);
    printf("\nkilledThreads: %d\n", killedThreads);
    printf("\nhScheduler KILLED ---------------------------------------------------------------->\n");
    pthread_exit(NULL);
}