/*
 * ============================================================
 *  POKEMON BATTLE SIMULATOR - Proyecto de Pilas y Colas
 *  Autor: Basado en lineamientos IS304 - 2025
 *  Descripcion: Simulador de batalla Pokemon por turnos
 *               con pilas de acciones y cola circular de jugadores.
 *
 *  Estructuras clave:
 *    - Accion (nodo de pila)     -> acciones de cada turno
 *    - Pokemon (entidad jugador) -> pila de acciones
 *    - ColaTurnos               -> cola circular de jugadores
 * ============================================================
 */
/*gcc main.c -o main
./main
*/

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

/* ============================================================
 *  CONSTANTES DEL JUEGO
 * ============================================================ */
#define MAX_NOMBRE      21
#define MAX_ENTRENADOR  31
#define MAX_TIPO_ACCION 20
#define MAX_JUGADORES   6
#define MAX_ACCIONES    10   /* acciones maximas por pokemon */
#define VIDA_INICIAL    100
#define MAX_ACCIONES_REPLAY 400

/* ============================================================
 *  COLORES ANSI (escalable: deshabilitar si se usa GUI)
 * ============================================================ */
#define COLOR_RESET   "\033[0m"
#define COLOR_ROJO    "\033[1;31m"
#define COLOR_VERDE   "\033[1;32m"
#define COLOR_AMARILLO "\033[1;33m"
#define COLOR_AZUL    "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_BLANCO  "\033[1;37m"
#define COLOR_GRIS    "\033[0;37m"

/* ============================================================
 *  ESTRUCTURA: Accion  (nodo de PILA)
 *  Cada pokemon tiene una pila de acciones pendientes.
 *  El ultimo en entrar es el primero en ejecutarse (LIFO).
 * ============================================================ */
typedef struct Accion {
    char  tipo[MAX_TIPO_ACCION]; /* "ataque", "defensa", "especial", "mover" */
    char  nombre[40];            /* nombre visible del ataque, ej: "Impactrueno" */
    int   poder;                 /* valor de daño o defensa */
    int   precision;             /* 0-100, porcentaje de acierto */
    char  descripcion[80];       /* descripcion corta del ataque */
    struct Accion* sig;          /* enlace al siguiente nodo de la pila */
} Accion;

typedef struct {
    const char* tipo;
    const char* nombre;
    int poder;
    int precision;
    const char* descripcion;
} AccionBase;

/* ============================================================
 *  ESTRUCTURA: Pokemon  (jugador/entidad)
 *  Contiene su pila de acciones y datos de estado.
 * ============================================================ */
typedef struct Pokemon {
    char  nombre[MAX_NOMBRE];    /* nombre del pokemon */
    char  entrenador[MAX_ENTRENADOR]; /* nombre del jugador/entrenador */
    char  tipo[15];              /* tipo: Fuego, Agua, Planta... */
    char  emoji[8];              /* simbolo ASCII para la consola */
    int   salud;                 /* puntos de vida actuales */
    int   saludMax;              /* puntos de vida maximos */
    int   defensa;               /* defensa base (absorbe parte del daño) */
    int   velocidad;             /* determina quien ataca primero */
    int   estaVivo;              /* 1 = vivo, 0 = eliminado */
    int   turnosSinAtacar;       /* contador de turnos sin accion */
    Accion* pilaAcciones;        /* tope de la pila de acciones */
    int   numAcciones;           /* cuantas acciones hay en la pila */
    struct Pokemon* sig;         /* enlace en la cola circular */
} Pokemon;

/* ============================================================
 *  ESTRUCTURA: ColaTurnos  (COLA CIRCULAR de jugadores)
 *  Gestiona el orden de participacion de los jugadores.
 * ============================================================ */
typedef struct {
    Pokemon* frente;  /* apunta al jugador actual en turno */
    Pokemon* final;   /* apunta al ultimo jugador de la cola */
    int      total;   /* cantidad de jugadores en la cola */
} ColaTurnos;

/* ============================================================
 *  ESTRUCTURA: HistorialCombate
 *  Registro de eventos (extensible para exportar a archivo)
 * ============================================================ */
#define MAX_HISTORIAL 200
typedef struct {
    char eventos[MAX_HISTORIAL][150];
    int  cantidad;
} HistorialCombate;

/* ============================================================
 *  ESTRUCTURA: EstadoJuego
 *  Agrupa todo el estado de la partida (escalable a GUI)
 * ============================================================ */
typedef struct {
    ColaTurnos      cola;
    HistorialCombate historial;
    int             turnoActual;
    int             modoAutomatico; /* 0=manual, 1=IA vs IA */
    int             jugador1EsHumano;
    int             jugador2EsHumano;
    int             modoRepeticion; /* 1=lee acciones desde ACCIONES.TXT */
    unsigned int    semillaPartida; /* semilla para reproducir partida */
} EstadoJuego;

typedef struct {
    char atacante[MAX_NOMBRE];
    char accion[40];
} RegistroAccionReplay;

static RegistroAccionReplay g_accionesReplay[MAX_ACCIONES_REPLAY];
static int g_totalAccionesReplay = 0;
static int g_indiceAccionReplay = 0;
static unsigned int g_semillaReplay = 0;

/* ============================================================
 *  PROTOTIPADO DE FUNCIONES
 * ============================================================ */

/* --- Pilas de Acciones --- */
Accion* crearAccion(const char* tipo, const char* nombre, int poder,
                    int precision, const char* desc);
void    pushAccion(Pokemon* p, Accion* nueva);
Accion* popAccion(Pokemon* p);
Accion* peekAccion(Pokemon* p);
void    liberarPilaAcciones(Pokemon* p);

/* --- Cola Circular --- */
void    inicializarCola(ColaTurnos* cola);
void    encolarJugador(ColaTurnos* cola, Pokemon* p);
Pokemon* desencolarJugador(ColaTurnos* cola);
Pokemon* frenteCola(ColaTurnos* cola);
void    avanzarTurno(ColaTurnos* cola);
void    eliminarDeCola(ColaTurnos* cola, Pokemon* eliminado);

/* --- Pokemon --- */
Pokemon* crearPokemon(const char* nombre, const char* tipo,
                      const char* emoji, int salud, int defensa, int velocidad);
void     cargarAccionesDefault(Pokemon* p);
void     liberarPokemon(Pokemon* p);

/* --- Logica del juego --- */
int  calcularDanio(Accion* ataque, Pokemon* atacante, Pokemon* defensor);
double obtenerMultiplicadorTipos(const char* tipoAtacante, const char* tipoDefensor);
void ejecutarAccion(Accion* accion, Pokemon* atacante, Pokemon* defensor,
                    HistorialCombate* hist);
void turnoJugador(EstadoJuego* estado, Pokemon* atacante, Pokemon* defensor);
void turnoIA(EstadoJuego* estado, Pokemon* ia, Pokemon* defensor);
void turnoRepeticion(EstadoJuego* estado, Pokemon* atacante, Pokemon* defensor);
int  quedanJugadoresVivos(ColaTurnos* cola);
Pokemon* buscarEnemigoVivo(ColaTurnos* cola, Pokemon* actual);

/* --- Archivos --- */
int  cargarAccionesDeArchivo(EstadoJuego* estado, const char* archivo);
void guardarHistorial(HistorialCombate* hist, const char* archivo);
void iniciarRegistroAcciones(const EstadoJuego* estado, const char* archivo);
void registrarAccionEnArchivo(const EstadoJuego* estado,
                              const Pokemon* atacante,
                              const Accion* accion,
                              const char* archivo);
void resetearRepeticion(void);
Pokemon* crearPokemonDesdeNombreTemplate(const char* nombre);

/* --- UI Consola --- */
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
void consumirEntradaHastaNuevaLinea(void);
const char* colorTipoAccion(const char* tipo);
const char* colorTipoPokemon(const char* tipo);

/* --- Setup --- */
void configurarPartidaConsola(EstadoJuego* estado);
void liberarJuego(EstadoJuego* estado);
void copiarTextoSeguro(char* destino, int tam, const char* origen);
Accion* extraerAccionPorIndice(Pokemon* p, int indice);

/* ============================================================
 *  IMPLEMENTACION: PILAS DE ACCIONES
 * ============================================================ */

/*
 * crearAccion: Reserva memoria para un nodo. de accion.
 * Retorna puntero a la nueva accion o NULL si falla.
 */
Accion* crearAccion(const char* tipo, const char* nombre, int poder,
                    int precision, const char* desc) {
    Accion* nueva = (Accion*)malloc(sizeof(Accion));
    if (!nueva) { perror("Error al crear accion"); return NULL; }

    copiarTextoSeguro(nueva->tipo, MAX_TIPO_ACCION, tipo);
    copiarTextoSeguro(nueva->nombre, 40, nombre);
    copiarTextoSeguro(nueva->descripcion, 80, desc);
    nueva->poder     = poder;
    nueva->precision = precision;
    nueva->sig       = NULL;
    return nueva;
}

/*
 * pushAccion: Inserta una accion en el TOPE de la pila del pokemon.
 * Complejidad: O(1)
 */
void pushAccion(Pokemon* p, Accion* nueva) {
    if (!p || !nueva) return;
    if (p->numAcciones >= MAX_ACCIONES) {
        /* Pila llena: no se agrega (podria expandirse dinamicamente) */
        free(nueva);
        return;
    }
    nueva->sig      = p->pilaAcciones;
    p->pilaAcciones = nueva;
    p->numAcciones++;
}

/*
 * popAccion: Extrae el elemento del TOPE de la pila (LIFO).
 * El llamador es responsable de liberar la memoria del nodo.
 * Complejidad: O(1)
 */
Accion* popAccion(Pokemon* p) {
    if (!p || !p->pilaAcciones) return NULL;
    Accion* tope    = p->pilaAcciones;
    p->pilaAcciones = tope->sig;
    tope->sig       = NULL;
    p->numAcciones--;
    return tope;
}

/*
 * peekAccion: Consulta el tope SIN extraerlo.
 * Complejidad: O(1)
 */
Accion* peekAccion(Pokemon* p) {
    if (!p) return NULL;
    return p->pilaAcciones;
}

/*
 * liberarPilaAcciones: Libera todos los nodos de la pila.
 */
void liberarPilaAcciones(Pokemon* p) {
    while (p->pilaAcciones) {
        Accion* tmp = popAccion(p);
        free(tmp);
    }
}

void resetearRepeticion(void) {
    g_totalAccionesReplay = 0;
    g_indiceAccionReplay = 0;
    g_semillaReplay = 0;
}

void copiarTextoSeguro(char* destino, int tam, const char* origen) {
    if (!destino || tam <= 0) return;
    if (!origen) {
        destino[0] = '\0';
        return;
    }
    strncpy(destino, origen, (size_t)(tam - 1));
    destino[tam - 1] = '\0';
}

void consumirEntradaHastaNuevaLinea(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
    }
}

const char* colorTipoAccion(const char* tipo) {
    if (!tipo) return COLOR_BLANCO;
    if (strcmp(tipo, "ataque") == 0) return COLOR_ROJO;
    if (strcmp(tipo, "especial") == 0) return COLOR_MAGENTA;
    if (strcmp(tipo, "defensa") == 0) return COLOR_CYAN;
    if (strcmp(tipo, "mover") == 0) return COLOR_VERDE;
    return COLOR_BLANCO;
}

const char* colorTipoPokemon(const char* tipo) {
    if (!tipo) return COLOR_BLANCO;
    if (strcmp(tipo, "Fuego") == 0) return COLOR_ROJO;
    if (strcmp(tipo, "Agua") == 0) return COLOR_AZUL;
    if (strcmp(tipo, "Planta") == 0) return COLOR_VERDE;
    if (strcmp(tipo, "Electrico") == 0) return COLOR_AMARILLO;
    if (strcmp(tipo, "Psiquico") == 0) return COLOR_MAGENTA;
    return COLOR_BLANCO;
}

Accion* extraerAccionPorIndice(Pokemon* p, int indice) {
    Accion* elegida = NULL;
    Accion* temporal[MAX_ACCIONES];
    int tempCount = 0;
    int i;

    if (!p || indice < 0) return NULL;

    while (p->pilaAcciones) {
        Accion* a = popAccion(p);
        if (tempCount == indice) {
            elegida = a;
            break;
        }
        temporal[tempCount++] = a;
    }

    for (i = tempCount - 1; i >= 0; i--) {
        pushAccion(p, temporal[i]);
    }

    return elegida;
}

/* Extrae una accion por nombre preservando el orden de la pila. */
Accion* extraerAccionPorNombre(Pokemon* p, const char* nombreAccion) {
    if (!p || !p->pilaAcciones) return NULL;

    Accion* elegida = NULL;
    Accion* temporal[MAX_ACCIONES];
    int tempCount = 0;

    while (p->pilaAcciones) {
        Accion* a = popAccion(p);
        if (nombreAccion && strcmp(a->nombre, nombreAccion) == 0) {
            elegida = a;
            break;
        }
        temporal[tempCount++] = a;
    }

    {
        int j;
        for (j = tempCount - 1; j >= 0; j--) {
            pushAccion(p, temporal[j]);
        }
    }

    if (!elegida) {
        elegida = popAccion(p);
    }
    return elegida;
}

/* ============================================================
 *  IMPLEMENTACION: COLA CIRCULAR DE JUGADORES
 * ============================================================ */

/*
 * inicializarCola: Prepara la cola vacia.
 */
void inicializarCola(ColaTurnos* cola) {
    cola->frente = NULL;
    cola->final  = NULL;
    cola->total  = 0;
}

/*
 * encolarJugador: Agrega un pokemon al FINAL de la cola circular.
 * Complejidad: O(1)
 */
void encolarJugador(ColaTurnos* cola, Pokemon* p) {
    if (!p) return;
    if (!cola->frente) {
        cola->frente = p;
        cola->final  = p;
        p->sig       = p; /* apunta a si mismo: circular */
    } else {
        cola->final->sig = p;
        p->sig           = cola->frente; /* cierra el circulo */
        cola->final      = p;
    }
    cola->total++;
}

/*
 * frenteCola: Retorna el jugador en turno sin removerlo.
 */
Pokemon* frenteCola(ColaTurnos* cola) {
    return cola->frente;
}

/*
 * avanzarTurno: Mueve el frente al siguiente jugador.
 * Complejidad: O(1)
 */
void avanzarTurno(ColaTurnos* cola) {
    if (!cola->frente) return;
    cola->frente = cola->frente->sig;
}

/*
 * eliminarDeCola: Elimina un pokemon especifico de la cola circular.
 * Se usa cuando un pokemon es derrotado.
 * Complejidad: O(n)
 */
void eliminarDeCola(ColaTurnos* cola, Pokemon* eliminado) {
    if (!cola->frente || !eliminado) return;

    /* Caso: solo un elemento */
    if (cola->total == 1 && cola->frente == eliminado) {
        cola->frente = NULL;
        cola->final  = NULL;
        cola->total  = 0;
        return;
    }

    /* Buscar el anterior al eliminado */
    Pokemon* actual = cola->frente;
    Pokemon* anterior = NULL;
    int i;
    for (i = 0; i < cola->total; i++) {
        if (actual->sig == eliminado) {
            anterior = actual;
            break;
        }
        actual = actual->sig;
    }

    if (!anterior) return; /* no encontrado */

    anterior->sig = eliminado->sig;

    if (cola->frente == eliminado) cola->frente = eliminado->sig;
    if (cola->final  == eliminado) cola->final  = anterior;

    eliminado->sig = NULL;
    cola->total--;
}

/* ============================================================
 *  IMPLEMENTACION: POKEMON
 * ============================================================ */

/*
 * crearPokemon: Reserva y configura un nuevo pokemon.
 */
Pokemon* crearPokemon(const char* nombre, const char* tipo,
                      const char* emoji, int salud, int defensa, int velocidad) {
    Pokemon* p = (Pokemon*)malloc(sizeof(Pokemon));
    if (!p) { perror("Error al crear pokemon"); return NULL; }

    copiarTextoSeguro(p->nombre, MAX_NOMBRE, nombre);
    copiarTextoSeguro(p->entrenador, MAX_ENTRENADOR, nombre);
    copiarTextoSeguro(p->tipo, 15, tipo);
    copiarTextoSeguro(p->emoji, 8, emoji);
    p->salud         = salud;
    p->saludMax      = salud;
    p->defensa       = defensa;
    p->velocidad     = velocidad;
    p->estaVivo      = 1;
    p->turnosSinAtacar = 0;
    p->pilaAcciones  = NULL;
    p->numAcciones   = 0;
    p->sig           = NULL;
    return p;
}

/*
 * cargarAccionesDefault: Carga el set de ataques segun el tipo del pokemon.
 * Cada ataque se inserta en la pila (push), el primero insertado
 * quedara al fondo; el ultimo sera el tope (se ejecuta primero si
 * el jugador no elige manualmente).
 *
 * NOTA ESCALABILIDAD: Esta funcion puede reemplazarse con carga desde
 * archivo ACCIONES.TXT sin cambiar el resto del codigo.
 */
void cargarAccionesDefault(Pokemon* p) {
    static const AccionBase ACCIONES_FUEGO[] = {
        {"ataque", "Lanzallamas",   90, 100, "Onda de fuego abrasadora"},
        {"especial", "Explosion",   150, 80, "Explosion devastadora"},
        {"ataque", "Ascuas",        40, 100, "Chispa de fuego basica"},
        {"defensa", "Pantalla Luz",  0, 100, "Reduce daño especial"},
        {"ataque", "Giro Fuego",    60, 85, "Atrapa al rival en llamas"},
        {"especial", "Lanzallamas+",110, 90, "Version potenciada"},
        {"ataque", "Llamarada",    120, 85, "Ataque de fuego supremo"},
        {"defensa", "Proteccion",    0, 100, "Bloquea el proximo ataque"}
    };
    static const AccionBase ACCIONES_AGUA[] = {
        {"ataque", "Pistola Agua",  40, 100, "Chorro de agua basico"},
        {"ataque", "Surf",          90, 100, "Ola gigante devastadora"},
        {"especial", "Hidrobomba", 110, 80, "Bomba de agua explosiva"},
        {"defensa", "Refugio",       0, 100, "Sube la defensa"},
        {"ataque", "Acua Cola",     90, 100, "Golpe de cola mojada"},
        {"especial", "Lluvia Danza", 0, 100, "Potencia ataques de agua"},
        {"ataque", "Cascada",       80, 100, "Ataque potente de agua"},
        {"defensa", "Proteccion",    0, 100, "Bloquea el proximo ataque"}
    };
    static const AccionBase ACCIONES_PLANTA[] = {
        {"ataque", "Latigo Cepa",    45, 100, "Golpe con zarcillo vegetal"},
        {"ataque", "Hoja Aguda",     55, 95, "Hojas cortantes"},
        {"especial", "Rayo Solar",  120, 100, "Energia solar acumulada"},
        {"defensa", "Sintesis",       0, 100, "Cura puntos de vida"},
        {"ataque", "Tormenta Hojas",130, 90, "Lluvia de hojas afiladas"},
        {"especial", "Danza Petal",   0, 100, "Sube el ataque special"},
        {"ataque", "Drenadoras",     20, 100, "Drena vida del rival"},
        {"defensa", "Proteccion",     0, 100, "Bloquea el proximo ataque"}
    };
    static const AccionBase ACCIONES_ELECTRICO[] = {
        {"ataque", "Impactrueno",   40, 100, "Descarga electrica basica"},
        {"ataque", "Rayo",         110, 70, "Rayo del cielo"},
        {"especial", "Trueno",     120, 70, "Tormenta electrica total"},
        {"defensa", "Agilidad",      0, 100, "Duplica la velocidad"},
        {"ataque", "Bola Voltio",   90, 100, "Esfera de electricidad"},
        {"especial", "Rayo+",      130, 75, "Version suprema del rayo"},
        {"ataque", "Chispa",        65, 100, "Chispa cargada"},
        {"defensa", "Proteccion",    0, 100, "Bloquea el proximo ataque"}
    };
    static const AccionBase ACCIONES_PSIQUICO[] = {
        {"ataque", "Confusion",     50, 100, "Onda psionica basica"},
        {"especial", "Psicoataque",100, 80, "Golpe mental devastador"},
        {"ataque", "Psiquico",      90, 100, "Ola psiquica poderosa"},
        {"defensa", "Amnesia",       0, 100, "Sube defensa especial"},
        {"especial", "Premonicion",120, 90, "Ve el futuro y ataca"},
        {"ataque", "Telequinesis",  50, 100, "Lanza objetos con la mente"},
        {"defensa", "Intercambio",   0, 100, "Cambia estadisticas"},
        {"defensa", "Proteccion",    0, 100, "Bloquea el proximo ataque"}
    };
    static const AccionBase ACCIONES_NORMAL[] = {
        {"ataque", "Placaje",       40, 100, "Golpe de cuerpo basico"},
        {"ataque", "Golpe Cuerpo",  85, 100, "Golpe con todo el cuerpo"},
        {"especial", "Hiperrayo",  150, 90, "El ataque mas poderoso"},
        {"defensa", "Fortaleza",     0, 100, "Aumenta la defensa"},
        {"ataque", "Rapidez",       80, 100, "Golpe a alta velocidad"},
        {"especial", "Tesoro",     110, 85, "Ataque sorpresa"},
        {"ataque", "Cabezazo",      70, 80, "Golpe en la cabeza"},
        {"defensa", "Proteccion",    0, 100, "Bloquea el proximo ataque"}
    };
    const AccionBase* set = ACCIONES_NORMAL;
    int cantidad = (int)(sizeof(ACCIONES_NORMAL) / sizeof(ACCIONES_NORMAL[0]));
    int i;

    /* Limpia la pila antes de cargar */
    liberarPilaAcciones(p);

    if (strcmp(p->tipo, "Fuego") == 0) {
        set = ACCIONES_FUEGO;
        cantidad = (int)(sizeof(ACCIONES_FUEGO) / sizeof(ACCIONES_FUEGO[0]));
    } else if (strcmp(p->tipo, "Agua") == 0) {
        set = ACCIONES_AGUA;
        cantidad = (int)(sizeof(ACCIONES_AGUA) / sizeof(ACCIONES_AGUA[0]));
    } else if (strcmp(p->tipo, "Planta") == 0) {
        set = ACCIONES_PLANTA;
        cantidad = (int)(sizeof(ACCIONES_PLANTA) / sizeof(ACCIONES_PLANTA[0]));
    } else if (strcmp(p->tipo, "Electrico") == 0) {
        set = ACCIONES_ELECTRICO;
        cantidad = (int)(sizeof(ACCIONES_ELECTRICO) / sizeof(ACCIONES_ELECTRICO[0]));
    } else if (strcmp(p->tipo, "Psiquico") == 0) {
        set = ACCIONES_PSIQUICO;
        cantidad = (int)(sizeof(ACCIONES_PSIQUICO) / sizeof(ACCIONES_PSIQUICO[0]));
    }

    for (i = 0; i < cantidad; i++) {
        pushAccion(p, crearAccion(set[i].tipo, set[i].nombre, set[i].poder,
                                  set[i].precision, set[i].descripcion));
    }
}

/*
 * liberarPokemon: Libera toda la memoria de un pokemon.
 */
void liberarPokemon(Pokemon* p) {
    if (!p) return;
    liberarPilaAcciones(p);
    free(p);
}

/* ============================================================
 *  IMPLEMENTACION: LOGICA DEL JUEGO
 * ============================================================ */

/*
 * calcularDanio: Calcula el daño real aplicando defensa y aleatoriedad.
 * Formula inspirada en juegos de rol por turnos.
 * Rango: poder * 0.8 a poder * 1.2, reducido por defensa.
 */
int calcularDanio(Accion* ataque, Pokemon* atacante, Pokemon* defensor) {
    if (!ataque || ataque->poder == 0) return 0;

    /* Variacion aleatoria moderada para reducir golpes extremos. */
    double factor = 0.85 + ((rand() % 26) / 100.0); /* 0.85 a 1.10 */
    double efectividad = 1.0;
    int especial = (strcmp(ataque->tipo, "especial") == 0);

    int danio = (int)(ataque->poder * factor);
    if (especial) {
        danio = (int)(danio * 1.08); /* especial conserva ventaja ligera */
    }

    /* Reduccion por defensa mas fuerte para alargar los combates. */
    int reduccion = defensor->defensa / 2;
    if (reduccion > (danio * 6) / 10) reduccion = (danio * 6) / 10;
    danio -= reduccion;

    /* Escalado global y counters por tipo. */
    efectividad = obtenerMultiplicadorTipos(atacante->tipo, defensor->tipo);
    danio = (int)(danio * 0.72 * efectividad);

    /* Evita KO inmediato desde vida completa en la mayoria de casos. */
    if (defensor->salud == defensor->saludMax) {
        int tope = especial ? (defensor->saludMax * 62) / 100
                            : (defensor->saludMax * 52) / 100;
        if (danio > tope) danio = tope;
    }

    if (danio < 1) danio = 1;
    return danio;
}

double obtenerMultiplicadorTipos(const char* tipoAtacante, const char* tipoDefensor) {
    if (!tipoAtacante || !tipoDefensor) return 1.0;

    if (strcmp(tipoAtacante, "Fuego") == 0) {
        if (strcmp(tipoDefensor, "Planta") == 0) return 1.40;
        if (strcmp(tipoDefensor, "Agua") == 0) return 0.78;
    } else if (strcmp(tipoAtacante, "Agua") == 0) {
        if (strcmp(tipoDefensor, "Fuego") == 0) return 1.38;
        if (strcmp(tipoDefensor, "Planta") == 0) return 0.80;
        if (strcmp(tipoDefensor, "Electrico") == 0) return 0.90;
    } else if (strcmp(tipoAtacante, "Planta") == 0) {
        if (strcmp(tipoDefensor, "Agua") == 0) return 1.38;
        if (strcmp(tipoDefensor, "Fuego") == 0) return 0.78;
    } else if (strcmp(tipoAtacante, "Electrico") == 0) {
        if (strcmp(tipoDefensor, "Agua") == 0) return 1.45;
        if (strcmp(tipoDefensor, "Planta") == 0) return 0.80;
    } else if (strcmp(tipoAtacante, "Psiquico") == 0) {
        if (strcmp(tipoDefensor, "Normal") == 0) return 1.20;
        if (strcmp(tipoDefensor, "Psiquico") == 0) return 0.92;
    } else if (strcmp(tipoAtacante, "Normal") == 0) {
        if (strcmp(tipoDefensor, "Psiquico") == 0) return 0.95;
    }

    return 1.0;
}

/*
 * ejecutarAccion: Aplica una accion extraida de la pila sobre el defensor.
 * Registra el evento en el historial.
 */
void ejecutarAccion(Accion* accion, Pokemon* atacante, Pokemon* defensor,
                    HistorialCombate* hist) {
    char buffer[150];
    char atacanteLabel[64];
    char defensorLabel[64];

    etiquetaParticipante(atacante, atacanteLabel, sizeof(atacanteLabel));
    etiquetaParticipante(defensor, defensorLabel, sizeof(defensorLabel));

    /* Verificar precision */
    int tirada = rand() % 100 + 1;
    if (tirada > accion->precision) {
        snprintf(buffer, sizeof(buffer),
                 "[FALLO] %s uso %s pero fallo! (tirada=%d > %d%%)",
                 atacanteLabel, accion->nombre, tirada, accion->precision);
        agregarHistorial(hist, buffer);
        printf(COLOR_GRIS "  ~~ El ataque %s FALLO! ~~\n" COLOR_RESET,
               accion->nombre);
        return;
    }

    if (strcmp(accion->tipo, "ataque") == 0 ||
        strcmp(accion->tipo, "especial") == 0) {

        int danio = calcularDanio(accion, atacante, defensor);
        double efectividad = obtenerMultiplicadorTipos(atacante->tipo, defensor->tipo);
        defensor->salud -= danio;
        if (defensor->salud < 0) defensor->salud = 0;

        snprintf(buffer, sizeof(buffer),
                 "[ATAQUE] %s uso %s -> %d de daño a %s (vida restante: %d)",
                 atacanteLabel, accion->nombre, danio,
                 defensorLabel, defensor->salud);
        agregarHistorial(hist, buffer);

        if (strcmp(accion->tipo, "especial") == 0) {
            printf(COLOR_MAGENTA "  *** ATAQUE ESPECIAL: %s ***\n" COLOR_RESET,
                   accion->nombre);
        }
        if (efectividad >= 1.20) {
            printf(COLOR_AMARILLO "  Es super efectivo!\n" COLOR_RESET);
        } else if (efectividad <= 0.85) {
            printf(COLOR_GRIS "  No es muy efectivo...\n" COLOR_RESET);
        }
        printf(COLOR_ROJO "  Daño causado: %d puntos!\n" COLOR_RESET, danio);
        printf("  %s -> Vida: %d/%d\n",
             defensorLabel, defensor->salud, defensor->saludMax);

        if (defensor->salud == 0) {
            defensor->estaVivo = 0;
            printf(COLOR_ROJO "\n  *** %s ha sido DERROTADO! ***\n\n" COLOR_RESET,
                     defensorLabel);
            snprintf(buffer, sizeof(buffer), "[KO] %s fue derrotado por %s",
                    defensorLabel, atacanteLabel);
            agregarHistorial(hist, buffer);
        }

    } else if (strcmp(accion->tipo, "defensa") == 0) {
        /* Accion de defensa: cura vida o sube defensa */
        int efecto = 0;
        if (strcmp(accion->nombre, "Proteccion") == 0) {
            atacante->defensa += 15;
            efecto = 15;
            printf(COLOR_CYAN "  %s se PROTEGE! Defensa +15 (total: %d)\n" COLOR_RESET,
                     atacanteLabel, atacante->defensa);
        } else if (strcmp(accion->nombre, "Sintesis") == 0 ||
                   strcmp(accion->nombre, "Refugio") == 0) {
            efecto = 25 + rand() % 20;
            atacante->salud += efecto;
            if (atacante->salud > atacante->saludMax)
                atacante->salud = atacante->saludMax;
            printf(COLOR_VERDE "  %s se CURA! Recupera %d de vida (total: %d/%d)\n"
                   COLOR_RESET,
                                     atacanteLabel, efecto, atacante->salud, atacante->saludMax);
        } else {
            efecto = 10;
            atacante->defensa += efecto;
            printf(COLOR_CYAN "  %s usa %s! Defensa +%d\n" COLOR_RESET,
                                     atacanteLabel, accion->nombre, efecto);
        }
        snprintf(buffer, sizeof(buffer),
                                 "[DEFENSA] %s uso %s (efecto=%d)", atacanteLabel,
                 accion->nombre, efecto);
        agregarHistorial(hist, buffer);

    } else if (strcmp(accion->tipo, "mover") == 0) {
        printf(COLOR_AMARILLO "  %s realiza un movimiento estrategico!\n" COLOR_RESET,
                             atacanteLabel);
        snprintf(buffer, sizeof(buffer), "[MOVER] %s se reposiciona",
                                 atacanteLabel);
        agregarHistorial(hist, buffer);
    }
}

/*
 * turnoJugador: Muestra el menu de acciones y ejecuta la elegida.
 * El jugador elige de la lista de acciones disponibles en su pila.
 *
 * NOTA ESCALABILIDAD: Esta funcion es la unica que interactua con stdin.
 * Para una GUI, se reemplaza esta funcion sin tocar la logica del juego.
 */
void turnoJugador(EstadoJuego* estado, Pokemon* atacante, Pokemon* defensor) {
    Accion* elegida = NULL;
    int opcion;
    char atacanteLabel[64];

    etiquetaParticipante(atacante, atacanteLabel, sizeof(atacanteLabel));

    /* Construir lista temporal de acciones para mostrar al jugador */
    Accion* lista[MAX_ACCIONES];
    int conteo = 0;
    Accion* tmp = atacante->pilaAcciones;
    while (tmp && conteo < MAX_ACCIONES) {
        lista[conteo++] = tmp;
        tmp = tmp->sig;
    }

    if (conteo == 0) {
        printf(COLOR_AMARILLO "  %s no tiene acciones disponibles! Pasa turno.\n"
             COLOR_RESET, atacanteLabel);
        atacante->turnosSinAtacar++;
        return;
    }

    printf(COLOR_AMARILLO "\n  === Turno de %s %s ===\n" COLOR_RESET,
            atacante->emoji, atacanteLabel);
    printf("  Elige una accion:\n\n");

    int i;
    for (i = 0; i < conteo; i++) {
        const char* colorTipo = colorTipoAccion(lista[i]->tipo);

        printf("  %s[%d]%s %-20s | Poder: %3d | Precision: %3d%% | %s%s\n",
               COLOR_AMARILLO, i + 1, COLOR_RESET,
               lista[i]->nombre,
               lista[i]->poder,
               lista[i]->precision,
               colorTipo, lista[i]->descripcion);
        printf(COLOR_RESET);
    }

    printf("\n  Tu eleccion (1-%d): ", conteo);
    fflush(stdout);

    /* Lectura robusta */
    char buf[10];
    if (!fgets(buf, sizeof(buf), stdin)) opcion = 1;
    else opcion = atoi(buf);

    if (opcion < 1 || opcion > conteo) {
        printf(COLOR_GRIS "  Opcion invalida. Se usara el primer ataque disponible.\n"
               COLOR_RESET);
        opcion = 1;
    }

    /*
     * El jugador elige una accion especifica.
     * Para respetar la semantica de PILA, si elige una accion que no
     * esta en el tope, se hace pop hasta llegar a ella y se re-insertan
     * las demas (estrategia de acceso por indice sobre pila).
     *
     * Alternativa simple: extraer y usar directamente la elegida.
     */
    elegida = extraerAccionPorIndice(atacante, opcion - 1);

    if (!elegida) {
        printf(COLOR_GRIS "  No se pudo obtener la accion. Turno perdido.\n" COLOR_RESET);
        return;
    }

    printf("\n");
    animarAtaque(elegida->nombre, COLOR_AMARILLO);
    ejecutarAccion(elegida, atacante, defensor, &estado->historial);
    registrarAccionEnArchivo(estado, atacante, elegida, "ACCIONES.TXT");

    /* Liberar la accion usada */
    free(elegida);
}

/*
 * turnoIA: La maquina elige automaticamente la mejor accion disponible.
 * Estrategia simple: si tiene poca vida, usa defensa; si no, ataca.
 *
 * ESCALABILIDAD: Aqui se puede implementar un algoritmo minimax
 * o un sistema de reglas mas complejo sin tocar el resto del codigo.
 */
void turnoIA(EstadoJuego* estado, Pokemon* ia, Pokemon* defensor) {
    char iaLabel[64];
    etiquetaParticipante(ia, iaLabel, sizeof(iaLabel));

    if (!ia->pilaAcciones) {
        printf(COLOR_GRIS "  %s no tiene acciones!\n" COLOR_RESET, iaLabel);
        return;
    }

    SLEEP(800);
    printf(COLOR_MAGENTA "\n  [IA] %s %s esta eligiendo...\n" COLOR_RESET,
           ia->emoji, iaLabel);
    SLEEP(600);

    /* Estrategia: vida baja -> buscar defensa/curacion */
    Accion* elegida      = NULL;
    Accion* mejorAtaque  = NULL;
    Accion* mejorDefensa = NULL;

    Accion* cur = ia->pilaAcciones;
    while (cur) {
        if ((strcmp(cur->tipo, "ataque")   == 0 ||
             strcmp(cur->tipo, "especial") == 0) && !mejorAtaque) {
            mejorAtaque = cur;
        }
        if (strcmp(cur->tipo, "defensa") == 0 && !mejorDefensa) {
            mejorDefensa = cur;
        }
        cur = cur->sig;
    }

    double porcentajeVida = (double)ia->salud / ia->saludMax;

    if (porcentajeVida < 0.30 && mejorDefensa) {
        elegida = mejorDefensa;
    } else if (mejorAtaque) {
        elegida = mejorAtaque;
    } else if (mejorDefensa) {
        elegida = mejorDefensa;
    } else {
        elegida = ia->pilaAcciones; /* cualquier cosa */
    }

    /* Extraer la accion elegida de la pila */
    Accion* temporal[MAX_ACCIONES];
    int tempCount = 0;
    Accion* actual;
    while ((actual = popAccion(ia)) != NULL) {
        if (actual == elegida) break;
        temporal[tempCount++] = actual;
    }
    /* Re-insertar el resto */
    int j;
    for (j = tempCount - 1; j >= 0; j--) pushAccion(ia, temporal[j]);

    if (!elegida) {
        printf(COLOR_GRIS "  [IA] No pudo seleccionar accion.\n" COLOR_RESET);
        return;
    }

    printf(COLOR_MAGENTA "  [IA] %s usa: %s!\n" COLOR_RESET,
            iaLabel, elegida->nombre);
    SLEEP(400);

    animarAtaque(elegida->nombre, COLOR_MAGENTA);
    ejecutarAccion(elegida, ia, defensor, &estado->historial);
    registrarAccionEnArchivo(estado, ia, elegida, "ACCIONES.TXT");
    free(elegida);
}

void turnoRepeticion(EstadoJuego* estado, Pokemon* atacante, Pokemon* defensor) {
    const char* nombreAccion = NULL;

    if (!atacante->pilaAcciones) {
        printf(COLOR_GRIS "  [REPLAY] %s no tiene acciones.\n" COLOR_RESET,
               atacante->nombre);
        return;
    }

    if (g_indiceAccionReplay < g_totalAccionesReplay) {
        RegistroAccionReplay* reg = &g_accionesReplay[g_indiceAccionReplay++];
        nombreAccion = reg->accion;
        if (strcmp(reg->atacante, atacante->nombre) != 0) {
            printf(COLOR_GRIS
                   "  [REPLAY] Aviso: se esperaba accion de %s, turno actual %s.\n"
                   COLOR_RESET,
                   reg->atacante, atacante->nombre);
        }
    }

    {
        Accion* elegida = extraerAccionPorNombre(atacante, nombreAccion);
        if (!elegida) {
            printf(COLOR_GRIS "  [REPLAY] No se pudo extraer accion.\n" COLOR_RESET);
            return;
        }

        printf(COLOR_CYAN "  [REPLAY] %s usa: %s\n" COLOR_RESET,
               atacante->nombre, elegida->nombre);
        SLEEP(350);
        animarAtaque(elegida->nombre, COLOR_CYAN);
        ejecutarAccion(elegida, atacante, defensor, &estado->historial);
        free(elegida);
    }
}

/*
 * quedanJugadoresVivos: Cuenta cuantos pokemon siguen vivos en la cola.
 */
int quedanJugadoresVivos(ColaTurnos* cola) {
    if (!cola->frente) return 0;
    int vivos = 0;
    Pokemon* actual = cola->frente;
    int i;
    for (i = 0; i < cola->total; i++) {
        if (actual->estaVivo) vivos++;
        actual = actual->sig;
    }
    return vivos;
}

/*
 * buscarEnemigoVivo: Retorna el primer enemigo vivo diferente al actual.
 */
Pokemon* buscarEnemigoVivo(ColaTurnos* cola, Pokemon* actual) {
    if (!cola->frente) return NULL;
    Pokemon* cur = cola->frente;
    int i;
    for (i = 0; i < cola->total; i++) {
        if (cur != actual && cur->estaVivo) return cur;
        cur = cur->sig;
    }
    return NULL;
}

/* ============================================================
 *  IMPLEMENTACION: ARCHIVOS
 * ============================================================ */

int cargarAccionesDeArchivo(EstadoJuego* estado, const char* archivo) {
    FILE* f;
    char linea[256];
    char nombresInferidos[2][MAX_NOMBRE];
    int nombresInferidosCount = 0;
    int jugadoresHeader = 0;

    if (!estado) return 0;

    f = fopen(archivo, "r");
    if (!f) return 0;

    resetearRepeticion();
    inicializarCola(&estado->cola);

    while (fgets(linea, sizeof(linea), f)) {
        char* turnoTxt;
        char* atacante;
        char* accion;
        char* finLinea;

        finLinea = strpbrk(linea, "\r\n");
        if (finLinea) *finLinea = '\0';

        if (linea[0] == '\0' || linea[0] == '#') continue;

        if (strncmp(linea, "SEED;", 5) == 0) {
            g_semillaReplay = (unsigned int)strtoul(linea + 5, NULL, 10);
            continue;
        }

        if (strncmp(linea, "P1;", 3) == 0 || strncmp(linea, "P2;", 3) == 0) {
            char* tag = strtok(linea, ";");
            char* nombre = strtok(NULL, ";");
            char* tipo = strtok(NULL, ";");
            char* emoji = strtok(NULL, ";");
            char* saludTxt = strtok(NULL, ";");
            char* defensaTxt = strtok(NULL, ";");
            char* velocidadTxt = strtok(NULL, ";");
            char* entrenadorTxt = strtok(NULL, ";");
            Pokemon* p;
            (void)tag;

            if (!nombre || !tipo || !emoji || !saludTxt || !defensaTxt || !velocidadTxt) {
                continue;
            }

            p = crearPokemon(nombre, tipo, emoji,
                             atoi(saludTxt), atoi(defensaTxt), atoi(velocidadTxt));
            if (p) {
                if (entrenadorTxt && entrenadorTxt[0] != '\0') {
                    copiarTextoSeguro(p->entrenador, MAX_ENTRENADOR, entrenadorTxt);
                }
                cargarAccionesDefault(p);
                encolarJugador(&estado->cola, p);
                jugadoresHeader++;
            }
            continue;
        }

        turnoTxt = strtok(linea, ";");
        atacante = strtok(NULL, ";");
        accion   = strtok(NULL, ";");
        if (!turnoTxt || !atacante || !accion) continue;

        if (nombresInferidosCount < 2) {
            int yaExiste = 0;
            int i;
            for (i = 0; i < nombresInferidosCount; i++) {
                if (strcmp(nombresInferidos[i], atacante) == 0) {
                    yaExiste = 1;
                    break;
                }
            }
            if (!yaExiste) {
                copiarTextoSeguro(nombresInferidos[nombresInferidosCount],
                                 MAX_NOMBRE, atacante);
                nombresInferidosCount++;
            }
        }

        if (g_totalAccionesReplay >= MAX_ACCIONES_REPLAY) break;

        copiarTextoSeguro(g_accionesReplay[g_totalAccionesReplay].atacante,
                 MAX_NOMBRE, atacante);
        copiarTextoSeguro(g_accionesReplay[g_totalAccionesReplay].accion,
                 40, accion);

        g_totalAccionesReplay++;
    }

    fclose(f);

    /* Compatibilidad con archivos legacy (sin P1/P2): inferir por nombre de pokemon. */
    if (estado->cola.total == 0 && nombresInferidosCount == 2) {
        Pokemon* p1 = crearPokemonDesdeNombreTemplate(nombresInferidos[0]);
        Pokemon* p2 = crearPokemonDesdeNombreTemplate(nombresInferidos[1]);
        if (p1 && p2) {
            encolarJugador(&estado->cola, p1);
            encolarJugador(&estado->cola, p2);
        } else {
            if (p1) liberarPokemon(p1);
            if (p2) liberarPokemon(p2);
        }
    }

    if (estado->cola.total < 2 || g_totalAccionesReplay == 0) {
        liberarJuego(estado);
        return 0;
    }

    (void)jugadoresHeader;
    return g_totalAccionesReplay;
}

void iniciarRegistroAcciones(const EstadoJuego* estado, const char* archivo) {
    FILE* f;
    if (!estado || estado->modoRepeticion) return;

    f = fopen(archivo, "w");
    if (!f) return;

    fprintf(f, "SEED;%u\n", estado->semillaPartida);
    if (estado->cola.frente && estado->cola.total >= 2) {
        Pokemon* p1 = estado->cola.frente;
        Pokemon* p2 = estado->cola.frente->sig;

        fprintf(f, "P1;%s;%s;%s;%d;%d;%d;%s\n",
                p1->nombre, p1->tipo, p1->emoji,
                p1->saludMax, p1->defensa, p1->velocidad, p1->entrenador);
        fprintf(f, "P2;%s;%s;%s;%d;%d;%d;%s\n",
                p2->nombre, p2->tipo, p2->emoji,
                p2->saludMax, p2->defensa, p2->velocidad, p2->entrenador);
    }
    fclose(f);
}

void registrarAccionEnArchivo(const EstadoJuego* estado,
                              const Pokemon* atacante,
                              const Accion* accion,
                              const char* archivo) {
    FILE* f;
    if (!estado || !atacante || !accion || estado->modoRepeticion) return;

    f = fopen(archivo, "a");
    if (!f) return;

    fprintf(f, "%d;%s;%s\n", estado->turnoActual, atacante->nombre, accion->nombre);
    fclose(f);
}

/*
 * guardarHistorial: Exporta el historial de combate a un archivo de texto.
 */
void guardarHistorial(HistorialCombate* hist, const char* archivo) {
    FILE* f = fopen(archivo, "w");
    if (!f) { printf("No se pudo guardar el historial.\n"); return; }

    fprintf(f, "=== HISTORIAL DE COMBATE POKEMON ===\n\n");
    int i;
    for (i = 0; i < hist->cantidad; i++) {
        fprintf(f, "%s\n", hist->eventos[i]);
    }
    fclose(f);
    printf(COLOR_VERDE "\n  Historial guardado en: %s\n" COLOR_RESET, archivo);
}

/* ============================================================
 *  IMPLEMENTACION: UI CONSOLA
 * ============================================================ */

void limpiarPantalla(void) {
    system(CLEAR);
}

void pausar(int ms) {
    SLEEP(ms);
}

void mostrarTitulo(void) {
    printf(COLOR_AMARILLO);
    printf("  +=================================================+\n");
    printf("  |                                                 |\n");
    printf("  |      *** POKEMON BATTLE SIMULATOR ***          |\n");
    printf("  |        Proyecto Pilas & Colas - IS304          |\n");
    printf("  |                                                 |\n");
    printf("  +=================================================+\n");
    printf(COLOR_RESET "\n");
}

/*
 * mostrarBarraVida: Dibuja una barra ASCII de vida del pokemon.
 * ESCALABILIDAD: Puede reemplazarse por una barra grafica en GUI.
 */
void mostrarBarraVida(Pokemon* p) {
    int barraLen = 30;
    int llenos   = (p->salud * barraLen) / p->saludMax;
    if (llenos < 0) llenos = 0;
    if (llenos > barraLen) llenos = barraLen;

    /* Color segun porcentaje de vida */
    const char* colorBarra;
    double porcentaje = (double)p->salud / p->saludMax;
    if (porcentaje > 0.50)      colorBarra = COLOR_VERDE;
    else if (porcentaje > 0.25) colorBarra = COLOR_AMARILLO;
    else                        colorBarra = COLOR_ROJO;

        printf("  %s %-14s (%-12s)%s [", p->emoji, p->entrenador, p->nombre,
            p->estaVivo ? "" : COLOR_ROJO " (KO)" COLOR_RESET);
    printf("%s", colorBarra);

    int i;
    for (i = 0; i < llenos; i++)    printf("|");
    for (i = llenos; i < barraLen; i++) printf(".");

    printf(COLOR_RESET "] %s%3d%s/%d HP",
           colorBarra, p->salud, COLOR_RESET, p->saludMax);
    printf("  DEF:%d  Acc:%d\n",
           p->defensa, p->numAcciones);
}

/*
 * mostrarEstadoBatalla: Renderiza el estado actual del campo de batalla.
 * Esta es la funcion de "render principal".
 */
void mostrarEstadoBatalla(EstadoJuego* estado) {
    if (!estado->modoRepeticion) {
        limpiarPantalla();
        mostrarTitulo();
    } else if (estado->turnoActual == 1) {
        limpiarPantalla();
        mostrarTitulo();
    }

    printf(COLOR_CYAN "  --- CAMPO DE BATALLA --- Turno #%d ---\n\n" COLOR_RESET,
           estado->turnoActual);

    /* Mostrar todos los pokemon en la cola */
    if (!estado->cola.frente) return;
    Pokemon* actual = estado->cola.frente;
    int i;
    for (i = 0; i < estado->cola.total; i++) {
        if (actual == estado->cola.frente && i == 0) {
            printf(COLOR_AMARILLO "  >>> EN TURNO:\n" COLOR_RESET);
        } else if (i == 1) {
            printf(COLOR_BLANCO "\n  --- RIVAL:\n" COLOR_RESET);
        } else if (i == 2) {
            printf(COLOR_GRIS "\n  --- OTROS:\n" COLOR_RESET);
        }
        mostrarBarraVida(actual);
        actual = actual->sig;
    }

    printf("\n" COLOR_GRIS "  ----------------------------------------\n"
           COLOR_RESET);
}

void mostrarMenuAcciones(Pokemon* p) {
    /* Integrado en turnoJugador */
    (void)p;
}

void mostrarMensaje(const char* color, const char* msg) {
    printf("%s  %s%s\n", color, msg, COLOR_RESET);
}

/*
 * animarAtaque: Muestra una animacion de texto simple al ejecutar un ataque.
 * ESCALABILIDAD: Aqui iria la animacion de sprite en versiones futuras.
 */
void animarAtaque(const char* nombreAtaque, const char* colorAtacante) {
    printf("\n");
    printf("%s  >> Usando: %-20s <<\n" COLOR_RESET,
           colorAtacante, nombreAtaque);
    SLEEP(300);
    printf("  . . .");
    fflush(stdout);
    SLEEP(400);
    printf(" . . .\n");
    SLEEP(300);
}

void agregarHistorial(HistorialCombate* hist, const char* msg) {
    if (hist->cantidad < MAX_HISTORIAL) {
        strncpy(hist->eventos[hist->cantidad], msg, 149);
        hist->cantidad++;
    }
}

void pedirNombreJugador(char* destino, int tam, const char* nombreDefault,
                        const char* prompt) {
    char buf[80];
    int i;

    if (!destino || tam <= 1) return;
    destino[0] = '\0';

    printf("%s", prompt);
    fflush(stdout);

    if (!fgets(buf, sizeof(buf), stdin)) {
        copiarTextoSeguro(destino, tam, nombreDefault);
        return;
    }

    for (i = 0; buf[i] != '\0'; i++) {
        if (buf[i] == '\n' || buf[i] == '\r') {
            buf[i] = '\0';
            break;
        }
    }

    if (buf[0] == '\0') {
        copiarTextoSeguro(destino, tam, nombreDefault);
        return;
    }

    copiarTextoSeguro(destino, tam, buf);
}

void etiquetaParticipante(const Pokemon* p, char* out, int outSize) {
    if (!p || !out || outSize <= 0) return;

    if (strcmp(p->entrenador, p->nombre) == 0) {
        snprintf(out, outSize, "%s", p->nombre);
    } else {
        snprintf(out, outSize, "%s (%s)", p->entrenador, p->nombre);
    }
}

void mostrarResultado(EstadoJuego* estado) {
    limpiarPantalla();
    mostrarTitulo();

    printf(COLOR_AMARILLO "\n  *** FIN DE LA BATALLA ***\n\n" COLOR_RESET);

    /* Buscar ganador */
    Pokemon* ganador = NULL;
    if (estado->cola.frente) {
        Pokemon* actual = estado->cola.frente;
        int i;
        for (i = 0; i < estado->cola.total; i++) {
            if (actual->estaVivo) { ganador = actual; break; }
            actual = actual->sig;
        }
    }

    if (ganador) {
        printf(COLOR_VERDE "  GANADOR: %s %s!\n" COLOR_RESET,
               ganador->emoji, ganador->nombre);
        printf("  Vida restante: %d/%d HP\n", ganador->salud, ganador->saludMax);
        printf("  Defensa final: %d\n\n", ganador->defensa);
    } else {
        printf(COLOR_ROJO "  Empate! Todos fueron derrotados.\n\n" COLOR_RESET);
    }

    printf("  Total de turnos: %d\n", estado->turnoActual);
    printf("  Eventos registrados: %d\n\n", estado->historial.cantidad);
}

void mostrarHistorial(HistorialCombate* hist) {
    printf(COLOR_CYAN "\n  === HISTORIAL DE COMBATE ===\n\n" COLOR_RESET);
    int i;
    for (i = 0; i < hist->cantidad; i++) {
        printf("  %3d. %s\n", i + 1, hist->eventos[i]);
    }
    printf("\n");
}

void mostrarMenuPrincipal(void) {
    limpiarPantalla();
    mostrarTitulo();

    printf("  Selecciona el modo de juego:\n\n");
    printf(COLOR_VERDE "  [1]" COLOR_RESET " Jugador 1 vs Jugador 2 (dos humanos)\n");
    printf(COLOR_AZUL  "  [2]" COLOR_RESET " Jugador vs Maquina\n");
    printf(COLOR_MAGENTA "  [3]" COLOR_RESET " Maquina vs Maquina (espectador)\n");
    printf(COLOR_AMARILLO "  [4]" COLOR_RESET " Repeticion desde archivo (ACCIONES.TXT)\n");
    printf(COLOR_ROJO  "  [5]" COLOR_RESET " Salir\n");
    printf("\n  Opcion: ");
    fflush(stdout);
}

/* ============================================================
 *  IMPLEMENTACION: SETUP Y CONFIGURACION
 * ============================================================ */

/*
 * Tabla de pokemon disponibles para seleccion en consola.
 * ESCALABILIDAD: Mover a archivo de configuracion o base de datos.
 */
typedef struct {
    const char* nombre;
    const char* tipo;
    const char* emoji;
    int salud;
    int defensa;
    int velocidad;
} PokemonTemplate;

static const PokemonTemplate POKEMON_DISPONIBLES[] = {
    {"Charizard",   "Fuego",     "[C]", 126, 16, 85},
    {"Blastoise",   "Agua",      "[B]", 132, 22, 78},
    {"Venusaur",    "Planta",    "[V]", 130, 20, 75},
    {"Pikachu",     "Electrico", "[P]", 112, 14, 95},
    {"Mewtwo",      "Psiquico",  "[M]", 140, 18, 90},
    {"Machamp",     "Normal",    "[X]", 138, 24, 70},
    {"Gengar",      "Psiquico",  "[G]", 116, 13, 100},
    {"Arcanine",    "Fuego",     "[A]", 124, 18, 92},
    {"Gyarados",    "Agua",      "[Y]", 144, 20, 80},
    {"Exeggutor",   "Planta",    "[E]", 128, 19, 65},
};
#define NUM_POKEMON_DISPONIBLES 10

static Pokemon* crearPokemonDesdeTemplateIndice(int indice, const char* entrenador) {
    Pokemon* p;
    if (indice < 0 || indice >= NUM_POKEMON_DISPONIBLES) return NULL;

    p = crearPokemon(
        POKEMON_DISPONIBLES[indice].nombre,
        POKEMON_DISPONIBLES[indice].tipo,
        POKEMON_DISPONIBLES[indice].emoji,
        POKEMON_DISPONIBLES[indice].salud,
        POKEMON_DISPONIBLES[indice].defensa,
        POKEMON_DISPONIBLES[indice].velocidad
    );
    if (!p) return NULL;

    if (entrenador) {
        copiarTextoSeguro(p->entrenador, MAX_ENTRENADOR, entrenador);
    }
    cargarAccionesDefault(p);
    return p;
}

Pokemon* crearPokemonDesdeNombreTemplate(const char* nombre) {
    int i;
    if (!nombre) return NULL;

    for (i = 0; i < NUM_POKEMON_DISPONIBLES; i++) {
        if (strcmp(POKEMON_DISPONIBLES[i].nombre, nombre) == 0) {
            return crearPokemonDesdeTemplateIndice(i, NULL);
        }
    }
    return NULL;
}

/*
 * mostrarPokemonDisponibles: Lista los pokemon que se pueden elegir.
 */
void mostrarPokemonDisponibles(void) {
    printf("\n  Pokemon disponibles:\n\n");
    int i;
    for (i = 0; i < NUM_POKEMON_DISPONIBLES; i++) {
        const char* colorTipo = colorTipoPokemon(POKEMON_DISPONIBLES[i].tipo);

        printf("  %s[%2d]%s %s%-12s%s | Tipo: %s%-9s%s | HP:%3d | DEF:%2d | VEL:%3d\n",
               COLOR_CYAN, i + 1, COLOR_RESET,
               COLOR_BLANCO, POKEMON_DISPONIBLES[i].nombre, COLOR_RESET,
               colorTipo, POKEMON_DISPONIBLES[i].tipo, COLOR_RESET,
               POKEMON_DISPONIBLES[i].salud,
               POKEMON_DISPONIBLES[i].defensa,
               POKEMON_DISPONIBLES[i].velocidad);
    }
    printf("\n");
}

/*
 * elegirPokemon: El jugador (o la IA) selecciona su pokemon.
 */
Pokemon* elegirPokemon(const char* nombreJugador, int esHumano) {
    int opcion;

    mostrarPokemonDisponibles();

    if (esHumano) {
        printf("  %s, elige tu Pokemon (1-%d): ",
               nombreJugador, NUM_POKEMON_DISPONIBLES);
        fflush(stdout);
        char buf[10];
        if (!fgets(buf, sizeof(buf), stdin)) opcion = 1;
        else opcion = atoi(buf);
    } else {
        opcion = rand() % NUM_POKEMON_DISPONIBLES + 1;
        printf("  La maquina elige automaticamente...\n");
        SLEEP(800);
    }

    if (opcion < 1 || opcion > NUM_POKEMON_DISPONIBLES) opcion = 1;
    opcion--; /* indice base 0 */

    Pokemon* p = crearPokemonDesdeTemplateIndice(opcion, nombreJugador);
    if (!p) return NULL;
    printf(COLOR_VERDE "\n  %s ha elegido a %s %s!\n\n" COLOR_RESET,
           nombreJugador, p->emoji, p->nombre);
    SLEEP(600);
    return p;
}

/*
 * configurarPartidaConsola: Configura la partida segun el modo elegido.
 */
void configurarPartidaConsola(EstadoJuego* estado) {
    char nombreJ1[MAX_ENTRENADOR];
    char nombreJ2[MAX_ENTRENADOR];

    mostrarMenuPrincipal();

    char buf[10];
    if (!fgets(buf, sizeof(buf), stdin)) buf[0] = '5';
    int opcion = atoi(buf);

    estado->turnoActual      = 1;
    estado->historial.cantidad = 0;
    estado->modoRepeticion   = 0;
    inicializarCola(&estado->cola);
    resetearRepeticion();

    switch (opcion) {
        case 1: /* PvP */
            estado->jugador1EsHumano = 1;
            estado->jugador2EsHumano = 1;
            estado->modoAutomatico   = 0;

            pedirNombreJugador(nombreJ1, sizeof(nombreJ1), "Jugador 1",
                               "  Nombre del Jugador 1: ");
            pedirNombreJugador(nombreJ2, sizeof(nombreJ2), "Jugador 2",
                               "  Nombre del Jugador 2: ");

            limpiarPantalla();
            mostrarTitulo();
            printf(COLOR_VERDE "  === MODO: 2 JUGADORES ===\n\n" COLOR_RESET);

            printf(COLOR_AMARILLO "  -- JUGADOR 1, elige tu Pokemon --\n" COLOR_RESET);
            encolarJugador(&estado->cola, elegirPokemon(nombreJ1, 1));

            printf(COLOR_AZUL   "  -- JUGADOR 2, elige tu Pokemon --\n" COLOR_RESET);
            encolarJugador(&estado->cola, elegirPokemon(nombreJ2, 1));
            break;

        case 2: /* PvE */
            estado->jugador1EsHumano = 1;
            estado->jugador2EsHumano = 0;
            estado->modoAutomatico   = 0;

            pedirNombreJugador(nombreJ1, sizeof(nombreJ1), "Jugador",
                               "  Tu nombre de entrenador: ");

            limpiarPantalla();
            mostrarTitulo();
            printf(COLOR_VERDE "  === MODO: JUGADOR vs MAQUINA ===\n\n" COLOR_RESET);

            printf(COLOR_AMARILLO "  -- TU, elige tu Pokemon --\n" COLOR_RESET);
            encolarJugador(&estado->cola, elegirPokemon(nombreJ1, 1));

            printf(COLOR_MAGENTA "  -- LA MAQUINA elige su Pokemon --\n" COLOR_RESET);
            encolarJugador(&estado->cola, elegirPokemon("Maquina", 0));
            break;

        case 3: /* IA vs IA */
            estado->jugador1EsHumano = 0;
            estado->jugador2EsHumano = 0;
            estado->modoAutomatico   = 1;

            limpiarPantalla();
            mostrarTitulo();
            printf(COLOR_MAGENTA "  === MODO: MAQUINA vs MAQUINA ===\n\n" COLOR_RESET);
            printf("  La maquina elige ambos Pokemon...\n\n");
            SLEEP(400);

            encolarJugador(&estado->cola, elegirPokemon("IA-Alpha", 0));
            encolarJugador(&estado->cola, elegirPokemon("IA-Beta",  0));
            break;

        case 4: { /* Repeticion desde archivo */
            estado->jugador1EsHumano = 0;
            estado->jugador2EsHumano = 0;
            estado->modoAutomatico   = 1;
            limpiarPantalla();
            mostrarTitulo();
            printf("  Cargando repeticion desde ACCIONES.TXT...\n");
            int nAcciones = cargarAccionesDeArchivo(estado, "ACCIONES.TXT");
            if (nAcciones <= 0) {
                printf(COLOR_ROJO
                       "  Error: ACCIONES.TXT no contiene una repeticion valida.\n"
                       "  Debe incluir al menos 2 jugadores y acciones registradas.\n"
                       "  Volviendo al menu...\n" COLOR_RESET);
                SLEEP(1500);
                configurarPartidaConsola(estado);
            } else {
                estado->modoRepeticion = 1;
                if (g_semillaReplay != 0) estado->semillaPartida = g_semillaReplay;
                srand(estado->semillaPartida);
                printf(COLOR_VERDE
                       "  Repeticion activada con %d acciones desde ACCIONES.TXT.\n"
                       COLOR_RESET, nAcciones);
                SLEEP(800);
            }
            break;
        }

        default:
            exit(0);
    }
}

/*
 * liberarJuego: Libera toda la memoria dinamica del juego.
 * Importante para evitar fugas de memoria (requisito del proyecto).
 */
void liberarJuego(EstadoJuego* estado) {
    if (!estado->cola.frente) return;

    /* Recolectar todos los punteros antes de liberar (cola circular) */
    Pokemon* ptrs[MAX_JUGADORES * 2];
    int count = 0;
    Pokemon* actual = estado->cola.frente;
    int i;
    for (i = 0; i < estado->cola.total && count < MAX_JUGADORES * 2; i++) {
        ptrs[count++] = actual;
        actual = actual->sig;
    }
    /* Romper la circularidad antes de liberar */
    for (i = 0; i < count; i++) {
        ptrs[i]->sig = NULL;
    }
    for (i = 0; i < count; i++) {
        liberarPokemon(ptrs[i]);
    }
    estado->cola.frente = NULL;
    estado->cola.final  = NULL;
    estado->cola.total  = 0;
}

/* ============================================================
 *  LOOP PRINCIPAL DEL JUEGO
 * ============================================================ */

/*
 * jugar: Bucle principal de la simulacion por turnos.
 * Extrae de la cola el jugador en turno, ejecuta su accion,
 * verifica si el enemigo fue derrotado y avanza el turno.
 */
void jugar(EstadoJuego* estado) {
    printf(COLOR_AMARILLO "\n  Presiona ENTER para comenzar la batalla!\n"
           COLOR_RESET);
    fflush(stdout);
    /* Consumir entrada pendiente y esperar Enter */
    consumirEntradaHastaNuevaLinea();

    while (quedanJugadoresVivos(&estado->cola) > 1) {

        /* Obtener jugador en turno */
        Pokemon* atacante = frenteCola(&estado->cola);

        /* Saltar jugadores derrotados en la cola */
        if (!atacante->estaVivo) {
            avanzarTurno(&estado->cola);
            continue;
        }

        /* Buscar enemigo vivo */
        Pokemon* defensor = buscarEnemigoVivo(&estado->cola, atacante);
        if (!defensor) break;

        /* Renderizar estado */
        mostrarEstadoBatalla(estado);

        printf(COLOR_CYAN "\n  Turno #%d\n" COLOR_RESET, estado->turnoActual);

        /* Determinar si el atacante es humano o IA */
        int esHumano = 0;
        if (estado->cola.total >= 1) {
            /* El primer encolado es J1, el segundo es J2 */
            /* Se determina por posicion relativa en la cola original */
            /* Simplificacion: turno impar = J1, par = J2 */
            if (estado->turnoActual % 2 == 1 && estado->jugador1EsHumano) {
                esHumano = 1;
            } else if (estado->turnoActual % 2 == 0 && estado->jugador2EsHumano) {
                esHumano = 1;
            }
        }

        if (estado->modoRepeticion) {
            turnoRepeticion(estado, atacante, defensor);
        } else if (esHumano) {
            turnoJugador(estado, atacante, defensor);
        } else {
            turnoIA(estado, atacante, defensor);
        }

        /* Verificar derrota del defensor */
        if (!defensor->estaVivo) {
            eliminarDeCola(&estado->cola, defensor);

            /* Mostrar estado actualizado tras KO */
            mostrarEstadoBatalla(estado);
            printf(COLOR_ROJO "\n  %s %s fue eliminado de la batalla!\n"
                   COLOR_RESET, defensor->emoji, defensor->nombre);
            if (!estado->modoRepeticion) {
                SLEEP(1200);
            }
        } else {
            /* Pequeña pausa entre turnos */
            if (!estado->modoRepeticion) {
                SLEEP(estado->modoAutomatico ? 1000 : 400);
            }
        }

        if (estado->modoRepeticion) {
            printf(COLOR_GRIS "\n  [REPLAY] Presiona ENTER para la siguiente accion..." COLOR_RESET);
            fflush(stdout);
            consumirEntradaHastaNuevaLinea();
        } else if (estado->modoAutomatico) {
            printf(COLOR_GRIS "\n  [IA vs IA] Presiona ENTER para la siguiente accion..." COLOR_RESET);
            fflush(stdout);
            consumirEntradaHastaNuevaLinea();
        }

        /* Avanzar al siguiente jugador en la cola */
        avanzarTurno(&estado->cola);
        estado->turnoActual++;

        if (!estado->modoAutomatico) {
            printf(COLOR_GRIS "\n  [Presiona ENTER para continuar...]" COLOR_RESET);
            fflush(stdout);
            consumirEntradaHastaNuevaLinea();
        }
    }
}

/* ============================================================
 *  MENU POST-BATALLA
 * ============================================================ */
void menuPostBatalla(EstadoJuego* estado) {
    mostrarResultado(estado);

    printf("  Que deseas hacer?\n\n");
    printf("  [1] Ver historial de combate\n");
    printf("  [2] Guardar historial en archivo\n");
    printf("  [3] Jugar de nuevo\n");
    printf("  [4] Salir\n");
    printf("\n  Opcion: ");
    fflush(stdout);

    char buf[10];
    if (!fgets(buf, sizeof(buf), stdin)) return;
    int opcion = atoi(buf);

    switch (opcion) {
        case 1:
            mostrarHistorial(&estado->historial);
            printf("  [Presiona ENTER para continuar...]");
            fflush(stdout);
            consumirEntradaHastaNuevaLinea();
            menuPostBatalla(estado);
            break;
        case 2:
            guardarHistorial(&estado->historial, "historial_batalla.txt");
            SLEEP(1000);
            menuPostBatalla(estado);
            break;
        case 3:
            liberarJuego(estado);
            break; /* Vuelve al main para reiniciar */
        default:
            break;
    }
}

/* ============================================================
 *  FUNCION MAIN
 * ============================================================ */
int main(void) {
    srand((unsigned int)time(NULL));

    int continuar = 1;
    while (continuar) {
        EstadoJuego estado;
        memset(&estado, 0, sizeof(EstadoJuego));
        estado.semillaPartida = (unsigned int)time(NULL);
        srand(estado.semillaPartida);

        configurarPartidaConsola(&estado);

        if (estado.cola.total < 2) {
            printf(COLOR_ROJO "  Error: se necesitan al menos 2 jugadores.\n"
                   COLOR_RESET);
            liberarJuego(&estado);
            break;
        }

        if (!estado.modoRepeticion) {
            iniciarRegistroAcciones(&estado, "ACCIONES.TXT");
        }

        jugar(&estado);
        menuPostBatalla(&estado);

        /* Preguntar si jugar otra vez */
        printf(COLOR_AMARILLO "\n  Jugar otra partida? [s/n]: " COLOR_RESET);
        fflush(stdout);
        char buf[10];
        if (!fgets(buf, sizeof(buf), stdin)) break;
        if (buf[0] != 's' && buf[0] != 'S') continuar = 0;

        liberarJuego(&estado);
    }

    printf(COLOR_CYAN "\n  Gracias por jugar! Hasta la proxima batalla!\n\n"
           COLOR_RESET);
    return 0;
}
