// wifi_manager.h
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>

extern volatile bool wifi_conectado;

void wifi_init_sta(const char *ssid, const char *password);

#endif