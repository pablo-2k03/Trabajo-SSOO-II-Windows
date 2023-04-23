#include <stdio.h>
#include <ctype.h>
#include <Windows.h>
#include "lomo2.h"
#include <time.h>
#define MAX_NTRENES 100
//Punteros a funciones de la biblioteca de enlazado dinamico.
typedef int(*tipoLomo_Inicio)(int, int, char const*, char const*);
typedef int(*tipoLomo_Generar_Mapa)(char const*, char const*);
typedef int(*tipoLomo_TrenNuevo)(void);
typedef int(*tipoLomo_PeticionAvance)(int nuevoTren,int *xcab,int *ycab);
typedef int(*tipoLomo_Avance)(int nt, int* xcola, int* ycola);
typedef char(*tipoLomo_GetColor)(int nt);
typedef void(*tipoLomo_LomoEspera)(int y,int yn);
typedef int(*tipoLomo_LomoFin)(void);
typedef void(*tipoLomo_ponError)(char* mensaje);

//Funciones de comprobación de argumentos y manejadora.
int comprobarPrimerArgumento(char* argv);
int comprobarSegundoArgumento(char* argv);
int comprobarTercerArgumento(char* argv);
BOOL WINAPI manejadora(DWORD param);
DWORD WINAPI receiveThreadMessage(LPVOID param);


 struct {
    int nTrenes;
    int tamMax;
    HANDLE hTrenes[MAX_NTRENES];
}recursosIPCS;

 typedef struct {
     long tipo; //A que casilla va a ir.
 }tipoMensaje;

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: lomo --mapa\n");
        fprintf(stderr, "Other Usage: lomo retardo longMax nTrenes\n");
        return -1;
    }
    HINSTANCE libreria = LoadLibrary(TEXT("lomo2.dll"));
    if (libreria == NULL) {
        fprintf(stderr, "La biblioteca no se ha cargado.\n");
        return -2;
    }
    else {
        if (argc == 2 && !strcmp(argv[1], "--mapa")) {
            tipoLomo_Generar_Mapa puntLomoGenerarMapa;
            if ((puntLomoGenerarMapa = (tipoLomo_Generar_Mapa)GetProcAddress(libreria, "LOMO_generar_mapa")) == NULL) {
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
            fprintf(stderr,"Ha dejado algun argumento vacio, por favor, rellenelo.\n");
            return -1; 
        }
        if (comprobarPrimerArgumento(argv[1]) != -1 && comprobarSegundoArgumento(argv[2]) != -1 && comprobarTercerArgumento(argv[3]) != -1) {
            recursosIPCS.nTrenes = atoi(argv[3]);
            recursosIPCS.tamMax = atoi(argv[2]);
            if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)manejadora, TRUE)) {
                fprintf(stderr, "Error en la funcion manejadora.\n");
            }
            int retardo = comprobarPrimerArgumento(argv[1]);
            //comprobar que argv1 está entre 0 y 10.
            if (retardo == -1) {
                fprintf(stderr, "El primer argumento debe ser mayor que 0.\n");
                return 1;
            }
            //comprobar que argv2 es numerico.
            if ((recursosIPCS.nTrenes = comprobarTercerArgumento(argv[3])) == -1) {
                fprintf(stderr, "El segundo argumento debe ser un numero.\n");
                return 1;
            }

            tipoLomo_Inicio puntInicioLomo;
            if ((puntInicioLomo = (tipoLomo_Inicio)GetProcAddress(libreria, "LOMO_inicio")) == NULL) {
                printf("Error obteniendo el puntero de LOMO_inicio.\n");
                return -3; 
            }
            if ((puntInicioLomo(retardo, recursosIPCS.tamMax, "i0959394", "i0919297")) == -1) {
                fprintf(stderr, "Error en lomo inicio.\n");
                return -4;
            }
            int x,i;
            tipoMensaje tipoMensajes;
            DWORD l;
            //Mandar mensajes de tipo x a todas las casillas.
            for (x = 1; x <= 75 * 17; x++) {
                tipoMensajes.tipo = x;
                l = x;
                PostThreadMessage(l, WM_USER, WPARAM(tipoMensajes.tipo), 0);
            }
            
            for (i = 0; i < recursosIPCS.nTrenes; i++) {
                recursosIPCS.hTrenes[i] = CreateThread(NULL, 0, receiveThreadMessage, NULL, 0, (LPDWORD)&l);
                if (recursosIPCS.hTrenes[i] == NULL) {
                    fprintf(stderr, "Error en la creación de los hilos.\n");
                    return -5;
                }
                tipoLomo_TrenNuevo punteroTrenNuevo;
                punteroTrenNuevo = (tipoLomo_TrenNuevo)GetProcAddress(libreria, "LOMO_trenNuevo");
                if (punteroTrenNuevo == NULL) {
                    fprintf(stderr, "Error en la obtención del puntero al tren nuevo.\n");
                    return -6;
                }
                int id;
                if ((id = punteroTrenNuevo()) == -1) {
                    fprintf(stderr, "Error en la asignación del nuevo tren.\n");
                }
                else {
                    printf("El nuevo tren se ha asignado con id %d\n", id);
                }
                tipoLomo_PeticionAvance punteroPeticionAvance;
                tipoLomo_Avance punteroAvance;
                tipoLomo_LomoEspera punteroEspera;
                int xCab, yCab;
                int xCola, yCola;
                int posAnterior;
                while (1) {
                    //LOMO_PETICIONAVANCE
                    punteroPeticionAvance = (tipoLomo_PeticionAvance)GetProcAddress(libreria, "LOMO_peticiOnAvance");
                    if (punteroPeticionAvance == NULL) {
                        fprintf(stderr, "Error en la obtencion del puntero de avance del tren.\n");
                        return -1;
                    }
                    if (punteroPeticionAvance(id, &xCab, &yCab) == -1) {
                        fprintf(stderr, "No se ha podido solicitar la peticion de avance.\n");
                    }
                    printf("Cabina: %d %d\n", xCab, yCab);
                    //LOMO_AVANCE
                    punteroAvance = (tipoLomo_Avance)GetProcAddress(libreria, "LOMO_avance");
                    if (punteroAvance == NULL) {
                        fprintf(stderr, "Error en la obtencion del puntero de avance del tren.\n");
                        return -1;
                    }
                    if (punteroAvance(id, &xCola, &yCola) == -1) {
                        fprintf(stderr, "No se ha podido solicitar la peticion de avance.\n");
                    }
                    printf("\nCola: %d %d", xCola, yCola);

                    posAnterior = yCola;
                    //LOMO_ESPERA
                    punteroEspera = (tipoLomo_LomoEspera)GetProcAddress(libreria, "LOMO_espera");
                    if (punteroEspera == NULL) {
                        fprintf(stderr, "Error en la obtencion del puntero de avance del tren.\n");
                        return -1;
                    }
                    punteroEspera(posAnterior, yCab);
                }
            }  
        }
        else {
            fprintf(stderr, "Introduzca unos argumentos validos.\n");
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
        WaitForMultipleObjects(recursosIPCS.nTrenes,recursosIPCS.hTrenes,true,INFINITE);
        exit(1);
        return TRUE;
    default:
        return FALSE;
    }

}
DWORD WINAPI receiveThreadMessage(LPVOID param) {
    MSG msg;
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE); // cola de mensajes.
    GetMessage(&msg, NULL, 0, 0); // leer mensaje.
    printf("%d\n", msg.wParam);
    return 1;
}