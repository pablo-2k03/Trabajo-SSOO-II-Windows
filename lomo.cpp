#include <stdio.h>
#include <Windows.h>
#include "lomo2.h"

typedef int(*tipoLomo_Inicio)(int, int, char const*, char const*);
typedef int(*tipoLomo_Generar_Mapa)(char const*, char const*);
int comprobarPrimerArgumento(char* argv);
int comprobarSegundoArgumento(char* argv);

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: lomo --mapa\n");
        fprintf(stderr, "Other Usage: lomo retardo nTrenes\n");
        return -1;
    }
    HINSTANCE libreria = LoadLibrary(TEXT("lomo2.dll"));
    if (libreria == NULL) {
        fprintf(stderr,"La biblioteca no se ha cargado.\n");
        return -2;
    }
    else {
        if (argc == 2 && !strcmp(argv[1], "--mapa")) {
            tipoLomo_Generar_Mapa puntLomoGenerarMapa;
            if ((puntLomoGenerarMapa = (tipoLomo_Generar_Mapa)GetProcAddress(libreria, "LOMO_generar_mapa")) == NULL) {
                fprintf(stderr,"No se ha podido generar el mapa.\n");
            }
            else {
                puntLomoGenerarMapa("i0959394", "i0919297");
                fprintf(stderr, "Mapa generado correctamente.\n");
                return 1;
            }
        }
        if (comprobarPrimerArgumento(argv[1]) != -1 && comprobarSegundoArgumento(argv[2]) != -1) {
            tipoLomo_Inicio puntInicioLomo;
            if ((puntInicioLomo = (tipoLomo_Inicio)GetProcAddress(libreria, "LOMO_inicio")) == NULL) {
                printf("Error en LOMO_inicio.\n");
                return -3; //cambiar -1 por codigo error.
            }
            puntInicioLomo(10, 10, "i0959394", "i0919297");
        }
    }
}

int comprobarPrimerArgumento(char* argv) {
    if (atoi(argv) < 0 || comprobarSegundoArgumento(argv) == -1)
        return -1;

    return atoi(argv);

}

int comprobarSegundoArgumento(char* argv) {
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