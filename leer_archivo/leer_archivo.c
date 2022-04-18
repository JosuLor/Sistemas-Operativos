#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

#define DIR_BASE_USUARIO 0x400000
#define PTES_MAX 49151
#define BUFSIZE 8
#define TAM_TLB 10

typedef struct mm_ mm_t;
typedef struct status_ status_t;
typedef struct pcb pcb_t;

typedef struct mm_ {
    unsigned char* code;
    unsigned char* data;
    unsigned char* pgb;
} mm_t;

typedef struct tlb_ {
    unsigned int* virtualPage;      // un array de numeros
    unsigned int* physicalPage;     // un array de copias de PTEs
    int* score;                     // un array de numeros
} tlb_t;

typedef struct status_ {
    int* regs;  // 16 registros generales
    int pc;     // PC
    int IR;     // IR
    int PTBR;   // PTBR
    tlb_t tlb;
} status_t;

typedef struct pcb {
    int pid;
    status_t status;
    mm_t mm;
} pcb_t;

unsigned char *pagetable;
unsigned char physical[0xFFFFFF];
int contPhysical = DIR_BASE_USUARIO;
char nombreProg[] = { 'p', 'r', 'o', 'g', 'X', 'Y', 'Z', '.', 'e', 'l', 'f' };
int elfActual = 9;
pcb_t *cola;
status_t STATUS_NULL;
int pidActual = -1;

char* concat(const char *s1, const char *s2) {
    char *s = malloc(strlen(s1) + strlen(s2) + 1);

    strcpy(s, s1); strcat(s, s2);
    
    return s;
}

void actualizarNombreProgALeer() {
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

char numProgActual[3];
char numProgEnBusqueda[3];
int numProgEnBus;
char ab[] = {'B', 'A', '4', '4', '0', '0', 'D', '8'};
char bbyte[2];

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
            printf("Se han ejecutado todos los programas. Apagando maquina...\n");
            // Hacer una especie de Sigkill de la maquina 
            return;
        }

        elfActual = maxMinLocal;
        closedir(directorio);
    }
}

void encolarPCB(pcb_t p) {
    if (pidActual >= 10) {
        printf("Se ha llegado al maximo de la cola de pcbs\n");
        return;
    }

    cola[p.pid] = p;
}

void cargarPrograma() {
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

    // Ocupar PTEs en la tabla de paginas absoluta
    for (i = (0x20EB00 + pagPosible); i < (0x20EB00 + pagPosible + (npags * 4)); i+=4) {
        physical[i] = 0x01;
    }

/*  ------------- Crear PCB ------------- */

    pcb_t *p;// = malloc(sizeof(pcb_t));
    p = malloc(sizeof(pcb_t));
    pidActual++;
    p->pid = pidActual;
    //p->status = STATUS_NULL;

    c1 = physical[(0x20EB00 + pagPosible + 1)];
    c2 = physical[(0x20EB00 + pagPosible + 2)];
    c3 = physical[(0x20EB00 + pagPosible + 3)];

    int numDirCode = 0;
    numDirCode += c1;
    numDirCode = numDirCode << 8;
    numDirCode += c2;
    numDirCode = numDirCode << 8;
    numDirCode += c3;

    printf("\n $$ Contenido del PTE que se ha ocupado: %x %x %x %x $$\n", physical[(0x20EB00 + pagPosible + 0)], c1, c2, c3);
    printf("\n $$ Puntero al codigo del programa en el mm.code: %x, siendo el indice %d (HEX:%x)\n\n", &(physical[numDirCode]), numDirCode, numDirCode);

    p->mm.code = &(physical[numDirCode]);
    numDirCode += dataSeg;
    p->mm.data = &(physical[numDirCode]);
    p->mm.pgb = &(physical[(0x20EB00 + pagPosible)]);
    encolarPCB((*p));

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

    printf("\n >> Programa cargado en memoria principal ---> %s\n\n", ruta);
    printf("Inicio de segmento de instrucciones: %x", textSeg);
    printf("\nInicio de segmento de datos: %x", dataSeg);
    printf("\nTamaño del programa: %d", tamaño);
    printf("\nInstrucciones del programa: %d", tamaño/4);
    printf("\n <--------------------------->\n");

    close(fd);
}

int pte = 0x00400000;   // 00 00 00 00 (3 bytes menos significativos direccion physical)
void inicializarPagetable() {
    int i, ip;

    printf("\n");

    cola = (pcb_t *)malloc(sizeof(pcb_t) * 10);
    
    pagetable = &physical[0x20EB00];

    for (i = 0, ip = 0; i <= (49151 * 4); i+=4, ip++) {
        physical[(0x20EB00 + i+0)] = 0x00;

        // Calcular nueva pagina; romper direccion en 3 bytes (3 unsigned chars)
        int d1, d2, d3;

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
        
        //if (ip < 10)
        //    printf("Debug :: Tabla de paginas configurada :: pagina %d | %x %x %x | PTE: %x\n", ip, physical[(0x20EB00 + i+1)], physical[(0x20EB00 + i+2)], physical[(0x20EB00 + i+3)], pte);

        pte += 256;
    }
}

void printPCBs() {
    int i;

    printf("\n\nInformacion de los PCBs\n==========================\n");
    printf("Direccion de cominezo de Physical: %x\n\n", physical);

    for (i = 0; i < pidActual+1; i++) {
        printf("  >> pid: %d | .code: %x | .data: %x | .pgb %x\n", cola[i].pid, cola[i].mm.code, cola[i].mm.data, cola[i].mm.pgb);
    }
}

void printPhysical() {
    int i;

    printf("\n\nInformacion de Physical\n ..................\n");

    for (i = 0; i < 60; i+=4)
        printf("  >> %2x %2x %2x %2x\n", physical[DIR_BASE_USUARIO + i+0], physical[DIR_BASE_USUARIO + i+1], physical[DIR_BASE_USUARIO + i+2], physical[DIR_BASE_USUARIO + i+3]);

}

int pc;
unsigned char* pcbase;
unsigned int ir = 0;
unsigned char* ptbr;
int* rgs;
tlb_t tlb;
unsigned char c0;
unsigned int r0, r1, r2, r3, dirvirtual, pagvirtual, offset;
unsigned char cl0, cl1, cl2, cl3;
int valorDir = 0;
int flag_exit = 0;

void ejecutarPrograma() {
    int i, j, k, w;

    // Cargar en el hilo hardware toda la informacion necesaria (IR, PTBR, PC, RGs, TLB)
    rgs = malloc(sizeof(int) * 16);
    memset(rgs, 0, sizeof(int) * 16);

    tlb.virtualPage = malloc(sizeof(unsigned int) * TAM_TLB);
    memset(tlb.virtualPage, -1, sizeof(unsigned int) * TAM_TLB);
    
    tlb.physicalPage = malloc(sizeof(unsigned int *) * TAM_TLB);
    memset(tlb.physicalPage, -1, sizeof(unsigned int *) * TAM_TLB);

    tlb.score = malloc(sizeof(int) * TAM_TLB);
    memset(tlb.score, -1, sizeof(int) * TAM_TLB);

    ptbr = cola[pidActual].mm.pgb;
    pc = 0;
    pcbase = cola[pidActual].mm.code;
    flag_exit = 0;

    while(1) {
        // Buscar la instruccion y conseguirla
        /* el pc esta apuntando a la direccion del array con el trozo de codigo del programa */

        c0 = -1;
        r0 = r1 = r2 = dirvirtual = pagvirtual = offset = -1;

        ir = 0;
        ir += (*(pcbase + pc + 0));
        ir = ir << 8;
        ir += (*(pcbase + pc + 1));
        ir = ir << 8;
        ir += (*(pcbase + pc + 2));
        ir = ir << 8;
        ir += (*(pcbase + pc + 3));

        printf("\n ºººº Ejecucion de instruccion ºººº | IR: %8x ", ir);
        
        pc+=4;  // Hasta aqui es la fase de Fetch        
        
        // Ver que tipo de instruccion es (ld, st, add, exit)      // Esto es la fase de Decodificacion, ver el codigo de instruccion y despejar los datos de la misma
        c0 = ir >> 28;

        switch(c0) {
            case 0:
                // Conseguir del IR el registro destino, direccion virtual de origen
                r0 = ir << 4;
                r0 = r0 >> 28;
                dirvirtual = ir << 8;
                dirvirtual = dirvirtual >> 8;
                printf("    >> Instruccion de tipo LD | %x %x(%d) %x |", c0, r0, r0, dirvirtual);
                break;
            case 1:
                // Conseguir del IR el registro origen, direccion virtual de destino
                r0 = ir << 4;
                r0 = r0 >> 28;
                dirvirtual = ir << 8;
                dirvirtual = dirvirtual >> 8;
                printf("    >> Instruccion de tipo ST | %x %x(%d) %x |", c0, r0, r0, dirvirtual);
                break;
            case 2:
                // Conseguir del IR el registro destino, registro origen 1, registro origen 2
                r0 = ir << 4;
                r0 = r0 >> 28;
                r1 = ir << 8;
                r1 = r1 >> 28;
                r2 = ir << 12;
                r2 = r2 >> 28;
                printf("    >> Instruccion de tipo ADD | %x %x(%d) %x(%d) %x(%d)\n", c0, r0, r0, r1, r1, r2, r2);
                break;
            case 0xF:
                // Nada; acabar con la ejecucion en la fase de operacion
                printf("    >> Instruccion de tipo EXIT | %x\n", c0);
                break;
            default:
                printf("    >> Error. Tipo de instruccion no compatible con la arquitectura. | IR: %x\n", ir);
                break;
        }

        // Busqueda de operandos
        /* En el caso de direcciones: usar / crear / actualizar tlb */
        /* En el caso de registros: usar registros generales locales (en esta fase no hace falta hacer nada, el trabajo se hace en la siguiente fase; operar) */
        unsigned int dirObjetivo = 0;
        if (dirvirtual != -1) {
            // Es una instruccion de tipo LD/ST; hay que resolver la direccion virtual a fisica mediante la tlb (o no, si se trata de un miss)
            /* El Byte menos significativo (los ultimos 8 bits) son el offset dentro de la pagina */
            /* El Byte 1 y el Byte 2 (siendo 0 1 2 3, 0 el Byte con mas peso) contienen la pagina virtual, la cual hay que traducir a direccion fisica */
            offset = dirvirtual << 24;
            offset = offset >> 24;
            pagvirtual = dirvirtual >> 8;
            printf(" pagina virtual %x(%d) :: offset %x(10:%d)\n", pagvirtual, pagvirtual, offset, offset);
            /* Pasar pagvirtual por la tlb, buscando la traduccion de la pagina virtual a direccion fisica */
            /* Si es hit, se obtiene directamente la direccion fisica */
            /* Si es miss, se tiene que buscar en la tabla de paginas del pcb y actualizar el tlb */
            int hit = 0;
            int maxscore = -1, minpos = -1;
            int minscore = 0x0FFFFFFF;
            dirObjetivo = 0;
            for (i = 0; i < TAM_TLB; i++) {
                if (tlb.virtualPage[i] == pagvirtual) {
                    /* Hit; mirar el contenido de physicalPage[i]; ahi se encuentra la direccion de physical a la que hay que sumarle el offset para conseguir la direccion physical a la que hay que acceder */
                    hit = 1;
                    // Actualizar las puntuaciones de las PTEs
                    for (k = 0; k < TAM_TLB; k++) {
                        if (tlb.score[k] > maxscore) {
                            maxscore = tlb.score[k];
                        }
                    }
                    maxscore++;
                    tlb.score[i] = maxscore;

                    // Resolver direccion y acceder al valor
                    dirObjetivo = tlb.physicalPage[i] << 8;
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

                    printf("\n              §§ TLB hit §§ se ha referenciado la entrada %d (score %d), con pagina virtual %x y PTE %x §§\n", i, tlb.score[i], tlb.virtualPage[i], tlb.physicalPage[i]);

                    /* en valorDir se tiene el valor de la direccion requesteada */
                }
            }

            if (hit == 0) {
                /* Miss; mirar la tabla de paginas, y actualizar el tlb */
                
                // Elegir PTE victima (con menos score, es decir, la mas antiguamente referenciada)
                for (i = 0; i < TAM_TLB; i++) {
                    if (tlb.score[i] < minscore) {
                        minscore = tlb.score[i];
                        minpos = i;
                    }
                }
                // Actualizar puntuacion de la PTE nueva
                maxscore = -1;
                for (i = 0; i < TAM_TLB; i++)
                    if (tlb.score[i] > maxscore)
                        maxscore = tlb.score[i];

                maxscore++;
                tlb.score[minpos] = maxscore;

                tlb.virtualPage[minpos] = pagvirtual;

                /* Para conseguir el PTE objetivo, coger el puntero a la tabla de paginas del proceso y sumarle (virtualPage * 4) */
                unsigned char* punteroPTE = ptbr + (pagvirtual * 4);
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
                tlb.physicalPage[minpos] = newPTE;

                printf("\n              §§ TLB miss §§ se ha ocupado la entrada %d (score %d) de la tlb, con pagina virtual %x y PTE %x §§ offset dentro de la pagina: %x\n", minpos, tlb.score[minpos], tlb.virtualPage[minpos], tlb.physicalPage[minpos], offset);
                printf("              ====================) minpos de TLB: %d | minscore conseguida: %d (==================== \n\n", minpos, minscore);
                
                // Resolver direccion y acceder al valor
                dirObjetivo = tlb.physicalPage[minpos];
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
                r0 = rgs[r0];
                break;
            case 2:
                r1 = rgs[r1];
                r2 = rgs[r2];
                break;
            case 0xF:
                /* Nada; despues se finaliza la ejecucion de este pcb */
                break;
            default:
                printf("\n$$ $$ $$Esto no se deberia de haber printeado $$ $$ $$\n");
                break;
        }

        // Operar / Comprobar si finalizar ejecucion (break y hilo ocioso)
        if (c0 == 0xF) {
            flag_exit = 1;
            break;
        } else if (c0 == 2) {
            r3 = r1 + r2;
        }
//      Forma guarra y de cuestionable seguridad de comprobar si se ha llegado a la instruccion de exit (F0000000)
//      if ((pcbase + pc) >= cola[pidActual].mm.data)
//          break;
  
        // Dejar resultado
        switch(c0) {
            case 0:
                rgs[r0] = r1;
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
                rgs[r0] = r3;
                break;
            default:
                printf("\n Error. Se ha intentado dejar un resultado de una instruccion sin resultado (codigo de instruccion %x)\n", c0);
                break;
        }

    }

    printf("\n ==== Se ha acabado con la ejecucion del programa ====\n");

    if (flag_exit == 1)  {
        /* Liberar adecuadamente el hilo para que el Scheduler cargue otro programa */

    } else {
        printf("\n flag_exit no se ha activado. A pesar de eso, la ejecucion del programa ha terminado por alguna razon.\n");
    }

}

void memHexDump() {
    int i;
    
    printf(" \n\n====== ======  HEXDUMP DE LA MEMORIA  ====== ======\n\n");

    for (i = 0; i < 60; i+=4)
        printf(" [%x] %2x %2x %2x %2x\n", (0x400000 + i), physical[0x400000 + i + 0], physical[0x400000 + i + 1], physical[0x400000 + i + 2], physical[0x400000 + i + 3]);

    printf("\n");

}

int main(int argc, char* argv[]) {

    inicializarPagetable();

    buscarSiguienteElf();
    actualizarNombreProgALeer();
    cargarPrograma();

    printPCBs();
    printPhysical();

    ejecutarPrograma();
    memHexDump();

    return 0;
}