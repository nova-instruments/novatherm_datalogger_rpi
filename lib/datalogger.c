/**
 * @file datalogger.c
 * @brief NovaTherm DataLogger - Data Logging Library Implementation
 * @author Nova Instruments
 */

#define _GNU_SOURCE  // Para strptime
#include "datalogger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

/**
 * @brief Cria diretório se não existir
 */
static bool create_directory_if_not_exists(const char* path) {
    struct stat st = {0};

    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) == -1) {
            fprintf(stderr, "Erro ao criar diretório %s: %s\n", path, strerror(errno));
            return false;
        }
        printf("Diretório criado: %s\n", path);
    }

    return true;
}

/**
 * @brief Lê o último número de registro do arquivo TXT
 */
static uint32_t get_last_record_number(const char* filepath) {
    FILE* fp = fopen(filepath, "r");
    if (!fp) return 0;

    uint32_t last_record = 0;
    char line[512];

    // Ler todas as linhas e pegar o último número de registro
    while (fgets(line, sizeof(line), fp)) {
        uint32_t record_num;
        // Formato: R;Data Hora;TPrincipal;PA
        if (sscanf(line, "%u;", &record_num) == 1) {
            last_record = record_num;
        }
    }

    fclose(fp);
    return last_record;
}

/**
 * @brief Executa comando hwclock para obter hora do RTC
 */
bool datalogger_get_rtc_time(struct tm* tm_info) {
    if (!tm_info) return false;
    
    // Primeiro tenta obter do RTC via hwclock
    FILE* fp = popen("hwclock -r 2>/dev/null", "r");
    if (fp) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), fp)) {
            pclose(fp);
            
            // Tentar fazer parse do formato do hwclock
            // Formato típico: "2024-09-15 14:30:25.123456-03:00"
            if (strptime(buffer, "%Y-%m-%d %H:%M:%S", tm_info)) {
                return true;
            }
        }
        pclose(fp);
    }
    
    // Fallback: usar hora do sistema
    time_t now = time(NULL);
    struct tm* sys_time = localtime(&now);
    if (sys_time) {
        *tm_info = *sys_time;
        return true;
    }
    
    return false;
}

datalogger_context_t* datalogger_init(const char* device_name, int num_channels) {
    if (!device_name || strlen(device_name) == 0) {
        fprintf(stderr, "Erro: Nome do dispositivo não pode ser vazio\n");
        return NULL;
    }

    // Validar número de canais
    if (num_channels < 1) num_channels = 1;
    if (num_channels > MODBUS_NUM_CHANNELS) num_channels = MODBUS_NUM_CHANNELS;

    datalogger_context_t* ctx = malloc(sizeof(datalogger_context_t));
    if (!ctx) {
        fprintf(stderr, "Erro: Falha ao alocar memória para contexto do datalogger\n");
        return NULL;
    }

    // Inicializar estrutura
    memset(ctx, 0, sizeof(datalogger_context_t));
    strncpy(ctx->device_name, device_name, sizeof(ctx->device_name) - 1);
    ctx->num_channels = num_channels;
    ctx->initialized = false;
    ctx->log_file = NULL;
    ctx->db = NULL;

    // Criar diretório de logs
    if (!create_directory_if_not_exists(DATALOGGER_LOG_DIR)) {
        free(ctx);
        return NULL;
    }

    // Gerar nome do arquivo de log (nome fixo sem timestamp)
    snprintf(ctx->log_file_path, sizeof(ctx->log_file_path),
             "%s/%s.txt",
             DATALOGGER_LOG_DIR,
             ctx->device_name);

    // Gerar nome do arquivo de banco de dados (nome fixo sem timestamp)
    snprintf(ctx->db_file_path, sizeof(ctx->db_file_path),
             "%s/%s.db",
             DATALOGGER_LOG_DIR,
             ctx->device_name);

    // Verificar se arquivo já existe e obter último número de registro
    bool file_exists = (access(ctx->log_file_path, F_OK) == 0);
    if (file_exists) {
        ctx->record_counter = get_last_record_number(ctx->log_file_path);
        printf("📊 Arquivo existente encontrado. Último registro: %u\n", ctx->record_counter);
    } else {
        ctx->record_counter = 0;
        printf("📄 Criando novos arquivos de log\n");
    }
    
    // Abrir arquivo de log em modo append
    ctx->log_file = fopen(ctx->log_file_path, "a");
    if (!ctx->log_file) {
        fprintf(stderr, "Erro ao abrir arquivo de log %s: %s\n",
                ctx->log_file_path, strerror(errno));
        free(ctx);
        return NULL;
    }

    // Criar cabeçalho apenas se arquivo NÃO existia antes
    if (!file_exists) {
        printf("📝 Criando cabeçalho do arquivo TXT\n");
        if (!datalogger_create_header(ctx)) {
            fclose(ctx->log_file);
            free(ctx);
            return NULL;
        }
    } else {
        printf("📝 Continuando gravação no arquivo TXT existente\n");
    }

    // Inicializar banco de dados
    if (!datalogger_init_database(ctx)) {
        printf("⚠️  Aviso: Falha ao inicializar banco SQLite (continuando apenas com TXT)\n");
    }

    ctx->initialized = true;

    printf("DataLogger inicializado:\n");
    printf("  Dispositivo: %s\n", ctx->device_name);
    printf("  Arquivo TXT: %s\n", ctx->log_file_path);
    if (ctx->db) {
        printf("  Arquivo DB: %s\n", ctx->db_file_path);
    }
    
    return ctx;
}

void datalogger_cleanup(datalogger_context_t* ctx) {
    if (!ctx) return;

    if (ctx->log_file) {
        datalogger_sync(ctx);
        fclose(ctx->log_file);
        ctx->log_file = NULL;
    }

    // Finalizar banco de dados
    datalogger_cleanup_database(ctx);

    printf("DataLogger finalizado. Total de registros: %u\n", ctx->record_counter);
    free(ctx);
}

bool datalogger_create_header(datalogger_context_t* ctx) {
    if (!ctx || !ctx->log_file) return false;

    // Escrever cabeçalho no formato NT18B07 (dinâmico baseado em num_channels)
    fprintf(ctx->log_file, "NAME: %s\n", ctx->device_name);
    fprintf(ctx->log_file, "R;Data Hora");

    // Adicionar colunas de canais dinamicamente
    for (int i = 0; i < ctx->num_channels; i++) {
        fprintf(ctx->log_file, ";CH%d", i + 1);
    }

    // Adicionar colunas de status dos relés
    fprintf(ctx->log_file, ";Compressor;Resistencia\n");

    fflush(ctx->log_file);
    return true;
}

bool datalogger_convert_modbus_data(const modbus_data_t* modbus_data,
                                   datalogger_record_t* record,
                                   uint32_t record_number) {
    if (!modbus_data || !record) return false;

    // Limpar estrutura
    memset(record, 0, sizeof(datalogger_record_t));

    // Preencher dados básicos
    record->record_number = record_number;

    // Obter timestamp do RTC
    if (!datalogger_get_rtc_time(&record->timestamp)) {
        return false;
    }

    // Converter dados Modbus dos 7 canais do NT18B07
    for (int i = 0; i < MODBUS_NUM_CHANNELS; i++) {
        record->ch_temp[i] = modbus_data->ch_temp[i];
        record->ch_valid[i] = modbus_data->ch_valid[i];
        record->ch_error[i] = modbus_data->ch_error[i];
    }

    return true;
}

bool datalogger_write_record(datalogger_context_t* ctx, const datalogger_record_t* record) {
    if (!ctx || !ctx->log_file || !record) return false;

    // Formatar data e hora (formato brasileiro: DD/MM/YYYY HH:MM:SS)
    char datetime_str[64];
    strftime(datetime_str, sizeof(datetime_str), "%d/%m/%Y %H:%M:%S", &record->timestamp);

    // Escrever registro no formato: R;Data Hora;CH1;CH2;...;CHn (dinâmico)
    fprintf(ctx->log_file, "%u;%s", record->record_number, datetime_str);

    // Escrever temperaturas apenas dos canais configurados
    for (int i = 0; i < ctx->num_channels; i++) {
        fprintf(ctx->log_file, ";");

        if (record->ch_valid[i]) {
            if (record->ch_error[i]) {
                fprintf(ctx->log_file, "SENSOR_ERROR");
            } else {
                fprintf(ctx->log_file, "%.1f", record->ch_temp[i]);
            }
        } else {
            fprintf(ctx->log_file, "ERROR");
        }
    }

    // Escrever status dos relés (0 = desligado, 1 = ligado)
    fprintf(ctx->log_file, ";%d;%d", record->compressor_on ? 1 : 0, record->heater_on ? 1 : 0);

    fprintf(ctx->log_file, "\n");
    fflush(ctx->log_file);
    return true;
}

bool datalogger_log_data(datalogger_context_t* ctx, const modbus_data_t* modbus_data,
                        bool compressor_on, bool heater_on) {
    if (!ctx || !ctx->initialized || !modbus_data) return false;

    // Incrementar contador
    ctx->record_counter++;

    // Converter dados
    datalogger_record_t record;
    if (!datalogger_convert_modbus_data(modbus_data, &record, ctx->record_counter)) {
        fprintf(stderr, "Erro ao converter dados Modbus para registro\n");
        return false;
    }

    // Adicionar status dos relés
    record.compressor_on = compressor_on;
    record.heater_on = heater_on;

    // Escrever registro no arquivo TXT
    if (!datalogger_write_record(ctx, &record)) {
        fprintf(stderr, "Erro ao escrever registro no arquivo de log TXT\n");
        return false;
    }

    // Escrever registro no banco SQLite (se disponível)
    if (ctx->db) {
        datalogger_db_record_t db_record;
        if (datalogger_convert_to_db_record(&record, &db_record)) {
            if (!datalogger_insert_db_record(ctx, &db_record)) {
                printf("⚠️  Aviso: Falha ao inserir registro no banco SQLite\n");
            }
        }
    }

    return true;
}

void datalogger_sync(datalogger_context_t* ctx) {
    if (ctx && ctx->log_file) {
        fflush(ctx->log_file);
        fsync(fileno(ctx->log_file));
    }
}

bool datalogger_get_log_info(datalogger_context_t* ctx, long* file_size, uint32_t* record_count) {
    if (!ctx) return false;
    
    if (record_count) {
        *record_count = ctx->record_counter;
    }
    
    if (file_size && ctx->log_file) {
        long current_pos = ftell(ctx->log_file);
        fseek(ctx->log_file, 0, SEEK_END);
        *file_size = ftell(ctx->log_file);
        fseek(ctx->log_file, current_pos, SEEK_SET);
    }
    
    return true;
}

void datalogger_print_stats(datalogger_context_t* ctx) {
    if (!ctx) return;
    
    long file_size = 0;
    uint32_t record_count = 0;
    
    datalogger_get_log_info(ctx, &file_size, &record_count);
    
    printf("=== Estatísticas do DataLogger ===\n");
    printf("Dispositivo: %s\n", ctx->device_name);
    printf("Arquivo: %s\n", ctx->log_file_path);
    printf("Registros: %u\n", record_count);
    printf("Tamanho do arquivo: %ld bytes\n", file_size);
    printf("==================================\n");
}

/**
 * @brief Inicializa o banco de dados SQLite
 */
bool datalogger_init_database(datalogger_context_t* ctx) {
    if (!ctx) return false;

    // Abrir banco de dados
    int rc = sqlite3_open(ctx->db_file_path, &ctx->db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Erro ao abrir banco SQLite: %s\n", sqlite3_errmsg(ctx->db));
        sqlite3_close(ctx->db);
        ctx->db = NULL;
        return false;
    }

    // Criar tabelas
    if (!datalogger_create_tables(ctx)) {
        sqlite3_close(ctx->db);
        ctx->db = NULL;
        return false;
    }

    printf("📊 Banco SQLite inicializado: %s\n", ctx->db_file_path);
    return true;
}

/**
 * @brief Cria as tabelas do banco de dados
 */
bool datalogger_create_tables(datalogger_context_t* ctx) {
    if (!ctx || !ctx->db) return false;

    char* err_msg = NULL;

    // Criar tabela principal DataGrpData (dinâmica baseada em num_channels)
    char create_data_table[1024];
    snprintf(create_data_table, sizeof(create_data_table),
        "CREATE TABLE IF NOT EXISTS DataGrpData ("
        "IndexID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "CollectTime INTEGER NOT NULL");

    // Adicionar colunas de canais dinamicamente
    for (int i = 0; i < ctx->num_channels; i++) {
        char ch_column[64];
        snprintf(ch_column, sizeof(ch_column), ",CH%d REAL NOT NULL", i + 1);
        strncat(create_data_table, ch_column, sizeof(create_data_table) - strlen(create_data_table) - 1);
    }

    // Adicionar colunas de status dos relés
    strncat(create_data_table, ",Compressor INTEGER NOT NULL", sizeof(create_data_table) - strlen(create_data_table) - 1);
    strncat(create_data_table, ",Heater INTEGER NOT NULL", sizeof(create_data_table) - strlen(create_data_table) - 1);

    strncat(create_data_table, ");", sizeof(create_data_table) - strlen(create_data_table) - 1);

    int rc = sqlite3_exec(ctx->db, create_data_table, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Erro ao criar tabela DataGrpData: %s\n", err_msg);
        sqlite3_free(err_msg);
        return false;
    }

    // Criar tabela de informações DBInfo
    const char* create_info_table =
        "CREATE TABLE IF NOT EXISTS DBInfo ("
        "version INTEGER DEFAULT 1,"
        "MaxID INTEGER DEFAULT 0,"
        "MinID INTEGER DEFAULT 0,"
        "StartTime INTEGER DEFAULT 0,"
        "EndTime INTEGER DEFAULT 0,"
        "Value0 INTEGER DEFAULT 0,"
        "Value1 INTEGER DEFAULT 0,"
        "Value2 INTEGER DEFAULT 0,"
        "Value3 INTEGER DEFAULT 0,"
        "Value4 INTEGER DEFAULT 0"
        ");";

    rc = sqlite3_exec(ctx->db, create_info_table, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Erro ao criar tabela DBInfo: %s\n", err_msg);
        sqlite3_free(err_msg);
        return false;
    }

    // Inserir registro inicial na DBInfo se não existir
    const char* init_info =
        "INSERT OR IGNORE INTO DBInfo (rowid, version, StartTime) "
        "SELECT 1, 1, strftime('%s', 'now') * 1000 "
        "WHERE NOT EXISTS (SELECT 1 FROM DBInfo);";

    rc = sqlite3_exec(ctx->db, init_info, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Erro ao inicializar DBInfo: %s\n", err_msg);
        sqlite3_free(err_msg);
        return false;
    }

    return true;
}

/**
 * @brief Converte registro TXT para registro do banco
 */
bool datalogger_convert_to_db_record(const datalogger_record_t* txt_record,
                                    datalogger_db_record_t* db_record) {
    if (!txt_record || !db_record) return false;

    // Limpar estrutura
    memset(db_record, 0, sizeof(datalogger_db_record_t));

    // Converter timestamp para milissegundos
    time_t timestamp = mktime((struct tm*)&txt_record->timestamp);
    db_record->CollectTime = (long long)timestamp * 1000;

    // Converter temperaturas dos 7 canais (arredondar para 1 casa decimal)
    float* ch_fields[MODBUS_NUM_CHANNELS] = {
        &db_record->CH1, &db_record->CH2, &db_record->CH3, &db_record->CH4,
        &db_record->CH5, &db_record->CH6, &db_record->CH7
    };

    for (int i = 0; i < MODBUS_NUM_CHANNELS; i++) {
        if (txt_record->ch_valid[i] && !txt_record->ch_error[i]) {
            *ch_fields[i] = roundf(txt_record->ch_temp[i] * 10.0f) / 10.0f;  // 1 casa decimal
        } else {
            *ch_fields[i] = -999.9f;  // Valor especial para erro/sensor desconectado
        }
    }

    // Converter status dos relés
    db_record->Compressor = txt_record->compressor_on ? 1 : 0;
    db_record->Heater = txt_record->heater_on ? 1 : 0;

    return true;
}

/**
 * @brief Insere registro no banco de dados
 */
bool datalogger_insert_db_record(datalogger_context_t* ctx,
                                const datalogger_db_record_t* db_record) {
    if (!ctx || !ctx->db || !db_record) return false;

    // Construir SQL dinamicamente baseado em num_channels
    char sql[512];
    char columns[256] = "CollectTime";
    char values[256] = "?";

    for (int i = 0; i < ctx->num_channels; i++) {
        char ch_col[16];
        snprintf(ch_col, sizeof(ch_col), ",CH%d", i + 1);
        strncat(columns, ch_col, sizeof(columns) - strlen(columns) - 1);
        strncat(values, ",ROUND(?, 1)", sizeof(values) - strlen(values) - 1);
    }

    // Adicionar colunas de status dos relés
    strncat(columns, ",Compressor,Heater", sizeof(columns) - strlen(columns) - 1);
    strncat(values, ",?,?", sizeof(values) - strlen(values) - 1);

    snprintf(sql, sizeof(sql), "INSERT INTO DataGrpData (%s) VALUES (%s);", columns, values);

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(ctx->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Erro ao preparar statement: %s\n", sqlite3_errmsg(ctx->db));
        return false;
    }

    // Bind dos parâmetros dinamicamente
    sqlite3_bind_int64(stmt, 1, db_record->CollectTime);

    // Array de ponteiros para os campos de temperatura (todos os 7 canais)
    const float* ch_fields[MODBUS_NUM_CHANNELS] = {
        &db_record->CH1, &db_record->CH2, &db_record->CH3, &db_record->CH4,
        &db_record->CH5, &db_record->CH6, &db_record->CH7
    };

    // Fazer bind apenas dos canais configurados
    for (int i = 0; i < ctx->num_channels; i++) {
        sqlite3_bind_double(stmt, i + 2, *ch_fields[i]);
    }

    // Bind dos status dos relés
    sqlite3_bind_int(stmt, ctx->num_channels + 2, db_record->Compressor);
    sqlite3_bind_int(stmt, ctx->num_channels + 3, db_record->Heater);

    // Executar
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Erro ao inserir registro: %s\n", sqlite3_errmsg(ctx->db));
        return false;
    }

    // Atualizar informações do banco
    datalogger_update_db_info(ctx);

    return true;
}

/**
 * @brief Atualiza informações do banco (tabela DBInfo)
 */
bool datalogger_update_db_info(datalogger_context_t* ctx) {
    if (!ctx || !ctx->db) return false;

    const char* sql =
        "UPDATE DBInfo SET "
        "MaxID = (SELECT MAX(IndexID) FROM DataGrpData),"
        "MinID = (SELECT MIN(IndexID) FROM DataGrpData),"
        "EndTime = strftime('%s', 'now') * 1000 "
        "WHERE rowid = 1;";

    char* err_msg = NULL;
    int rc = sqlite3_exec(ctx->db, sql, NULL, NULL, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Erro ao atualizar DBInfo: %s\n", err_msg);
        sqlite3_free(err_msg);
        return false;
    }

    return true;
}

/**
 * @brief Finaliza o banco de dados SQLite
 */
void datalogger_cleanup_database(datalogger_context_t* ctx) {
    if (!ctx) return;

    if (ctx->db) {
        // Atualizar informações finais
        datalogger_update_db_info(ctx);

        // Fechar banco
        sqlite3_close(ctx->db);
        ctx->db = NULL;

        printf("📊 Banco SQLite finalizado\n");
    }
}
