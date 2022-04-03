#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "types.h"
#include "defines.h"

void crear_pcb(pcb_t** p, int id) {
    *p          = malloc(sizeof(pcb_t));
    (*p)->pid   = id;
    (*p)->vida  = rand() % 10;

    if (((*p)->vida) < 3)
        (*p)->vida += 3;
}

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

void printLista(tipoLista_e tipo) {
    node_t* sig;
    int i, j = 0;
    switch(tipo) {
        case preparados:
            sig = listas.preparados.first;
            i = 0;
            printf(" ╔═══════════════════════╗\n");
            printf(" ║  Lista de Preparados  ║\n");
            printf(" ║  Size: %d              ║\n", listas.preparados.size);
            printf(" ╠═══════════════════════╣\n");
            while(!cmpnode(sig, nullNode)) {
                printf(" ║   Nodo %d | PID: %d     ║\n", i, sig->data->pid);
                i++;
                sig = sig->next;
            }
            printf(" ╚═══════════════════════╝\n");
            i = 0;
            break;
        case terminated:
            sig = listas.terminated.first;
            i = 0;
            printf(" ╔═══════════════════════╗\n");
            printf(" ║  Lista de Terminated  ║\n");
            printf(" ╠═══════════════════════╣\n");
            while(!cmpnode(sig, nullNode)) {
                printf(" ║   Nodo %d | PID: %d     ║\n", i, sig->data->pid);
                i++;
                sig = sig->next;
            }
            printf(" ╚═══════════════════════╝\n");
            i = 0;
            break;
    }
}

void printNode(node_t* n) {
    if (cmpnode(n, nullNode)) {
        printf(" Nodo nulo\n");
    } else {
        printf(" PID: %d ║ VIDA: %d ║ ", n->data->pid, n->data->vida);
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