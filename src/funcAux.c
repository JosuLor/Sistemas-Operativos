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

char* concat(const char *s1, const char *s2) {
    char *s = malloc(strlen(s1) + strlen(s2) + 1);

    strcpy(s, s1); strcat(s, s2);
    
    return s;
}

/*
// esto tiene que irse
void crear_pcb(pcb_t** p, int id) {
    *p          = malloc(sizeof(pcb_t));
    (*p)->pid   = id;
    (*p)->vida  = rand() % 10;

    if (((*p)->vida) < 3)
        (*p)->vida += 3;
}
*/

void crear_node(node_t** n, pcb_t* p) {
    *n          = malloc(sizeof(node_t));
    (*n)->data  = p;
    (*n)->next  = nullNode;
}

int cmpnode(node_t* a, node_t* b) {
    int pidA = a->data->pid;
    int pidB = b->data->pid;
    if (pidA == pidB)
        return true;
    
    return false;
}

void encolar(node_t* n, tipoLista_e tipo) {
    if (cmpnode(n, nullNode)) {
        printf("ESTO NO SE DEBERIA DE PODER VER");
        return;
    }
    n->next = nullNode;
    switch(tipo) {
        case preparados:
            if (listas.preparados.size == 0) {
                listas.preparados.first = n;
                listas.preparados.last = n;
            } else {
                if (listas.preparados.size == 1)
                    listas.preparados.first->next = n;
                else
                    listas.preparados.last->next = n;
                
                listas.preparados.last = n;
            }
            listas.preparados.size++;
            break;
        case terminated:
            if (listas.terminated.size == 0) {
                listas.terminated.first = n;
                listas.terminated.last = n;
            } else {
                if (listas.terminated.size == 1)
                    listas.terminated.first->next = n;
                else
                    listas.terminated.last->next = n;
                
                listas.terminated.last = n;
            }
            listas.terminated.size++;
            break;
    }
}

node_t* desencolar() {
    if (listas.preparados.size <= 0) {
        return nullNode;
    } else {
        node_t* res = listas.preparados.first;
        if (listas.preparados.size == 1) {
            listas.preparados.first = nullNode;
            listas.preparados.last = nullNode;
        } else {
            listas.preparados.first = listas.preparados.first->next; 
        }
        listas.preparados.size--;
        res->next = nullNode;
        return res;        
    }
}

void actualizarNombreProgALeer() {
    //printf("\n\n\nactualizarNombreProgALeer: elfActual %d | numEnNombre: %c%c%c\n\n\n", elfActual, nombreProg[4], nombreProg[5], nombreProg[6]);
    sprintf(&nombreProg[4], "%ld", elfActual);

    if (elfActual < 10) {
        nombreProg[6] = nombreProg[4];
        nombreProg[5] = '0';
        nombreProg[4] = '0';
    } else if (elfActual > 9 && elfActual < 100) {
        nombreProg[6] = nombreProg[5];
        nombreProg[5] = nombreProg[4];
        nombreProg[4] = '0';
    } else if (elfActual > 99 && elfActual < 1000) {
        /* nada, el sprintf lo coloca ya bien */
    }
}

char numProgEnBusqueda[3];
int numProgEnBus;

void buscarSiguienteElf() {
    DIR *directorio;
    struct dirent *dir_struct;
    directorio = opendir("./progs/");
    int maxMinLocal = 1000;
    if (directorio) {
        while ((dir_struct = readdir(directorio)) != NULL) {
            numProgEnBusqueda[0] = dir_struct->d_name[4];
            numProgEnBusqueda[1] = dir_struct->d_name[5];
            numProgEnBusqueda[2] = dir_struct->d_name[6];
            numProgEnBus = strtol(numProgEnBusqueda, NULL, 10);
            
            if (numProgEnBus < maxMinLocal && numProgEnBus > elfActual) {
                maxMinLocal = numProgEnBus;
            }
        }

        if (maxMinLocal == 1000) {
            printf("Se han ejecutado todos los programas. Esperando a que todos los programas terminen su ejecucion...\n");
            // Hacer una especie de Sigkill de la maquina
            sigkill = 1;
            return;
        }

        elfActual = maxMinLocal;
        //printf("\n====elfActual: %d (HEX: %x)====\n", elfActual, elfActual);
        closedir(directorio);
    }
}

void memHexDump(int palabrasDesdeBase) {
    int i;
    
    printf(" \n\n====== ======  HEXDUMP DE LA MEMORIA  ====== ======\n\n");

    for (i = 0; i < (palabrasDesdeBase*4); i+=4)
        printf(" [%x] %2x %2x %2x %2x\n", (DIR_BASE_USUARIO + i), physical[DIR_BASE_USUARIO + i + 0], physical[DIR_BASE_USUARIO + i + 1], physical[DIR_BASE_USUARIO + i + 2], physical[DIR_BASE_USUARIO + i + 3]);

    printf("\n");
}

void printNode(node_t* n) {
    if (cmpnode(n, nullNode)) {
        printf(" Nodo nulo\n");
    } else {
        printf(" PID: %d ║ ", n->data->pid);
        if (cmpnode(n->next, nullNode)) {
            printf("NEXT NODE: NULL\n");
        } else {
            printf("NEXT PID: %d\n", n->next->data->pid);
        }
    }
}

void printlnTodasListas() {
    node_t* sig;
    printf(" ╔══════════════════════════════════════════════\n");
    printf(" ║ Lista de preparados: ");
    if (listas.preparados.size != 0) {
        sig = listas.preparados.first;
        while(!cmpnode(sig, nullNode)) {
            printf("PID: %d  ", sig->data->pid);
            sig = sig->next;
            if (!cmpnode(sig, nullNode)) 
                printf("⟼   ");
        }
        printf("\n");
    } else {
        printf("Lista vacia\n");
    }
    printf(" ╠══════════════════════════════════════════════\n");
    printf(" ║ Lista de terminados: ");
    if (listas.terminated.size != 0) {
        sig = listas.terminated.first;
        while(!cmpnode(sig, nullNode)) {
            printf("PID: %d  ", sig->data->pid);
            sig = sig->next;
            if (!cmpnode(sig, nullNode)) 
                printf("⟼   ");
        }
        printf("\n");
    } else {
        printf("Lista vacia\n");
    }
    printf(" ╚══════════════════════════════════════════════\n");
}