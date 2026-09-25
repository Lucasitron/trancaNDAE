#include "nvs_flash.h"
#include "esp_partition.h"

void app_main(void)
{
    // 1. Encontra a partição de chaves da NVS
    const esp_partition_t *key_part = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS, NULL);
    if (key_part == NULL)
    {
        // Tratar erro: partição de chaves não encontrada
        return;
    }

    // 2. Lê a configuração de segurança (chaves) da partição
    nvs_sec_cfg_t cfg;
    esp_err_t err = nvs_flash_read_security_cfg(key_part, &cfg);
    if (err != ESP_OK)
    {
        // Tratar erro: falha ao ler as chaves
        return;
    }

    // 3. Inicializa a partição NVS padrão com a configuração de segurança
    err = nvs_flash_secure_init(&cfg);
    if (err != ESP_OK)
    {
        // Tratar erro: falha ao inicializar a NVS
        return;
    }

    // A partir daqui, você pode usar as APIs normais da NVS (nvs_open, nvs_set_str, etc.)
    // e os dados serão automaticamente criptografados/descriptografados.
}