/**
 * @file lcd_bargraph_demo.c
 * @brief NovaTherm DataLogger - Demonstração BigFont (Números Grandes)
 * @author Nova Instruments
 *
 * Este programa demonstra o uso da biblioteca BigFont para exibir
 * temperatura em números grandes no LCD 20x4:
 *
 * Linha 0: WiFi            SP xx.x
 * Linha 1:      12.3°C           (temperatura grande - parte superior)
 * Linha 2:      12.3°C           (temperatura grande - parte inferior)
 * Linha 3: MAX xx.x      MIN xx.x
 *
 * Características:
 * - Baseado em https://coeleveld.com/bigfont/ (BigFont02)
 * - Usa 8 caracteres customizados (CGRAM) para formar dígitos grandes
 * - Cada dígito ocupa 3 colunas x 2 linhas
 * - Temperatura exibida com 1 casa decimal
 * - Atualiza dinamicamente conforme temperatura varia
 * - Mostra setpoint, máximo e mínimo
 * - Indicador de WiFi
 * - Atualização a cada 500ms
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include "lcd_i2c.h"

// Configurações da simulação
#define UPDATE_INTERVAL_MS  500     // Atualizar display a cada 500ms
#define TEMP_MIN_SCALE     -10.0f   // Temperatura mínima da escala
#define TEMP_MAX_SCALE      30.0f   // Temperatura máxima da escala
#define TEMP_AMPLITUDE      10.0f   // Amplitude da variação de temperatura
#define TEMP_OFFSET         10.0f   // Temperatura média

// Variável global para controle do loop
static volatile bool running = true;

/**
 * @brief Handler para sinais de interrupção (Ctrl+C)
 */
void signal_handler(int signum) {
    (void)signum;
    running = false;
}

/**
 * @brief Desenha o layout estático do display
 *
 * Linha 0: Indicador WiFi, título "TEMP", setpoint
 * Linha 3: Labels MAX e MIN
 */
void draw_static_layout(lcd_context_t* ctx) {
    if (!ctx) return;

    lcd_clear(ctx);

    // Linha 0: WiFi + TEMP + SP
    lcd_set_cursor(ctx, 0, 0);
    lcd_print(ctx, "WiFi");

    lcd_set_cursor(ctx, 7, 0);
    lcd_print(ctx, "TEMP");

    lcd_set_cursor(ctx, 14, 0);
    lcd_print(ctx, "SP");

    // Linha 3: Labels MAX e MIN
    lcd_set_cursor(ctx, 0, 3);
    lcd_print(ctx, "MAX");

    lcd_set_cursor(ctx, 11, 3);
    lcd_print(ctx, "MIN");
}

/**
 * @brief Atualiza os valores dinâmicos do display
 *
 * Atualiza setpoint, MAX e MIN na linha 0 e 3
 */
void update_dynamic_values(lcd_context_t* ctx) {
    if (!ctx) return;

    char value_str[8];

    // Atualizar setpoint (após "SP" na posição 16)
    lcd_set_cursor(ctx, 16, 0);
    snprintf(value_str, sizeof(value_str), "%4.1f", ctx->setpoint);
    lcd_print(ctx, value_str);

    // Atualizar MAX (após "MAX" na posição 4)
    lcd_set_cursor(ctx, 4, 3);
    snprintf(value_str, sizeof(value_str), "%5.1f", ctx->temp_max);
    lcd_print(ctx, value_str);

    // Atualizar MIN (após "MIN" na posição 15)
    lcd_set_cursor(ctx, 15, 3);
    snprintf(value_str, sizeof(value_str), "%5.1f", ctx->temp_min);
    lcd_print(ctx, value_str);
}

/**
 * @brief Desenha a temperatura grande no centro do display
 *
 * Usa dígitos grandes (3 colunas x 2 linhas cada) para exibir
 * a temperatura com 1 casa decimal
 */
void draw_temperature_large(lcd_context_t* ctx, float temperature) {
    if (!ctx) return;

    // A função lcd_draw_bargraph agora desenha temperatura grande
    // Passa 0 como coluna (será ignorado, temperatura é centralizada)
    lcd_draw_bargraph(ctx, 0, temperature, TEMP_MIN_SCALE, TEMP_MAX_SCALE);
}

/**
 * @brief Simula uma variação de temperatura ao longo do tempo
 * 
 * Usa uma função senoidal para criar uma variação suave
 */
float simulate_temperature(void) {
    static float time_counter = 0.0f;
    
    // Incrementar contador de tempo
    time_counter += 0.1f;
    
    // Gerar temperatura usando função senoidal
    float temp = TEMP_OFFSET + TEMP_AMPLITUDE * sinf(time_counter);
    
    return temp;
}

int main(void) {
    printf("=== NovaTherm LCD Bargraph Demo ===\n");
    printf("Demonstração de barras verticais no LCD 20x4\n");
    printf("Pressione Ctrl+C para sair\n\n");
    
    // Configurar handler para Ctrl+C
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Inicializar LCD
    lcd_context_t* lcd = lcd_init();
    if (!lcd) {
        fprintf(stderr, "❌ Erro ao inicializar LCD\n");
        return 1;
    }

    printf("✅ LCD inicializado\n");
    printf("✅ Caracteres customizados criados\n");

    // Configurar parâmetros iniciais
    lcd->setpoint = 5.0f;
    lcd->wifi_connected = true;  // Simular WiFi conectado

    // Desenhar layout estático (já inclui lcd_clear)
    draw_static_layout(lcd);

    printf("✅ Layout estático desenhado\n");
    printf("\n🔄 Iniciando simulação de temperatura...\n\n");

    // Loop principal
    int iteration = 0;
    while (running) {
        // Simular leitura de temperatura
        float current_temp = simulate_temperature();

        // Atualizar valores de máximo e mínimo
        lcd_update_temp_minmax(lcd, current_temp);

        // Atualizar valores dinâmicos (SP, MAX, MIN)
        update_dynamic_values(lcd);

        // Desenhar temperatura grande no centro
        draw_temperature_large(lcd, current_temp);

        // Exibir informações no console
        if (iteration % 4 == 0) {  // A cada 2 segundos
            printf("📊 Temp: %5.1f°C | SP: %4.1f°C | MAX: %5.1f°C | MIN: %5.1f°C\n",
                   current_temp, lcd->setpoint, lcd->temp_max, lcd->temp_min);
        }

        iteration++;

        // Aguardar intervalo de atualização
        usleep(UPDATE_INTERVAL_MS * 1000);
    }

    // Limpeza
    printf("\n\n🛑 Encerrando...\n");
    lcd_clear(lcd);
    lcd_cleanup(lcd);

    printf("✅ Demo finalizado com sucesso!\n");

    return 0;
}

