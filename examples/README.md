# 📊 Exemplos LCD NovaTherm DataLogger

Este diretório contém exemplos de uso do display LCD 20x4 I2C.

## 📋 Exemplos Disponíveis

### 1. `lcd_bargraph_demo.c` - Demonstração de Barras Verticais

Demonstra uma interface de display com barras verticais animadas que representam temperatura.

**Layout do Display:**

```
┌────────────────────┐
│ ☁ TEMP      SP 5.0 │  Linha 0: WiFi + Título + Setpoint
│ ████████████████   │  Linha 1: Barras verticais (parte superior)
│ ████████████████   │  Linha 2: Barras verticais (parte inferior)
│ MAX 15.2   MIN 4.8 │  Linha 3: Temperatura máxima e mínima
└────────────────────┘
```

**Características:**
- ✅ Usa caracteres customizados (CGRAM) para barras verticais
- ✅ 8 níveis de preenchimento por caractere
- ✅ Atualização dinâmica a cada 500ms
- ✅ Simulação de temperatura com função senoidal
- ✅ Rastreamento automático de MAX e MIN
- ✅ Indicador de status WiFi
- ✅ Código bem comentado e organizado

**Funções Implementadas:**
- `lcd_create_bargraph_chars()` - Cria caracteres customizados no CGRAM
- `lcd_draw_bargraph()` - Desenha uma barra vertical
- `lcd_update_temp_minmax()` - Atualiza valores de máximo e mínimo
- `lcd_set_wifi_status()` - Define status da conexão WiFi
- `draw_static_layout()` - Desenha elementos fixos do display
- `update_dynamic_values()` - Atualiza valores variáveis
- `draw_temperature_bars()` - Desenha múltiplas barras
- `simulate_temperature()` - Simula variação de temperatura

## 🔨 Compilação

### Pré-requisitos

- Cross-compiler ARM instalado (`arm-linux-gnueabihf-gcc`)
- CMake 3.16 ou superior
- Dependências do projeto principal compiladas

### Compilar para Raspberry Pi

```bash
cd examples
make setup    # Configurar ambiente
make build    # Compilar
```

### Compilar para PC (teste local - sem I2C real)

```bash
cd examples
mkdir build-local
cd build-local
cmake ..
make
```

## 🚀 Deploy e Execução

### Enviar para Raspberry Pi

```bash
make deploy RPI_IP=192.168.3.99 RPI_USER=nova
```

### Executar no Raspberry Pi

```bash
ssh nova@192.168.3.99
sudo ./lcd_bargraph_demo
```

**Nota:** É necessário executar com `sudo` para acessar o barramento I2C.

### Parar a Execução

Pressione `Ctrl+C` para encerrar o programa.

## 📊 Caracteres Customizados (CGRAM)

O LCD HD44780 permite criar até 8 caracteres customizados de 5x8 pixels.
Este exemplo usa todos os 8 caracteres para criar níveis de barra:

```
Char 0: Vazio       Char 4: 4/7 cheio
┌─────┐             ┌─────┐
│     │             │█████│
│     │             │█████│
│     │             │█████│
│     │             │█████│
│     │             │     │
│     │             │     │
└─────┘             └─────┘

Char 1: 1/7 cheio   Char 5: 5/7 cheio
Char 2: 2/7 cheio   Char 6: 6/7 cheio
Char 3: 3/7 cheio   Char 7: Cheio
```

## 🔧 Adaptação para Sensores Reais

Para usar com sensores reais de temperatura, substitua a função `simulate_temperature()`:

```c
// Substituir isto:
float current_temp = simulate_temperature();

// Por isto:
float current_temp = read_real_temperature_sensor();
```

Exemplo de integração com Modbus:

```c
modbus_data_t data;
if (modbus_read_all_channels(modbus_ctx, &data)) {
    float current_temp = data.ch_temp[0];  // Canal 1
    lcd_update_temp_minmax(lcd, current_temp);
    draw_temperature_bars(lcd, current_temp);
}
```

## 📁 Estrutura de Arquivos

```
examples/
├── README.md                  # Este arquivo
├── CMakeLists.txt            # Configuração CMake
├── Makefile                  # Comandos de build simplificados
└── lcd_bargraph_demo.c       # Exemplo de barras verticais
```

## 🎯 Próximos Passos

1. ✅ Compilar e testar o exemplo
2. ✅ Verificar funcionamento no LCD real
3. ⬜ Integrar com leitura real de sensores
4. ⬜ Adicionar mais exemplos (gráficos, animações, etc.)

## 📝 Notas Técnicas

- **Endereço I2C:** 0x27 (padrão para PCF8574)
- **Barramento I2C:** /dev/i2c-1
- **Resolução das barras:** 14 pixels verticais (2 linhas × 7 pixels úteis)
- **Taxa de atualização:** 500ms (2 Hz)
- **Faixa de temperatura:** -10°C a 30°C (configurável)

## 🐛 Troubleshooting

**Erro: "Could not open I2C device"**
- Verifique se o I2C está habilitado: `sudo raspi-config`
- Verifique permissões: `sudo chmod 666 /dev/i2c-1`

**Display não responde:**
- Verifique conexões físicas (SDA, SCL, VCC, GND)
- Teste endereço I2C: `sudo i2cdetect -y 1`
- Verifique se o endereço é 0x27

**Barras não aparecem:**
- Verifique se `lcd_create_bargraph_chars()` foi chamado
- Verifique contraste do LCD (potenciômetro no módulo)

## 📚 Referências

- [HD44780 Datasheet](https://www.sparkfun.com/datasheets/LCD/HD44780.pdf)
- [PCF8574 I2C Expander](https://www.ti.com/lit/ds/symlink/pcf8574.pdf)
- [NovaTherm DataLogger Documentation](../README.md)

