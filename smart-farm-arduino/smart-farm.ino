
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
#define TIME_5SEC 5000
#define TIME_1MIN 60000
#define TIME_1HOUR 3600000
#define TIME_12HOUR 43200000

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
int LedOffFlag = 0;

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
  MsTimer2::set(TIME_12HOUR, LedBlink);
  MsTimer2::start();
}

void LedBlink(){
  Serial.println("ledsadfadsf3,,,");
  if(LedOffFlag == 0){
  Serial.println("led3,,,");
    digitalWrite(LED_PIN, 1); //LED OFF
    LedOffFlag = 1;
    MsTimer2::stop();
  }
  else if(LedOffFlag == 1){ 
  Serial.println("led4,,,");   
    digitalWrite(LED_PIN, 0);//LED ON
    LedOffFlag = 0;
  }
  else if(LedOffFlag == 2){ 
  Serial.println("led5,,,");
  digitalWrite(LED_PIN, 0);//LED ON
    LedOffFlag = 0;
  }
}

void loop() {
  unsigned long now = millis();

  if(0 == (now % TIME_5SEC))
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
     lcd.print("%");
     lcd.setCursor(0,1);
     lcd.print("H:");
     lcd.print(humidity);
     lcd.print("%,T:");
     lcd.print(temperature);
     lcd.print("C");
  }
  if(1 == LedOffFlag)
  {
    Serial.println("led6,,,");
    MsTimer2::start();
    LedOffFlag = 2;
  }

 
}
