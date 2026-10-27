"""Execute current firmware on the host with controlled hardware/clock adapters.
No application source is edited. This does not test electronics or real timing.
Run from any directory with Python 3 and GCC available.
"""
from pathlib import Path
import subprocess, tempfile, re, hashlib, json
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
source = ROOT / 'greenhouse/main/greenhouse.c'
code = source.read_text()
# Keep actual function bodies, removing only includes and the commented calibration app.
code = re.sub(r'/\*.*?\*/', '', code, flags=re.S)
code = re.sub(r'^#include.*$', '', code, flags=re.M)
stubs = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
typedef int esp_err_t;
typedef int adc_oneshot_unit_handle_t;
typedef struct {int unit_id, ulp_mode;} adc_oneshot_unit_init_cfg_t;
typedef struct {int bitwidth, atten;} adc_oneshot_chan_cfg_t;
typedef struct {uint64_t pin_bit_mask;int mode,pull_up_en,pull_down_en,intr_type;} gpio_config_t;
typedef struct {int dummy;} SSD1306_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_ARG -2
#define ESP_ERR_INVALID_SIZE -3
#define GPIO_NUM_16 16
#define GPIO_NUM_17 17
#define GPIO_NUM_21 21
#define GPIO_NUM_22 22
#define GPIO_NUM_33 33
#define GPIO_NUM_34 34
#define GPIO_NUM_35 35
#define ADC_CHANNEL_5 5
#define ADC_CHANNEL_6 6
#define ADC_CHANNEL_7 7
#define ADC_UNIT_1 1
#define ADC_ULP_MODE_DISABLE 0
#define ADC_BITWIDTH_12 12
#define ADC_ATTEN_DB_12 12
#define GPIO_MODE_OUTPUT 1
#define GPIO_PULLUP_DISABLE 0
#define GPIO_PULLDOWN_DISABLE 0
#define GPIO_INTR_DISABLE 0
#define DHT_TYPE_DHT11 11
#define ESP_ERROR_CHECK(x) assert((x)==ESP_OK)
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
#define pdMS_TO_TICKS(x) (x)
static uint64_t clock_ms=1000;
static int gpio_level, gpio_on_count, soil_calls, air_calls, soil_raw=1883, water_raw=1500, light_raw=1200;
static int soil_error, air_error, queue_error, queue_count, telemetry_count, displays_while_on;
static bool alternating;
static char last_event[512], last_telemetry[512], screen[8][64];
static jmp_buf end_loop;
static int64_t esp_timer_get_time(void){return (int64_t)clock_ms*1000;}
static int gpio_set_level(int pin,int level){assert(pin==16);gpio_level=level;if(level)gpio_on_count++;return 0;}
static int gpio_config(const gpio_config_t*c){return 0;}
static int adc_oneshot_new_unit(void*c,int*h){*h=1;return 0;}
static int adc_oneshot_config_channel(int h,int ch,void*c){return 0;}
static int adc_oneshot_read(int h,int ch,int*out){if(ch==5){soil_calls++;if(soil_error)return -1;*out=soil_raw+(alternating?(soil_calls%2):0);}else *out=ch==7?water_raw:light_raw;return 0;}
static int dht_read_float_data(int t,int pin,float*h,float*c){air_calls++;*h=50;*c=22;return air_error;}
static void i2c_master_init(SSD1306_t*o,int a,int b,int c){}
static int ssd1306_init(SSD1306_t*o,int w,int h){return 0;}
static void ssd1306_clear_screen(SSD1306_t*o,bool b){memset(screen,0,sizeof(screen));}
static void ssd1306_display_text(SSD1306_t*o,int row,const char*t,size_t n,bool inv){assert(n<64);memcpy(screen[row],t,n);screen[row][n]=0;if(gpio_level)displays_while_on++;}
static int networkInit(void){return 0;}
static int mqttInit(void){return 0;}
static int mqttQueueWateringEvent(const char*p){strcpy(last_event,p);queue_count++;return queue_error;}
static int mqttQueueTelemetry(const char*p){strcpy(last_telemetry,p);telemetry_count++;return queue_error;}
static void esp_fill_random(void*p,size_t n){memset(p,0x12,n);}
static void vTaskDelay(int ticks){if(ticks==5000)longjmp(end_loop,1);clock_ms+=ticks;}
'''
tests = r'''
static LatestReadings valid(void){LatestReadings r={.temperature=22,.humidity=50,.light=1200,.water=1500,.moisture=1883};validateReadings(&r);return r;}
static void start(LatestReadings*r){strcpy(watering_boot_id,"1234567890abcdef");updateWatering(r,&pump_on);assert(pump_on&&gpio_level==1);assert(strstr(last_event,"\"event\":\"started\""));assert(strstr(last_event,"\"duration_ms\":null"));}
int main(int argc,char**argv){
 assert(argc==2);LatestReadings r=valid();SSD1306_t oled={0};char out[512];
 if(!strcmp(argv[1],"conversion")){
  assert(r.moisture_pct==0&&r.light_pct==50);r.moisture=1822;validateReadings(&r);assert(r.moisture_pct==100);
  r.moisture=0;r.light=4095;validateReadings(&r);assert(r.moisture_pct==100&&r.light_pct==0);
  r.temperature=NAN;validateReadings(&r);assert(!r.air_valid);r.temperature=22;r.humidity=101;validateReadings(&r);assert(!r.air_valid);
  r=valid();alternating=true;readSensors(&r,1,true);assert(soil_calls==32&&r.moisture==1884&&air_calls==1);
  readSensors(&r,1,false);assert(air_calls==1&&soil_calls==64);soil_error=1;readSensors(&r,1,false);validateReadings(&r);assert(!r.moist_valid&&!r.activate_pump&&isnan(r.moisture_pct));
  puts("PASS VER-01: conversion endpoints, clamping, finite/range checks, 32-sample rounded mean, ADC error");
 }else if(!strcmp(argv[1],"reservoir")){
  int a[]={0,1199,1200,1201,1700,1701,-1,4095};
  for(int i=0;i<8;i++){r=valid();r.water=a[i];validateReadings(&r);bool good=a[i]>=0&&a[i]<=1700;assert(r.water_valid==good);assert(r.water_low==(!good||a[i]<=1200));assert(r.activate_pump==(good&&a[i]>1200));}
  puts("PASS VER-02: eight reservoir boundary/error inputs block or allow the start request correctly");
 }else if(!strcmp(argv[1],"timing")){
  start(&r);clock_ms=2999;updateWatering(&r,&pump_on);assert(pump_on);clock_ms=3000;updateWatering(&r,&pump_on);assert(!pump_on);assert(strstr(last_event,"\"duration_ms\":2000"));assert(strstr(last_event,"time_limit"));
  clock_ms=62999;updateWatering(&r,&pump_on);assert(!pump_on);clock_ms=63000;updateWatering(&r,&pump_on);assert(pump_on);assert(strstr(last_event,"\"cycle_id\":2"));
  puts("PASS VER-03: stays on at 1999 ms, stops on check at 2000 ms, blocks restart at 59999 ms, allows at 60000 ms cooldown");
 }else if(!strcmp(argv[1],"late_check")){
  start(&r);clock_ms=3007;updateWatering(&r,&pump_on);assert(!pump_on&&strstr(last_event,"\"duration_ms\":2007"));
  puts("LIMIT VER-03: a control check at 2007 ms records 2007 ms; this is not a measured physical pump duration");
 }else if(!strcmp(argv[1],"startup")){
  pump_on=true;initPump();assert(!pump_on&&gpio_level==0&&gpio_on_count==0);
  r.water_valid=false;updateWatering(&r,&pump_on);assert(!pump_on);r=valid();r.moist_valid=false;updateWatering(&r,&pump_on);assert(!pump_on);
  puts("PASS VER-04: startup commands OFF; invalid control inputs prevent starts");
 }else if(!strcmp(argv[1],"wet")||!strcmp(argv[1],"low")||!strcmp(argv[1],"fault")){
  start(&r);clock_ms+=100;if(!strcmp(argv[1],"wet"))r.moisture=1860;else if(!strcmp(argv[1],"low"))r.water_low=true;else {r.water_valid=false;r.water_low=true;}
  updateWatering(&r,&pump_on);assert(!pump_on);const char*reason=!strcmp(argv[1],"wet")?"soil_wet":!strcmp(argv[1],"low")?"low_water":"invalid_sensor";assert(strstr(last_event,reason));
  printf("PASS VER-03/04: stop at next check with %s reason\n",reason);
 }else if(!strcmp(argv[1],"rail")){
  r.moisture=4095;validateReadings(&r);assert(r.moist_valid&&r.activate_pump);start(&r);
  puts("GAP VER-04: soil ADC 4095 is accepted and starts watering; actual disconnected-sensor voltage was not measured");
 }else if(!strcmp(argv[1],"display")){
  displayResults(&oled,&r);assert(!strcmp(screen[6],"MONITORING")&&!strcmp(screen[4],"Soil: 1883"));r.water_low=true;displayResults(&oled,&r);assert(!strcmp(screen[6],"LOW WATER"));r.moist_valid=false;displayResults(&oled,&r);assert(!strcmp(screen[6],"SENSOR FAULT"));
  puts("PASS VER-05: normal, low-water and control-sensor fault display branches; soil shown as raw ADC");
 }else if(!strcmp(argv[1],"payload")){
  r.sampled_at_us=12345000;assert(formatTelemetry(&r,out,sizeof(out)));assert(strstr(out,"\"sampled_at_ms\":12345"));assert(strstr(out,"\"moisture_pct\":0.0"));assert(!formatTelemetry(&r,out,5));
  r.air_valid=r.light_valid=r.water_valid=r.moist_valid=false;assert(formatTelemetry(&r,out,sizeof(out)));assert(strstr(out,"\"temperature_c\":null")&&strstr(out,"\"humidity_pct\":null")&&strstr(out,"\"water_low\":null")&&strstr(out,"\"light_pct\":null")&&strstr(out,"\"moisture_pct\":null"));
  assert(sendWateringEvent(NULL)==ESP_ERR_INVALID_ARG);puts("PASS VER-06: telemetry values, nulls, timestamp, buffer bounds and event argument guard");
 }else if(!strcmp(argv[1],"main")||!strcmp(argv[1],"queue_failure")){
  if(!strcmp(argv[1],"queue_failure"))queue_error=-1;
  if(!setjmp(end_loop))app_main();
  assert(gpio_on_count==1&&!pump_on&&queue_count==2&&telemetry_count==1);assert(displays_while_on==0);assert(strstr(last_telemetry,"\"pump_on\":false"));
  printf("%s: one full main-loop cycle, no OLED write while pump ON, telemetry sent after OFF; queue failures=%s\n",queue_error?"PASS VER-10 local control / GAP VER-05":"GAP VER-05/08",queue_error?"injected":"none");
 }else abort();
 return 0;
}
'''
cases=['conversion','reservoir','timing','late_check','startup','wet','low','fault','rail','display','payload','main','queue_failure']
lines=['Firmware host execution — 26 October 2026', 'Source SHA256 '+hashlib.sha256(source.read_bytes()).hexdigest(), 'Hardware, clock and MQTT calls replaced by controlled adapters; actual application function bodies retained.']
with tempfile.TemporaryDirectory(prefix='greenhouse-host-') as d:
    c=Path(d)/'test.c'; exe=Path(d)/'test'; c.write_text(stubs+code+tests)
    subprocess.run(['gcc','-std=c11','-O0','-g',str(c),'-lm','-o',str(exe)],check=True)
    for case in cases:
        r=subprocess.run([str(exe),case],capture_output=True,text=True)
        lines.append(r.stdout.strip())
        if r.returncode:raise RuntimeError(case+': '+r.stderr)
lines.append(f'{len(cases)}/{len(cases)} scenario assertions completed; GAP/LIMIT observations are not requirement passes.')
(HERE/'firmware-results.txt').write_text('\n'.join(lines)+'\n')
print('\n'.join(lines))
