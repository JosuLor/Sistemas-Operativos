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

typedef struct pcb {
    int pid;
    int vida;
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
    int calls_ProcessGeneratorTick;
    int calls_SchedulerTick;
    int frec;
} machine_info_t;

typedef struct hilo {
	node_t* executing;
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
