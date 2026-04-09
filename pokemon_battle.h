#ifndef POKEMON_BATTLE_H
#define POKEMON_BATTLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #include <windows.h>
  #define SLEEP(ms) Sleep(ms)
  #define CLEAR "cls"
#else
  #include <unistd.h>
  #define SLEEP(ms) usleep((ms) * 1000)
  #define CLEAR "clear"
#endif

#define MAX_NOMBRE           21
#define MAX_ENTRENADOR       31
#define MAX_TIPO_ACCION      20
#define MAX_JUGADORES        6
#define MAX_ACCIONES         10
#define VIDA_INICIAL         100
#define MAX_ACCIONES_REPLAY  400

#define COLOR_RESET    "\033[0m"
#define COLOR_ROJO     "\033[1;31m"
#define COLOR_VERDE    "\033[1;32m"
#define COLOR_AMARILLO "\033[1;33m"
#define COLOR_AZUL     "\033[1;34m"
#define COLOR_MAGENTA  "\033[1;35m"
#define COLOR_CYAN     "\033[1;36m"
#define COLOR_BLANCO   "\033[1;37m"
#define COLOR_GRIS     "\033[0;37m"

typedef struct Accion {
    char  tipo[MAX_TIPO_ACCION];
    char  nombre[40];
    int   poder;
    int   precision;
    char  descripcion[80];
    struct Accion* sig;
} Accion;

typedef struct {
    const char* tipo;
    const char* nombre;
    int poder;
    int precision;
    const char* descripcion;
} AccionBase;

typedef struct Pokemon {
    char  nombre[MAX_NOMBRE];
    char  entrenador[MAX_ENTRENADOR];
    char  tipo[15];
    char  emoji[8];
    int   salud;
    int   saludMax;
    int   defensa;
    int   velocidad;
    int   estaVivo;
    int   turnosSinAtacar;
    Accion* pilaAcciones;
    int   numAcciones;
    struct Pokemon* sig;
} Pokemon;

typedef struct {
    Pokemon* frente;
    Pokemon* final;
    int      total;
} ColaTurnos;

#define MAX_HISTORIAL 200
typedef struct {
    char eventos[MAX_HISTORIAL][150];
    int  cantidad;
} HistorialCombate;

typedef struct {
    char atacante[MAX_NOMBRE];
    char accion[40];
} RegistroAccionReplay;

typedef struct {
    const char* nombre;
    const char* tipo;
    const char* emoji;
    int salud;
    int defensa;
    int velocidad;
} PokemonTemplate;

typedef struct {
    ColaTurnos      cola;
    HistorialCombate historial;
    int             turnoActual;
    int             modoAutomatico;
    int             jugador1EsHumano;
    int             jugador2EsHumano;
    int             modoRepeticion;
    unsigned int    semillaPartida;
    unsigned int    semillaReplay;
    RegistroAccionReplay accionesReplay[MAX_ACCIONES_REPLAY];
    int             totalAccionesReplay;
    int             indiceAccionReplay;
} EstadoJuego;

Accion* crearAccion(const char* tipo, const char* nombre, int poder,
                    int precision, const char* desc);
void    pushAccion(Pokemon* p, Accion* nueva);
Accion* popAccion(Pokemon* p);
Accion* peekAccion(Pokemon* p);
void    liberarPilaAcciones(Pokemon* p);

void    inicializarCola(ColaTurnos* cola);
void    encolarJugador(ColaTurnos* cola, Pokemon* p);
Pokemon* desencolarJugador(ColaTurnos* cola);
Pokemon* frenteCola(ColaTurnos* cola);
void    avanzarTurno(ColaTurnos* cola);
void    eliminarDeCola(ColaTurnos* cola, Pokemon* eliminado);

Pokemon* crearPokemon(const char* nombre, const char* tipo,
                      const char* emoji, int salud, int defensa, int velocidad);
void     cargarAccionesDefault(Pokemon* p);
void     liberarPokemon(Pokemon* p);
void     liberarJuego(EstadoJuego* estado);

void     copiarTextoSeguro(char* destino, int tam, const char* origen);
void     consumirEntradaHastaNuevaLinea(void);
const char* colorTipoAccion(const char* tipo);
const char* colorTipoPokemon(const char* tipo);
Accion* extraerAccionPorIndice(Pokemon* p, int indice);
Accion* extraerAccionPorNombre(Pokemon* p, const char* nombreAccion);

int                    obtenerCantidadPokemonDisponibles(void);
const PokemonTemplate* obtenerPokemonDisponibles(void);
Pokemon*               crearPokemonDesdeTemplateIndice(int indice, const char* entrenador);
Pokemon*               crearPokemonDesdeNombreTemplate(const char* nombre);

int    calcularDanio(Accion* ataque, Pokemon* atacante, Pokemon* defensor);
double obtenerMultiplicadorTipos(const char* tipoAtacante, const char* tipoDefensor);
void   ejecutarAccion(Accion* accion, Pokemon* atacante, Pokemon* defensor,
                      HistorialCombate* hist);
void   turnoJugador(EstadoJuego* estado, Pokemon* atacante, Pokemon* defensor);
void   turnoIA(EstadoJuego* estado, Pokemon* ia, Pokemon* defensor);
void   turnoRepeticion(EstadoJuego* estado, Pokemon* atacante, Pokemon* defensor);
int    quedanJugadoresVivos(ColaTurnos* cola);
Pokemon* buscarEnemigoVivo(ColaTurnos* cola, Pokemon* actual);
void   jugar(EstadoJuego* estado);

void   resetearRepeticion(EstadoJuego* estado);
int    cargarAccionesDeArchivo(EstadoJuego* estado, const char* archivo);
void   guardarHistorial(HistorialCombate* hist, const char* archivo);
void   iniciarRegistroAcciones(const EstadoJuego* estado, const char* archivo);
void   registrarAccionEnArchivo(const EstadoJuego* estado,
                                const Pokemon* atacante,
                                const Accion* accion,
                                const char* archivo);

void limpiarPantalla(void);
void pausar(int ms);
void mostrarTitulo(void);
void mostrarBarraVida(Pokemon* p);
void mostrarEstadoBatalla(EstadoJuego* estado);
void mostrarMenuAcciones(Pokemon* p);
void mostrarMensaje(const char* color, const char* msg);
void mostrarResultado(EstadoJuego* estado);
void mostrarHistorial(HistorialCombate* hist);
void mostrarMenuPrincipal(void);
void animarAtaque(const char* nombreAtaque, const char* colorAtacante);
void agregarHistorial(HistorialCombate* hist, const char* msg);
void pedirNombreJugador(char* destino, int tam, const char* nombreDefault,
                        const char* prompt);
void etiquetaParticipante(const Pokemon* p, char* out, int outSize);
void mostrarPokemonDisponibles(void);
Pokemon* elegirPokemon(const char* nombreJugador, int esHumano);
void configurarPartidaConsola(EstadoJuego* estado);
void menuPostBatalla(EstadoJuego* estado);

#endif
