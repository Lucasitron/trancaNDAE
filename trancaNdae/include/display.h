// display.h — LCD 20x4 I2C (layout em 4 zonas) com auto-detecção.
// Só escreve no barramento as linhas que mudaram (anti-spam I2C), mas
// reescreve tudo periodicamente para recuperar o controlador do LCD caso
// ele perca o estado (ex.: queda de alimentação no acionamento do relé).
// Sem display físico, o conteúdo (sem segredos) é espelhado no log.
#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

// 0 = roda sem I2C (só Serial/Telnet). Com 1, se o LCD não responder no
// boot ele desliga sozinho (display_ok()==false) sem travar o sistema.
#define USA_LCD 1

// true quando o LCD respondeu ao scan e está ativo.
bool display_ok();

// Wire + scan + init/backlight. Chame no setup().
bool display_init();

// Vigia o LCD: reescreve as 4 linhas periodicamente e reinicializa o
// controlador se o endereço I2C parar de responder. Chame todo loop().
void display_loop();

// Zonas: status | mensagem | teclado (máscara!) | rodapé.
void mostrarNoLCD(const String &status, const String &mensagem,
                  const String &teclado = "", const String &rodape = "");

// Tela padrão de espera (mostra nº de chaves ativas, sem PINs).
void telaAguardando();

// Zona do teclado com N asteriscos (PIN nunca em claro, sem alocar String).
void display_set_mascara(unsigned int n);

// Limpa a zona do teclado.
void display_limpar_teclado();

#endif
