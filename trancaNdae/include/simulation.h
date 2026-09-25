// simulation.h
#ifndef SIMULATION_H
#define SIMULATION_H

#include <Arduino.h>

// Chamada no setup() principal — inicializa botão TEST e LED de status
void simulation_setup();

// Chamada no loop() principal — processa botão TEST e espelha o relé no LED
void simulation_loop();

#endif