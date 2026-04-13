# NovaTherm DataLogger RPi (ARMv7)

Sistema de aquisição Modbus RTU para Raspberry Pi Zero 2W com cross-compilation ARMv7.

## Escopo atual

- Arquitetura suportada: ARMv7 (Raspberry Pi Zero 2W)
- Toolchain: `arm-linux-gnueabihf`
- Binário de saída: `app_armv7`
- Dependências de terceiros linkadas estaticamente no binário:
  - `libmodbus.a`
  - `libgpiod.a`
  - `libudev.a`
  - `libsqlite3.a`

## Organização de build

O projeto usa **apenas um diretório de build**:

- `build-rpi/`

O perfil de display é alterado por reconfiguração do CMake nesse mesmo diretório.

## Setup do ambiente

```bash
make setup
make check
```

O setup compila as dependências em `deps/` e já configura o perfil padrão (LCD 20x4).

## Perfis de display

- Padrão: LCD 20x4 I2C
- Alternativo: ILI9341 + LVGL
- Alternativo: ST7567

Comandos:

```bash
make configure-lcd
make configure-lvgl
make configure-st7567
```

Atalhos de build:

```bash
make build-lcd
make build-lvgl
make build-st7567
```

Build genérico (usa o perfil já configurado):

```bash
make build
```

Saída esperada:

```bash
build-rpi/bin/app_armv7
```

## Observação sobre LVGL

Para o perfil `ILI9341 + LVGL`, o CMake exige:

```bash
deps/lvgl/install/lib/liblvgl.a
```

Se esse arquivo não existir, o `make configure-lvgl` falha com erro de dependência ausente.

## Deploy

Deploy do binário atual:

```bash
make deploy RPI_IP=<IP_DA_PI> RPI_USER=<USUARIO>
```

Deploy com troca automática de perfil:

```bash
make deploy-lcd RPI_IP=<IP_DA_PI> RPI_USER=<USUARIO>
make deploy-lvgl RPI_IP=<IP_DA_PI> RPI_USER=<USUARIO>
make deploy-st7567 RPI_IP=<IP_DA_PI> RPI_USER=<USUARIO>
```

Deploy manual:

```bash
scp build-rpi/bin/app_armv7 <USUARIO>@<IP_DA_PI>:~/app_armv7
ssh <USUARIO>@<IP_DA_PI> "chmod +x ~/app_armv7"
```

## Serviço systemd

Na Raspberry:

```bash
sudo ./install_service.sh
sudo systemctl status novatherm-datalogger
```

## Comandos úteis

```bash
make info
make clean
make rebuild
```

## Estrutura relevante

- `src/main.c`: loop principal
- `lib/`: módulos de Modbus, datalogger, GPIO, USB, display e controle
- `scripts/`: setup/check/build das dependências ARMv7
- `user_cross_compile_setup.cmake`: toolchain ARMv7
