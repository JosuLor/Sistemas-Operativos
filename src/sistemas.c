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


int kills = 0;
void* hclock() {
    int i, j, k;
    int cont = 0;
    int contUsage = 0;

    while(1) {
        pthread_mutex_lock(&mutexTimers);   
        contUsage = 0;
        printMachine();
        // si todos los hilos hardware han terminado de ejecutar programas, y el loader ha detectado que ya no hay mas programas a cargar en ./progs/, activar secuencia de finalizacion            
        if (kills == (maquina.info.cpus * maquina.info.cores * maquina.info.threads)) {
            sigKill = 1;   // activar secuencia de muerte
            if (killedThreads == 4)
                break;
        } else {    
            // caso general; ejecutar instrucciones
            for (i = 0; i < maquina.info.cpus; i++) {
                for (j = 0; j < maquina.info.cores; j++) {
                    for (k = 0; k < maquina.info.threads; k++) {
                        if ((maquina.cpus[i].cores[j].hilos[k].flag_ocioso == 1) && (loaderKill == 1) && (listas.preparados.size == 0) && (maquina.cpus[i].cores[j].hilos[k].killed == 0)) {
                            kills++;  // contar cuantos hilos han terminado de trabajar finalmente
                            maquina.cpus[i].cores[j].hilos[k].killed = 1;   // matar hilo hardware
                        }
                        if ((maquina.cpus[i].cores[j].hilos[k].flag_ocioso == 0) && (!cmpnode(maquina.cpus[i].cores[j].hilos[k].executing, nullNode))) {
                            executeProgram(i, j, k);    // ejecutar instrucciones
                            maquina.info.instrExec++;
                            contUsage++;
                            if (contUsage > maquina.info.maxUsage)
                                maquina.info.maxUsage = contUsage;
                        }
                    }
                }
            }
        }

        int delta = rand() % 500;
        int masmenos = rand() % 2;

        if (masmenos == 1) {    // sumar frecuencia generalmente
            if ((maquina.info.frec + delta) > (maquina.info.frecOG + (maquina.info.frecOG * 0.2))) {
                maquina.info.frec -= delta; // frecuencia demasiado alta, restar
            } else {
                maquina.info.frec += delta;
            }
        } else {                // restar frecuencia generalmente
            if ((maquina.info.frec - delta) < (maquina.info.frecOG - (maquina.info.frecOG * 0.2))) {
                maquina.info.frec += delta; // frecuencia demasiado baja, sumar
            } else {
                maquina.info.frec -= delta;
            }
        }

        if (maquina.info.maxFrec < maquina.info.frec)
            maquina.info.maxFrec = maquina.info.frec;
        if (maquina.info.minFrec > maquina.info.frec)
            maquina.info.minFrec = maquina.info.frec;    
        
        cont++;
        //printf("\n%d | Alemania del Este | kills: %d\n", cont, kills);

        // abrir barreras y esperar a que el resto del sistema termine de ejecutarse en esta iteracion 
        while(contTimers<2) {
            if (killedThreads == 4)
                break;
            pthread_cond_wait(&condTimers, &mutexTimers);
        }

        //printf("\n%d | Alemania Occidental | kills: %d\n", cont, killedThreads);
        
        if (killedThreads == 4) 
            break;

        contTimers = 0;
        usleep(maquina.info.frec);

        // A terminado la iteracion actual, habilitar la siguiente iteracion
        pthread_cond_broadcast(&condAB);
        pthread_mutex_unlock(&mutexTimers);
    }

    clear();
    printf("\n\n\n==================================================================================\n\n");
    printf("   >> >> Se han cargado y ejecutado todos los programas de ./progs/ << <<\n\n");
    printf("          >> Se ha detenido el reloj y los flujos de ejecucion  <<\n\n");
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

        // funcion principal del TimerLoader; desbloquear el Loader cada calls_LoaderTick ticks del clock central
        contLocalTimer++;
        if ((killedThreads < 2) && (contLocalTimer >= maquina.info.calls_LoaderTick)) {
            pthread_cond_signal(&condLoader);
            pthread_cond_wait(&condLoader, &mutexLoader);
            contLocalTimer = 0;
        }

        pthread_mutex_unlock(&mutexLoader);

        // si se ha dado la señal de muerte, matar hilo (se sale del while perpetuo, y se hace pthread_exit())
        if (sigKill)
            if (killedThreads >= 2) break;
        
        // se ha hecho todo lo que se tenia que hacer en esta iteracion; se indica que se ha terminado la iteracion y se espera a iniciar la siguiente iteracion
        contTimers++;
        pthread_cond_signal(&condTimers);
        pthread_cond_wait(&condAB, &mutexTimers);
        
    }

    // secuencia de finalizacion del hilo (muerte controlada del hilo)
    contTimers++;
    killedThreads++;
    pthread_cond_signal(&condTimers);
    pthread_mutex_unlock(&mutexTimers);
    //printf("\nhTimerLoader KILLED ---------------------------------------------------------------->\n");
    pthread_exit(NULL);
}

void* hTimerScheduler() {
    usleep(100000);
    pthread_mutex_lock(&mutexTimers);
    int contLocalTimer = 0;
    while(1) {
        pthread_mutex_lock(&mutexScheduler);
        
        // funcion principal del TimerScheduler; desbloquear el Scheduler cada calls_SchedulerTick ticks del clock central
        contLocalTimer++;
        if ((killedThreads < 2) && (contLocalTimer >= maquina.info.calls_SchedulerTick)) {
            pthread_cond_signal(&condScheduler);
            pthread_cond_wait(&condScheduler, &mutexScheduler);
            contLocalTimer = 0;
        }

        pthread_mutex_unlock(&mutexScheduler);

        // si se ha dado la señal de muerte, matar hilo (se sale del while perpetuo, y se hace pthread_exit())
        if (sigKill)
            if (killedThreads >= 2) break;

        // se ha hecho todo lo que se tenia que hacer en esta iteracion; se indica que se ha terminado la iteracion y se espera a iniciar la siguiente iteracion
        contTimers++;
        pthread_cond_signal(&condTimers);
        pthread_cond_wait(&condAB, &mutexTimers);
        
    }

    // secuencia de finalizacion del hilo (muerte controlada del hilo)
    contTimers++;
    killedThreads++;
    pthread_cond_signal(&condTimers);
    pthread_mutex_unlock(&mutexTimers);
    //printf("\nhTimerScheduler KILLED ---------------------------------------------------------------->\n");
    pthread_exit(NULL);
}

void* hLoader() {
    usleep(200000);             
    pthread_mutex_lock(&mutexLoader);
    int pidActual = -1;
    while(1) {
        if (loaderKill == 0) {
            buscarSiguienteElf();   // buscar siguiente programa a cargar en ./progs/
            if (loaderKill == 0) {
                maquina.info.progExec++;
                actualizarNombreProgALeer();    // actualizar el nombre del programa a leer con el nombre del .elf que se ha encontrado
                loadProgram(++pidActual);       // cargar el .elf y crear pcb
                //printf("[ Loader ] Programa cargado en memoria (prog%d.elf) y PCB creado (pid %d) ···\n", elfActual, pidActual);
            }
        }
        
        // si se han cargado todos los programas que hay en ./progs/, matar hilo 
        if (sigKill == 1) {
            if (killedThreads == 0 && (maquina.info.calls_LoaderTick >= maquina.info.calls_SchedulerTick)) break;   // si tardo en saltar mas que el Scheduler, finalizar ya
            if (killedThreads == 1) break;  // si se ha finalizado el thread del Scheduler, finalizar este tambien
        }

        //Aqui se acaba todo lo que tengo que hacer, y signaleo que ya he acabado mi tarea/iteracion
        pthread_cond_signal(&condLoader);
        pthread_cond_wait(&condLoader, &mutexLoader);
    }

    // secuencia de finalizacion del hilo (muerte controlada del hilo)
    killedThreads++;
    pthread_cond_signal(&condLoader);
    pthread_mutex_unlock(&mutexLoader);
    //printf("\nhLoader KILLED ---------------------------------------------------------------->\n");
    pthread_exit(NULL);
}

void* hScheduler() {
    usleep(250000);               
    pthread_mutex_lock(&mutexScheduler);
    int i, j, k;
    while(1) {
        // Sacar pcb's acabados de los hilos y limpiar memoria
        for (i = 0; i < maquina.info.cpus; i++) {
            for (j = 0; j < maquina.info.cores; j++) {
                for (k = 0; k < maquina.info.threads; k++) {
                    if ((maquina.cpus[i].cores[j].hilos[k].flag_ocioso == 1) && !cmpnode(maquina.cpus[i].cores[j].hilos[k].executing, nullNode)) {
                        terminateStatus(i, j, k);                                               // Hacer una "foto finish" del estado del hilo cuando el programa termina
                        encolar(maquina.cpus[i].cores[j].hilos[k].executing, terminated);       // Encolar nodo en la cola de terminados
                        //printf("[Scheduler] Se ha liberado el hilo[%d][%d][%d] del PCB %d\n", i, j, k, maquina.cpus[i].cores[j].hilos[k].executing->data->pid);
                        freeThread(i, j, k);                                                    // Limpiar estructuras del hilo
                        freeProgram(maquina.cpus[i].cores[j].hilos[k].executing);               // Liberar memoria
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
                            //printf("\n[Scheduler Warning] Se ha intentado cargar el nodo nulo en un hilo de ejecucion\n");
                        } else {
                            loadThread(sugerencia, i, j, k);  // Cargar informacion del pcb y programa en el hilo
                            //printf("[Scheduler] PCB cargado en hilo[%d][%d][%d]: PCB PID: %d\n", i, j, k, maquina.cpus[i].cores[j].hilos[k].executing->data->pid);
                        }
                    }
                }
            }
        }
        
        // si se han cargado todos los programas que hay en ./progs/, matar hilo 
        if (sigKill == 1) {
            if (killedThreads == 0 && (maquina.info.calls_SchedulerTick >= maquina.info.calls_LoaderTick)) break;   // si tardo en saltar mas que el Loader, finalizar ya
            if (killedThreads == 1) break; // si se ha finalizado el thread del Loader, finalizar este tambien
        }

        //printlnTodasListas();
        //Aqui se acaba todo lo que tengo que hacer, y signaleo que ya he acabado mi tarea/iteracion
        pthread_cond_signal(&condScheduler);
        pthread_cond_wait(&condScheduler, &mutexScheduler);

    }
    
    // secuencia de finalizacion del hilo (muerte controlada del hilo)
    killedThreads++;
    pthread_cond_signal(&condScheduler);
    pthread_mutex_unlock(&mutexScheduler);
    //printf("\nhScheduler KILLED ---------------------------------------------------------------->\n");
    pthread_exit(NULL);
}