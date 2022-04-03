#define MAX_LOCAL_QUEUE 3

#define false 0
#define true 1

machine_t maquina;
pthread_t* masterThreads;
coleccion_listas_t listas;

pthread_mutex_t mutexTimers, mutexGenerator, mutexScheduler;
pthread_cond_t condTimers, condAB, condGenerator, condScheduler;

pcb_t* nullProc;
node_t* nullNode;

void* hclock();
void* hTimerProcessGenerator();
void* hTimerScheduler();
void* hProcessGenerator();
void* hScheduler();

void crear_pcb(pcb_t** p, int id);
void crear_node(node_t** n, pcb_t* p);
int cmpnode(node_t* a, node_t* b);

void encolar(node_t* n, tipoLista_e tipo);
node_t* desencolar();

void printLista(tipoLista_e tipo);
void printNode(node_t* n);
void printlnTodasListas();



