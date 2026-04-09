#include "pokemon_battle.h"

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

enum { NUM_POKEMON_DISPONIBLES = (int)(sizeof(POKEMON_DISPONIBLES) / sizeof(POKEMON_DISPONIBLES[0])) };

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

void pushAccion(Pokemon* p, Accion* nueva) {
    if (!p || !nueva) return;
    if (p->numAcciones >= MAX_ACCIONES) {
        free(nueva);
        return;
    }
    nueva->sig      = p->pilaAcciones;
    p->pilaAcciones = nueva;
    p->numAcciones++;
}

Accion* popAccion(Pokemon* p) {
    if (!p || !p->pilaAcciones) return NULL;
    Accion* tope    = p->pilaAcciones;
    p->pilaAcciones = tope->sig;
    tope->sig       = NULL;
    p->numAcciones--;
    return tope;
}

Accion* peekAccion(Pokemon* p) {
    if (!p) return NULL;
    return p->pilaAcciones;
}

void liberarPilaAcciones(Pokemon* p) {
    if (!p) return;
    while (p->pilaAcciones) {
        Accion* tmp = popAccion(p);
        free(tmp);
    }
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

void inicializarCola(ColaTurnos* cola) {
    if (!cola) return;
    cola->frente = NULL;
    cola->final  = NULL;
    cola->total  = 0;
}

void encolarJugador(ColaTurnos* cola, Pokemon* p) {
    if (!cola || !p) return;
    if (!cola->frente) {
        cola->frente = p;
        cola->final  = p;
        p->sig       = p;
    } else {
        cola->final->sig = p;
        p->sig           = cola->frente;
        cola->final      = p;
    }
    cola->total++;
}

Pokemon* desencolarJugador(ColaTurnos* cola) {
    Pokemon* eliminado;

    if (!cola || !cola->frente) return NULL;
    eliminado = cola->frente;

    if (cola->total == 1) {
        cola->frente = NULL;
        cola->final = NULL;
        cola->total = 0;
    } else {
        cola->frente = eliminado->sig;
        cola->final->sig = cola->frente;
        cola->total--;
    }

    eliminado->sig = NULL;
    return eliminado;
}

Pokemon* frenteCola(ColaTurnos* cola) {
    if (!cola) return NULL;
    return cola->frente;
}

void avanzarTurno(ColaTurnos* cola) {
    if (!cola || !cola->frente) return;
    cola->frente = cola->frente->sig;
}

void eliminarDeCola(ColaTurnos* cola, Pokemon* eliminado) {
    if (!cola || !cola->frente || !eliminado) return;

    if (cola->total == 1 && cola->frente == eliminado) {
        cola->frente = NULL;
        cola->final  = NULL;
        cola->total  = 0;
        return;
    }

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

    if (!anterior) return;

    anterior->sig = eliminado->sig;

    if (cola->frente == eliminado) cola->frente = eliminado->sig;
    if (cola->final  == eliminado) cola->final  = anterior;

    eliminado->sig = NULL;
    cola->total--;
}

Pokemon* crearPokemon(const char* nombre, const char* tipo,
                      const char* emoji, int salud, int defensa, int velocidad) {
    Pokemon* p = (Pokemon*)malloc(sizeof(Pokemon));
    if (!p) { perror("Error al crear pokemon"); return NULL; }

    copiarTextoSeguro(p->nombre, MAX_NOMBRE, nombre);
    copiarTextoSeguro(p->entrenador, MAX_ENTRENADOR, nombre);
    copiarTextoSeguro(p->tipo, 15, tipo);
    copiarTextoSeguro(p->emoji, 8, emoji);
    p->salud           = salud;
    p->saludMax        = salud;
    p->defensa         = defensa;
    p->velocidad       = velocidad;
    p->estaVivo        = 1;
    p->turnosSinAtacar = 0;
    p->pilaAcciones    = NULL;
    p->numAcciones     = 0;
    p->sig             = NULL;
    return p;
}

void cargarAccionesDefault(Pokemon* p) {
    static const AccionBase ACCIONES_FUEGO[] = {
        {"ataque", "Lanzallamas",   90, 100, "Onda de fuego abrasadora"},
        {"especial", "Explosion",   150, 80, "Explosion devastadora"},
        {"ataque", "Ascuas",        40, 100, "Chispa de fuego basica"},
        {"defensa", "Pantalla Luz",  0, 100, "Reduce dano especial"},
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

void liberarPokemon(Pokemon* p) {
    if (!p) return;
    liberarPilaAcciones(p);
    free(p);
}

int obtenerCantidadPokemonDisponibles(void) {
    return NUM_POKEMON_DISPONIBLES;
}

const PokemonTemplate* obtenerPokemonDisponibles(void) {
    return POKEMON_DISPONIBLES;
}

Pokemon* crearPokemonDesdeTemplateIndice(int indice, const char* entrenador) {
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

void liberarJuego(EstadoJuego* estado) {
    if (!estado || !estado->cola.frente) return;

    Pokemon* ptrs[MAX_JUGADORES * 2];
    int count = 0;
    Pokemon* actual = estado->cola.frente;
    int i;
    for (i = 0; i < estado->cola.total && count < MAX_JUGADORES * 2; i++) {
        ptrs[count++] = actual;
        actual = actual->sig;
    }

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
