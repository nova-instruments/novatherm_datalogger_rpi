# Configuração do RTC DS3231 no Raspberry Pi Zero 2W

Este guia descreve o processo completo para habilitar e utilizar o módulo **RTC DS3231** em um Raspberry Pi Zero 2W, substituindo o uso do `fake-hwclock` pelo RTC físico.

---

## 1. Pré-requisitos

- Raspberry Pi Zero 2W com Raspberry Pi OS atualizado.  
- Módulo **DS3231** conectado ao barramento I²C:  
  - **VCC → 3V3 (ou 5V dependendo do módulo)**  
  - **GND → GND**  
  - **SDA → GPIO2 (pino físico 3)**  
  - **SCL → GPIO3 (pino físico 5)**

---

## 2. Instalar pacotes necessários

```bash
sudo apt-get update
sudo apt-get install -y python3-smbus i2c-tools
```

---

## 3. Habilitar o I²C

Execute:

```bash
sudo raspi-config
```

- Vá em **Interface Options → I2C → Enable**  
- Reinicie o Raspberry.

Verifique se o dispositivo `/dev/i2c-1` existe:

```bash
ls /dev/i2c*
```

---

## 4. Detectar o DS3231 no barramento

Com o módulo conectado, rode:

```bash
sudo i2cdetect -y 1
```

A saída deve mostrar o endereço `0x68` ocupado:

```
60: -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- --
```

---

## 5. Ativar o overlay do RTC

Edite o arquivo de configuração do boot:

```bash
sudo nano /boot/firmware/config.txt
```

Adicione no final:

```
dtoverlay=i2c-rtc,ds3231
```

Salve e reinicie:

```bash
sudo reboot
```

---

## 6. Verificar carregamento do driver

Após reiniciar, rode:

```bash
dmesg | grep rtc
```

Você deve ver algo como:

```
rtc-ds1307 1-0068: registered as rtc0
```

---

## 7. Desativar o fake-hwclock

O pacote `fake-hwclock` grava a hora em um arquivo no desligamento, podendo causar conflito.  
Remova-o:

```bash
sudo apt-get remove -y fake-hwclock
sudo systemctl disable fake-hwclock
```

---

## 8. Usar o RTC

- Ler a hora do RTC:

```bash
sudo hwclock -r
```

- Ajustar o RTC com a hora atual do sistema:

```bash
sudo hwclock -w
```

- Carregar a hora do RTC no sistema:

```bash
sudo hwclock -s
```

> ⚠️ Normalmente, o `systemd` já faz isso automaticamente no boot.

---

## 9. Teste final

1. Ajuste a hora correta do sistema via internet (NTP).  
2. Grave no RTC:

   ```bash
   sudo hwclock -w
   ```

3. Desligue o Raspberry Pi, remova a fonte e reconecte.  
4. Ligue novamente e verifique se a hora foi preservada:

   ```bash
   date
   ```

---

✅ Pronto! Agora o Raspberry Pi Zero 2W utilizará o **DS3231 como relógio de hardware**.  
