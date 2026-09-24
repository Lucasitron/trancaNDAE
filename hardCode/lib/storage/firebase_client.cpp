// firebase_client.c
#include "firebase_client.h"
#include "secrets.h"
#include <string.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"

static const char *TAG = "FIREBASE";

// Buffer global para receber a resposta HTTP
static char response_buffer[1024];

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        if (evt->data_len > 0)
        {
            strncat(response_buffer, (char *)evt->data, evt->data_len);
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

/**
 * Lê um nó específico do Realtime Database via REST API.
 * Ex: caminho = "/comandos/dispositivo1.json"
 * A resposta do Firebase é JSON. Se for uma string pura, virá entre aspas.
 */
esp_err_t firebase_ler_no(const char *caminho, char *out_buffer, size_t out_len)
{
    memset(response_buffer, 0, sizeof(response_buffer));

    // Monta a URL completa: <database_url><caminho>?auth=<token>
    char url[512];
    snprintf(url, sizeof(url), "%s%s?auth=%s",
             DATABASE_URL, caminho, API_KEY);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Status = %d, conteúdo = %s", status, response_buffer);

        if (status == 200 && strlen(response_buffer) > 0 && strcmp(response_buffer, "null") != 0)
        {
            // Se a resposta for uma string JSON (ex: "\"minha_chave\""), remove as aspas
            cJSON *json = cJSON_Parse(response_buffer);
            if (json != NULL && cJSON_IsString(json))
            {
                strncpy(out_buffer, json->valuestring, out_len - 1);
                out_buffer[out_len - 1] = '\0';
                cJSON_Delete(json);
            }
            else
            {
                strncpy(out_buffer, response_buffer, out_len - 1);
                out_buffer[out_len - 1] = '\0';
                if (json)
                    cJSON_Delete(json);
            }
        }
        else
        {
            err = ESP_ERR_NOT_FOUND;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Falha na requisição HTTP: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}