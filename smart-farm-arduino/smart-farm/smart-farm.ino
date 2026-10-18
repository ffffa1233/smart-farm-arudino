
/* ********************
 * Include
 * ********************/
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <DHT_U.h>


/* ********************
 * Define
 * ********************/
//LCD 설정
#define LCD_ADDR 0x27
#define LCD_ROW 16
#define LCD_COLUMN 2
//pin 설정
#define SENSOR_SOIL_HUMID_PIN A1
#define SENSOR_TEMP_HUMID_PIN A0
#define LED_PIN 2
#define MOTOR_PWM_PIN 6
#define MOTOR_GPIO_PIN 7
#define HEAT_PAD_PIN 8

//키우는 식물 종류
#define PLANT_MODE_LETTUCE 0   //상추
#define PLANT_MODE_UNDEFINED 1 //미정

//동작 모드
#define SMART_FARM_MODE_0_DEFAULT 0   //아래 모드를 선택하도록 화면 보여줌
#define SMART_FARM_MODE_1_PLANT_SET 1 //식물 종류 고르는 모드 
#define SMART_FARM_MODE_2_TIME_SET 2  //시간 조정 모드
#define SMART_FARM_MODE_UNDEFINED 3   // 미정

//시간 (초 단위)
#define TIME_1SEC 1UL
#define TIME_1MIN 60UL
#define TIME_10MIN 600UL
#define TIME_1HOUR 3600UL
#define TIME_10HOUR 36000UL
#define TIME_12HOUR 43200UL
#define TIME_14HOUR 50400UL

//Timer1 1초 주기 설정 (CTC 모드, prescaler 256)
//16MHz 보드 기준 62500 카운트 = 1초. 보드 클럭이 빠르거나 느리면 이 값을 조정해서 보정
//(1 카운트 = 약 16ppm = 하루 약 1.4초)
#define TIMER1_PRESCALER 256UL
#define TIMER1_COMPARE_1SEC ((F_CPU / TIMER1_PRESCALER) - 1)

//LED 타이머
#define LED_ON_TIME TIME_10HOUR
#define LED_OFF_TIME TIME_14HOUR
#define LED_ON_START 0
#define LED_ON_RUNNING 1
#define LED_OFF_START 2
#define LED_OFF_RUNNING 3

//Water Pump value
#define WATER_PUMP_OFF_VAL 0
#define WATER_PUMP_START_VAL 180
#define WATER_PUMP_MAX_VAL 255
#define WATER_PUMP_STOP 0
#define WATER_PUMP_STOP_TIMER_START 1
#define WATER_PUMP_START 2
#define WATER_PUMP_RUNNING 3
#define WATER_PUMP_DONE 4
#define WATER_PUMP_RUN_TIME TIME_10MIN


/* ********************
 * Sensor Instance
 * ********************/
DHT dht(SENSOR_TEMP_HUMID_PIN, DHT11);
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_ROW, LCD_COLUMN);

/* ********************
 * Global variables
 * ********************/
int soilHumidity;
float humidity, temperature;
int LedOffFlag = LED_ON_START;
unsigned long LedPhaseEndSec = 0; //현재 LED 구간(ON/OFF)이 끝나는 시각 (초)
volatile unsigned long SecondsCount = 0; //Timer1 인터럽트로 1초마다 증가

/* ********************
 * Custom Function
 * ********************/
//Timer1 하드웨어 타이머로 1초마다 인터럽트 발생
//카운터는 하드웨어에서 계속 돌기 때문에 DHT 읽기처럼 인터럽트가 잠시 막혀도 초가 누락되지 않음
void InitSecondTimer(void) {
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = (1 << WGM12) | (1 << CS12); //CTC 모드, prescaler 256
  TCNT1 = 0;
  OCR1A = TIMER1_COMPARE_1SEC;
  TIMSK1 = (1 << OCIE1A);
  interrupts();
}

ISR(TIMER1_COMPA_vect) {
  SecondsCount++;
}

unsigned long GetSeconds(void) {
  unsigned long sec;
  noInterrupts(); //4바이트 값을 읽는 도중 인터럽트로 값이 바뀌지 않도록 보호
  sec = SecondsCount;
  interrupts();
  return sec;
}

//deadline 시각이 지났는지 확인 (뺄셈 비교라 오버플로에도 안전)
bool IsTimeReached(unsigned long now, unsigned long deadline) {
  return (long)(now - deadline) >= 0;
}

void InitLcd(void) {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Welcome YJ's =");
  lcd.setCursor(0,1);
  lcd.print("SMART Farm!!!");
}

void InitPin(void){
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTOR_PWM_PIN, OUTPUT); 
  pinMode(MOTOR_GPIO_PIN, OUTPUT); 
  pinMode(HEAT_PAD_PIN, OUTPUT);
  digitalWrite(LED_PIN, 0);
  analogWrite(MOTOR_PWM_PIN, 0); 
  digitalWrite(MOTOR_GPIO_PIN, 0); 
  digitalWrite(HEAT_PAD_PIN, 0);
}

void CalcSoilHumidity() {
  soilHumidity = map(analogRead(SENSOR_SOIL_HUMID_PIN), 1000, 400, 0, 100);
  if (soilHumidity > 100) soilHumidity = 100;
  else if (soilHumidity < 0) soilHumidity = 0;
}

void ReadTempAndHumid(void) {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
}

/* ********************
 * Arduino function
 * ********************/
void setup() {
  InitLcd();
  InitPin();
  Serial.begin(9600);
  dht.begin();
  InitSecondTimer();
}

void LedOnOffMainfunction(unsigned long now) {
  switch(LedOffFlag)
  {
    //다음 구간의 끝 시각은 이전 구간의 끝 시각에서 더함 -> loop 지연이 누적되지 않음
    case LED_ON_START:
      LedPhaseEndSec += LED_ON_TIME;
      LedOffFlag = LED_ON_RUNNING;
    break;
    
    case LED_ON_RUNNING:
      digitalWrite(LED_PIN, 0);
      if(IsTimeReached(now, LedPhaseEndSec))
      {
        LedOffFlag = LED_OFF_START;
      }
    break;
    
    case LED_OFF_START:
      LedPhaseEndSec += LED_OFF_TIME;
      LedOffFlag = LED_OFF_RUNNING;
    break;
    
    case LED_OFF_RUNNING:
      digitalWrite(LED_PIN, 1);
      if(IsTimeReached(now, LedPhaseEndSec))
      {
        LedOffFlag = LED_ON_START;
      }
    break;
    
    default:
      /* do nothing */
    break;
  }  
}

void WaterPumpMainfunction(unsigned long now){
  static unsigned long pumpEndSec;
  static int pumpFlag = WATER_PUMP_STOP;
  
  //하루에 한번 10분, 즉 불 켜진 직후 10분
  switch(pumpFlag)
  {
    case WATER_PUMP_STOP:
      analogWrite(MOTOR_PWM_PIN, WATER_PUMP_OFF_VAL);
      if(LED_ON_RUNNING == LedOffFlag){
        pumpFlag = WATER_PUMP_STOP_TIMER_START;
      }
    break;
    
    case WATER_PUMP_STOP_TIMER_START:
      analogWrite(MOTOR_PWM_PIN, WATER_PUMP_OFF_VAL);
      pumpEndSec = now + WATER_PUMP_RUN_TIME;
      pumpFlag = WATER_PUMP_START;
    break;
    
    case WATER_PUMP_START:
      analogWrite(MOTOR_PWM_PIN, WATER_PUMP_START_VAL);
      pumpFlag = WATER_PUMP_RUNNING;
    break;
    
    case WATER_PUMP_RUNNING:
      analogWrite(MOTOR_PWM_PIN, WATER_PUMP_MAX_VAL);
      if(IsTimeReached(now, pumpEndSec))
      {
        pumpFlag = WATER_PUMP_DONE; //10분 지나면 정지
      }
      else if(LED_ON_RUNNING != LedOffFlag)
      {
        pumpFlag = WATER_PUMP_STOP;
      }
    break;
    
    case WATER_PUMP_DONE:
      //LED가 꺼질 때까지 대기 -> 다음 LED ON 구간에 다시 10분 동작
      analogWrite(MOTOR_PWM_PIN, WATER_PUMP_OFF_VAL);
      if(LED_ON_RUNNING != LedOffFlag)
      {
        pumpFlag = WATER_PUMP_STOP;
      }
    break;
    
    default:
      /* do nothing */
    break;
  }
}

void loop() {
  static unsigned long lastDisplaySec = 0xFFFFFFFFUL;
  unsigned long now = GetSeconds();

  //초가 바뀔 때마다 한 번씩 갱신 (millis() % 1000 == 0 은 루프가 늦으면 건너뛰어짐)
  if(now != lastDisplaySec)
  {
     unsigned long remainSec = LedPhaseEndSec - now;
     lastDisplaySec = now;

     //토양습도 계산
     CalcSoilHumidity();
    
     //외부 온도, 습도 읽기
     ReadTempAndHumid();
    
     //LCD에 출력
     lcd.clear();
     lcd.setCursor(0,0);
     lcd.print("SH:");
     lcd.print(soilHumidity);
     lcd.print("%,");
     lcd.print(remainSec / TIME_1MIN);//시간 분단위
     lcd.print("m");
     lcd.print(remainSec % TIME_1MIN); //시간 초단위
     lcd.print("s R");
     
     lcd.setCursor(0,1);
     lcd.print("H:");
     lcd.print(humidity);
     lcd.print("%,T:");
     lcd.print(temperature);
     lcd.print("C");
  }
  
  LedOnOffMainfunction(now);
  WaterPumpMainfunction(now);
  
}
