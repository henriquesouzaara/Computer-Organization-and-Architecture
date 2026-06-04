# 🛸 Mission Control AI

> Sistema inteligente de monitoramento para missão espacial experimental — Arduino Uno + IoT + Gestão Energética Renovável

<br>

[![Arduino](https://img.shields.io/badge/Arduino-Uno%20R3-00979D?style=for-the-badge&logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![Wokwi](https://img.shields.io/badge/Simulação-Wokwi-7B2FBE?style=for-the-badge)](https://wokwi.com/)
[![Language](https://img.shields.io/badge/Linguagem-C%2B%2B%20%28Arduino%29-blue?style=for-the-badge&logo=cplusplus)](https://isocpp.org/)
[![Version](https://img.shields.io/badge/Versão-1.4-success?style=for-the-badge)]()
[![License](https://img.shields.io/badge/Licença-MIT-yellow?style=for-the-badge)](LICENSE)

<br>

---

## 📋 Índice

- [Introdução](#-introdução)
- [Objetivos](#-objetivos)
- [Tecnologias](#-tecnologias)
- [Componentes de Hardware](#-componentes-de-hardware)
- [Arquitetura do Sistema](#-arquitetura-do-sistema)
- [Funcionamento](#-funcionamento)
- [Resultados Esperados](#-resultados-esperados)
- [Melhorias Futuras](#-melhorias-futuras)
- [Como Executar](#-como-executar)
- [Estrutura do Repositório](#-estrutura-do-repositório)

---

## 🚀 Introdução

O **Mission Control AI** é um sistema embarcado de monitoramento em tempo real desenvolvido como projeto acadêmico integrador de duas disciplinas da FIAP:

| Disciplina | Contribuição |
|---|---|
| **Computer Organization and Architecture** | Arduino Uno, sensores digitais e analógicos, protocolo I2C, interrupções externas, máquina de estados |
| **Soluções em Energias Renováveis e Sustentáveis** | Geração fotovoltaica simulada, balanço energético, eficiência operacional, tomada de decisão autônoma |

---

## 🎯 Objetivos

- Monitorar temperatura, luminosidade e vibração de uma cápsula espacial experimental em tempo real
- Calcular geração fotovoltaica, consumo, balanço e eficiência energética
- Gerar alertas automáticos em 4 níveis: INFO, WARNING, CRITICAL, EMERGENCY
- Tomar decisões autônomas de modo operacional via máquina de estados finitos

---

## 🛠 Tecnologias

| Tecnologia | Versão | Uso |
|---|---|---|
| Arduino IDE | 2.x | Desenvolvimento e compilação |
| C++ (AVR-GCC) | C++11 | Linguagem do firmware |
| Wokwi | Web | Simulação do circuito |
| LiquidCrystal_I2C | 1.1.2 | Controle do LCD via I2C |
| DallasTemperature | 3.9.x | Leitura do sensor DS18B20 |
| OneWire | 2.3.x | Protocolo 1-Wire do DS18B20 |
| Wire.h | Nativa | Protocolo I2C |

---

## 🔌 Componentes de Hardware

| ID | Componente | Especificação | Quantidade |
|---|---|---|---|
| U1 | Microcontrolador | Arduino Uno R3 (ATmega328P, 16 MHz) | 1 |
| U2 | Display | LCD 16x2 com I2C Backpack (PCF8574, 0x27) | 1 |
| U3 | Sensor de temperatura | DS18B20 (protocolo 1-Wire, digital, −55°C a +125°C) | 1 |
| R1 | Sensor de luminosidade | LDR (divisor resistivo com 10 kΩ) | 1 |
| R2 | Resistor pull-down LDR | 10 kΩ | 1 |
| R3 | Resistor pull-up DS18B20 | 4,7 kΩ (obrigatório no protocolo 1-Wire) | 1 |
| SW1 | Sensor de vibração | Push Button (simula SW-420) | 1 |
| LED1 | Indicador de alerta | LED Vermelho + resistor 220 Ω | 1 |
| LED2 | Indicador de status | LED Verde + resistor 220 Ω | 1 |

### Mapeamento de Pinos

```
Arduino Uno
┌─────────────────────────────────────┐
│  5V  ──── VCC (alimentação geral)   │
│  GND ──── GND (terra geral)         │
│  D4  ──── DS18B20 (pino DQ)         │  ← digital, protocolo 1-Wire
│  A1  ──── LDR + resistor 10 kΩ      │  ← analógico, ADC canal 1
│  A4  ──── SDA (LCD I2C)             │
│  A5  ──── SCL (LCD I2C)             │
│  D2  ──── Push Button               │  ← INT0 (interrupção externa)
│  D8  ──── Resistor 220 Ω → LED Verm.│
│  D9  ──── Resistor 220 Ω → LED Verde│
└─────────────────────────────────────┘
```

---

## 🏗 Arquitetura do Sistema

### 7 Módulos Funcionais

| Módulo | Responsabilidade |
|---|---|
| `lerSensores()` | Leitura do DS18B20, LDR e vibração (ISR) |
| `calcAlertas()` | Avaliação de thresholds → INFO / WARNING / CRITICAL / EMERGENCY |
| `calcDecisao()` | Máquina de estados: NORMAL / ECONOMY / SAFE / EMERGENCY |
| `calcLEDs()` | Controle dos LEDs D8 e D9 |
| `calcEnergia()` | Geração fotovoltaica, consumo, balanço, eficiência |
| `atualizarDisplay()` | Rotação de 4 telas no LCD |
| `enviarSerial()` | Telemetria estruturada via Serial 9600 baud |

---

## ⚙️ Funcionamento

### Sistema de Alertas

| Nível | Condição | LED | Display |
|---|---|---|---|
| `INFO` | Tudo normal | Verde aceso | Rotação das 4 telas |
| `WARNING` | Parâmetro próximo ao limite | Vermelho aceso | Tela de aviso |
| `CRITICAL` | Parâmetro fora do limite | Vermelho aceso | Backlight piscante |
| `EMERGENCY` | 2+ CRITICAL simultâneos | Vermelho aceso | Tela exclusiva |

### Telas do LCD (rotação a cada 2s)

```
Tela 0 → MISSION CTRL AI / Modo operacional
Tela 1 → TEMP + LUZ
Tela 2 → VIB + ALERTA
Tela 3 → BAL + EFI
```

### Gatilhos de Alerta

| Sensor | WARNING | CRITICAL |
|---|---|---|
| Temperatura (DS18B20) | > 50 °C | > 60 °C ou < 0 °C |
| Luminosidade (LDR) | ADC < 100 ou ADC > 800 | — |
| Vibração (Botão) | ≥ 2 eventos / 5 s | ≥ 3 eventos / 5 s |
| Energia | Déficit (0 a −50 mW) | Crítico (< −50 mW) |

---

## 📊 Resultados Esperados

| Métrica | Valor |
|---|---|
| Frequência de amostragem — modo NORMAL | 1 ciclo a cada 2.000 ms |
| Frequência de amostragem — modo ECONOMY | 1 ciclo a cada 5.000 ms |
| Latência de resposta a alerta CRITICAL | < 2.000 ms |
| Sensor de temperatura | DS18B20 — leitura digital estável (±0,5°C) |
| Faixa de temperatura monitorada | −55 °C a +125 °C |

---

## 🔮 Melhorias Futuras

- [ ] Módulo RTC DS1307 para timestamps reais
- [ ] Cartão microSD para log persistente
- [ ] Buzzer ativo para alertas sonoros
- [ ] Módulo Bluetooth para telemetria wireless
- [ ] Migração para ESP32 com dashboard web

---

## ▶️ Como Executar

### No Wokwi

1. Acesse [wokwi.com](https://wokwi.com) → New Project → Arduino Uno
2. Cole o `diagram.json` no editor de circuito
3. Cole o `mission_control_ai.ino` no editor de código
4. Crie `libraries.txt` com:
```
LiquidCrystal I2C
DallasTemperature
OneWire
```
5. Clique em ▶️ **Start Simulation**
6. Abra o **Serial Monitor** (9600 baud)
7. Arraste o slider do DS18B20 para variar a temperatura
8. Arraste o slider do LDR para variar a luminosidade
9. Clique o botão 3x para simular vibração

---

## 📁 Estrutura do Repositório

```
Computer-Organization-and-Architecture/
│
├── 📄 README.md
├── 📄 mission_control_ai.ino
├── 📄 diagram.json
└── 📄 relatorio_computer_organization.pdf
```

---

## 👨‍💻 Autor

Desenvolvido como projeto acadêmico — FIAP 2025
Disciplina: **Computer Organization and Architecture**

---

<div align="center">

**Mission Control AI v1.4** · Arduino Uno · DS18B20 · Wokwi · FIAP

</div>
