// senhas_store.cpp — implementação com buffers estáticos e NVS em blob.
#include "senhas_store.h"
#include "telnet_log.h"
#include <Preferences.h>

// Registro compacto (28 bytes): PIN numérico + flag + nome.
// 30 x 28 = 840 bytes de RAM estática, sem heap.
struct Registro
{
    uint16_t pin; // 0..9999 (4 dígitos); 0 = reservado (0000)
    uint8_t ativa;
    char nome[NOME_MAX_LEN + 1];
};

static_assert(sizeof(Registro) == 2 + 1 + NOME_MAX_LEN + 1,
              "Registro com padding: ajuste o layout/versao da NVS");

static Registro s_reg[MAX_SENHAS];
static uint8_t s_total = 0;
static bool s_mudou = false;

static Preferences s_prefs;

// NVS: namespace "senhas", chave "v" marca o formato novo.
static const char *NVS_ESPACO = "senhas";
static const char *NVS_REGISTROS = "regs";
static const char *NVS_VERSAO = "v";
static const uint8_t NVS_VERSAO_ATUAL = 1; // 0 = formato legado (sem "v")

// PIN reservado para reinício (fechadura.h: CODIGO_RESET).
static const uint16_t PIN_RESERVADO = 0;

// PIN de 4 dígitos (0-9) -> número. Rejeita "0000" (reservado).
static bool pin_para_num(const String &s, uint16_t &out)
{
    if (s.length() != PIN_LEN)
        return false;
    uint16_t v = 0;
    for (uint8_t i = 0; i < PIN_LEN; i++)
    {
        const char c = s[i];
        if (c < '0' || c > '9')
            return false;
        v = v * 10 + (c - '0');
    }
    if (v == PIN_RESERVADO)
        return false;
    out = v;
    return true;
}

// Sanitiza etiqueta: tamanho + remove chars que quebram JSON/protocolo/
// chave do RTDB, mantendo consistente com a validação da página.
static void limpar_nome(const String &orig, char *dst)
{
    static const char PROIBIDOS[] = "\"\\:.#$/[]";
    uint8_t j = 0;
    for (unsigned int i = 0; i < orig.length() && j < NOME_MAX_LEN; i++)
    {
        const char c = orig[i];
        // Fora da faixa imprimível ou reservado a JSON/RTDB.
        if (c < 32 || c > 126 || strchr(PROIBIDOS, c))
            continue;
        dst[j++] = c;
    }
    dst[j] = '\0';
}

// Blob válido? Protege contra NVS corrompida/versão antiga.
static bool registro_valido(const Registro &r)
{
    return r.pin <= 9999 && r.ativa <= 1 && r.nome[0] != '\0' &&
           memchr(r.nome, '\0', NOME_MAX_LEN + 1) != nullptr;
}

static void carregar()
{
    const size_t tam = sizeof(Registro);
    uint8_t n = s_prefs.getUChar("n", 0);
    size_t bytes = s_prefs.getBytesLength(NVS_REGISTROS);

    if (n > MAX_SENHAS)
        n = MAX_SENHAS;
    if ((size_t)n * tam > bytes)
        n = bytes / tam; // contador à frente do blob (escrita interrompida)
    if (n == 0 || s_prefs.getBytes(NVS_REGISTROS, s_reg, n * tam) != n * tam)
        return;

    // Compacta registros inválidos (NVS corrompida/versão antiga).
    uint8_t validos = 0;
    for (uint8_t i = 0; i < n; i++)
    {
        if (registro_valido(s_reg[i]))
            s_reg[validos++] = s_reg[i];
    }
    s_total = validos;
}

// Formato antigo: "n" (contador) + "t%02d"/"n%02d"/"a%02d" por chave.
static void migrarLegado()
{
    uint8_t n = s_prefs.getUChar("n", 0);
    if (n > MAX_SENHAS)
        n = MAX_SENHAS;

    char key[8];
    uint8_t validos = 0;
    for (uint8_t i = 0; i < n; i++)
    {
        snprintf(key, sizeof(key), "t%02u", i);
        uint16_t pin;
        if (!pin_para_num(s_prefs.getString(key, ""), pin))
            continue;

        snprintf(key, sizeof(key), "n%02u", i);
        char nome[NOME_MAX_LEN + 1];
        limpar_nome(s_prefs.getString(key, ""), nome);
        if (nome[0] == '\0')
            continue;

        snprintf(key, sizeof(key), "a%02u", i);
        Registro &r = s_reg[validos];
        r.pin = pin;
        r.ativa = s_prefs.isKey(key) ? (s_prefs.getUChar(key, 1) != 0) : 1;
        memcpy(r.nome, nome, sizeof(nome));
        validos++;
    }
    s_total = validos;
}

static void removerLegado()
{
    char key[8];
    s_prefs.begin(NVS_ESPACO, false);
    for (uint8_t i = 0; i < MAX_SENHAS; i++)
    {
        snprintf(key, sizeof(key), "t%02u", i);
        if (s_prefs.isKey(key))
            s_prefs.remove(key);
        snprintf(key, sizeof(key), "n%02u", i);
        if (s_prefs.isKey(key))
            s_prefs.remove(key);
        snprintf(key, sizeof(key), "a%02u", i);
        if (s_prefs.isKey(key))
            s_prefs.remove(key);
    }
    s_prefs.end();
}

// Grava a tabela inteira em uma única entrada NVS.
static void persistir()
{
    s_prefs.begin(NVS_ESPACO, false);
    s_prefs.putUChar(NVS_VERSAO, NVS_VERSAO_ATUAL);
    if (s_total)
        s_prefs.putBytes(NVS_REGISTROS, s_reg, s_total * sizeof(Registro));
    else if (s_prefs.isKey(NVS_REGISTROS))
        s_prefs.remove(NVS_REGISTROS);
    s_prefs.putUChar("n", s_total); // por último: contador só vale com blob
    s_prefs.end();
}

void senhas_init()
{
    bool migrou = false;
    s_total = 0;

    s_prefs.begin(NVS_ESPACO, true);
    const uint8_t versao = s_prefs.getUChar(NVS_VERSAO, 0);
    if (versao == NVS_VERSAO_ATUAL)
        carregar();
    else if (versao == 0 && s_prefs.isKey("n"))
        migrou = true; // formato antigo (t00/n00/a00)
    else if (versao != 0)
        tlogf("NVS de chaves em formato %u (esperado %u): ignorada.\n",
              versao, NVS_VERSAO_ATUAL);
    s_prefs.end();

    if (migrou)
    {
        s_prefs.begin(NVS_ESPACO, true);
        migrarLegado();
        s_prefs.end();
        removerLegado();
        persistir();
    }

    tlogf("%d chave(s) recuperada(s) da NVS.\n", s_total);
}

static int indice_pin(uint16_t pin)
{
    for (uint8_t i = 0; i < s_total; i++)
    {
        if (s_reg[i].pin == pin)
            return i;
    }
    return -1;
}

static int indice_nome(const char *nome)
{
    for (uint8_t i = 0; i < s_total; i++)
    {
        if (strcmp(nome, s_reg[i].nome) == 0)
            return i;
    }
    return -1;
}

static int indice_nome(const String &nome)
{
    return indice_nome(nome.c_str());
}

static void apagar_indice(int idx)
{
    for (int i = idx; i < s_total - 1; i++)
        s_reg[i] = s_reg[i + 1];
    s_total--;
}

bool senhas_adicionar(const String &pin, const String &nome)
{
    uint16_t numero;
    if (!pin_para_num(pin, numero))
        return false; // PIN inválido/reservado

    char nomeLimpo[NOME_MAX_LEN + 1];
    limpar_nome(nome, nomeLimpo);
    if (nomeLimpo[0] == '\0')
        return false; // nome vazio

    if (indice_pin(numero) >= 0 || indice_nome(nomeLimpo) >= 0)
        return false; // PIN ou nome duplicado

    if (s_total >= MAX_SENHAS)
    {
        tlogf("Tabela cheia (%d). PIN ignorado.\n", MAX_SENHAS);
        return false;
    }

    Registro &r = s_reg[s_total];
    r.pin = numero;
    r.ativa = 1; // novo cadastro nasce ativo
    memcpy(r.nome, nomeLimpo, sizeof(nomeLimpo));
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
    uint16_t numero;
    if (!pin_para_num(pinNovo, numero))
        return false;

    int idx = indice_nome(nome);
    if (idx < 0 || indice_pin(numero) >= 0)
        return false;

    s_reg[idx].pin = numero;
    persistir();
    s_mudou = true;
    tlogln("Chave renovada (nome mantido).");
    return true;
}

bool senhas_bloquear(const String &nome, bool bloquear)
{
    int idx = indice_nome(nome);
    if (idx < 0 || s_reg[idx].ativa == !bloquear)
        return false;
    s_reg[idx].ativa = !bloquear;
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
    for (uint8_t i = 0; i < s_total; i++)
    {
        if (s_reg[i].ativa)
            n++;
    }
    return n;
}

bool senhas_contem(const String &pin)
{
    uint16_t numero;
    if (!pin_para_num(pin, numero))
        return false;
    int idx = indice_pin(numero);
    return idx >= 0 && s_reg[idx].ativa; // bloqueada não abre
}

bool senhas_consumirMudanca()
{
    bool m = s_mudou;
    s_mudou = false;
    return m;
}
