// senhas_store.cpp — implementação com buffers estáticos e NVS limitada.
#include "senhas_store.h"
#include "telnet_log.h"
#include <Preferences.h>

// RAM estática: 20 x (5 + 25 + 1) = 620 bytes. Sem heap.
static char s_pin[MAX_SENHAS][TOKEN_HEX_LEN + 1];
static char s_nome[MAX_SENHAS][NOME_MAX_LEN + 1];
static bool s_ativa[MAX_SENHAS];
static uint8_t s_total = 0;
static bool s_mudou = false;

static Preferences s_prefs;

// PIN válido: PIN_LEN dígitos 0-9.
static bool eh_pin(const String &s)
{
    if (s.length() != PIN_LEN)
        return false;
    for (int i = 0; i < PIN_LEN; i++)
    {
        if (s[i] < '0' || s[i] > '9')
            return false;
    }
    return true;
}

// Sanitiza etiqueta: tamanho + remove chars que quebram JSON/protocolo/
// chave do RTDB, mantendo consistente com a validação da página.
static void limpar_nome(const String &orig, char *dst)
{
    uint8_t j = 0;
    for (unsigned int i = 0; i < orig.length() && j < NOME_MAX_LEN; i++)
    {
        char c = orig[i];
        if (c == '"' || c == '\\' || c == ':' || c == '\n' || c == '\r')
            continue;
        if (c == '.' || c == '#' || c == '$' || c == '/' ||
            c == '[' || c == ']')
            continue;
        if (c < 32 || c > 126)
            continue;
        dst[j++] = c;
    }
    dst[j] = '\0';
}

static void persistir()
{
    s_prefs.begin("senhas", false);
    s_prefs.putUChar("n", s_total);
    char key[5];
    for (int i = 0; i < MAX_SENHAS; i++)
    {
        snprintf(key, sizeof(key), "t%02d", i);
        if (i < s_total)
        {
            s_prefs.putString(key, s_pin[i]);
            snprintf(key, sizeof(key), "n%02d", i);
            s_prefs.putString(key, s_nome[i]);
            snprintf(key, sizeof(key), "a%02d", i);
            s_prefs.putUChar(key, s_ativa[i] ? 1 : 0);
        }
        else
        {
            if (s_prefs.isKey(key))
                s_prefs.remove(key);
            snprintf(key, sizeof(key), "n%02d", i);
            if (s_prefs.isKey(key))
                s_prefs.remove(key);
            snprintf(key, sizeof(key), "a%02d", i);
            if (s_prefs.isKey(key))
                s_prefs.remove(key);
        }
    }
    s_prefs.end();
}

void senhas_init()
{
    s_prefs.begin("senhas", true);
    uint8_t n = s_prefs.isKey("n") ? s_prefs.getUChar("n", 0) : 0;
    if (n > MAX_SENHAS)
        n = MAX_SENHAS;
    char key[5];
    uint8_t validos = 0;
    for (int i = 0; i < n; i++)
    {
        snprintf(key, sizeof(key), "t%02d", i);
        if (!s_prefs.isKey(key))
            continue;
        String t = s_prefs.getString(key, "");
        if (!eh_pin(t))
            continue;
        t.toCharArray(s_pin[validos], TOKEN_HEX_LEN + 1);

        snprintf(key, sizeof(key), "n%02d", i);
        s_nome[validos][0] = '\0';
        if (s_prefs.isKey(key))
        {
            String nm = s_prefs.getString(key, "");
            limpar_nome(nm, s_nome[validos]);
        }

        snprintf(key, sizeof(key), "a%02d", i);
        s_ativa[validos] = s_prefs.isKey(key) ? (s_prefs.getUChar(key, 1) != 0) : true;

        validos++;
    }
    s_prefs.end();
    s_total = validos;
    tlogf("%d chave(s) recuperada(s) da NVS.\n", s_total);
}

static int indice_de(const String &pin)
{
    for (int i = 0; i < s_total; i++)
    {
        if (pin.equals(s_pin[i]))
            return i;
    }
    return -1;
}

static int indice_nome(const String &nome)
{
    for (int i = 0; i < s_total; i++)
    {
        if (nome.equals(s_nome[i]))
            return i;
    }
    return -1;
}

static void apagar_indice(int idx)
{
    for (int i = idx; i < s_total - 1; i++)
    {
        memcpy(s_pin[i], s_pin[i + 1], TOKEN_HEX_LEN + 1);
        memcpy(s_nome[i], s_nome[i + 1], NOME_MAX_LEN + 1);
        s_ativa[i] = s_ativa[i + 1];
    }
    s_total--;
}

bool senhas_adicionar(const String &pin, const String &nome)
{
    char nomeLimpo[NOME_MAX_LEN + 1];
    limpar_nome(nome, nomeLimpo);
    if (!eh_pin(pin) || nomeLimpo[0] == '\0')
        return false; // PIN inválido ou nome vazio
    if (indice_de(pin) >= 0 || indice_nome(String(nomeLimpo)) >= 0)
        return false; // PIN ou nome duplicado
    if (s_total >= MAX_SENHAS)
    {
        tlogln("Tabela cheia (20). PIN ignorado.");
        return false;
    }
    pin.toCharArray(s_pin[s_total], TOKEN_HEX_LEN + 1);
    memcpy(s_nome[s_total], nomeLimpo, NOME_MAX_LEN + 1);
    s_ativa[s_total] = true; // novo cadastro nasce ativo
    s_total++;
    persistir();
    s_mudou = true;
    tlogf("Chave cadastrada no ESP (%d ativa(s)).\n", s_total);
    return true;
}

bool senhas_remover_nome(const String &nome)
{
    int idx = indice_nome(nome);
    if (idx < 0)
        return false;
    apagar_indice(idx);
    persistir();
    s_mudou = true;
    tlogf("Chave removida (%d no total).\n", s_total);
    return true;
}

bool senhas_renovar(const String &nome, const String &pinNovo)
{
    int idx = indice_nome(nome);
    if (idx < 0 || !eh_pin(pinNovo) || indice_de(pinNovo) >= 0)
        return false;
    pinNovo.toCharArray(s_pin[idx], TOKEN_HEX_LEN + 1);
    persistir();
    s_mudou = true;
    tlogln("Chave renovada (nome mantido).");
    return true;
}

bool senhas_bloquear(const String &nome, bool bloquear)
{
    int idx = indice_nome(nome);
    if (idx < 0 || s_ativa[idx] == !bloquear)
        return false;
    s_ativa[idx] = !bloquear;
    persistir();
    s_mudou = true;
    tlogln(bloquear ? "Chave bloqueada." : "Chave liberada.");
    return true;
}

bool senhas_limpar()
{
    if (s_total == 0)
        return false;
    s_total = 0;
    persistir();
    s_mudou = true;
    tlogln("Todas as chaves apagadas do ESP.");
    return true;
}

int senhas_total()
{
    return s_total;
}

int senhas_total_ativas()
{
    int n = 0;
    for (int i = 0; i < s_total; i++)
    {
        if (s_ativa[i])
            n++;
    }
    return n;
}

bool senhas_contem(const String &pin)
{
    int idx = indice_de(pin);
    return idx >= 0 && s_ativa[idx]; // bloqueada não abre
}

bool senhas_consumirMudanca()
{
    bool m = s_mudou;
    s_mudou = false;
    return m;
}
