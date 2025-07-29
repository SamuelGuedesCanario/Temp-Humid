# 🌤️ Estação Meteorológica Embarcada

Sistema completo de monitoramento meteorológico baseado em Raspberry Pi Pico com interface web em tempo real, sensores múltiplos e sistema de alertas.

## 📋 Descrição

Este projeto implementa uma estação meteorológica embarcada que monitora temperatura, umidade, pressão atmosférica e altitude em tempo real. O sistema oferece interface local através de display OLED, indicadores visuais com matriz de LEDs, alertas sonoros e acesso remoto via servidor web responsivo.

## 🎯 Funcionalidades

- **Monitoramento em Tempo Real**: Leitura contínua de sensores AHT20 e BMP280
- **Interface Local**: Display OLED com múltiplas telas de navegação
- **Indicadores Visuais**: Matriz de LEDs WS2812B com padrões de status
- **Sistema de Alertas**: Notificações sonoras e visuais para valores fora dos limites
- **Servidor Web**: Dashboard interativo com gráficos em tempo real
- **Configuração Remota**: Ajuste de limites e calibração via interface web
- **Conectividade Wi-Fi**: Acesso remoto através de rede local

## 🛠️ Componentes Hardware

### Microcontrolador
- **Raspberry Pi Pico**: Unidade central de processamento

### Sensores
- **AHT20**: Sensor de temperatura e umidade relativa
- **BMP280**: Sensor de pressão atmosférica e temperatura

### Interface de Usuário
- **SSD1306**: Display OLED 128x64 pixels
- **WS2812B**: Matriz de LEDs 5x5 pixels
- **Buzzers**: Sistema de alerta sonoro (2 unidades)
- **Botões**: Navegação e reset do sistema

### Comunicação
- **CYW43**: Módulo Wi-Fi integrado

## 📁 Estrutura do Projeto

```
Embarcatech_F2T11_estacao_meteorologica/
├── Embarcatech_F2T11_estacao_meteorologica.c  # Arquivo principal
├── CMakeLists.txt                              # Configuração de build
├── ws2812.pio                                  # Programa PIO para LEDs
└── lib/                                        # Bibliotecas de sensores
    ├── aht20.c/h                              # Driver sensor AHT20
    ├── bmp280.c/h                             # Driver sensor BMP280
    ├── ssd1306.c/h                            # Driver display OLED
    └── font.h                                 # Fontes para display
```

## 🔧 Configuração e Instalação

### Pré-requisitos
- Raspberry Pi Pico
- Pico SDK configurado
- Sensores e componentes listados
- Conexão Wi-Fi disponível

### Conexões Hardware

#### I2C Sensores (i2c0)
- **SDA**: GPIO 0
- **SCL**: GPIO 1
- **Sensores**: AHT20, BMP280

#### I2C Display (i2c1)
- **SDA**: GPIO 14
- **SCL**: GPIO 15
- **Display**: SSD1306 (Endereço 0x3C)

#### Controles
- **Botão A**: GPIO 5 (Navegação)
- **Botão B**: GPIO 6 (Navegação)
- **Joystick**: GPIO 22 (Reset)

#### LEDs e Indicadores
- **Matriz LED**: GPIO 7 (WS2812B)
- **LED Verde**: GPIO 11
- **LED Azul**: GPIO 12
- **LED Vermelho**: GPIO 13

#### Alertas Sonoros
- **Buzzer A**: GPIO 21
- **Buzzer B**: GPIO 10

### Configuração de Rede
Edite as credenciais Wi-Fi no arquivo principal:
```c
#define WIFI_SSID "Sua_Rede_WiFi"
#define WIFI_PASS "Sua_Senha_WiFi"
```

### Compilação
```bash
mkdir build
cd build
cmake ..
make
```

### Upload
```bash
cp Embarcatech_F2T11_estacao_meteorologica.uf2 /media/pico/
```

## 🚀 Operação

### Inicialização
1. Conecte o hardware conforme especificado
2. Alimente o Raspberry Pi Pico
3. Aguarde a inicialização dos sensores
4. Verifique a conexão Wi-Fi no display
5. Acesse o endereço IP exibido no navegador

### Interface Local
- **Tela 1**: Status Wi-Fi e endereço IP
- **Tela 2**: Dados dos sensores (temperatura, pressão, altitude, umidade)
- **Tela 3**: Configurações de temperatura e alertas
- **Tela 4**: Configurações de umidade e alertas

### Navegação
- **Botão A**: Navegar para tela anterior
- **Botão B**: Navegar para próxima tela
- **Joystick**: Reset do sistema

## 🌐 Interface Web

### Dashboard Principal
- Gráficos em tempo real dos últimos 20 pontos
- Valores atuais com formatação adequada
- Médias calculadas automaticamente
- Design responsivo para dispositivos móveis

### Configuração de Limites
- Temperatura mínima e máxima
- Umidade mínima e máxima
- Aplicação imediata das configurações

### Calibração de Sensores
- Offset de temperatura
- Offset de pressão
- Offset de altitude
- Offset de umidade

## 📊 Especificações Técnicas

### Sensores
- **AHT20**: 
  - Temperatura: -40°C a +85°C (±0.3°C)
  - Umidade: 0-100% (±2%)
- **BMP280**:
  - Pressão: 300-1100 hPa (±1 hPa)
  - Temperatura: -40°C a +85°C (±0.5°C)

### Comunicação
- **I2C**: 400 kHz para sensores e display
- **Wi-Fi**: 802.11 b/g/n
- **Servidor Web**: Porta 80

### Atualização de Dados
- **Frequência**: A cada 300ms
- **Histórico**: 20 pontos por sensor
- **Latência**: < 1 segundo

## 🔍 Monitoramento e Alertas

### Condições de Alerta
- Temperatura fora dos limites configurados
- Umidade fora dos limites configurados
- Falha na leitura de sensores
- Perda de conectividade Wi-Fi

### Indicadores Visuais
- **Normal**: Padrão verde na matriz de LEDs
- **Alerta**: Padrão vermelho na matriz de LEDs
- **Status Wi-Fi**: LEDs RGB indicam estado da conexão

### Alertas Sonoros
- **Frequência**: 1000 Hz
- **Duração**: Configurável (padrão 200ms)
- **Ativação**: Mudanças de configuração e alertas

## 🛡️ Tratamento de Erros

### Robustez do Sistema
- Verificação contínua de conectividade Wi-Fi
- Tratamento de falhas de leitura de sensores
- Sistema de debounce para botões
- Recuperação automática de erros temporários

### Logs e Debug
- Mensagens de debug via UART
- Indicadores visuais de status
- Feedback sonoro para operações críticas

## 📈 Aplicações

### Monitoramento Ambiental
- Estações meteorológicas
- Estufas e agricultura
- Laboratórios e salas de controle
- Monitoramento industrial

### IoT e Automação
- Integração com sistemas de automação
- Coleta de dados para análise
- Alertas automáticos
- Controle de HVAC

## 🤝 Contribuição

Para contribuir com o projeto:

1. Faça um fork do repositório
2. Crie uma branch para sua feature
3. Implemente as mudanças
4. Teste adequadamente
5. Envie um pull request


## 👥 Autores

- **Muriel Costa**
- **Projeto**: Estação Meteorológica Embarcada
- **Versão**: 1.0

## 📞 Suporte

Para dúvidas ou problemas:
- Abra uma issue no repositório
- Consulte a documentação técnica
- Verifique as conexões hardware

---
