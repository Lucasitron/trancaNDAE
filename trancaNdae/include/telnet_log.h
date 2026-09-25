// telnet_log.h — Log dual USB (Serial) + Telnet (rede).
//
// Uso:
//   telnet_setup();  // após Wi-Fi conectado
//   telnet_loop();   // no loop() principal
//   tlog("texto"); tlogln("linha"); tlogf("fmt %d\n", x);
//
// As funções tlog* escrevem no Serial E no cliente telnet (se conectado).
// Sem cliente conectado, o custo é só o Serial (zero overhead de rede).
#ifndef TELNET_LOG_H
#define TELNET_LOG_H

#include <Arduino.h>

#ifndef TELNET_PORT
#define TELNET_PORT 23
#endif

// Sobe o servidor TCP (porta TELNET_PORT). Chame após Wi-Fi OK.
void telnet_setup();

// Aceita clientes, responde comandos, descarta entrada inválida.
// Chame TODO loop (não bloqueia).
void telnet_loop();

// Escrita dual (Serial + telnet). Versões print/println/printf.
void tlog_raw(const String &s);
void tlog_raw(const char *s);
void tlog(const String &s);
void tlogln(const String &s = "");
void tlogf(const char *fmt, ...);

// true se há um cliente telnet conectado agora.
bool telnet_conectado();

#endif
