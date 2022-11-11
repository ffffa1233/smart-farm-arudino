
/* ********************
 * Include
 * ********************/
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <DHT_U.h>
#include <MsTimer2.h>


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

//시간
#define TIME_1SEC 1000
#define TIME_2SEC 2000
#define TIME_5SEC 5000
#define TIME_30SEC 30000
#define TIME_1MIN 60000
#define TIME_2MIN 120000
#define TIME_10MIN 600000
#define TIME_1HOUR 3600000
#define TIME_10HOUR 36000000
#define TIME_12HOUR 43200000
#define TIME_14HOUR 50400000

//Timer define
#define MSTIMTER_LED_ON_TIME TIME_10HOUR
#define MSTIMER_LED_ON_START 0
#define MSTIMER_LED_ON_RUNNING 1
#define MSTIMER_LED_ON_STOP 2
#define MSTIMTER_LED_OFF_TIME TIME_14HOUR
#define MSTIMER_LED_OFF_START 3
#define MSTIMER_LED_OFF_RUNNING 4
#define MSTIMER_LED_OFF_STOP 5

//Water Pump value
#define WATER_PUMP_OFF_VAL 0
#define WATER_PUMP_START_VAL 180
#define WATER_PUMP_MAX_VAL 255
#define WATER_PUMP_STOP 0
#define WATER_PUMP_STOP_TIMER_START 1
#define WATER_PUMP_START 2
#define WATER_PUMP_RUNNING 3


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
int LedOffFlag = MSTIMER_LED_ON_START;
unsigned long LedTimeRemained = 0;

/* ********************
 * Custom Function
 * ********************/
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
  //MsTimer2::set(TIME_10HOUR, LedBlink);
  //MsTimer2::start();
}

void LedBlink(){
  switch(LedOffFlag)
  {
    case MSTIMER_LED_ON_RUNNING:
      LedOffFlag = MSTIMER_LED_ON_STOP;
      MsTimer2::stop();
    break;
    case MSTIMER_LED_OFF_RUNNING:
      LedOffFlag = MSTIMER_LED_OFF_STOP;
      MsTimer2::stop();
    break;
    default:
      /* do nothing */
    break;
  }
}

void LedOnOffMainfunction() {
  switch(LedOffFlag)
  {
    case MSTIMER_LED_ON_START:
      MsTimer2::set(MSTIMTER_LED_ON_TIME, LedBlink);
      MsTimer2::start();
      LedOffFlag = MSTIMER_LED_ON_RUNNING;
      LedTimeRemained = MSTIMTER_LED_ON_TIME + millis();
    break;
    
    case MSTIMER_LED_ON_RUNNING:
       digitalWrite(LED_PIN, 0);
    break;
    
    case MSTIMER_LED_ON_STOP:
      LedOffFlag = MSTIMER_LED_OFF_START;
    break;
    
    case MSTIMER_LED_OFF_START:
      MsTimer2::set(MSTIMTER_LED_OFF_TIME, LedBlink);
      MsTimer2::start();
      LedOffFlag = MSTIMER_LED_OFF_RUNNING;
      LedTimeRemained = MSTIMTER_LED_OFF_TIME + millis();
    break;
    
    case MSTIMER_LED_OFF_RUNNING:
      digitalWrite(LED_PIN, 1);
    break;
    
    case MSTIMER_LED_OFF_STOP:
      LedOffFlag = MSTIMER_LED_ON_START;
    break;
    
    default:
      /* do nothing */
    break;
  }  
}

void WaterPumpMainfunction(){
  static unsigned long tenMinuteTimer;
  static unsigned long pumpFlag = 0;
  
  //하루에 한번 10분, 즉 불 켜진 직후 10분
  switch(pumpFlag)
  {
    case WATER_PUMP_STOP:
      analogWrite(MOTOR_PWM_PIN, WATER_PUMP_OFF_VAL);
      if(MSTIMER_LED_ON_RUNNING == LedOffFlag){
        pumpFlag = WATER_PUMP_STOP_TIMER_START;
      }
    break;
    
    case WATER_PUMP_STOP_TIMER_START:
      analogWrite(MOTOR_PWM_PIN, WATER_PUMP_OFF_VAL);
      tenMinuteTimer = millis() + TIME_10MIN;
      pumpFlag = WATER_PUMP_START;
    break;
    
    case WATER_PUMP_START:
      analogWrite(MOTOR_PWM_PIN, WATER_PUMP_START_VAL);
      if(tenMinuteTimer > millis())
      {
        pumpFlag = WATER_PUMP_RUNNING;
      }
      else
      {
        pumpFlag = WATER_PUMP_STOP;
      }
    break;
    
    case WATER_PUMP_RUNNING:
      analogWrite(MOTOR_PWM_PIN, WATER_PUMP_MAX_VAL);
      if(MSTIMER_LED_ON_RUNNING != LedOffFlag)
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
  unsigned long now = millis();

  if(0 == (now % TIME_1SEC))
  {
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
     lcd.print((LedTimeRemained - now)/TIME_1MIN);//시간 분단위
     lcd.print("m");
     lcd.print(((LedTimeRemained - now)%TIME_1MIN)/TIME_1SEC); //시간 초단위
     lcd.print("s R");
     
     lcd.setCursor(0,1);
     lcd.print("H:");
     lcd.print(humidity);
     lcd.print("%,T:");
     lcd.print(temperature);
     lcd.print("C");
  }
  
  LedOnOffMainfunction();
  WaterPumpMainfunction();
  
}
