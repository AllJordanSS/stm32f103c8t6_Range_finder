# Range Finder Project

Este projeto implementa um medidor de distância preciso utilizando o microcontrolador STM32F103C8T6 (Blue Pill) em conjunto com o sensor de distância VL53L0X.

## Ambiente de Desenvolvimento

### Ferramentas Utilizadas
1. **STM32CubeMX (v6.9.1)**
   - Geração do código base do projeto
   - Configuração dos periféricos
   - Configuração do clock system
   - Geração do Makefile

2. **STM32CubeIDE (v1.13.1)**
   - IDE baseada em Eclipse
   - Compilador GCC ARM
   - Debug integrado
   - Build system
   - Análise de consumo de memória

3. **STM32CubeProgrammer (v2.14.0)**
   - Programação do microcontrolador
   - Verificação do firmware
   - Acesso aos registradores de memória
   - Interface com ST-Link

### Processo de Build
1. Configuração inicial no STM32CubeMX
2. Geração do código base
3. Importação no STM32CubeIDE
4. Compilação usando make
5. Programação via ST-Link

### Requisitos do Sistema
- Windows 10/11 ou Linux
- Java Runtime Environment (JRE)
- Drivers ST-Link
- STM32CubeMX, CubeIDE e CubeProgrammer instalados

## Hardware Utilizado

### STM32F103C8T6 (Blue Pill)
- Microcontrolador ARM Cortex-M3
- Frequência: 72 MHz
- Flash: 64 KB
- RAM: 20 KB
- Encapsulamento: LQFP48

### VL53L0X
- Sensor Time-of-Flight (ToF) de alta precisão
- Range de medição: 30mm até 2000mm
- Interface I2C
- Precisão: ±3% da distância medida
- Taxa de atualização: até 50Hz no modo padrão

### Conexões
- I2C1:
  - SCL: PB6
  - SDA: PB7
- UART1:
  - TX: PA9
  - RX: PA10
- LED:
  - LED_AZUL: PC13 (ativo baixo)

## Configuração no STM32CubeMX

### Clock Configuration
- HSE: Crystal/Ceramic Resonator (8 MHz)
- PLLMUL: x9 (72 MHz SYSCLK)
- APB1: 36 MHz (PCLK1)
- APB2: 72 MHz (PCLK2)

### Periféricos Habilitados
1. **I2C1**
   - Modo: I2C
   - Clock Speed: 100 kHz (Standard mode)
   - Modo de endereçamento: 7-bit

2. **USART1**
   - Modo: Asynchronous
   - Baud Rate: 115200
   - Word Length: 8 bits
   - Stop Bits: 1
   - Paridade: None
   - Flow Control: None

3. **GPIO**
   - PC13: Output Push-Pull (LED)
   - Demais pinos configurados automaticamente para I2C e UART

## Estrutura do Software

### Drivers
O projeto utiliza os seguintes drivers:
- HAL (Hardware Abstraction Layer) STM32F1
- Driver personalizado para VL53L0X

### Arquivos Principais

1. **vl53l0x.h/c**
```c
// Principais funcionalidades:
- Inicialização do sensor
- Configuração de alta precisão
- Leitura de distância com filtragem
- Validação de medições
```

2. **main.c**
```c
// Funcionalidades:
- Inicialização do hardware
- Loop principal de medição
- Controle do LED
- Comunicação UART
```

### Características do Software

1. **Medição de Distância**
- Taxa de atualização: 5 Hz (200ms)
- Filtragem de medições inválidas
- Validação de múltiplas leituras consecutivas
- Detecção de variações bruscas

2. **Interface com Usuário**
- LED indicador:
  - Piscando: primeiros 10 segundos após inicialização
  - Aceso: objeto detectado próximo (<100mm)
  - Apagado: sem objeto próximo ou erro
- Comunicação Serial:
  - Formato: `Dist: XXX mm, Status: Y, Signal: ZZZ`
  - Comandos disponíveis:
    - `i2c_bar`: Executa varredura do barramento I2C

3. **Validação de Medições**
- Status da medição
- Taxa de sinal mínima
- Variação máxima entre medidas
- Múltiplas leituras consecutivas

### Parâmetros Configuráveis

O sensor pode ser ajustado através dos seguintes parâmetros no arquivo `vl53l0x.h`:

#### Timing Budget
```c
#define VL53L0X_HIGH_ACCURACY_TIMING_BUDGET   200000  // 200ms
```
- **Descrição**: Tempo dedicado para cada medição
- **Unidade**: Microssegundos (µs)
- **Efeito Prático**:
  - Valores maiores (200-500ms): Maior precisão, menor ruído
  - Valores menores (20-50ms): Resposta mais rápida, mais ruído
  - Default (200ms): Bom equilíbrio entre precisão e velocidade
- **Quando Ajustar**: 
  - Aumentar se precisar de medidas mais estáveis
  - Diminuir se precisar de resposta mais rápida

#### Signal Rate Limit
```c
#define VL53L0X_SIGNAL_RATE_LIMIT            0.25    // mcps
```
- **Descrição**: Limite mínimo para a taxa de sinal de retorno
- **Unidade**: MCPS (Mega Counts Per Second)
- **Efeito Prático**:
  - Valores maiores (>0.25): Rejeita mais medições duvidosas
  - Valores menores (<0.25): Aceita medições mais fracas
- **Quando Ajustar**:
  - Aumentar para superfícies muito reflexivas
  - Diminuir para superfícies escuras/não reflexivas

#### Número de Leituras Válidas
```c
#define VL53L0X_VALID_READS_BEFORE_UPDATE    3       // leituras
```
- **Descrição**: Número de leituras válidas consecutivas necessárias
- **Efeito Prático**:
  - Valores maiores (>3): Mais estabilidade, menos falsos positivos
  - Valores menores (1-2): Resposta mais rápida a mudanças
- **Quando Ajustar**:
  - Aumentar se tiver muitos falsos positivos
  - Diminuir se a resposta estiver muito lenta

#### Variação Máxima Entre Medidas
```c
#define VL53L0X_MAX_MEASUREMENT_JUMP         100     // mm
```
- **Descrição**: Máxima variação permitida entre medidas consecutivas
- **Unidade**: Milímetros (mm)
- **Efeito Prático**:
  - Valores maiores (>100mm): Aceita mudanças bruscas
  - Valores menores (<100mm): Filtra variações súbitas
- **Quando Ajustar**:
  - Aumentar para objetos que se movem rapidamente
  - Diminuir para medições mais estáveis de objetos estáticos

### Exemplos de Configuração

1. **Alta Precisão (Objetos Estáticos)**
```c
#define VL53L0X_HIGH_ACCURACY_TIMING_BUDGET   500000  // 500ms
#define VL53L0X_SIGNAL_RATE_LIMIT            0.30    // mcps
#define VL53L0X_VALID_READS_BEFORE_UPDATE    5       // leituras
#define VL53L0X_MAX_MEASUREMENT_JUMP         50      // mm
```

2. **Resposta Rápida (Objetos em Movimento)**
```c
#define VL53L0X_HIGH_ACCURACY_TIMING_BUDGET   50000   // 50ms
#define VL53L0X_SIGNAL_RATE_LIMIT            0.20    // mcps
#define VL53L0X_VALID_READS_BEFORE_UPDATE    2       // leituras
#define VL53L0X_MAX_MEASUREMENT_JUMP         200     // mm
```

3. **Configuração Balanceada (Default)**
```c
#define VL53L0X_HIGH_ACCURACY_TIMING_BUDGET   200000  // 200ms
#define VL53L0X_SIGNAL_RATE_LIMIT            0.25    // mcps
#define VL53L0X_VALID_READS_BEFORE_UPDATE    3       // leituras
#define VL53L0X_MAX_MEASUREMENT_JUMP         100     // mm
```

## Uso do Sistema

### Inicialização
1. O sistema inicia realizando a configuração do hardware
2. Tenta inicializar o sensor VL53L0X
3. Se bem sucedido, configura modo de alta precisão
4. Inicia período de 10s com LED piscando
5. Começa a realizar medições a 5Hz

### Operação
- As medições são realizadas continuamente
- Cada medição passa por validação:
  - Status da medição
  - Força do sinal
  - Variação em relação à última medida válida
- Medições são enviadas pela UART
- LED indica presença de objetos próximos

### Interpretação dos Dados
- **Status**: 
  - 0: Medição normal
  - 4: Sinal fraco
  - 5: Modo alta precisão
  - 6-8: Erros diversos

- **Signal**: 
  - >200: Sinal bom
  - <100: Sinal fraco/não confiável

## Contribuição
Sinta-se livre para contribuir com o projeto através de Pull Requests ou reportando issues.

## Licença
Este projeto está sob a licença MIT. Veja o arquivo LICENSE para mais detalhes.