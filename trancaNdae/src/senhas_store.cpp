// senhas_store.cpp — implementação com buffers estáticos e NVS limitada.
#include "senhas_store.h"
#include "telnet_log.h"
#include <Preferences.h>

// RAM estática: 20 x 33 bytes = 660 bytes. Sem heap, sem fragmentação.
static char s_tokens[MAX_SENHAS][TOKEN_HEX_LEN + 1];
static uint8_t s_total = 0;
static bool s_mudou = false;

static Preferences s_prefs;

static bool eh_hex32(const char *s)
{
    for (int i = 0; i < TOKEN_HEX_LEN; i++)
    {
        char c = s[i];
        bool hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (!hex)
            return false;
    }
    return true;
}

static void persistir()
{
    s_prefs.begin("senhas", false);
    s_prefs.putUChar("n", s_total);
    for (int i = 0; i < MAX_SENHAS; i++)
    {
        char key[4];
        snprintf(key, sizeof(key), "t%02d", i);
        if (i < s_total)
            s_prefs.putString(key, s_tokens[i]);
        else
            s_prefs.remove(key); // libera slots órfãos (bloqueadas/excluídas)
    }
    s_prefs.end();
}

void senhas_init()
{
    s_prefs.begin("senhas", true);
    // isKey() evita spam "[E] getString ... NOT_FOUND" quando a NVS está vazia.
    uint8_t n = s_prefs.isKey("n") ? s_prefs.getUChar("n", 0) : 0;
    if (n > MAX_SENHAS)
        n = MAX_SENHAS;
    uint8_t validos = 0;
    for (int i = 0; i < n; i++)
    {
        char key[4];
        snprintf(key, sizeof(key), "t%02d", i);
        if (!s_prefs.isKey(key))
            continue;
        String t = s_prefs.getString(key, "");
        if (t.length() == TOKEN_HEX_LEN)
        {
            t.toCharArray(s_tokens[validos], TOKEN_HEX_LEN + 1);
            if (eh_hex32(s_tokens[validos]))
                validos++;
        }
    }
    s_prefs.end();
    s_total = validos;
    tlogf("🔑 %d chave(s) recuperada(s) da NVS.\n", s_total);
}

bool senhas_sync_csv(const String &csv)
{
    // Defesa: CSV maior que o máximo teórico (20*32 + 19 vírgulas) é descartado.
    if (csv.length() > (size_t)(MAX_SENHAS * (TOKEN_HEX_LEN + 1)))
    {
        tlogln("⚠️ CSV de senhas excede o limite. Ignorado.");
        return false;
    }

    char nova[MAX_SENHAS][TOKEN_HEX_LEN + 1];
    uint8_t n = 0;

    int inicio = 0;
    while (n < MAX_SENHAS)
    {
        int fim = csv.indexOf(',', inicio);
        String pedaco = (fim < 0) ? csv.substring(inicio) : csv.substring(inicio, fim);
        pedaco.trim();
        if (pedaco.length() == TOKEN_HEX_LEN)
        {
            char buf[TOKEN_HEX_LEN + 1];
            pedaco.toCharArray(buf, sizeof(buf));
            if (eh_hex32(buf))
            {
                // Deduplica
                bool repetido = false;
                for (int i = 0; i < n; i++)
                {
                    if (strncmp(nova[i], buf, TOKEN_HEX_LEN + 1) == 0)
                    {
                        repetido = true;
                        break;
                    }
                }
                if (!repetido)
                {
                    memcpy(nova[n], buf, TOKEN_HEX_LEN + 1);
                    n++;
                }
            }
        }
        if (fim < 0)
            break;
        inicio = fim + 1;
    }

    // Compara com a tabela atual
    bool igual = (n == s_total);
    if (igual)
    {
        for (int i = 0; i < n; i++)
        {
            if (strncmp(s_tokens[i], nova[i], TOKEN_HEX_LEN + 1) != 0)
            {
                igual = false;
                break;
            }
        }
    }
    if (igual)
        return false;

    for (int i = 0; i < n; i++)
        memcpy(s_tokens[i], nova[i], TOKEN_HEX_LEN + 1);
    s_total = n;
    persistir();
    s_mudou = true;
    tlogf("🔑 Tabela de senhas atualizada: %d ativa(s).\n", s_total);
    return true;
}

int senhas_total()
{
    return s_total;
}

bool senhas_contem(const String &token)
{
    if (token.length() != TOKEN_HEX_LEN)
        return false;
    for (int i = 0; i < s_total; i++)
    {
        if (token.equals(s_tokens[i]))
            return true;
    }
    return false;
}

bool senhas_consumirMudanca()
{
    bool m = s_mudou;
    s_mudou = false;
    return m;
}
