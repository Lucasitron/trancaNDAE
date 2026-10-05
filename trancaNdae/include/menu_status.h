// menu_status.h — Modo status no display (diagnóstico local).
// Entrada: senha de admin (ADMIN_PASSWORD, em secrets.h) + '*'.
// Navegação: 2/8 rolam linhas | 4/6 trocam de tela | '*' sai.
// Mostra só contadores/estado — PINs nunca aparecem.
#ifndef MENU_STATUS_H
#define MENU_STATUS_H

#include <Arduino.h>

// Registra o último evento do sistema (log + tela "Chaves"). Sem segredos.
void registrarEvento(const String &ev);

// true dentro do modo status (teclado normal pausado).
bool emModoStatus();

// Entra (limpa o buffer e desenha a 1ª tela).
void entrarStatus();

// Sai e volta à tela de espera.
void sairStatus();

// Consome a tecla de navegação. Retorna true (sempre consome).
bool processarTeclaStatus(char tecla);

#endif
