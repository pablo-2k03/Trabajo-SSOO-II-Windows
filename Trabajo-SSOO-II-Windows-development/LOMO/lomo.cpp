#include <stdio.h>
#include <ctype.h>
#include <Windows.h>
#include "lomo2.h"
#include <time.h>
#include <thread>
#define DEBUG
#define MAX_NTRENES 100
//Punteros a funciones de la biblioteca de enlazado dinamico.
typedef int(*tipoLomo_Inicio)(int, int, char const*, char const*);
typedef int(*tipoLomo_Generar_Mapa)(char const*, char const*);
typedef int(*tipoLomo_TrenNuevo)(void);
typedef int(*tipoLomo_PeticionAvance)(int nuevoTren, int* xcab, int* ycab);
typedef int(*tipoLomo_Avance)(int nt, int* xcola, int* ycola);
typedef char*(*tipoLomo_GetColor)(int nt);
typedef void(*tipoLomo_LomoEspera)(int y, int yn);
typedef int(*tipoLomo_LomoFin)(void);
typedef void(*tipoLomo_ponError)(char* mensaje);

//Funciones de comprobacion de argumentos y manejadora.
int comprobarPrimerArgumento(char* argv);
int comprobarSegundoArgumento(char* argv);
int comprobarTercerArgumento(char* argv);
BOOL WINAPI manejadora(DWORD param);
DWORD WINAPI receiveThreadMessage(LPVOID param);

struct {
    HINSTANCE libreria;
    int nTrenes;
    int tamMax;
    HANDLE hTrenes[MAX_NTRENES];
    int idTrenes[MAX_NTRENES];
    HANDLE hMutex;
    int matrix[75][17];
}recursosIPCS;

typedef struct {
    int tipo;
}tipoMensaje;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: lomo --mapa\n");
        fprintf(stderr, "Other Usage: lomo retardo longMax nTrenes\n");
        return -1;
    }
    recursosIPCS.libreria = LoadLibrary(TEXT("lomo2.dll"));
    if (recursosIPCS.libreria == NULL) {
        fprintf(stderr, "La biblioteca no se ha cargado.\n");
        return -2;
    }
    else {
        if (argc == 2 && !strcmp(argv[1], "--mapa")) {
            tipoLomo_Generar_Mapa puntLomoGenerarMapa;
            if ((puntLomoGenerarMapa = (tipoLomo_Generar_Mapa)GetProcAddress(recursosIPCS.libreria, "LOMO_generar_mapa")) == NULL) {
                fprintf(stderr, "No se ha podido generar el mapa.\n");
                return -3;
            }
            else {
                puntLomoGenerarMapa("i0959394", "i0919297");
                fprintf(stderr, "Mapa generado correctamente.\n");
                return 1;
            }
        }
        if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL) {
            fprintf(stderr, "Ha dejado algun argumento vacio, por favor, rellenelo.\n");
            return -1;
        }
        if (comprobarPrimerArgumento(argv[1]) != -1 && comprobarSegundoArgumento(argv[2]) != -1 && comprobarTercerArgumento(argv[3]) != -1) {
            //Creacion del mapa de memoria compartida.
            TCHAR memNombre[] = TEXT("MemoriaCompartida");
            HANDLE hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(recursosIPCS.matrix), memNombre);
            if (hFileMap == NULL) {
                fprintf(stderr, "Error en la creacion de la memoria mapeada.\n");
                return -1;
            }
            //Creación de la matriz de 75 filas y 17 columnas en memoria compartida.
            int* pointer = (int*)MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(recursosIPCS.matrix));
            if (pointer == NULL) {
                fprintf(stderr, "Error en la creacion de la matriz.\n");
                return -1;
            }
            //Inicializacion de la matriz.
            int i;
            memset(pointer, 0, sizeof(recursosIPCS.matrix));
            recursosIPCS.nTrenes = atoi(argv[3]);
            recursosIPCS.tamMax = atoi(argv[2]);
            if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)manejadora, TRUE)) {
                fprintf(stderr, "Error en la funcion manejadora.\n");
            }
            fprintf(stderr, "\n%d\n", atoi(argv[1]));
            //comprobar que argv1 es mayor que 0.
            if (atoi(argv[1]) == -1) {
                fprintf(stderr, "El primer argumento debe ser mayor que 0.\n");
                return 1;
            }
            //comprobar que argv2 es numerico.
            if ((recursosIPCS.nTrenes = comprobarTercerArgumento(argv[3])) == -1) {
                fprintf(stderr, "El segundo argumento debe ser un numero.\n");
                return 1;
            }
            TCHAR mutexName[] = TEXT("Mutex");
            recursosIPCS.hMutex = CreateMutex(NULL, true, mutexName);
            tipoLomo_Inicio puntInicioLomo;
            if ((puntInicioLomo = (tipoLomo_Inicio)GetProcAddress(recursosIPCS.libreria, "LOMO_inicio")) == NULL) {
                printf("Error obteniendo el puntero de LOMO_inicio.\n");
                return -3;
            }
            
            if ((puntInicioLomo(atoi(argv[1]), recursosIPCS.tamMax, "i0959394", "i0919297")) == -1) {
                fprintf(stderr, "Error en lomo inicio.\n");
                return -4;
            }
            int x = 1;
            LPDWORD l;
            for (i = 0; i < recursosIPCS.nTrenes; i++) {
                recursosIPCS.hTrenes[i] = CreateThread(NULL, 0, receiveThreadMessage, (LPVOID)pointer, 0, (LPDWORD)&l);
                recursosIPCS.idTrenes[i] = i;
                if (recursosIPCS.hTrenes[i] == NULL) {
                    fprintf(stderr, "Error al obtener el HANDLE de los hilos.\n");
                    return -5;
                }
            }
            WaitForMultipleObjects(recursosIPCS.nTrenes, recursosIPCS.hTrenes, true, INFINITE);
            UnmapViewOfFile(pointer);
            CloseHandle(hFileMap);
            FreeLibrary(recursosIPCS.libreria);
            CloseHandle(recursosIPCS.hMutex);
            for(int i = 0; i< recursosIPCS.nTrenes; i++)
                CloseHandle(recursosIPCS.hTrenes[i]);
            return 0;
        }
        else {
            fprintf(stderr, "Introduzca unos argumentos validos.\n");
            FreeLibrary(recursosIPCS.libreria);
            return -1;
        }
    }
}

int comprobarPrimerArgumento(char* argv) {
    if (atoi(argv) < 0 || comprobarTercerArgumento(argv) == -1)
        return -1;

    return atoi(argv);

}

int comprobarSegundoArgumento(char* argv) {

    if (atoi(argv) < 3 || atoi(argv) > 19 || comprobarTercerArgumento(argv) == -1) {
        return -1;
    }
    return atoi(argv);
}


int comprobarTercerArgumento(char* argv) {
    int lenArgv = strlen(argv);
    int i;
    for (i = 0; i < lenArgv; i++) {
        if (!isdigit(argv[i]))
            return -1;
    }
    if (atoi(argv) < 0 || atoi(argv) > 100)
        return -1;

    return atoi(argv);
}

BOOL WINAPI manejadora(DWORD param) {
    switch (param) {
    case CTRL_C_EVENT:
        exit(0);
    default:
        return FALSE;
    }
}
DWORD WINAPI   receiveThreadMessage(LPVOID param) {

    int* punteroMem = (int*)param;
    int estaInterbloqueado = 0;
    //Sincronizacion y movimiento de trenes.
    int i = (int)param;
    tipoLomo_TrenNuevo puntLomoTrenNuevo;
    int id;
    if ((puntLomoTrenNuevo = (tipoLomo_TrenNuevo)GetProcAddress(recursosIPCS.libreria, "LOMO_trenNuevo")) == NULL) {
        printf("Error obteniendo el puntero de LOMO_tren_nuevo.\n");
        return -3;
    }
    id = puntLomoTrenNuevo();
    if (id == -1) {
        fprintf(stderr, "Error en lomo tren nuevo.\n");
        return -4;
    }
    tipoLomo_PeticionAvance punteroPeticionAvance;
    tipoLomo_Avance punteroAvance;
    tipoLomo_LomoEspera punteroEspera;
    tipoLomo_GetColor punteroColor;
    int xCab, yCab;
    int xCola=0, yCola;
    int posAnterior;
    time_t start_time = time(NULL); // Tiempo actual
    time_t end_time = start_time + 60; // Tiempo en un minuto
    TCHAR mutexName[] = TEXT("Mutex");
    int casillaOcupada = 1;
    int casillaLibre = 0;
    while (time(NULL) < end_time) {
        
        punteroPeticionAvance = (tipoLomo_PeticionAvance)GetProcAddress(recursosIPCS.libreria, "LOMO_peticiOnAvance");
        if (punteroPeticionAvance == NULL) {
            printf("Error obteniendo el puntero de LOMO_peticionAvance.\n");
            return -3;
        }
        punteroPeticionAvance(id, &xCab, &yCab);
        posAnterior = yCab;
        

        //Compruebas que la posicion de la matriz de memoria compartida no esta ocupada.
        if (*(punteroMem + xCab * 17 + yCab) == 0) {
            
            //Si no esta ocupada, la ocupas.
            OpenMutex(1, true, mutexName);
            CopyMemory(punteroMem + xCab * 17 + yCab, &casillaOcupada, sizeof(int));
            ReleaseMutex(recursosIPCS.hMutex);
            //Avanzas.
            punteroAvance = (tipoLomo_Avance)GetProcAddress(recursosIPCS.libreria, "LOMO_avance");
            if (punteroAvance == NULL) {
                ReleaseMutex(recursosIPCS.hMutex);
                printf("Error obteniendo el puntero de LOMO_avance.\n");
                return -3;
            }
            
            punteroAvance(id, &xCola, &yCola);
            //Si esta ocupada, esperas.
            punteroEspera = (tipoLomo_LomoEspera)GetProcAddress(recursosIPCS.libreria, "LOMO_espera");
            if (punteroEspera == NULL) {
                printf("Error obteniendo el puntero de LOMO_espera.\n");
                ReleaseMutex(recursosIPCS.hMutex);
                return -3;
            }
            //coordenada y e y de la siguiente.
            punteroEspera(posAnterior, yCab);

            
            //Desocupas la casilla anterior liberando la cola del tren.
            if (xCola >= 0 && yCola >= 0) {
                OpenMutex(1, true, mutexName);
                CopyMemory(punteroMem + xCola * 17 + yCola, &casillaLibre, sizeof(int));
                ReleaseMutex(recursosIPCS.hMutex);
            }
             
        }
        else {
            //Si esta ocupada, esperas.
            punteroEspera = (tipoLomo_LomoEspera)GetProcAddress(recursosIPCS.libreria, "LOMO_espera");
            if (punteroEspera == NULL) {
                printf("Error obteniendo el puntero de LOMO_espera.\n");
                return -3;
            }
            //coordenada y e y de la siguiente.
            punteroEspera(posAnterior, yCab);
            if (estaInterbloqueado) {
                 punteroColor = (tipoLomo_GetColor)GetProcAddress(recursosIPCS.libreria, "LOMO_getColor");
                 if (punteroColor == NULL) {
                     return 1;
                 }
                 char* color = punteroColor(id);
                 fprintf(stderr, "%s\t", color);
            }
        }

    }
    return 1;
}
