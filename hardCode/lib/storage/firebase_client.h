// firebase_client.h
#ifndef FIREBASE_CLIENT_H
#define FIREBASE_CLIENT_H

#include "esp_err.h"

esp_err_t firebase_obter_token(void);
esp_err_t firebase_ler_no(const char *caminho, char *out_buffer, size_t out_len);

#endif