# NovaTherm DataLogger RPi

Sistema de aquisição de dados Modbus RTU para Raspberry Pi, desenvolvido para o projeto NovaTherm.

## 📋 Descrição

Este projeto implementa um datalogger que realiza leitura de registradores Modbus via comunicação serial, especificamente projetado para funcionar em Raspberry Pi com cross-compilation para ARM.

### Características Principais

- 🔌 **Comunicação Modbus RTU** via porta serial (`/dev/serial0`)
- 🎯 **Cross-compilation** para ARM (Raspberry Pi 1/Zero 2W/3)
- 📊 **Leitura de registradores** 0x200 (Temperatura), 0x214 (Alarmes), 0x21F (Porta), 0x2803 (Setpoint)
- 📝 **DataLogger duplo** com formatos TXT e SQLite
- 🕐 **Sincronização com RTC** (DS3231) para timestamps precisos
- 🔄 **Triplo modo de logging**: periódico (5 min) + imediato (mudança de porta/alarme)
- 🚪 **Detecção de mudança de estado** da porta e alarme com registro instantâneo
- 🔌 **Extração automática via pen drive** com monitoramento contínuo
- 🔊 **Sinalização sonora** via buzzer no GPIO23 ao finalizar extração
- 💡 **Controle de relés**: Lâmpada (GPIO24) e Discadora (GPIO25)
- ⚙️ **Configuração via arquivo**: Nome do dispositivo em `/boot/firmware/config.txt`
- 🚀 **Serviço systemd**: Instalação automática para inicialização no boot
- 📱 **Deploy automatizado** via SSH
- 💾 **Armazenamento local** em `/home/nova/`

## 🎯 Compatibilidade

| Modelo Raspberry Pi | Arquitetura | Status | Comando |
|---------------------|-------------|--------|---------|
| **Raspberry Pi 1 A/B** | ARMv6 | ✅ Suportado | `make setup-armv6` |
| **Raspberry Pi Zero** | ARMv6 | ✅ Suportado | `make setup-armv6` |
| **Raspberry Pi 2** | ARMv7 | ✅ Suportado | `make setup` |
| **Raspberry Pi 3** | ARMv7 | ✅ Suportado | `make setup` |
| **Raspberry Pi Zero 2W** | ARMv7 | ✅ Suportado | `make setup` |
| **Raspberry Pi 4** | ARMv8 | ⚠️ Não testado | `make setup` |

**Nota:** O problema `libmodbus.so.5: cannot open shared object file` em Raspberry Pi 1 foi resolvido com **compilação estática** específica para ARMv6 usando toolchain `arm-linux-gnueabi` (soft-float).

### 🔧 Diferenças Técnicas entre Arquiteturas

| Aspecto | ARMv6 (Pi 1/Zero) | ARMv7 (Pi 2/3/Zero 2W) |
|---------|-------------------|-------------------------|
| **Toolchain** | `arm-linux-gnueabi` | `arm-linux-gnueabihf` |
| **ABI** | Soft-float | Hard-float |
| **Compilação** | Estática (`-static`) | Dinâmica |
| **Flags** | `-march=armv6 -mfloat-abi=soft` | `-mcpu=cortex-a53 -mfpu=neon-fp-armv8` |
| **Dependências** | `deps-armv6/` | `deps/` |
| **Build Dir** | `build-rpi1/` | `build-rpi/` |
| **Tamanho** | ~2.5MB (estático) | ~60KB (dinâmico) |

## 🛠️ Configuração do Ambiente

### Pré-requisitos

- Ubuntu/Debian (host de desenvolvimento)
- Cross-compiler ARM (`gcc-arm-linux-gnueabihf`)
- CMake 3.16+
- Raspberry Pi com UART habilitado

### Instalação Automática

#### Para Raspberry Pi 2/3/Zero 2W (ARMv7):
```bash
# Configurar ambiente completo de cross-compilation
make setup

# Ou manualmente:
./scripts/setup_cross_compilation.sh
```

#### Para Raspberry Pi 1/Zero (ARMv6):
```bash
# Configurar ambiente para ARMv6
make setup-armv6

# Ou manualmente:
./scripts/setup_armv6.sh
```

### Verificação do Ambiente

```bash
# Verificar se tudo está configurado
make check

# Ou manualmente:
./scripts/check_cross_compilation.sh
```

## 🔨 Compilação

### Usando Makefile (Recomendado)

```bash
# Compilar projeto (ARMv7 - Pi 2/3/Zero 2W)
make build

# Compilar projeto (ARMv6 - Pi 1/Zero)
make build-armv6

# Limpar e recompilar
make rebuild

# Ver informações do projeto
make info

# Ver informações ARMv6
make info-armv6
```

### Usando CMake Diretamente

```bash
# Configurar CMake
cmake -DCMAKE_TOOLCHAIN_FILE=./user_cross_compile_setup.cmake -B build-rpi -S .

# Compilar
make -C build-rpi -j$(nproc)
```

## 🚀 Deploy e Execução

### Deploy Automático

```bash
# Deploy para IP padrão (192.168.3.22)
make deploy

# Deploy para IP específico
make deploy RPI_IP=192.168.1.100 RPI_USER=pi
```

### Deploy Manual

```bash
# ARMv7 (Pi 2/3/Zero 2W)
./deploy_to_rpi.sh <IP_DA_RPI> <USUARIO>
scp build-rpi/bin/app pi@<IP>:~/

# ARMv6 (Pi 1/Zero) - Deploy automático
make deploy-armv6 RPI_IP=<IP> RPI_USER=pi

# ARMv6 (Pi 1/Zero) - Deploy manual
scp build-rpi1/bin/app pi@<IP>:~/app_armv6
```

### Execução na Raspberry Pi

#### Execução Manual
```bash
# ARMv7 (Pi 2/3/Zero 2W)
sudo ./app

# ARMv6 (Pi 1/Zero) - Executável estático
sudo ./app_armv6
```

#### Instalação como Serviço (Recomendado)
```bash
# Instalar serviço systemd
sudo ./install_service.sh

# Comandos do serviço
sudo systemctl status novatherm-datalogger   # Ver status
sudo systemctl stop novatherm-datalogger     # Parar
sudo systemctl start novatherm-datalogger    # Iniciar
sudo systemctl restart novatherm-datalogger  # Reiniciar
sudo journalctl -u novatherm-datalogger -f   # Ver logs em tempo real

# Desinstalar serviço
sudo ./uninstall_service.sh
```

## 📁 Estrutura do Projeto

```
novatherm_datalogger_rpi/
├── src/                              # Código fonte principal
│   └── main.c                        # Aplicação principal
├── lib/                              # Bibliotecas do projeto
│   ├── modbus.c/.h                   # Biblioteca Modbus RTU
│   ├── datalogger.c/.h               # Biblioteca DataLogger
│   ├── usb_manager.c/.h              # Gerenciador USB
├── CMakeLists.txt                    # Configuração CMake
├── user_cross_compile_setup.cmake    # Toolchain ARM
├── Makefile                          # Comandos facilitados
├── deploy_to_rpi.sh                  # Script de deploy
├── scripts/                          # Scripts de build
│   ├── setup_cross_compilation.sh    # Configuração completa
│   ├── check_cross_compilation.sh    # Verificação do ambiente
│   ├── build_libmodbus_arm.sh        # Build libmodbus ARM
│   ├── build_libgpiod_arm.sh         # Build libgpiod ARM
│   └── build_libudev_arm.sh          # Build libudev ARM
└── deps/                             # Dependências compiladas (ignorado no Git)
    ├── libmodbus/
    ├── libgpiod/
    └── eudev/
```

## ⚙️ Configuração da Raspberry Pi

### Habilitar UART

Adicionar ao `/boot/config.txt`:
```
enable_uart=1
dtoverlay=disable-bt
```

### Verificar Dispositivo Serial

```bash
# Verificar se /dev/serial0 existe
ls -la /dev/serial*

# Verificar configuração UART
dmesg | grep tty
```

## 📊 Dados Lidos

O sistema lê quatro registradores Modbus:

- **0x200**: Temperatura (÷10 para °C)
- **0x214**: Alarmes (0 ou 1) → Controla Discadora (GPIO25)
- **0x21F**: Porta (0 ou 1) → Controla Lâmpada (GPIO24)
- **0x2803**: Setpoint (÷10 para °C)

### Exemplo de Saída

```
Iniciando leitura Modbus...
Dispositivo: /dev/serial0
Configuração: 9600-N-8-1
Slave ID: 1
Endereços: 0x200 (0x200) e 0x21F (0x21F)
----------------------------------------
Conexão estabelecida com sucesso!

Lendo registradores...
Endereço 0x200: 1234 (0x04D2)
Endereço 0x21F: 1 (0x0001) - Binário: 1
----------------------------------------
```

## 🧪 Testes

```bash
# Executar verificações básicas
make test

# Verificar se executável é ARM
file build-rpi/bin/app
```

## 🔧 Comandos Úteis

```bash
# Ver todos os comandos disponíveis
make help

# Informações do projeto
make info

# Status do ambiente
make check
```

## 📝 Configurações

### Modbus RTU
- **Dispositivo**: `/dev/serial0`
- **Baud Rate**: 9600
- **Paridade**: Nenhuma (N)
- **Data Bits**: 8
- **Stop Bits**: 1
- **Slave ID**: 1
- **Timeout**: 500ms (resposta), 200ms (byte)
- **Delay entre leituras**: 100ms (evita sobrecarga no barramento)
- **Registradores**:
  - 0x200 (Temperatura)
  - 0x214 (Alarmes)
  - 0x21F (Porta)
  - 0x2803 (Setpoint)

### DataLogger
- **Nome do dispositivo**: Configurável em `/boot/firmware/config.txt` (ex: `NI00002`)
- **Diretório de logs**: `/home/nova/`
- **Formatos de arquivo** (nome fixo para continuidade):
  - **TXT**: `NOME.txt` (Temperatura e Porta)
  - **SQLite**: `NOME.db` (Temperatura, Setpoint, Alarme, Porta)
- **Modo de logging**:
  - **Periódico**: A cada 5 minutos (300 segundos)
  - **Imediato**: Quando detecta mudança de estado da porta ou alarme
- **Continuidade**: Ao reiniciar, continua gravando nos mesmos arquivos
- **Estrutura do banco SQLite**:
  - **Tabela DataGrpData**: IndexID, CollectTime, Tprincipal, Setpoint, Alarme, Porta
  - **Tabela DBInfo**: Metadados do banco (versão, IDs, timestamps)
- **Frequência de verificação**: A cada 2 segundos (para detectar mudanças)
- **Fonte de tempo**: RTC (DS3231) com fallback para sistema

### Formato do Log TXT
```
NAME: NI00002
R;Data Hora;TPrincipal;PA
1;2024-09-15 16:47:30;1234;1
2;2024-09-15 16:47:32;1235;0
3;2024-09-15 16:47:34;ERROR;ERROR
```

Onde:
- **NAME**: Nome do dispositivo (configurável)
- **R**: Número sequencial do registro
- **Data Hora**: Timestamp do RTC (DD/MM/YYYY HH:MM:SS)
- **TPrincipal**: Temperatura do registrador 0x200 (valor real: 231 → 23.1°C)
- **PA**: Porta Aberta (0=fechada, 1=aberta) do registrador 0x21F

### Comportamento do Logging

#### 📅 Log Periódico (5 minutos)
- Registra dados automaticamente a cada 5 minutos
- Mantém histórico contínuo independente de mudanças

#### 🚪 Log por Mudança de Porta/Alarme (Imediato)
- Detecta mudanças no estado da porta (0↔1) e alarme (0↔1)
- Registra **imediatamente** quando detecta mudança
- Exibe mensagens:
  - `🚪 MUDANÇA DE ESTADO DA PORTA: 0 → 1`
  - `🚨 MUDANÇA DE ESTADO DO ALARME: 0 → 1`
- Não interfere no ciclo periódico

#### ⚡ Frequência de Verificação
- **Leitura Modbus**: A cada 2 segundos
- **Log periódico**: A cada 5 minutos
- **Log de mudança**: Instantâneo quando detectado

#### 📊 Exemplo de Comportamento
```
16:00:00 - Log periódico (temperatura: 23.1°C, porta: 0)
16:01:30 - Porta muda para 1 → Log imediato
16:03:45 - Porta muda para 0 → Log imediato
16:05:00 - Log periódico (temperatura: 23.3°C, porta: 0)
```

#### 📁 Exemplo de Arquivo Gerado
**Arquivo:** `/home/nova/NI00002.txt` (nome fixo, continua crescendo)

## 🔊 Sinalização Sonora (Buzzer)

### **Configuração do Hardware:**
- **GPIO**: 23 (pino físico 16)
- **Biblioteca**: libgpiod
- **Tipo**: Buzzer ativo (3.3V/5V)

### **Funcionamento:**
- **Inicialização**: Automática junto com o USB Manager
- **Acionamento**: Apenas ao finalizar extração com sucesso
- **Sequência**: 3 beeps curtos (200ms ligado + 200ms desligado)
- **Finalização**: Automática ao encerrar aplicação

### **Conexão Sugerida:**
```
Raspberry Pi          Buzzer
GPIO23 (Pino 16) ──── Positivo (+)
GND    (Pino 20) ──── Negativo (-)
```

### **Mensagens:**
```
🔊 Buzzer inicializado no GPIO 23
🔊 Sinalizando extração concluída...
🔊 Sinalização sonora concluída
🔊 Buzzer finalizado
```

**Nota:** Se o buzzer não puder ser inicializado, a aplicação continua funcionando normalmente sem sinalização sonora.

## 💡 Controle de Relés

### **Configuração do Hardware:**
- **GPIO 24 (Lâmpada)**: Pino físico 18
- **GPIO 25 (Discadora)**: Pino físico 22
- **Biblioteca**: libgpiod

### **Funcionamento:**
- **Lâmpada (GPIO24)**: Controlada pelo estado da porta (0x21F)
  - Porta ABERTA (1) → Lâmpada LIGA
  - Porta FECHADA (0) → Lâmpada DESLIGA

- **Discadora (GPIO25)**: Controlada pelo estado do alarme (0x214)
  - Alarme ATIVO (1) → Discadora LIGA
  - Alarme INATIVO (0) → Discadora DESLIGA

### **Conexão Sugerida:**
```
Raspberry Pi          Relé Lâmpada
GPIO 24 (Pino 18) ──── Sinal
GND     (Pino 20) ──── GND
5V      (Pino 2)  ──── VCC

Raspberry Pi          Relé Discadora
GPIO 25 (Pino 22) ──── Sinal
GND     (Pino 20) ──── GND
5V      (Pino 2)  ──── VCC
```

## ⚙️ Configuração do Dispositivo

O nome do dispositivo é configurado através do arquivo `/boot/firmware/config.txt` (arquivo de configuração do sistema Raspberry Pi).

### **Como Configurar:**

Adicione a seguinte linha **no final** do arquivo `/boot/firmware/config.txt`:

```bash
# Editar arquivo de configuração (requer sudo)
sudo nano /boot/firmware/config.txt

# Adicionar no final do arquivo:
DEVICE_NAME=NI00003
```

**Formato:**
- Linha no formato: `DEVICE_NAME=NomeDoDispositivo`
- Máximo 32 caracteres
- Sem espaços ou caracteres especiais no nome
- Se houver múltiplas linhas `DEVICE_NAME=`, a **última** prevalece

**Exemplo de `/boot/firmware/config.txt`:**
```
# ... outras configurações do Raspberry Pi ...
enable_uart=1
dtoverlay=disable-bt
dtparam=i2c_arm=on
dtoverlay=i2c-rtc,ds3231

# Nome do dispositivo DataLogger (adicionar no final)
DEVICE_NAME=NI00003
```

**Se a configuração não existir:**
- O sistema usará o nome padrão: `NI00002`
- Os arquivos de log serão criados com esse nome padrão

**Vantagens dessa abordagem:**
- ✅ Configuração centralizada no arquivo do sistema
- ✅ Não precisa criar arquivo separado
- ✅ Persiste entre atualizações do software
- ✅ Fácil de editar via SSH ou interface gráfica

## 🔌 Extração Automática via Pen Drive

### **Como Funciona:**

1. **🔍 Monitoramento Contínuo**: A aplicação monitora continuamente a inserção de pen drives
2. **🔌 Detecção Automática**: Quando um pen drive é inserido, é detectado automaticamente
3. **📁 Montagem**: O pen drive é montado automaticamente no sistema
4. **📋 Cópia**: Bancos de dados do DataLogger (`NI*.db`) são copiados para o pen drive (sobrescreve se já existir)
5. **💾 Sincronização**: Os dados são sincronizados para garantir integridade
6. **⏏️ Ejeção**: O pen drive é desmontado automaticamente após a cópia
7. **🔊 Sinalização**: Buzzer emite 3 beeps curtos para confirmar sucesso
8. **✅ Finalização**: Pen drive pode ser removido com segurança

### **Processo Automático:**

```
🔌 Pen drive inserido
    ↓
🔍 Detectado automaticamente
    ↓
📁 Montagem automática
    ↓
📋 Copiando bancos de dados NI*.db...
    ↓
💾 Sincronizando dados...
    ↓
⏏️ Desmontando pen drive
    ↓
🔊 Buzzer: 3 beeps de sucesso
    ↓
✅ Extração concluída!
💡 Pen drive pode ser removido
```

### **Mensagens na Tela:**

```
🔌 Pen drive detectado! Iniciando extração automática...
📦 USB [20%]: Montando dispositivo USB...
📦 USB [30%]: Copiando bancos de dados...
📦 USB [80%]: Sincronizando dados... (3 bancos copiados)
📦 USB [90%]: Desmontando dispositivo USB...
✅ USB: 3 bancos de dados extraídos com sucesso para USB
🔊 Sinalizando extração concluída...
🔊 Sinalização sonora concluída
✅ Extração concluída com sucesso!
💡 Pen drive pode ser removido com segurança
```

### **Características:**

- **✅ Plug & Play**: Inserir pen drive → extração automática
- **✅ Sem intervenção**: Processo completamente automático
- **✅ Seguro**: Desmontagem correta antes da remoção
- **✅ Filtro inteligente**: Copia apenas bancos de dados do DataLogger (`NI*.db`)
- **✅ Sobrescrita inteligente**: Mantém sempre a versão mais recente (sobrescreve arquivos existentes)
- **✅ Contagem de arquivos**: Mostra quantos bancos foram copiados
- **✅ Sinalização sonora**: Buzzer confirma sucesso com 3 beeps (GPIO23)
- **✅ Reutilizável**: Funciona com qualquer pen drive
- **✅ Paralelo**: Não interfere no logging principal
- **✅ Preserva outros arquivos**: Não apaga arquivos existentes no pen drive

## 📄 Licença

Este projeto é propriedade da Nova Instruments.

## 🏢 Desenvolvido por

**Nova Instruments**
Sistema de DataLogger NovaTherm
