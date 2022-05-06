#define false 0
#define true 1

#define MAX_LOCAL_QUEUE 3
#define DIR_BASE_USUARIO 0x400000
#define PTES_MAX 49151
#define BUFSIZE 8
#define TAM_TLB 10
#define NUM_RGs 16

extern unsigned char physical[0xFFFFFF];
extern char* nombreProg;
extern int loaderKill;
extern int elfActual;

extern machine_t maquina;
extern pthread_t* masterThreads;
extern coleccion_listas_t listas;

extern pthread_mutex_t mutexTimers, mutexLoader, mutexScheduler;
extern pthread_cond_t condTimers, condAB, condLoader, condScheduler;

extern pthread_cond_t c_kill_scheduler, c_kill_loader;
extern pthread_mutex_t m_kill_scheduler, m_kill_loader;

extern pcb_t* nullProc;
extern node_t* nullNode;
extern tlb_t* nullTlb;
extern mm_t* nullMm;
extern status_t* nullStatus;

extern void* hclock();
extern void* hTimerLoader();
extern void* hTimerScheduler();
extern void* hLoader();
extern void* hScheduler();

extern char* concat(const char *s1, const char *s2);
extern void crear_pcb(pcb_t** p, int id);
extern void crear_node(node_t** n, pcb_t* p);
extern int cmpnode(node_t* a, node_t* b);

extern void encolar(node_t* n, tipoLista_e tipo);
extern node_t* desencolar();

extern void buscarSiguienteElf();
extern void actualizarNombreProgALeer();

extern void loadProgram(int pidActual);
extern void freeProgram(node_t* n);
extern void loadThread(node_t* n, int i, int j, int k);
extern void freeThread(int i, int j, int k);
extern void terminateStatus(int i, int j, int k);

extern void initializePageTable();
extern void executeProgram(int i2, int j2, int k2);

extern void memHexDump(int palabrasDesdeBase);
extern void printNode(node_t* n);
extern void printlnTodasListas();



