#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "hardware/pio.h"
#include "lwip/tcp.h"
#include "ws2812.pio.h"
#include "aht20.h"
#include "bmp280.h"
#include "ssd1306.h"
#include "font.h"

#define WIFI_SSID "Silvia Helena"
#define WIFI_PASS "milinha1"

#define SEA_LEVEL_PRESSURE_PA 101325.0

#define SENSOR_I2C_PORT i2c0
#define SENSOR_I2C_SDA 0
#define SENSOR_I2C_SCL 1

#define DISPLAY_I2C_PORT i2c1
#define DISPLAY_I2C_SDA 14
#define DISPLAY_I2C_SCL 15
#define DISPLAY_I2C_ADDRESS 0x3C

#define BUTTON_A_GPIO 5
#define BUTTON_B_GPIO 6
#define BUTTON_JOYSTICK_GPIO 22

#define LED_MATRIX_GPIO 7
#define LED_MATRIX_COUNT 25
#define LED_GREEN_GPIO 11
#define LED_BLUE_GPIO 12
#define LED_RED_GPIO 13

#define BUZZER_A_GPIO 21
#define BUZZER_B_GPIO 10

struct rgb_pixel_t {
    uint8_t green, red, blue;
};
typedef struct rgb_pixel_t rgb_pixel_t;
typedef rgb_pixel_t ws2812_led_t;

struct http_response_state
{
    char response_buffer[20000];
    size_t response_length;
    size_t bytes_sent;
};

struct bmp280_calib_param bmp280_calibration_params;
AHT20_Data aht20_sensor_data;
ssd1306_t oled_display;

static volatile uint32_t last_button_press_time = 0;

volatile int32_t bmp280_raw_temperature;
volatile int32_t bmp280_raw_pressure;
volatile int32_t current_pressure_pa = 0;
volatile double calculated_altitude_m = 0;

volatile float final_temperature_c = 0;
volatile float final_pressure_kpa = 0;
volatile float final_altitude_m = 0;
volatile float final_humidity_percent = 0;

volatile float temperature_offset_c = 0;
volatile float pressure_offset_kpa = 0;
volatile float altitude_offset_m = 0;
volatile float humidity_offset_percent = 0;

volatile float temperature_min_threshold_c = 10.0;
volatile float temperature_max_threshold_c = 35.0;
volatile float humidity_min_threshold_percent = 30.0;
volatile float humidity_max_threshold_percent = 70.0;

volatile int current_display_screen = 1;
volatile int wifi_status_text = 1;

char ip_address_string[24];
char temperature_string[5];
char pressure_string[6];
char altitude_string[5];
char humidity_string[5];
char temperature_min_string[5];
char temperature_max_string[5];
char humidity_min_string[5];
char humidity_max_string[5];

ws2812_led_t led_matrix[LED_MATRIX_COUNT];
PIO pio_instance;
uint state_machine;

const char HTML_BODY[] =
"<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Estação Meteorológica</title>"
"<meta name='viewport' content='width=device-width,initial-scale=1.0'>"
"<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>"
"<style>"
"body{margin:0;font-family:sans-serif;background:#f0f2f5;color:#222}"
".c{max-width:1200px;margin:auto;padding:10px}"
"h1{text-align:center;margin-bottom:10px;font-size:24px}"
"p{text-align:center;font-size:14px;margin-bottom:20px}"
".d{display:grid;grid-template-columns:repeat(auto-fit,minmax(260px,1fr));gap:15px}"
".card{background:#fff;border:1px solid #ccc;border-radius:10px;padding:15px}"
".ct{font-weight:bold;margin-bottom:10px}"
".val{font-size:22px;font-weight:bold;margin:10px 0}"
".avg{font-size:12px;color:#666;text-align:center}"
".chart{height:150px}"
"form{margin-top:10px}"
"label{font-size:13px;display:block;margin:10px 0 5px}"
"input{width:100%;padding:6px 8px;border:1px solid #aaa;border-radius:6px}"
"button{margin-top:10px;width:100%;padding:8px;background:#555;color:#fff;border:none;border-radius:6px;font-weight:bold}"
"@media(max-width:600px){h1{font-size:18px}.d{grid-template-columns:1fr}}"
"</style></head><body>"
"<div class='c'>"
"<h1>Estação Meteorológica</h1><p>Dados atualizados em tempo real</p>"

"<div class='d'>"
"<div class='card'><div class='ct'>Temperatura</div><div id='valorTemp' class='val'>-- °C</div><div class='chart'><canvas id='chartTemp'></canvas></div><div id='mediaTemp' class='avg'>Média: -- °C</div></div>"
"<div class='card'><div class='ct'>Pressão</div><div id='valorPres' class='val'>-- kPa</div><div class='chart'><canvas id='chartPres'></canvas></div><div id='mediaPres' class='avg'>Média: -- kPa</div></div>"
"<div class='card'><div class='ct'>Altitude</div><div id='valorAlt' class='val'>-- m</div><div class='chart'><canvas id='chartAlt'></canvas></div><div id='mediaAlt' class='avg'>Média: -- m</div></div>"
"<div class='card'><div class='ct'>Umidade</div><div id='valorUmi' class='val'>-- %</div><div class='chart'><canvas id='chartUmi'></canvas></div><div id='mediaUmi' class='avg'>Média: -- %</div></div>"
"</div>"

"<form onsubmit='return enviarLimites();'>"
"<label>Temp Mín (°C)</label><input type='number' step='0.1' id='temp_min' value='10.0' required>"
"<label>Temp Máx (°C)</label><input type='number' step='0.1' id='temp_max' value='35.0' required>"
"<label>Umidade Mín (%)</label><input type='number' step='0.1' id='umi_min' value='30.0' required>"
"<label>Umidade Máx (%)</label><input type='number' step='0.1' id='umi_max' value='70.0' required>"
"<button type='submit'>Salvar Limites</button>"
"</form>"

"<form onsubmit='return enviarOffsets();'>"
"<label>Offset Temp (°C)</label><input type='number' step='0.1' id='temp_off' value='0.0' required>"
"<label>Offset Pressão (kPa)</label><input type='number' step='0.1' id='pres_off' value='0.0' required>"
"<label>Offset Altitude (m)</label><input type='number' step='0.1' id='alt_off' value='0.0' required>"
"<label>Offset Umidade (%)</label><input type='number' step='0.1' id='umi_off' value='0.0' required>"
"<button type='submit'>Salvar Calibração</button>"
"</form>"
"</div>"

"<script>"
"function enviarLimites(){"
"fetch(`/set_limits?temp_min=${temp_min.value}&temp_max=${temp_max.value}&umi_min=${umi_min.value}&umi_max=${umi_max.value}`);return false;}"
"function enviarOffsets(){"
"fetch(`/set_offsets?temp_off=${temp_off.value}&pres_off=${pres_off.value}&alt_off=${alt_off.value}&umi_off=${umi_off.value}`);return false;}"
"let t=[],dT=[],dP=[],dA=[],dU=[];"
"const op=l=>({responsive:true,maintainAspectRatio:false,scales:{y:{beginAtZero:false},x:{display:false}},plugins:{legend:{display:false},title:{display:true,text:l}}});"
"const cg=(id,l,c)=>new Chart(document.getElementById(id),{type:'line',data:{labels:t,datasets:[{label:l,data:[],borderColor:c,backgroundColor:c+'22',tension:.4,fill:true}]},options:op(l)});"
"let cT=cg('chartTemp','Temperatura','#f66'),cP=cg('chartPres','Pressão','#4ecdc4'),cA=cg('chartAlt','Altitude','#45b7d1'),cU=cg('chartUmi','Umidade','#a55eea');"
"function atualiza(){"
"fetch('/dados').then(r=>r.json()).then(d=>{"
"let tm=new Date().toLocaleTimeString();if(t.length>=20)t.shift();t.push(tm);"
"let u=(a,v,g,m)=>{if(a.length>=20)a.shift();a.push(v);g.data.labels=t;g.data.datasets[0].data=a;g.update('none');let med=(a.reduce((x,y)=>x+y,0)/a.length).toFixed(2);document.getElementById(m).innerHTML='Média: '+med;};"
"if(d.tem!==undefined){document.getElementById('valorTemp').innerHTML=d.tem.toFixed(1)+' °C';u(dT,parseFloat(d.tem),cT,'mediaTemp');}"
"if(d.pre!==undefined){document.getElementById('valorPres').innerHTML=d.pre.toFixed(2)+' kPa';u(dP,parseFloat(d.pre),cP,'mediaPres');}"
"if(d.alt!==undefined){document.getElementById('valorAlt').innerHTML=d.alt.toFixed(0)+' m';u(dA,parseFloat(d.alt),cA,'mediaAlt');}"
"if(d.umi!==undefined){document.getElementById('valorUmi').innerHTML=d.umi.toFixed(1)+' %';u(dU,parseFloat(d.umi),cU,'mediaUmi');}"
"});}"
"setInterval(atualiza,1000);"
"</script></body></html>";


void set_buzzer_pwm(uint gpio, bool active);
void configure_buzzer_frequency(uint gpio, uint freq);
void play_buzzer_beep(uint time_ms);
void set_led_color(const uint led_index, const uint8_t red, const uint8_t green, const uint8_t blue);
void turn_off_all_leds(void);
void update_led_buffer(void);
int convert_matrix_position(int x, int y);
double calculate_altitude_from_pressure(double pressure_pa);
void read_bmp280_sensor(void);
void read_aht20_sensor(void);
void refresh_display_content(void);
void update_led_matrix_display(bool alert_condition);
void update_final_sensor_values(void);
void handle_button_interrupt(uint gpio_pin, uint32_t events);
static err_t http_response_sent(void *arg, struct tcp_pcb *tcp_connection, u16_t bytes_sent);
static err_t http_request_handler(void *arg, struct tcp_pcb *tcp_connection, struct pbuf *packet_buffer, err_t error);
static err_t http_connection_callback(void *arg, struct tcp_pcb *new_connection, err_t error);
static void initialize_http_server(void);

int64_t buzzer_alarm_callback(alarm_id_t id, void *user_data){
    set_buzzer_pwm(BUZZER_A_GPIO, false);
    set_buzzer_pwm(BUZZER_B_GPIO, false);
    return 0;
}

void configure_buzzer_frequency(uint gpio, uint freq) {
    uint pwm_slice = pwm_gpio_to_slice_num(gpio);
    uint clock_divisor = 4;
    uint wrap_value = (125000000 / (clock_divisor * freq)) - 1;

    pwm_set_clkdiv(pwm_slice, clock_divisor);
    pwm_set_wrap(pwm_slice, wrap_value);
    pwm_set_chan_level(pwm_slice, pwm_gpio_to_channel(gpio), wrap_value / 40);
}

void set_buzzer_pwm(uint gpio, bool active){
    uint pwm_slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_enabled(pwm_slice, active);
}

void play_buzzer_beep(uint time_ms){
    set_buzzer_pwm(BUZZER_A_GPIO, true);
    set_buzzer_pwm(BUZZER_B_GPIO, true);
    add_alarm_in_ms(time_ms, buzzer_alarm_callback, NULL, false);
}

void set_led_color(const uint led_index, const uint8_t red, const uint8_t green, const uint8_t blue) {
    led_matrix[led_index].red = red;
    led_matrix[led_index].green = green;
    led_matrix[led_index].blue = blue;
}

void turn_off_all_leds(void) {
    for (uint i = 0; i < LED_MATRIX_COUNT; ++i) {
        set_led_color(i, 0, 0, 0);
    }
}

void update_led_buffer(void) {
    for (uint i = 0; i < LED_MATRIX_COUNT; ++i) {
        pio_sm_put_blocking(pio_instance, state_machine, led_matrix[i].green);
        pio_sm_put_blocking(pio_instance, state_machine, led_matrix[i].red);
        pio_sm_put_blocking(pio_instance, state_machine, led_matrix[i].blue);
    }
}

int convert_matrix_position(int x, int y) {
    if (y % 2 == 0) {
        return 24-(y * 5 + x);
    } else {
        return 24-(y * 5 + (4 - x));
    }
}

double calculate_altitude_from_pressure(double pressure_pa){
    return 44330.0 * (1.0 - pow(pressure_pa / SEA_LEVEL_PRESSURE_PA, 0.1903));
}

void read_bmp280_sensor(void){
    bmp280_read_raw(SENSOR_I2C_PORT, &bmp280_raw_temperature, &bmp280_raw_pressure);
    int32_t temperature_celsius = bmp280_convert_temp(bmp280_raw_temperature, &bmp280_calibration_params);
    current_pressure_pa = bmp280_convert_pressure(bmp280_raw_pressure, bmp280_raw_temperature, &bmp280_calibration_params);

    calculated_altitude_m = calculate_altitude_from_pressure(current_pressure_pa);

    printf("Pressao: %.3f kPa\n", current_pressure_pa / 1000.0);
    printf("Temperatura BMP: %.2f C\n", temperature_celsius / 100.0);
    printf("Altitude estimada: %.2f m\n", calculated_altitude_m);  
}

void read_aht20_sensor(void){
    if(aht20_read(SENSOR_I2C_PORT, &aht20_sensor_data)){
        printf("Temperatura aht: %.2f C\n", aht20_sensor_data.temperature);
        printf("Umidade: %.2f %%\n\n\n", aht20_sensor_data.humidity);
    }else{
        printf("Erro na leitura do AHT20!\n\n\n");
    }
}

void update_final_sensor_values(void){
    final_temperature_c = aht20_sensor_data.temperature + temperature_offset_c;
    final_pressure_kpa = (current_pressure_pa / 1000) + pressure_offset_kpa;
    final_altitude_m = calculated_altitude_m + altitude_offset_m;
    final_humidity_percent = aht20_sensor_data.humidity + humidity_offset_percent;
}

void refresh_display_content(void){
    ssd1306_fill(&oled_display, false);
    ssd1306_rect(&oled_display, 0, 0, 127, 63, true, false);

    if(current_display_screen == 1){
        ssd1306_draw_string(&oled_display, "ESTACAO", 12, 3);
        ssd1306_draw_string(&oled_display, "METEOROLOGICA", 12, 12);

        ssd1306_line(&oled_display, 1, 21, 126, 21, true);

        ssd1306_draw_string(&oled_display, "Wi-Fi:", 12, 23);
        if(wifi_status_text == 1){
            ssd1306_draw_string(&oled_display, "Iniciando...", 16, 32);    
        }else if(wifi_status_text == 2){
            ssd1306_draw_string(&oled_display, "Conectando...", 12, 32);
        }else if(wifi_status_text == 3){
            ssd1306_draw_string(&oled_display, "Falha!", 12, 32);
        }else if(wifi_status_text == 4){
            ssd1306_draw_string(&oled_display, "Conectado!", 12, 32);
        }else{
            ssd1306_draw_string(&oled_display, "Erro!", 12, 32);
        }

        ssd1306_line(&oled_display, 1, 41, 126, 41, true);

        ssd1306_draw_string(&oled_display, "IP servidor:", 12, 43);
        ssd1306_draw_string(&oled_display, ip_address_string, 16, 52);

    }else if(current_display_screen == 2){
        ssd1306_draw_string(&oled_display, "Dados do local:", 4, 3);

        ssd1306_line(&oled_display, 1, 12, 126, 12, true);

        sprintf(temperature_string, "%.1fC", final_temperature_c);
        ssd1306_draw_string(&oled_display, "Tem:", 4, 15);
        ssd1306_draw_string(&oled_display, temperature_string, 40, 15);

        ssd1306_line(&oled_display, 1, 25, 126, 25, true);

        sprintf(pressure_string, "%.2fkPa", final_pressure_kpa);
        ssd1306_draw_string(&oled_display, "Pre:", 4, 28);
        ssd1306_draw_string(&oled_display, pressure_string, 40, 28);

        ssd1306_line(&oled_display, 1, 38, 126, 38, true);

        sprintf(altitude_string, "%.0fm", final_altitude_m);
        ssd1306_draw_string(&oled_display, "Alt:", 4, 41);
        ssd1306_draw_string(&oled_display, altitude_string, 40, 41);

        ssd1306_line(&oled_display, 1, 51, 126, 51, true);

        sprintf(humidity_string, "%.1f%%", final_humidity_percent);
        ssd1306_draw_string(&oled_display, "Umi:", 4, 53);
        ssd1306_draw_string(&oled_display, humidity_string, 40, 53);

    }else if(current_display_screen == 3){
        ssd1306_draw_string(&oled_display, "TEMPERATURA", 20, 3);

        ssd1306_line(&oled_display, 1, 12, 126, 12, true);

        sprintf(temperature_string, "%.1fC", final_temperature_c);
        ssd1306_draw_string(&oled_display, "Atual:", 4, 15);
        ssd1306_draw_string(&oled_display, temperature_string, 56, 15);

        ssd1306_line(&oled_display, 1, 25, 126, 25, true);

        sprintf(temperature_min_string, "%.1fC", temperature_min_threshold_c);
        ssd1306_draw_string(&oled_display, "Min:", 4, 28);
        ssd1306_draw_string(&oled_display, temperature_min_string, 40, 28);

        ssd1306_line(&oled_display, 1, 38, 126, 38, true);

        sprintf(temperature_max_string, "%.1fC", temperature_max_threshold_c);
        ssd1306_draw_string(&oled_display, "Max:", 4, 41);
        ssd1306_draw_string(&oled_display, temperature_max_string, 40, 41);

        ssd1306_line(&oled_display, 1, 51, 126, 51, true);

        if(final_temperature_c >= temperature_max_threshold_c){
            ssd1306_draw_string(&oled_display, "Alerta: T > Max", 2, 53);
        }else if(final_temperature_c <= temperature_min_threshold_c){
            ssd1306_draw_string(&oled_display, "Alerta: T < Min", 2, 53);
        }else{
            ssd1306_draw_string(&oled_display, "Status: Ok", 24, 53);
        }
    }else if(current_display_screen == 4){
        ssd1306_draw_string(&oled_display, "UMIDADE", 36, 3);

        ssd1306_line(&oled_display, 1, 12, 126, 12, true);

        sprintf(humidity_string, "%.1f%%", final_humidity_percent);
        ssd1306_draw_string(&oled_display, "Atual:", 4, 15);
        ssd1306_draw_string(&oled_display, humidity_string, 56, 15);

        ssd1306_line(&oled_display, 1, 25, 126, 25, true);

        sprintf(humidity_min_string, "%.1f%%", humidity_min_threshold_percent);
        ssd1306_draw_string(&oled_display, "Min:", 4, 28);
        ssd1306_draw_string(&oled_display, humidity_min_string, 40, 28);

        ssd1306_line(&oled_display, 1, 38, 126, 38, true);

        sprintf(humidity_max_string, "%.1f%%", humidity_max_threshold_percent);
        ssd1306_draw_string(&oled_display, "Max:", 4, 41);
        ssd1306_draw_string(&oled_display, humidity_max_string, 40, 41);

        ssd1306_line(&oled_display, 1, 51, 126, 51, true);

        if(final_humidity_percent >= humidity_max_threshold_percent){
            ssd1306_draw_string(&oled_display, "Alerta: U > Max", 2, 53);
        }else if(final_humidity_percent <= humidity_min_threshold_percent){
            ssd1306_draw_string(&oled_display, "Alerta: U < Min", 2, 53);
        }else{
            ssd1306_draw_string(&oled_display, "Status: Ok", 24, 53);
        }
    }
    ssd1306_send_data(&oled_display);
}

void update_led_matrix_display(bool alert_condition){
    turn_off_all_leds();
    if(alert_condition){
            int alert_frame[5][5][3] = {
            {{150, 0, 0}, {150, 0, 0}, {150, 0, 0}, {150, 0, 0}, {150, 0, 0}},
            {{150, 0, 0}, {150, 0, 0}, {150, 0, 0}, {150, 0, 0}, {150, 0, 0}},    
            {{150, 0, 0}, {150, 0, 0}, {150, 0, 0}, {150, 0, 0}, {150, 0, 0}},
            {{150, 0, 0}, {150, 0, 0}, {150, 0, 0}, {150, 0, 0}, {150, 0, 0}},
            {{150, 0, 0}, {150, 0, 0}, {150, 0, 0}, {150, 0, 0}, {150, 0, 0}}
        };
        for (int row = 0; row < 5; row++){
            for (int col = 0; col < 5; col++){
                int led_position = convert_matrix_position(row, col);
                set_led_color(led_position, alert_frame[col][row][0], alert_frame[col][row][1], alert_frame[col][row][2]);
            }
        };
        update_led_buffer();
    }else{
        int normal_frame[5][5][3] = {
            {{0, 150, 0}, {0, 150, 0}, {0, 150, 0}, {0, 150, 0}, {0, 150, 0}},
            {{0, 150, 0}, {0, 150, 0}, {0, 150, 0}, {0, 150, 0}, {0, 150, 0}},    
            {{0, 150, 0}, {0, 150, 0}, {0, 150, 0}, {0, 150, 0}, {0, 150, 0}},
            {{0, 150, 0}, {0, 150, 0}, {0, 150, 0}, {0, 150, 0}, {0, 150, 0}},
            {{0, 150, 0}, {0, 150, 0}, {0, 150, 0}, {0, 150, 0}, {0, 150, 0}}
        };
        for (int row = 0; row < 5; row++){
            for (int col = 0; col < 5; col++){
                int led_position = convert_matrix_position(row, col);
                set_led_color(led_position, normal_frame[col][row][0], normal_frame[col][row][1], normal_frame[col][row][2]);
            }
        };
        update_led_buffer();
    }
}

void handle_button_interrupt(uint gpio_pin, uint32_t events){
    uint32_t current_time_us = to_us_since_boot(get_absolute_time());
    if(current_time_us - last_button_press_time > 1000000){
        last_button_press_time = current_time_us;
        if(gpio_pin == BUTTON_A_GPIO){
            if(current_display_screen <= 1){
                current_display_screen = 4;
            }else{
                current_display_screen = current_display_screen - 1;
            }
            refresh_display_content();
        }else if(gpio_pin == BUTTON_B_GPIO){
            if(current_display_screen >= 4){
                current_display_screen = 1;
            }else{
                current_display_screen = current_display_screen + 1;
            }
            refresh_display_content();
        }else if(gpio_pin == BUTTON_JOYSTICK_GPIO){
            reset_usb_boot(0, 0);
        }
    }
}

static err_t http_response_sent(void *arg, struct tcp_pcb *tcp_connection, u16_t bytes_sent)
{
    struct http_response_state *response_state = (struct http_response_state *)arg;
    response_state->bytes_sent += bytes_sent;
    if (response_state->bytes_sent >= response_state->response_length)
    {
        tcp_close(tcp_connection);
        free(response_state);
    }
    return ERR_OK;
}

static err_t http_request_handler(void *arg, struct tcp_pcb *tcp_connection, struct pbuf *packet_buffer, err_t error)
{
    if (!packet_buffer)
    {
        tcp_close(tcp_connection);
        return ERR_OK;
    }

    char *request_data = (char *)packet_buffer->payload;
    struct http_response_state *response_state = malloc(sizeof(struct http_response_state));
    if (!response_state)
    {
        pbuf_free(packet_buffer);
        tcp_close(tcp_connection);
        return ERR_MEM;
    }
    response_state->bytes_sent = 0;

    if (strstr(request_data, "GET /set_limits?")) {
        play_buzzer_beep(200);
        float temp_min, temp_max, humidity_min, humidity_max;
        sscanf(request_data, "GET /set_limits?temp_min=%f&temp_max=%f&umi_min=%f&umi_max=%f", &temp_min, &temp_max, &humidity_min, &humidity_max);

        temperature_min_threshold_c = temp_min;
        temperature_max_threshold_c = temp_max;
        humidity_min_threshold_percent = humidity_min;
        humidity_max_threshold_percent = humidity_max;

        const char *response_message = "Limites atualizados com sucesso";
        response_state->response_length = snprintf(response_state->response_buffer, sizeof(response_state->response_buffer),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: %d\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "%s",
                        (int)strlen(response_message), response_message);
    }
    else if (strstr(request_data, "GET /dados"))
    {
        char json_response[2048];
        int json_length = snprintf(json_response, sizeof(json_response),
                                "{\"tem\":%.1f,\"pre\":%.2f,\"alt\":%.0f,\"umi\":%.1f}\r\n",
                                final_temperature_c, final_pressure_kpa, final_altitude_m, final_humidity_percent);

        printf("[DEBUG] JSON: %s\n", json_response);

        response_state->response_length = snprintf(response_state->response_buffer, sizeof(response_state->response_buffer),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: application/json\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "%s",
                           json_length, json_response);
    }
    else if (strstr(request_data, "GET /set_offsets?")) {
        play_buzzer_beep(200);
        float temp_offset, pressure_offset, altitude_offset, humidity_offset;
        sscanf(request_data, "GET /set_offsets?temp_off=%f&pres_off=%f&alt_off=%f&umi_off=%f", &temp_offset, &pressure_offset, &altitude_offset, &humidity_offset);

        temperature_offset_c = temp_offset;
        pressure_offset_kpa = pressure_offset;
        altitude_offset_m = altitude_offset;
        humidity_offset_percent = humidity_offset;

        const char *response_message = "Offsets atualizados com sucesso";
        response_state->response_length = snprintf(response_state->response_buffer, sizeof(response_state->response_buffer),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: %d\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "%s",
                        (int)strlen(response_message), response_message);
    }
    else
    {
        response_state->response_length = snprintf(response_state->response_buffer, sizeof(response_state->response_buffer),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/html\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "%s",
                           (int)strlen(HTML_BODY), HTML_BODY);
    }

    tcp_arg(tcp_connection, response_state);
    tcp_sent(tcp_connection, http_response_sent);

    tcp_write(tcp_connection, response_state->response_buffer, response_state->response_length, TCP_WRITE_FLAG_COPY);
    tcp_output(tcp_connection);

    pbuf_free(packet_buffer);
    return ERR_OK;
}

static err_t http_connection_callback(void *arg, struct tcp_pcb *new_connection, err_t error)
{
    tcp_recv(new_connection, http_request_handler);
    return ERR_OK;
}

static void initialize_http_server(void)
{
    struct tcp_pcb *tcp_pcb = tcp_new();
    if (!tcp_pcb)
    {
        printf("Erro ao criar PCB TCP\n");
        return;
    }
    if (tcp_bind(tcp_pcb, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }
    tcp_pcb = tcp_listen(tcp_pcb);
    tcp_accept(tcp_pcb, http_connection_callback);
    printf("Servidor HTTP rodando na porta 80...\n");
}

int main(void){
    stdio_init_all();
    sleep_ms(2000);

    gpio_init(LED_GREEN_GPIO);
    gpio_set_dir(LED_GREEN_GPIO, GPIO_OUT);
    gpio_init(LED_BLUE_GPIO);
    gpio_set_dir(LED_BLUE_GPIO, GPIO_OUT);
    gpio_init(LED_RED_GPIO);
    gpio_set_dir(LED_RED_GPIO, GPIO_OUT);

    gpio_set_function(BUZZER_A_GPIO, GPIO_FUNC_PWM);
    gpio_set_function(BUZZER_B_GPIO, GPIO_FUNC_PWM);
    configure_buzzer_frequency(BUZZER_A_GPIO, 1000);
    configure_buzzer_frequency(BUZZER_B_GPIO, 1000);

    gpio_init(BUTTON_A_GPIO);
    gpio_set_dir(BUTTON_A_GPIO, GPIO_IN);
    gpio_pull_up(BUTTON_A_GPIO);
    gpio_init(BUTTON_B_GPIO);
    gpio_set_dir(BUTTON_B_GPIO, GPIO_IN);
    gpio_pull_up(BUTTON_B_GPIO);
    gpio_init(BUTTON_JOYSTICK_GPIO);
    gpio_set_dir(BUTTON_JOYSTICK_GPIO, GPIO_IN);
    gpio_pull_up(BUTTON_JOYSTICK_GPIO);
    gpio_set_irq_enabled_with_callback(BUTTON_A_GPIO, GPIO_IRQ_EDGE_FALL, true, &handle_button_interrupt);
    gpio_set_irq_enabled_with_callback(BUTTON_B_GPIO, GPIO_IRQ_EDGE_FALL, true, &handle_button_interrupt);
    gpio_set_irq_enabled_with_callback(BUTTON_JOYSTICK_GPIO, GPIO_IRQ_EDGE_FALL, true, &handle_button_interrupt);

    pio_instance = pio0;
    state_machine = pio_claim_unused_sm(pio_instance, true);
    uint program_offset = pio_add_program(pio0, &ws2818b_program);
    ws2818b_program_init(pio_instance, state_machine, program_offset, LED_MATRIX_GPIO, 800000);
    turn_off_all_leds();
    update_led_buffer();

    i2c_init(DISPLAY_I2C_PORT, 400 * 1000);
    gpio_set_function(DISPLAY_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(DISPLAY_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(DISPLAY_I2C_SDA);
    gpio_pull_up(DISPLAY_I2C_SCL);
    ssd1306_init(&oled_display, WIDTH, HEIGHT, false, DISPLAY_I2C_ADDRESS, DISPLAY_I2C_PORT);
    ssd1306_config(&oled_display);
    ssd1306_send_data(&oled_display);
    ssd1306_fill(&oled_display, false);
    ssd1306_send_data(&oled_display);

    i2c_init(SENSOR_I2C_PORT, 400 * 1000);
    gpio_set_function(SENSOR_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SENSOR_I2C_SDA);
    gpio_pull_up(SENSOR_I2C_SCL);

    bmp280_init(SENSOR_I2C_PORT);
    bmp280_get_calib_params(SENSOR_I2C_PORT, &bmp280_calibration_params);

    aht20_reset(SENSOR_I2C_PORT);
    aht20_init(SENSOR_I2C_PORT);

    gpio_put(LED_GREEN_GPIO, 0);
    gpio_put(LED_BLUE_GPIO, 1);
    gpio_put(LED_RED_GPIO, 0);

    refresh_display_content();

    if (cyw43_arch_init())
    {
        wifi_status_text = 3;
        gpio_put(LED_GREEN_GPIO, 0);
        gpio_put(LED_BLUE_GPIO, 0);
        gpio_put(LED_RED_GPIO, 1);
        refresh_display_content();
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    wifi_status_text = 2;
    gpio_put(LED_GREEN_GPIO, 1);
    gpio_put(LED_BLUE_GPIO, 0);
    gpio_put(LED_RED_GPIO, 1);
    refresh_display_content();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000))
    {
        wifi_status_text = 3;
        gpio_put(LED_GREEN_GPIO, 0);
        gpio_put(LED_BLUE_GPIO, 0);
        gpio_put(LED_RED_GPIO, 1);
        refresh_display_content();
        return 1;
    }

    wifi_status_text = 4;
    gpio_put(LED_GREEN_GPIO, 1);
    gpio_put(LED_BLUE_GPIO, 0);
    gpio_put(LED_RED_GPIO, 0);
    refresh_display_content();

    uint8_t *ip_address_bytes = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
    snprintf(ip_address_string, sizeof(ip_address_string), "%d.%d.%d.%d", ip_address_bytes[0], ip_address_bytes[1], ip_address_bytes[2], ip_address_bytes[3]);
    refresh_display_content();

    initialize_http_server();

    while (true) {

        cyw43_arch_poll();
        
        read_bmp280_sensor();
        read_aht20_sensor();

        update_final_sensor_values();
        
        if(final_temperature_c <= temperature_min_threshold_c || final_temperature_c >= temperature_max_threshold_c || final_humidity_percent <= humidity_min_threshold_percent || final_humidity_percent >= humidity_max_threshold_percent){
            update_led_matrix_display(true);
        }else{
            update_led_matrix_display(false);
        }
        
        refresh_display_content();

        sleep_ms(300);
    }

    cyw43_arch_deinit();
    return 0;
}