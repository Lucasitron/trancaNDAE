// display.h — LCD 20x4 I2C (layout em 4 zonas) com auto-detecção.
// Só escreve no barramento quando o conteúdo muda (anti-spam I2C).
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

// Zonas: status | mensagem | teclado (máscara!) | rodapé.
void mostrarNoLCD(const String &status, const String &mensagem,
                  const String &teclado = "", const String &rodape = "");

// Escreve 4 linhas cruas (para menus). Sem formatação.
void display_mostrar4(const String &l0, const String &l1,
                      const String &l2, const String &l3);

// Tela padrão de espera (mostra nº de chaves ativas, sem PINs).
void telaAguardando();

// Máscara anti-vazamento: "1234" -> "****". PIN nunca em claro no display.
String mascarar(const String &s);

// Atualiza só a zona do teclado com a máscara já pronta.
void display_set_teclado(const String &mascara);

// Limpa a zona do teclado.
void display_limpar_teclado();

#endif
