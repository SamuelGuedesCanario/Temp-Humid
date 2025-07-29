🌤️ Estação Meteorológica Embarcada
Sistema completo de monitoramento meteorológico baseado no Raspberry Pi Pico W com interface web responsiva, sensores I²C, alertas visuais e sonoros — ideal para aplicações IoT.

📋 Descrição
Este projeto implementa uma estação meteorológica embarcada que coleta e exibe, em tempo real, dados de temperatura, umidade, pressão e altitude. As leituras são feitas com sensores de alta precisão e exibidas tanto localmente (via display OLED e matriz de LEDs) quanto remotamente (via interface web). O sistema inclui alertas automáticos quando valores saem de limites configurados.

🎯 Funcionalidades
Leitura em Tempo Real dos sensores AHT20 (temperatura/umidade) e BMP280 (pressão/temperatura).

Exibição Local via display OLED SSD1306 com múltiplas telas navegáveis.

Alertas Visuais por matriz de LEDs RGB WS2812B.

Alertas Sonoros com buzzer duplo controlado por PWM.

Dashboard Web com gráficos atualizados dinamicamente (AJAX + JSON).

Calibração e Configuração de limites diretamente pela interface web.

Conectividade Wi-Fi com visualização do IP no display.

🛠️ Componentes de Hardware
🧠 Microcontrolador
Raspberry Pi Pico W – CPU principal com Wi-Fi embutido (CYW43)

🌡️ Sensores
AHT20 – Temperatura e umidade

BMP280 – Pressão e temperatura, cálculo de altitude

💡 Interface com Usuário
Display OLED (SSD1306) – 128x64 via I2C

Matriz de LEDs WS2812B – 5x5 LEDs endereçáveis RGB

LEDs Individuais RGB – Verde (status ok), azul (conectando), vermelho (erro)

Buzzers (2x) – PWM para sinal sonoro de alerta

Botões físicos – Navegação entre telas e reset (via joystick)

🔌 Pinagem e Conexões
Sensores I2C – i2c0
SDA: GPIO 0

SCL: GPIO 1

Display OLED – i2c1
SDA: GPIO 14

SCL: GPIO 15

Endereço: 0x3C

LEDs e Buzzer
Matriz WS2812B: GPIO 7

LED Verde: GPIO 11

LED Azul: GPIO 12

LED Vermelho: GPIO 13

Buzzer A: GPIO 21

Buzzer B: GPIO 10

Botões
Botão A: GPIO 5

Botão B: GPIO 6

Joystick (Reset): GPIO 22

🌐 Interface Web
Página HTML responsiva (mobile e desktop)

Dashboard com gráficos em tempo real (Chart.js)

Formulários para:

Definir limites (mín/máx) de temperatura e umidade

Calibrar offsets de sensores

Atualização via JSON (AJAX) a cada segundo

Servidor HTTP próprio via Wi-Fi

🚦 Estados do Sistema
Estado	LED RGB	Buzzer	Matriz de LEDs
Conectado	Verde	—	Padrão verde
Conectando	Azul	—	—
Erro de conexão	Vermelho	—	—
Alerta disparado	—	1000Hz, 200ms	Padrão vermelho
Dentro dos limites	—	—	Padrão normal verde

🧠 Lógica de Funcionamento
Leitura contínua dos sensores com tratamento de erro

Atualização de display e interface web a cada 300ms

Trigger automático de alertas com base em limites definidos

Navegação entre telas com botões físicos

Reset via botão de joystick

📁 Estrutura do Projeto
bash
Copiar
Editar
estacao_meteorologica/
├── main.c                          # Código principal
├── CMakeLists.txt                  # Build system
├── ws2812.pio                      # Programa PIO para matriz de LEDs
└── lib/                            # Drivers dos periféricos
    ├── aht20.c/h                   # AHT20
    ├── bmp280.c/h                  # BMP280
    ├── ssd1306.c/h                 # Display OLED
    └── font.h                      # Fonte usada no display
🔧 Compilação e Instalação
1. Edite as credenciais Wi-Fi:
c
Copiar
Editar
#define WIFI_SSID "Seu_SSID"
#define WIFI_PASS "Sua_Senha"
2. Compile o projeto:
bash
Copiar
Editar
mkdir build && cd build
cmake ..
make
3. Envie o .uf2 para o Pico:
bash
Copiar
Editar
cp estacao_meteorologica.uf2 /media/pico/
🚀 Execução
Conecte os componentes conforme a pinagem

Alimente o Raspberry Pi Pico

O sistema inicializa e tenta conectar ao Wi-Fi

Após conexão, o IP é exibido no display

Acesse o IP no navegador para abrir a interface web

📊 Especificações Técnicas
Parâmetro	Sensor	Faixa / Precisão
Temperatura	AHT20	-40°C a +85°C (±0.3°C)
Umidade	AHT20	0–100% UR (±2%)
Pressão	BMP280	300–1100 hPa (±1 hPa)
Altitude (calc.)	BMP280	~0–9000m (estimado)

Comunicação I²C @ 400kHz

Atualização: 300ms por ciclo

Histórico: 20 pontos por gráfico

Servidor: HTTP porta 80

🧪 Aplicações
Monitoramento ambiental (salas, estufas, ar-condicionado)

Coleta de dados para IoT

Controle de sistemas de climatização (HVAC)

Educação em sistemas embarcados e web

👨‍💻 Autor
Samuel Guedes Canário

Projeto individual — Polo: Bom Jesus da Lapa

Curso: Desenvolvimento de Sistemas Embarcados IoT – BitDogLab

🎥 Demonstração
📹 Link para vídeo: [https://youtu.be/VvGa0-rqYi8]
💾 Código-fonte completo: [https://github.com/SamuelGuedesCanario/Temp-Humid.git]
Link do Relatório Completo: [https://1drv.ms/b/c/d93638f8ce3970bd/ETkuQVbb_FdOm_EF3KKsdmwB_yfD_6y-2B3B-L3U4hklNg?e=sl3ogr]


