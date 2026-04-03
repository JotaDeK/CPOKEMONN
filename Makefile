# ============================================================
#  Makefile - Pokemon Battle Simulator
#  Proyecto Pilas y Colas - IS304 2025
# ============================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -g
TARGET  = pokemon_battle
SRC     = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)
	@echo ""
	@echo "  Compilacion exitosa!"
	@echo "  Ejecuta con: ./$(TARGET)"
	@echo ""

clean:
	rm -f $(TARGET) historial_batalla.txt

.PHONY: all clean
