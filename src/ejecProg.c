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

void initializePageTable() {
    int i;
    int pte = 0x00400000;   // 00 00 00 00 (3 bytes menos significativos direccion physical)
    int d1, d2, d3;

    for (i = 0; i <= (49151 * 4); i+=4) {
        physical[(0x20EB00 + i+0)] = 0x00;

        // Calcular nueva pagina; romper direccion en 3 bytes (3 unsigned chars)
        d1 = pte << 8;
        d1 = d1 >> 24;
        d2 = pte >> 8;
        d2 = d2 << 24;
        d2 = d2 >> 24;
        d3 = pte << 24;
        d3 = d3 >> 24;

        physical[(0x20EB00 + i+1)] = (unsigned char)d1;
        physical[(0x20EB00 + i+2)] = (unsigned char)d2;
        physical[(0x20EB00 + i+3)] = (unsigned char)d3;
        
        pte += 256;
    }
}

char bbyte[2];

void loadProgram(int pidActual) {
    int fd, n, i, tamaño, textSeg, dataSeg;
    char* ruta = concat("./progs/", nombreProg);
    struct stat st;
    
    char *buf13 = (char *)malloc(sizeof(char) * 13);
    char *buf = (char *)malloc(sizeof(char) * BUFSIZE);
    char *bdir = (char *)malloc(sizeof(char) * 6);
    unsigned char bb;

    fd = open(ruta, O_RDONLY);

    if (fd < 0) {
        printf("Error. No se ha podido abrir el programa %s.\n", ruta);
        int o;
        for (o = 0; o < 20; o++)
            printf("%c", ruta[o]);
        printf("\nnombrePrograma: ");
        for (o = 0; o < 11; o++)
            printf("%c", nombreProg[o]);
        
        printf("\n");
        return;
    }

// Calcular tamaño en bytes que va a necesitar en physical (instrucciones = tamaño / 4, cada pagina de la tabla de paginas hasta 64 instrucciones)
    stat(ruta, &st);
    tamaño = st.st_size;
    tamaño -= 26;
    tamaño = tamaño - (tamaño / 9);
    tamaño = (tamaño / 8) * 4;

// Leer comienzo de instrucciones (siempre 000000)
    read(fd, buf13, 13);
    buf13[12] = '\0';
    printf("\n%s\n", buf13); //write(1, buf13, 13);
    
    // caracteres 6 - 11 tienen la informacion
    bdir[0] = buf13[6]; bdir[1] = buf13[7]; bdir[2] = buf13[8]; bdir[3] = buf13[9]; bdir[4] = buf13[10]; bdir[5] = buf13[11];
    textSeg = (int)strtol(bdir, NULL, 16);
    
// Leer comienzo de datos
    read(fd, buf13, 13);
    buf13[12] = '\0'; 
    printf("%s\n", buf13); //write(1, buf13, 13);
    
    // caracteres 6 - 11 tienen la informacion
    bdir[0] = buf13[6]; bdir[1] = buf13[7]; bdir[2] = buf13[8]; bdir[3] = buf13[9]; bdir[4] = buf13[10]; bdir[5] = buf13[11];
    dataSeg = (int)strtol(bdir, NULL, 16);
    
/*  ------------- Crear tabla de paginas ------------- */
    // Encontrar sitio libre consecutivo suficiente en la tabla de paginas absoluta (mirar el primer byte de las ptes)
    // Cuando se encuentre sitio suficiente, guardar en pcb.pgb la dir de comienzo de mi tabla de paginas local (puntero a pagetable[X])
    // Calcular y guardar donde empieza el segmento de instrucciones en memoria fisica (puntero a physical[X])
    // Calcular y guardar donde empieza el segmento de datos en memoria fisical (puntero a physical[X+Y], siendo Y dataseg)

    // Escanear la tabla de paginas absoluta hasta encontrar "tamaño" bytes seguidos libres (hasta encontrar las paginas libres necesarias)
    int npags = (tamaño / 256) + 1;
    int pagPosible = -1, pagCont = 0;
    unsigned char c0, c1, c2, c3;
    int ip;

    for (i = 0, ip = 0; i <= (49151 * 4); i+=4, ip++) {
        c0 = physical[(0x20EB00 + i+0)];

        if (c0) {
            pagPosible = -1;
            pagCont = 0;
            continue;
        }

        if (!pagCont)
            pagPosible = i;
        
        pagCont++;

        if (pagCont == npags)
            break;
    }

    if (pagPosible == -1) {
        printf("Memoria llena. No hay sitio en memoria en el que cargar el programa. Esperando a que se libere espacio... (a que termine un programa)");
        // Esperar a que el Scheduler limpie algo en la tabla una vez termine un programa en ejecucion 
    }

    printf("\n :: Pagina encontrada :: pagina %d | %x | dir physical: %x\n", ip, (0x20EB00 + pagPosible), &physical[(0x20EB00 + pagPosible)]);

    // Ocupar PTEs en la tabla de paginas
    for (i = (0x20EB00 + pagPosible); i < (0x20EB00 + pagPosible + (npags * 4)); i+=4)
        physical[i] = 0x01;

/*  ------------- Crear PCB y nodo ------------- */

    node_t* localNode;
    pcb_t *p = malloc(sizeof(pcb_t));
    p->pid = pidActual;
    p->status = nullStatus;

    c1 = physical[(0x20EB00 + pagPosible + 1)];
    c2 = physical[(0x20EB00 + pagPosible + 2)];
    c3 = physical[(0x20EB00 + pagPosible + 3)];

    int numDirCode = 0;
    numDirCode += c1;
    numDirCode = numDirCode << 8;
    numDirCode += c2;
    numDirCode = numDirCode << 8;
    numDirCode += c3;

    printf("\n $$ Contenido del PTE que se ha ocupado: %x %x %x %x | numDirCode: %x $$\n", physical[(0x20EB00 + pagPosible + 0)], c1, c2, c3, numDirCode);
    printf("\n $$ Puntero al codigo del programa en el mm.code: %x, siendo el indice %d (HEX:%x)\n\n", &(physical[numDirCode]), numDirCode, numDirCode);

    p->mm = malloc(sizeof(mm_t));
    p->mm->code = &(physical[numDirCode]);
    //printf("\n\nNo hay violacion aun\n\n");
    numDirCode += dataSeg;
    p->mm->data = &(physical[numDirCode]);
    p->mm->pgb = &(physical[(0x20EB00 + pagPosible)]);
    p->mm->psize = tamaño;


    /* Hay que escribir de la direccion conseguida en la tabla de paginas para adelante */
    int contPhysical = numDirCode - dataSeg;

    while ((n = read(fd, buf, BUFSIZE)) > 0) {
/* --------------------------------------------------------- */
        bbyte[0] = buf[0]; bbyte[1] = buf[1];
        bb = (unsigned char)strtol(bbyte, NULL, 16);
        physical[contPhysical++] = bb;
/* --------------------------------------------------------- */
        bbyte[0] = buf[2]; bbyte[1] = buf[3];
        bb = (unsigned char)strtol(bbyte, NULL, 16);
        physical[contPhysical++] = bb;
/* --------------------------------------------------------- */
        bbyte[0] = buf[4]; bbyte[1] = buf[5];
        bb = (unsigned char)strtol(bbyte, NULL, 16);
        physical[contPhysical++] = bb;
/* --------------------------------------------------------- */
        bbyte[0] = buf[6]; bbyte[1] = buf[7];
        bb = (unsigned char)strtol(bbyte, NULL, 16);
        physical[contPhysical++] = bb;

        printf(" Physical: [%2x][%2x][%2x][%2x] | contPhysical: %x\n", physical[contPhysical-4], physical[contPhysical-3], physical[contPhysical-2], physical[contPhysical-1], contPhysical);

        read(fd, buf, 1);   // Leer el '\n' del final de linea
    }

    crear_node(&localNode, p);
    encolar(localNode, preparados);
    printNode(localNode);

    printf("\n <--------------------------->\n");
    printf("\nPrograma cargado en memoria principal ---> %s\n\n", ruta);
    printf("Inicio del segmento de instrucciones: %x", textSeg);
    printf("\nInicio de segmento de datos: %x", dataSeg);
    printf("\nTamaño del programa: %d bytes", tamaño);
    printf("\nTamaño del programa: %d lineas", tamaño/4);
    printf("\n <--------------------------->\n\n");

    close(fd);
}

void loadThread(node_t* n, int i, int j, int k) {
    int w;

    // Cargar hilo general
    maquina.cpus[i].cores[j].hilos[k].executing = n;
    for (w = 0; w < NUM_RGs; w++)
        maquina.cpus[i].cores[j].hilos[k].rgs[w] = 0;
    maquina.cpus[i].cores[j].hilos[k].pc = n->data->mm->code;
    maquina.cpus[i].cores[j].hilos[k].ir = 0;
    maquina.cpus[i].cores[j].hilos[k].ptbr = n->data->mm->pgb;
    maquina.cpus[i].cores[j].hilos[k].flag_ocioso = 0;

    // Limpiar TLB
    for (w = 0; w < TAM_TLB; w++) {
        maquina.cpus[i].cores[j].hilos[k].tlb->virtualPage[w] = -1;
        maquina.cpus[i].cores[j].hilos[k].tlb->physicalPage[w] = -1;
        maquina.cpus[i].cores[j].hilos[k].tlb->score[w] = -1;
    }
}

void freeThread(int i, int j, int k) {
    int w = 0;

    // Limpiar hilo general
    maquina.cpus[i].cores[j].hilos[k].executing = nullNode; 
    for (w = 0; w < NUM_RGs; w++)
        maquina.cpus[i].cores[j].hilos[k].rgs[w] = 0;
    maquina.cpus[i].cores[j].hilos[k].pc = NULL;
    maquina.cpus[i].cores[j].hilos[k].ir = -1;
    maquina.cpus[i].cores[j].hilos[k].ptbr = NULL;
    maquina.cpus[i].cores[j].hilos[k].flag_ocioso = 1;

    // Limpiar TLB
    for (w = 0; w < TAM_TLB; w++) {
        maquina.cpus[i].cores[j].hilos[k].tlb->virtualPage[w] = -1;
        maquina.cpus[i].cores[j].hilos[k].tlb->physicalPage[w] = -1;
        maquina.cpus[i].cores[j].hilos[k].tlb->score[w] = -1;
    }

}

void terminateStatus(int i, int j, int k) {
    int w;

    // Mirar memcpy para que quede mejor
    for (w = 0; w < NUM_RGs; w++)
        maquina.cpus[i].cores[j].hilos[k].executing->data->status->regs[w] = maquina.cpus[i].cores[j].hilos[k].rgs[w];
    
    for (w = 0; w < TAM_TLB; w++) {
        maquina.cpus[i].cores[j].hilos[k].executing->data->status->tlb->virtualPage[w] = maquina.cpus[i].cores[j].hilos[k].tlb->virtualPage[w];
        maquina.cpus[i].cores[j].hilos[k].executing->data->status->tlb->physicalPage[w] = maquina.cpus[i].cores[j].hilos[k].tlb->physicalPage[w];
        maquina.cpus[i].cores[j].hilos[k].executing->data->status->tlb->score[w] = maquina.cpus[i].cores[j].hilos[k].tlb->score[w];
    }

    maquina.cpus[i].cores[j].hilos[k].executing->data->status->pc = maquina.cpus[i].cores[j].hilos[k].pc;
    maquina.cpus[i].cores[j].hilos[k].executing->data->status->ir = maquina.cpus[i].cores[j].hilos[k].ir;
    maquina.cpus[i].cores[j].hilos[k].executing->data->status->ptbr = maquina.cpus[i].cores[j].hilos[k].ptbr;
}

int i, j, k, w, flag_exit = 0, valorDir;
unsigned int r0, r1, r2, r3, dirvirtual, pagvirtual, offset;
unsigned char c0, cl0, cl1, cl2, cl3;
void executeProgram(int i2, int j2, int k2) {

    c0 = -1;
    r0 = r1 = r2 = r3 = dirvirtual = pagvirtual = offset = -1;

    // Fase de Fetch. Buscar la instruccion y conseguirla
    /* el pc esta apuntando a la direccion del array physical con el trozo de codigo (la palabra objetivo actual) del programa */
    maquina.cpus[i2].cores[j2].hilos[k2].ir = 0;
    maquina.cpus[i2].cores[j2].hilos[k2].ir += *(maquina.cpus[i2].cores[j2].hilos[k2].pc + 0);
    maquina.cpus[i2].cores[j2].hilos[k2].ir = maquina.cpus[i2].cores[j2].hilos[k2].ir << 8;
    maquina.cpus[i2].cores[j2].hilos[k2].ir += *(maquina.cpus[i2].cores[j2].hilos[k2].pc + 1);
    maquina.cpus[i2].cores[j2].hilos[k2].ir = maquina.cpus[i2].cores[j2].hilos[k2].ir << 8;
    maquina.cpus[i2].cores[j2].hilos[k2].ir += *(maquina.cpus[i2].cores[j2].hilos[k2].pc + 2);
    maquina.cpus[i2].cores[j2].hilos[k2].ir = maquina.cpus[i2].cores[j2].hilos[k2].ir << 8;
    maquina.cpus[i2].cores[j2].hilos[k2].ir += *(maquina.cpus[i2].cores[j2].hilos[k2].pc + 3);

    maquina.cpus[i2].cores[j2].hilos[k2].pc += 4;

    printf("\n ºººº Ejecucion de instruccion ºººº | IR: %x | PC: %x | Contenido de lo que apunta el PC: %x\n", maquina.cpus[i2].cores[j2].hilos[k2].ir, maquina.cpus[i2].cores[j2].hilos[k2].pc-4, *(maquina.cpus[i2].cores[j2].hilos[k2].pc-4));
        
    // Fase de Decodificacion. Ver que tipo de instruccion es (ld, st, add, exit) y despejar los datos de la misma
    c0 = maquina.cpus[i2].cores[j2].hilos[k2].ir >> 28;

    switch(c0) {
        case 0:
            // Conseguir del IR el registro destino, direccion virtual de origen
            r0 = maquina.cpus[i2].cores[j2].hilos[k2].ir << 4;
            r0 = r0 >> 28;
            dirvirtual = maquina.cpus[i2].cores[j2].hilos[k2].ir << 8;
            dirvirtual = dirvirtual >> 8;
            printf("    >> Instruccion de tipo LD | %x %x(%d) %x |", c0, r0, r0, dirvirtual);
            break;
        case 1:
            // Conseguir del IR el registro origen, direccion virtual de destino
            r0 = maquina.cpus[i2].cores[j2].hilos[k2].ir << 4;
            r0 = r0 >> 28;
            dirvirtual = maquina.cpus[i2].cores[j2].hilos[k2].ir << 8;
            dirvirtual = dirvirtual >> 8;
            printf("    >> Instruccion de tipo ST | %x %x(%d) %x |", c0, r0, r0, dirvirtual);
            break;
        case 2:
            // Conseguir del IR el registro destino, registro origen 1, registro origen 2
            r0 = maquina.cpus[i2].cores[j2].hilos[k2].ir << 4;
            r0 = r0 >> 28;
            r1 = maquina.cpus[i2].cores[j2].hilos[k2].ir << 8;
            r1 = r1 >> 28;
            r2 = maquina.cpus[i2].cores[j2].hilos[k2].ir << 12;
            r2 = r2 >> 28;
            printf("    >> Instruccion de tipo ADD | %x %x(%d) %x(%d) %x(%d)\n", c0, r0, r0, r1, r1, r2, r2);
            break;
        case 0xF:
            // Nada; acabar con la ejecucion en la fase de operacion
            printf("    >> Instruccion de tipo EXIT | %x\n", c0);
            break;
        default:
            printf("    >> Error. Tipo de instruccion no compatible con la arquitectura. | IR: %x\n", maquina.cpus[i2].cores[j2].hilos[k2].ir);
            break;
    }

    // Busqueda de operandos
    /* En el caso de direcciones: usar / crear / actualizar tlb */
    /* En el caso de registros: usar registros generales locales (en esta fase no hace falta hacer nada, el trabajo se hace en la siguiente fase; operar) */
    unsigned int dirObjetivo = 0;
    if (dirvirtual != -1) {
        // Es una instruccion de tipo LD/ST; hay que resolver la direccion virtual a fisica mediante la tlb (o hacer algo mas, si se trata de un miss)
        /* El Byte menos significativo (los ultimos 8 bits) son el offset dentro de la pagina */
        /* El Byte 1 y el Byte 2 (siendo 0 1 2 3, 0 el Byte con mas peso) contienen la pagina virtual, la cual hay que traducir a direccion fisica */
        offset = dirvirtual << 24;
        offset = offset >> 24;
        pagvirtual = dirvirtual >> 8;
        printf(" pagina virtual %x(%d) :: offset %x(10:%d)\n", pagvirtual, pagvirtual, offset, offset);
        /* Pasar pagvirtual por la tlb, buscando la traduccion de la pagina virtual a direccion fisica */
        /* Si es hit, se obtiene directamente la direccion fisica de la tlb */
        /* Si es miss, se tiene que buscar en la tabla de paginas del pcb y actualizar el tlb */
        int hit = 0;
        int maxscore = -1, minpos = -1;
        int minscore = 0x0FFFFFFF;
        dirObjetivo = 0;
        for (i = 0; i < TAM_TLB; i++) {
            if (maquina.cpus[i2].cores[j2].hilos[k2].tlb->virtualPage[i] == pagvirtual) {
                /* Hit; mirar el contenido de physicalPage[i]; ahi se encuentra la direccion de physical a la que hay que sumarle el offset para conseguir la direccion physical a la que hay que acceder */
                hit = 1;
                // Actualizar las puntuaciones de las PTEs
                for (k = 0; k < TAM_TLB; k++) {
                    if (maquina.cpus[i2].cores[j2].hilos[k2].tlb->score[k] > maxscore) {
                        maxscore = maquina.cpus[i2].cores[j2].hilos[k2].tlb->score[k];
                    }
                }
                maxscore++;
                maquina.cpus[i2].cores[j2].hilos[k2].tlb->score[i] = maxscore;

                // Resolver direccion y acceder al valor
                dirObjetivo = maquina.cpus[i2].cores[j2].hilos[k2].tlb->physicalPage[i] << 8;
                dirObjetivo = dirObjetivo >> 8;
                dirObjetivo += offset;

                cl0 = physical[dirObjetivo + 0];
                cl1 = physical[dirObjetivo + 1];
                cl2 = physical[dirObjetivo + 2];
                cl3 = physical[dirObjetivo + 3];

                valorDir = 0;
                valorDir += cl0;
                valorDir = valorDir << 8;
                valorDir += cl1;
                valorDir = valorDir << 8;
                valorDir += cl2;
                valorDir = valorDir << 8;
                valorDir += cl3;

                printf("\n              §§ TLB hit §§ se ha referenciado la entrada %d (score %d), con pagina virtual %x y PTE %x §§\n", i, maquina.cpus[i2].cores[j2].hilos[k2].tlb->score[i], maquina.cpus[i2].cores[j2].hilos[k2].tlb->virtualPage[i], maquina.cpus[i2].cores[j2].hilos[k2].tlb->physicalPage[i]);

                /* en valorDir se tiene el valor de la direccion requesteada */
            }
        }

        if (hit == 0) {
            /* Miss; mirar la tabla de paginas, y actualizar el tlb */
                
            // Elegir PTE victima (con menos score, es decir, la mas antiguamente referenciada)
            for (i = 0; i < TAM_TLB; i++) {
                if (maquina.cpus[i2].cores[j2].hilos[k2].tlb->score[i] < minscore) {
                    minscore = maquina.cpus[i2].cores[j2].hilos[k2].tlb->score[i];
                    minpos = i;
                }
            }
            // Actualizar puntuacion de la PTE nueva
            maxscore = -1;
            for (i = 0; i < TAM_TLB; i++)
                if (maquina.cpus[i2].cores[j2].hilos[k2].tlb->score[i] > maxscore)
                    maxscore = maquina.cpus[i2].cores[j2].hilos[k2].tlb->score[i];

            maxscore++;
            maquina.cpus[i2].cores[j2].hilos[k2].tlb->score[minpos] = maxscore;

            maquina.cpus[i2].cores[j2].hilos[k2].tlb->virtualPage[minpos] = pagvirtual;

            /* Para conseguir el PTE objetivo, coger el puntero a la tabla de paginas del proceso y sumarle (virtualPage * 4) */
            unsigned char* punteroPTE = maquina.cpus[i2].cores[j2].hilos[k2].ptbr + (pagvirtual * 4);
            unsigned int newPTE = 0;
            newPTE += *(punteroPTE + 0);
            newPTE = newPTE << 8;
            newPTE += *(punteroPTE + 1);
            newPTE = newPTE << 8;
            newPTE += *(punteroPTE + 2);
            newPTE = newPTE << 8;
            newPTE += *(punteroPTE + 3);
            newPTE = newPTE << 8;
            newPTE = newPTE >> 8;
            maquina.cpus[i2].cores[j2].hilos[k2].tlb->physicalPage[minpos] = newPTE;

            printf("\n              §§ TLB miss §§ se ha ocupado la entrada %d (score %d) de la tlb, con pagina virtual %x y PTE %x §§ offset dentro de la pagina: %x\n", minpos, maquina.cpus[i2].cores[j2].hilos[k2].tlb->score[minpos], maquina.cpus[i2].cores[j2].hilos[k2].tlb->virtualPage[minpos], maquina.cpus[i2].cores[j2].hilos[k2].tlb->physicalPage[minpos], offset);
            printf("              ====================) minpos de TLB: %d | minscore conseguida: %d (==================== \n\n", minpos, minscore);
            
            // Resolver direccion y acceder al valor
            dirObjetivo = maquina.cpus[i2].cores[j2].hilos[k2].tlb->physicalPage[minpos];
            dirObjetivo += offset;

            cl0 = physical[dirObjetivo + 0];
            cl1 = physical[dirObjetivo + 1];
            cl2 = physical[dirObjetivo + 2];
            cl3 = physical[dirObjetivo + 3];

            valorDir = 0;
            valorDir += cl0;
            valorDir = valorDir << 8;
            valorDir += cl1;
            valorDir = valorDir << 8;
            valorDir += cl2;
            valorDir = valorDir << 8;
            valorDir += cl3;

            /* en valorDir se tiene el valor de la direccion requesteada */
            printf(" dirObjetivo: %x | contenido (valor) de la direccion: %x (10:%d)\n", dirObjetivo, valorDir, valorDir);
        }
    }

    switch(c0) {
        case 0:
            r1 = valorDir;
            break;
        case 1:
            r0 = maquina.cpus[i2].cores[j2].hilos[k2].rgs[r0];
            break;
        case 2:
            r1 = maquina.cpus[i2].cores[j2].hilos[k2].rgs[r1];
            r2 = maquina.cpus[i2].cores[j2].hilos[k2].rgs[r2];
            break;
        case 0xF:
            /* Nada; despues se finaliza la ejecucion de este pcb */
            break;
        default:
            printf("\n$$ $$ $$Esto no se deberia de haber printeado $$ $$ $$\n");
            break;
    }

    // Fase de Operar. Operar y comprobar si finalizar la ejecucion del programa
    if (c0 == 0xF) {
        //printf("\n\n\n\n\n\n\n Alemania del Este | flag_exit activado con IR: %x\n", maquina.cpus[i2].cores[j2].hilos[k2].ir);
        flag_exit = 1;
    } else if (c0 == 2) {
        r3 = r1 + r2;
    }
  
    // Fase de Resultados. Dejar resultado en registros o memoria
    switch(c0) {
        case 0:
            maquina.cpus[i2].cores[j2].hilos[k2].rgs[r0] = r1;
            break;
        case 1:
            /* Dejar en physical el valor de r0 */
            printf(" \n              >> st %x, %x\n", r0, dirObjetivo);
            int r00, r01, r02, r03;
            r00 = r01 = r02 = r03 = r0;
            r00 = r00 >> 24;
            physical[dirObjetivo + 0] = r00;
            r01 = r01 << 8;
            r01 = r01 >> 24;
            physical[dirObjetivo + 1] = r01;
            r02 = r02 << 16;
            r02 = r02 >> 24;
            physical[dirObjetivo + 2] = r02;
            r03 = r03 << 24;
            r03 = r03 >> 24;
            physical[dirObjetivo + 3] = r03;
            break;
        case 2:
            maquina.cpus[i2].cores[j2].hilos[k2].rgs[r0] = r3;
            break;
        case 0xF:
            printf("    >> Instruccion de tipo EXIT | No se va a operar\n");
            break;
        default:
            printf("\n Error. Se ha intentado dejar un resultado de una instruccion sin resultado (codigo de instruccion %x)\n", c0);
            break;
    }


    if (flag_exit == 1)  {
        /* Liberar adecuadamente el hilo para que el Scheduler cargue otro programa */
        printf("\n\n\n\n ==== Se ha acabado con la ejecucion del programa con PID %d====\n\n\n\n", maquina.cpus[i2].cores[j2].hilos[k2].executing->data->pid);
        maquina.cpus[i2].cores[j2].hilos[k2].flag_ocioso = 1;
        flag_exit = 0;
    }

}

void freeProgram(node_t* n) {
    unsigned char* i;
    int npags = (n->data->mm->psize / 256);
    unsigned char* ultimaDir = (n->data->mm->code + n->data->mm->psize);

    // Limpiar tabla de paginas
    for (i = n->data->mm->pgb; i < (n->data->mm->pgb + (npags*4)); i+=4)
        (*i) = 0x00;

    // Limpiar zona de memoria del usuario
    for (i = n->data->mm->code; i < ultimaDir; i++)
        (*i) = 0x00;    
        
    //physical[i] = 0x00;
}