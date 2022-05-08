typedef struct mm_ mm_t;
typedef struct tlb_ tlb_t;
typedef struct status_ status_t;
typedef struct pcb pcb_t;
typedef struct node node_t;
typedef struct lista lista_t;
typedef struct coleccion_listas coleccion_listas_t;

typedef struct hilo hilo_t;
typedef struct core core_t;
typedef struct cpu cpu_t;
typedef struct machine machine_t;
typedef struct machine_info machine_info_t; 

typedef enum tipoLista_e tipoLista_e; 

enum tipoLista_e {
    preparados,
    running,
    bloqueados,
    terminated
};

typedef struct mm_ {
    unsigned char* code;
    unsigned char* data;
    unsigned char* pgb;
    int psize;                      // tamaño del programa en Bytes
} mm_t;

typedef struct tlb_ {
    unsigned int* virtualPage;      // un array de numeros (numero de pagina virtual)
    unsigned int* physicalPage;     // un array de copias de PTEs
    int* score;                     // un array de numeros (puntuaciones de las PTEs)
} tlb_t;

typedef struct status_ {
    int* regs;
    unsigned char* pc;
    int ir;
    unsigned char* ptbr;
    tlb_t* tlb;
} status_t;

typedef struct pcb {
    int pid;
    status_t* status;
    mm_t* mm;
} pcb_t;

typedef struct node {
    node_t* next;
    pcb_t* data;
} node_t;

typedef struct lista {
    node_t* first;
    node_t* last;
    int size;
} lista_t;

typedef struct coleccion_listas {
    lista_t preparados;
    lista_t terminated;
} coleccion_listas_t;

typedef struct machine_info {
    int cpus;
    int cores;
    int threads;
    int calls_LoaderTick;
    int calls_SchedulerTick;
    int frecOG;
    int frec;
    int progExec;
    int instrExec;
    int maxUsage;
    double tej;
    int maxFrec;
    int minFrec;
} machine_info_t;

typedef struct hilo {
	node_t* executing;
    int* rgs;
    unsigned char* pc;
    unsigned int ir;
    unsigned char* ptbr;
    tlb_t *tlb;
    int flag_ocioso;
    int killed;
} hilo_t;

typedef struct core {
	hilo_t* hilos;
} core_t;

typedef struct cpu {
	core_t* cores;
} cpu_t;

typedef struct machine {
	cpu_t* cpus;
    machine_info_t info;
} machine_t;
