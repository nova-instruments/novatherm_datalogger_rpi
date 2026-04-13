# 📊 Implementação de Interface LCD com Barras Verticais

## 📋 Visão Geral

Este documento descreve a implementação de uma interface de display LCD 20x4 com barras verticais animadas para visualização de temperatura no NovaTherm DataLogger.

## 🎯 Especificação do Layout

```
┌────────────────────┐
│ ☁ TEMP      SP 5.0 │  Linha 0: Indicador WiFi + Título + Setpoint
│ ████████████████   │  Linha 1: Barras verticais (parte superior)
│ ████████████████   │  Linha 2: Barras verticais (parte inferior)
│ MAX 15.2   MIN 4.8 │  Linha 3: Temperatura máxima e mínima
└────────────────────┘
```

### Linha 0 (Cabeçalho)
- **Coluna 0:** Indicador de WiFi (1 caractere)
- **Colunas 7-10:** Texto "TEMP" centralizado
- **Colunas 14-19:** "SP xx.x" (setpoint com 1 casa decimal)

### Linhas 1-2 (Barras Verticais)
- **Colunas 1-18:** 18 barras verticais (2 linhas de altura cada)
- Cada barra usa caracteres customizados do CGRAM
- Altura da barra proporcional ao valor da temperatura

### Linha 3 (Rodapé)
- **Colunas 0-8:** "MAX xx.x" (temperatura máxima)
- **Colunas 14-19:** "MIN xx.x" (temperatura mínima)

## 🔧 Implementação Técnica

### 1. Caracteres Customizados (CGRAM)

O LCD HD44780 possui 64 bytes de CGRAM que permitem definir até 8 caracteres customizados de 5x8 pixels.

#### Estrutura de um Caractere

Cada caractere é definido por 8 bytes (1 byte por linha):
```c
uint8_t custom_char[8] = {
    0b11111,  // Linha 0: █████
    0b10001,  // Linha 1: █   █
    0b10001,  // Linha 2: █   █
    0b10001,  // Linha 3: █   █
    0b10001,  // Linha 4: █   █
    0b10001,  // Linha 5: █   █
    0b10001,  // Linha 6: █   █
    0b11111   // Linha 7: █████
};
```

#### Caracteres Implementados

| Char | Descrição | Pixels Preenchidos | Uso |
|------|-----------|-------------------|-----|
| 0 | Vazio | 0/7 | Temperatura muito baixa |
| 1 | 1 linha | 1/7 | ~14% da escala |
| 2 | 2 linhas | 2/7 | ~29% da escala |
| 3 | 3 linhas | 3/7 | ~43% da escala |
| 4 | 4 linhas | 4/7 | ~57% da escala |
| 5 | 5 linhas | 5/7 | ~71% da escala |
| 6 | 6 linhas | 6/7 | ~86% da escala |
| 7 | Cheio | 7/7 | Temperatura máxima |

### 2. Função `lcd_create_bargraph_chars()`

Cria os 8 caracteres customizados no CGRAM do LCD.

```c
void lcd_create_bargraph_chars(lcd_context_t* ctx) {
    // Para cada caractere (0-7)
    for (int i = 0; i < 8; i++) {
        // Posicionar no CGRAM (endereço = char_num * 8)
        lcd_command(ctx, LCD_CMD_SET_CGRAM | (i * 8));
        
        // Escrever 8 bytes (8 linhas do caractere)
        for (int j = 0; j < 8; j++) {
            lcd_write_char(ctx, chars[i][j]);
        }
    }
    
    // Retornar ao modo DDRAM (display normal)
    lcd_command(ctx, LCD_CMD_SET_DDRAM);
}
```

**Chamada:** Executada automaticamente em `lcd_init()`.

### 3. Função `lcd_draw_bargraph()`

Desenha uma barra vertical em uma coluna específica.

```c
void lcd_draw_bargraph(lcd_context_t* ctx, uint8_t col, 
                       float value, float min_value, float max_value)
```

#### Algoritmo

1. **Normalização:** Converte o valor para faixa 0.0-1.0
   ```c
   float normalized = (value - min_value) / (max_value - min_value);
   ```

2. **Cálculo de pixels:** Determina quantos pixels preencher (0-14)
   ```c
   int total_pixels = 14;  // 2 linhas × 7 pixels úteis
   int filled_pixels = (int)(normalized * total_pixels);
   ```

3. **Seleção de caracteres:** Escolhe qual caractere usar em cada linha
   ```c
   if (filled_pixels <= 7) {
       char_bottom = filled_pixels;  // Linha inferior
       char_top = 0;                 // Linha superior vazia
   } else {
       char_bottom = 7;              // Linha inferior cheia
       char_top = filled_pixels - 7; // Linha superior parcial
   }
   ```

4. **Renderização:** Escreve os caracteres nas posições corretas
   ```c
   lcd_set_cursor(ctx, col, 1);  // Linha superior
   lcd_write_char(ctx, char_top);
   
   lcd_set_cursor(ctx, col, 2);  // Linha inferior
   lcd_write_char(ctx, char_bottom);
   ```

### 4. Função `lcd_update_temp_minmax()`

Rastreia automaticamente os valores máximo e mínimo de temperatura.

```c
void lcd_update_temp_minmax(lcd_context_t* ctx, float current_temp) {
    // Atualizar máximo
    if (current_temp > ctx->temp_max || ctx->temp_max < -900.0f) {
        ctx->temp_max = current_temp;
    }
    
    // Atualizar mínimo
    if (current_temp < ctx->temp_min || ctx->temp_min > 900.0f) {
        ctx->temp_min = current_temp;
    }
}
```

### 5. Função `lcd_set_wifi_status()`

Define o status da conexão WiFi (para exibir indicador).

```c
void lcd_set_wifi_status(lcd_context_t* ctx, bool connected) {
    ctx->wifi_connected = connected;
}
```

## 📊 Exemplo de Uso

### Código Completo

```c
#include "lcd_i2c.h"

int main(void) {
    // 1. Inicializar LCD
    lcd_context_t* lcd = lcd_init();
    if (!lcd) return 1;
    
    // 2. Configurar parâmetros
    lcd->setpoint = 5.0f;
    lcd->wifi_connected = true;
    
    // 3. Desenhar layout estático
    lcd_clear(lcd);
    draw_static_layout(lcd);
    
    // 4. Loop de atualização
    while (running) {
        // Ler temperatura do sensor
        float temp = read_temperature_sensor();
        
        // Atualizar min/max
        lcd_update_temp_minmax(lcd, temp);
        
        // Atualizar valores dinâmicos
        update_dynamic_values(lcd);
        
        // Desenhar barras
        for (int col = 1; col < 19; col++) {
            lcd_draw_bargraph(lcd, col, temp, -10.0f, 30.0f);
        }
        
        // Aguardar 500ms
        usleep(500000);
    }
    
    // 5. Limpeza
    lcd_cleanup(lcd);
    return 0;
}
```

## 🎨 Personalização

### Alterar Número de Barras

```c
// Desenhar apenas 10 barras (mais espaçadas)
for (int col = 1; col < 20; col += 2) {
    lcd_draw_bargraph(lcd, col, temp, min, max);
}
```

### Alterar Faixa de Temperatura

```c
#define TEMP_MIN -20.0f  // Mínimo da escala
#define TEMP_MAX  50.0f  // Máximo da escala

lcd_draw_bargraph(lcd, col, temp, TEMP_MIN, TEMP_MAX);
```

### Customizar Caracteres

Modifique os arrays em `lcd_create_bargraph_chars()`:

```c
// Exemplo: Barra com padrão xadrez
uint8_t char_custom[8] = {
    0b11111,
    0b10101,
    0b11111,
    0b10101,
    0b11111,
    0b10101,
    0b11111,
    0b11111
};
```

## 📈 Performance

- **Taxa de atualização:** 2 Hz (500ms por frame)
- **Resolução vertical:** 14 pixels (2 linhas × 7 níveis)
- **Resolução horizontal:** 18 barras
- **Latência I2C:** ~5ms por atualização completa

## 🔍 Debugging

### Verificar CGRAM

```c
// Testar todos os caracteres customizados
for (int i = 0; i < 8; i++) {
    lcd_set_cursor(lcd, i, 0);
    lcd_write_char(lcd, i);
}
```

### Testar Barras Individualmente

```c
// Testar cada nível de preenchimento
for (float level = 0.0f; level <= 1.0f; level += 0.1f) {
    lcd_draw_bargraph(lcd, 10, level * 30.0f, 0.0f, 30.0f);
    sleep(1);
}
```

## 📚 Referências

- [HD44780 Datasheet](https://www.sparkfun.com/datasheets/LCD/HD44780.pdf) - Seção CGRAM
- [PCF8574 I2C Expander](https://www.ti.com/lit/ds/symlink/pcf8574.pdf)
- [Character LCD Guide](https://www.arduino.cc/en/Reference/LiquidCrystal)

## ✅ Checklist de Implementação

- [x] Criar caracteres customizados no CGRAM
- [x] Implementar função de desenho de barras
- [x] Implementar rastreamento de min/max
- [x] Criar layout estático do display
- [x] Criar função de atualização dinâmica
- [x] Implementar simulação de temperatura
- [x] Adicionar suporte a indicador WiFi
- [x] Documentar código
- [x] Criar exemplo funcional
- [x] Testar em hardware real

