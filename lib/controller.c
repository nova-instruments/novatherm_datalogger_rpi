/**
 * @file controller.c
 * @brief NovaTherm DataLogger - Temperature Controller Implementation
 * @author Nova Instruments
 */

#include "controller.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

controller_context_t* controller_init(float setpoint) {
    controller_context_t* ctx = (controller_context_t*)malloc(sizeof(controller_context_t));
    if (!ctx) {
        fprintf(stderr, "❌ Erro ao alocar memória para controlador\n");
        return NULL;
    }

    // Inicializar estrutura
    memset(ctx, 0, sizeof(controller_context_t));

    // Configurações iniciais
    ctx->setpoint = setpoint;
    ctx->hysteresis = CONTROLLER_HYSTERESIS;
    ctx->state = CONTROLLER_STATE_IDLE;
    ctx->compressor_on = false;
    ctx->heater_on = false;

    // Inicializar timestamp do último degelo (agora)
    ctx->last_defrost_time = time(NULL);
    ctx->defrost_start_time = 0;
    ctx->defrost_needed = false;

    // Estatísticas
    ctx->cooling_cycles = 0;
    ctx->defrost_cycles = 0;
    ctx->defrost_interrupted = 0;

    printf("🎛️  Controlador inicializado:\n");
    printf("   └─ Setpoint: %.1f°C | Histerese: %.1f°C\n", ctx->setpoint, ctx->hysteresis);
    printf("   └─ Degelo: a cada %d horas | Threshold: %.1f°C\n",
           DEFROST_INTERVAL_HOURS, DEFROST_TEMP_THRESHOLD);

    return ctx;
}

void controller_cleanup(controller_context_t* ctx) {
    if (ctx) {
        free(ctx);
    }
}

void controller_set_setpoint(controller_context_t* ctx, float setpoint) {
    if (ctx) {
        ctx->setpoint = setpoint;
    }
}

void controller_set_hysteresis(controller_context_t* ctx, float hysteresis) {
    if (!ctx) {
        return;
    }

    if (hysteresis < 0.1f) {
        hysteresis = 0.1f;
    }
    if (hysteresis > 5.0f) {
        hysteresis = 5.0f;
    }

    if (ctx->hysteresis != hysteresis) {
        printf("🎯 Histerese alterada: %.1f°C -> %.1f°C\n", ctx->hysteresis, hysteresis);
        ctx->hysteresis = hysteresis;
    }
}

float controller_get_setpoint(controller_context_t* ctx) {
    return ctx ? ctx->setpoint : 0.0f;
}

float controller_get_hysteresis(controller_context_t* ctx) {
    return ctx ? ctx->hysteresis : CONTROLLER_HYSTERESIS;
}

bool controller_update(controller_context_t* ctx, float temp_ch1, float temp_ch2,
                      bool ch1_valid, bool ch2_valid) {
    if (!ctx) {
        return false;
    }

    time_t now = time(NULL);

    // ========== VERIFICAR NECESSIDADE DE DEGELO ==========
    // Calcular tempo desde último degelo
    double hours_since_defrost = difftime(now, ctx->last_defrost_time) / 3600.0;

    // Se passou 24 horas, marcar necessidade de degelo
    if (hours_since_defrost >= DEFROST_INTERVAL_HOURS && !ctx->defrost_needed) {
        ctx->defrost_needed = true;
        printf("\n⏰ DEGELO: Intervalo de %d horas atingido - verificação agendada\n", DEFROST_INTERVAL_HOURS);
    }

    // ========== CONTROLE DE DEGELO ==========
    if (ctx->state == CONTROLLER_STATE_DEFROSTING) {
        // Já está em degelo - verificar condições de parada

        double defrost_duration = difftime(now, ctx->defrost_start_time);

        // Condição 1: PR2 atingiu 0°C - parar degelo
        if (ch2_valid && temp_ch2 >= DEFROST_STOP_TEMP) {
            printf("🛑 DEGELO INTERROMPIDO: PR2 atingiu %.1f°C (duração: %.0fs)\n",
                   temp_ch2, defrost_duration);
            ctx->state = CONTROLLER_STATE_IDLE;
            ctx->heater_on = false;
            ctx->defrost_needed = false;
            ctx->last_defrost_time = now;
            ctx->defrost_interrupted++;
            return true;
        }

        // Condição 2: Tempo máximo de 5 minutos atingido
        if (defrost_duration >= DEFROST_MAX_DURATION_SEC) {
            printf("✅ DEGELO CONCLUÍDO: Tempo máximo atingido (%d segundos)\n",
                   DEFROST_MAX_DURATION_SEC);
            ctx->state = CONTROLLER_STATE_IDLE;
            ctx->heater_on = false;
            ctx->defrost_needed = false;
            ctx->last_defrost_time = now;
            return true;
        }

        // Continuar degelo
        return true;
    }

    // ========== INICIAR DEGELO SE NECESSÁRIO ==========
    if (ctx->defrost_needed && ch2_valid) {
        // Verificar se PR2 <= -12°C
        if (temp_ch2 <= DEFROST_TEMP_THRESHOLD) {
            printf("\n❄️  INICIANDO DEGELO:\n");
            printf("   └─ PR2: %.1f°C (threshold: %.1f°C)\n", temp_ch2, DEFROST_TEMP_THRESHOLD);
            printf("   └─ Duração máxima: %d segundos\n", DEFROST_MAX_DURATION_SEC);
            ctx->state = CONTROLLER_STATE_DEFROSTING;
            ctx->heater_on = true;
            ctx->compressor_on = false;  // Desligar compressor durante degelo
            ctx->defrost_start_time = now;
            ctx->defrost_cycles++;
            return true;
        } else {
            // PR2 > -12°C, não precisa degelo
            printf("✅ DEGELO NÃO NECESSÁRIO: PR2 = %.1f°C (> %.1f°C)\n",
                   temp_ch2, DEFROST_TEMP_THRESHOLD);
            ctx->defrost_needed = false;
            ctx->last_defrost_time = now;  // Resetar timer
        }
    }

    // ========== CONTROLE DE TEMPERATURA ON/OFF ==========
    if (!ch1_valid) {
        // Sensor PR1 inválido - modo seguro (desligar compressor)
        if (ctx->compressor_on) {
            printf("\n⚠️  MODO SEGURO: PR1 inválido - desligando compressor\n");
            ctx->compressor_on = false;
            ctx->state = CONTROLLER_STATE_IDLE;
        }
        return false;
    }

    // Lógica ON/OFF com histerese
    // Compressor liga quando: temp > setpoint + histerese
    // Compressor desliga quando: temp < setpoint

    if (temp_ch1 > (ctx->setpoint + ctx->hysteresis)) {
        // Temperatura acima do limite superior - ligar compressor
        if (!ctx->compressor_on) {
            printf("\n🌡️  CONTROLE: PR1=%.1f°C > Setpoint+Histerese=%.1f°C\n",
                   temp_ch1, ctx->setpoint + ctx->hysteresis);
            ctx->compressor_on = true;
            ctx->state = CONTROLLER_STATE_COOLING;
            ctx->cooling_cycles++;
        }
    } else if (temp_ch1 < ctx->setpoint) {
        // Temperatura abaixo do setpoint - desligar compressor
        if (ctx->compressor_on) {
            printf("\n🌡️  CONTROLE: PR1=%.1f°C < Setpoint=%.1f°C\n",
                   temp_ch1, ctx->setpoint);
            ctx->compressor_on = false;
            ctx->state = CONTROLLER_STATE_WAITING;
        }
    }
    // Entre setpoint e setpoint+histerese: manter estado atual (histerese)

    return true;
}

controller_state_t controller_get_state(controller_context_t* ctx) {
    return ctx ? ctx->state : CONTROLLER_STATE_IDLE;
}

const char* controller_get_state_name(controller_state_t state) {
    switch (state) {
        case CONTROLLER_STATE_IDLE:       return "IDLE";
        case CONTROLLER_STATE_COOLING:    return "COOLING";
        case CONTROLLER_STATE_WAITING:    return "WAITING";
        case CONTROLLER_STATE_DEFROSTING: return "DEFROSTING";
        default:                          return "UNKNOWN";
    }
}

bool controller_should_compressor_be_on(controller_context_t* ctx) {
    if (!ctx) return false;
    // Compressor desliga durante degelo
    if (ctx->state == CONTROLLER_STATE_DEFROSTING) {
        return false;
    }
    return ctx->compressor_on;
}

bool controller_should_heater_be_on(controller_context_t* ctx) {
    return ctx ? ctx->heater_on : false;
}

void controller_force_defrost(controller_context_t* ctx) {
    if (ctx) {
        printf("🔧 Degelo forçado manualmente\n");
        ctx->defrost_needed = true;
        // Resetar timestamp para forçar verificação imediata
        ctx->last_defrost_time = time(NULL) - (DEFROST_INTERVAL_HOURS * 3600);
    }
}

uint32_t controller_get_time_to_next_defrost(controller_context_t* ctx) {
    if (!ctx) return 0;

    time_t now = time(NULL);
    double seconds_since_defrost = difftime(now, ctx->last_defrost_time);
    double seconds_interval = DEFROST_INTERVAL_HOURS * 3600.0;

    if (seconds_since_defrost >= seconds_interval) {
        return 0;  // Já passou do horário
    }

    return (uint32_t)(seconds_interval - seconds_since_defrost);
}
