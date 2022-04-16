#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>

#define BUFSIZE 8

unsigned char physical[10000];
int contPhysical = 0;
char nombreProg[] = { 'p', 'r', 'o', 'g', 'X', 'Y', 'Z', '.', 'e', 'l', 'f'};
int elfActual = -1;

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

void cargarPrograma() {
    int fd, n, i;
    char buf13[13];
    char buf[BUFSIZE];
    unsigned char bb;

    char* ruta = concat("./progs/", nombreProg);

    fd = open(ruta, O_RDONLY);

    if (fd < 0) {
        printf("Error. No se ha podido abrir el programa %s.\n", ruta);
        return;
    }

    read(fd, buf13, 13); read(fd, buf13, 13);

    while ((n = read(fd, buf, BUFSIZE)) > 0) {
        
///////////////////////////////////////////////////////////
        bbyte[0] = buf[0];
        bbyte[1] = buf[1];
        bb = (unsigned char)strtol(bbyte, NULL, 16);
        physical[contPhysical++] = bb;
///////////////////////////////////////////////////////////
        bbyte[0] = buf[2];
        bbyte[1] = buf[3];
        bb = (unsigned char)strtol(bbyte, NULL, 16);
        physical[contPhysical++] = bb;
///////////////////////////////////////////////////////////
        bbyte[0] = buf[4];
        bbyte[1] = buf[5];
        bb = (unsigned char)strtol(bbyte, NULL, 16);
        physical[contPhysical++] = bb;
///////////////////////////////////////////////////////////
        bbyte[0] = buf[6];
        bbyte[1] = buf[7];
        bb = (unsigned char)strtol(bbyte, NULL, 16);
        physical[contPhysical++] = bb;

        printf(" Physical: [%2x][%2x][%2x][%2x] | contPhysical: %d\n", physical[contPhysical-4], physical[contPhysical-3], physical[contPhysical-2], physical[contPhysical-1], contPhysical);
    
        read(fd, buf, 1);
    }

    printf("\n >> Programa cargado en memoria principal ---> %s\n\n", ruta);

    close(fd);
}

int main(int argc, char* argv[]) {

    buscarSiguienteElf();
    actualizarNombreProgALeer();
    cargarPrograma();

    buscarSiguienteElf();
    actualizarNombreProgALeer();
    cargarPrograma();

    buscarSiguienteElf();
    actualizarNombreProgALeer();
    cargarPrograma();

    return 0;
}