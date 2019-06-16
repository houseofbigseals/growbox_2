// sketch for simple grow machine
#include <Wire.h>
#include <EEPROM.h>
#include "DHT.h"
#include <LiquidCrystal_I2C.h>

class day_timer
{
  public:
    unsigned long current_time;//seconds
    unsigned long fan_sleep_time;
    unsigned long fan_work_time;
    unsigned long pump_sleep_time;
    unsigned long pump_work_time;
    unsigned long led_sleep_time;
    unsigned long led_work_time;
    void update();
    void set_time(unsigned long new_time);
    day_timer(byte mode, unsigned long time);
    boolean get_fan_state();
    boolean get_pump_state();
    boolean get_led_state();
};

day_timer::day_timer(byte mode, unsigned long new_time)
{
  current_time = new_time;//in seconds
  switch(mode)
  {
    case 0:
      //nothing works
      fan_sleep_time = 86400;//24*60*60 - seconds
      fan_work_time = 0;
      pump_sleep_time = 86400;
      pump_work_time = 0;
      led_sleep_time = 86400;
      led_work_time = 0;
      break;
    case 1:
      //stable mode with constant parameters 18 hours of lite
      fan_sleep_time = 1200;//20*60 - seconds
      fan_work_time = 240;//4*60 sec
      pump_sleep_time = 3450;//57*60 
      pump_work_time = 150;//3*60
      led_sleep_time = 21600;//6*60*60
      led_work_time = 64800;//18*60*60
      break;
    case 2:
      //stable mode with constant parameters 14 hours of lite
      fan_sleep_time = 1200;//20*60 - seconds
      fan_work_time = 240;//4*60 sec
      pump_sleep_time = 3450;//57*60 
      pump_work_time = 150;//3*60
      led_sleep_time = 36000;//10*60*60
      led_work_time = 50400;//14*60*60
      break;
    case 3:
      //stable mode with constant parameters 23 hours of lite
      fan_sleep_time = 1200;//20*60 - seconds
      fan_work_time = 240;//4*60 sec
      pump_sleep_time = 3450;//57*60 
      pump_work_time = 150;//3*60
      led_sleep_time = 3600;//1*60*60
      led_work_time = 82800;//23*60*60
      break;
    default:
      //stable mode with constant parameters 14 hours of lite
      fan_sleep_time = 1200;//20*60 - seconds
      fan_work_time = 240;//4*60 sec
      pump_sleep_time = 3450;//57*60 
      pump_work_time = 150;//2.5*60
      led_sleep_time = 36000;//10*60*60
      led_work_time = 50400;//14*60*60
      break;
  }
}

void day_timer::update()
{
  current_time++;
}

boolean day_timer::get_fan_state()
{
  unsigned long local = current_time%(fan_work_time + fan_sleep_time);
  if(local <= fan_work_time)
  {
    return true;
  }
  else
  {
    return false;
  }
}
boolean day_timer::get_pump_state()
{
  unsigned long local = current_time%(pump_work_time + pump_sleep_time);
  if(local <= pump_work_time)
  {
    return true;
  }
  else
  {
    return false;
  }
}
boolean day_timer::get_led_state()
{
  unsigned long local = current_time%(led_work_time + led_sleep_time);
  if(local <= led_work_time)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void day_timer::set_time(unsigned long new_time)
{
  current_time = new_time;
}


class shedule
{
  public:
    unsigned long all_time;//in sec
    unsigned long current_sec;
    unsigned long current_min;
    unsigned long current_hour;
    unsigned long current_day;

    int length_of_cycle;
    byte *day_modes;
    day_timer *current_timer;
    
    byte inscribed;
    void write_EEPROM();
    unsigned long read_EEPROM();
    void clear_EEPROM();
    void update();
    void set_time(unsigned long new_time);
    shedule();
};

shedule::shedule()
{
  inscribed = 100;
  length_of_cycle = 100; // make it simple
  //check EEPROM, if that just a reload after power off
  int addr = 0;
  Serial.println("Start of shedule");
  byte written = EEPROM.read(addr);
  Serial.println(written);
  if(written == inscribed)
  {
    //that means, that we already have data in EEPROM, so read it
    all_time = read_EEPROM();
    Serial.println(all_time);
    current_day = byte(all_time/(84600));
    Serial.println(current_day);
    current_hour = byte((all_time - (current_day*(84600)))/3600);
    Serial.println(current_hour);
    current_min = byte((all_time - (current_day*(84600)) - (current_hour*3600))/60);
    Serial.println(current_min);
    current_sec = byte((all_time - (current_day*(84600)) - (current_hour*3600) - (current_min*60))); 
    Serial.println(current_sec);
  }
  else
  {
    //init start time as zeros
    all_time = 0;
    current_sec = 0;
    current_min = 0;
    current_hour = 0;
    current_day = 0;
  }
  // allocate memory to the standart array of modes for all days
  day_modes = new byte[length_of_cycle];
  //fill it with standart shedule: first 15 days mode 1,  then mode 2
  for(int i=0; i<(length_of_cycle*0.75); i++)
  {
    day_modes[i] = 3;
  }
  for(int i=(length_of_cycle*0.75); i<length_of_cycle; i++)
  {
    day_modes[i] = 1;
  }
  //create first day_timer object
  unsigned long curr_day_sec = all_time - (current_day*(84600));
  current_timer = new day_timer(day_modes[current_day], curr_day_sec);
  Serial.println("End of making shedule");
}

void shedule::update()
{
  //we call this function from main if one second passed
  all_time++;
  current_timer->update();
  byte old_day = current_day;
  current_day = (all_time/(84600));
  current_hour = ((all_time - (current_day*(84600)))/3600);
  current_min = ((all_time - (current_day*(84600)) - (current_hour*3600))/60);
  current_sec = ((all_time - (current_day*(84600)) - (current_hour*3600) - (current_min*60)));
  if(old_day!=current_day)
  {
    delete current_timer;
    unsigned long curr_day_sec = all_time - (current_day*(84600));
    current_timer = new day_timer(day_modes[current_day], curr_day_sec);
  }
  if((all_time%(21600))==0)
  {
    //make respawn point for case if happens suddenly power off
    write_EEPROM();
  }
  
}

void shedule::write_EEPROM()
{
  //save current aal_time to EEPROM and write "inscribed" to first byte
  int addr = 0;
  EEPROM.update(addr, inscribed);
  addr += sizeof(inscribed);
  EEPROM.put(addr, all_time);
}


unsigned long shedule::read_EEPROM()
{
  int addr = 0;
  //EEPROM.update(addr, inscribed);
  addr += sizeof(inscribed);
  unsigned long new_time;
  EEPROM.get(addr, new_time);
  return new_time;
}

void shedule::clear_EEPROM()
{
  int addr = 0;
  EEPROM.update(addr, 0);
}

void shedule::set_time(unsigned long new_time)
{
  all_time = new_time;
  current_timer->set_time(new_time);
  update();
  
}

void show_dht(DHT dht, LiquidCrystal_I2C lcd)
{
  lcd.backlight();
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) 
  {
    lcd.clear(); 
    lcd.print("DHT error");
    return;
  }
  // Выводим показания влажности и температуры
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);              // Устанавливаем курсор в начало 1 строки
  lcd.print("Humidity =    % ");     // Выводим текст
  lcd.setCursor(11, 0); 
  lcd.print(h);
  lcd.setCursor(0, 1);              // Устанавливаем курсор в начало 2 строки
  lcd.print("Temp     =   C ");    // Выводим текст
  lcd.setCursor(11, 1);             
  lcd.print(t);  
}

const float grow_version = 0.01; 
// pins for loads
const int pump_pin = 9;      
const int led_pin = 10;
const int fan_pin = 11;
//pins for buttons
const int manual_but_pin = 3;
const int info_but_pin = 4;
const int led_but_pin = 5;
const int pump_but_pin = 6;
const int fan_but_pin = 7;
//pins for sensors
const int dht22_pin = 2;

shedule plan;
LiquidCrystal_I2C lcd(0x3F,16,2);
DHT dht(dht22_pin, DHT11);

boolean manual;

void setup()
{
  Serial.begin(9600);
  plan = shedule();
  Serial.println("Start of setup");
  lcd.init();
  Serial.println("LCD done");           
  lcd.noBacklight();
  dht.begin();
  Serial.println("DHT done");
  manual = false;
  // init digital pins
  pinMode(pump_pin, OUTPUT);
  pinMode(fan_pin, OUTPUT);
  pinMode(led_pin, OUTPUT);
  digitalWrite(pump_pin, HIGH);//cose it has inverted logic
  digitalWrite(fan_pin, HIGH);//cose it has inverted logic
  digitalWrite(led_pin, HIGH);//cose it has inverted logic
  
  pinMode(manual_but_pin, INPUT);
  pinMode(info_but_pin, INPUT);
  pinMode(led_but_pin, INPUT);
  pinMode(pump_but_pin, INPUT);
  pinMode(fan_but_pin, INPUT);
  
  pinMode(led_pin, OUTPUT);
  Serial.println("Pin settings done");
  Serial.println("End of setup \n"); 
}



void loop()
{

  unsigned long new_time = millis();
  // after 50 days of work millis() overflows to zero, but we dont care
  if(new_time%1000 == 0)
  {
    //lcd.noBacklight();
    plan.update();
    if(digitalRead(manual_but_pin)==HIGH)
    {
      manual = true;
      if(digitalRead(fan_but_pin)==HIGH)
        {digitalWrite(fan_pin, LOW);}
      else
        {digitalWrite(fan_pin, HIGH);}
      if(digitalRead(pump_but_pin)==HIGH)
        {digitalWrite(pump_pin, LOW);}
      else
        {digitalWrite(pump_pin, HIGH);}
      if(digitalRead(led_but_pin)==HIGH)
        {digitalWrite(led_pin, LOW);}
      else
        {digitalWrite(led_pin, HIGH);}
      lcd.backlight();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("manual mode");
    }
    else
    {
      manual = false;
      if(plan.current_timer->get_fan_state())
        {digitalWrite(fan_pin, LOW);}
      else
        {digitalWrite(fan_pin, HIGH);}
      if(plan.current_timer->get_pump_state())
        {digitalWrite(pump_pin, LOW);}
      else
        {digitalWrite(pump_pin, HIGH);}
      if(plan.current_timer->get_led_state())
        {digitalWrite(led_pin, LOW);}
      else
        {digitalWrite(led_pin, HIGH);}
      show_dht(dht, lcd);
    }
    if(digitalRead(info_but_pin)==HIGH)
    {
      //send current time to the LCD
      lcd.backlight();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("info mode");
      lcd.setCursor(1, 1);
      lcd.print(plan.current_day);
      lcd.setCursor(4, 1);
      lcd.print(":");
      lcd.setCursor(5, 1);
      lcd.print(plan.current_hour);
      lcd.setCursor(7, 1);
      lcd.print(":");
      lcd.setCursor(8, 1);
      lcd.print(plan.current_min);
      lcd.setCursor(10, 1);
      lcd.print(":");
      lcd.setCursor(11, 1);
      lcd.print(plan.current_sec);
      
      //send info to the Serial
      Serial.println("Sending info from growbox");
      Serial.print("Version of software is ");
      Serial.println(grow_version);
      Serial.print("Current time from start is ");
      Serial.println(plan.all_time);
      Serial.print("Current day from start is ");
      Serial.println(plan.current_day);
      Serial.print("Current time in that day is ");
      Serial.print(plan.current_hour);
      Serial.print(" : ");
      Serial.print(plan.current_min);
      Serial.print(" : ");
      Serial.println(plan.current_sec);
      Serial.print("Current cycle of growing is ");
      Serial.println(plan.length_of_cycle);
      Serial.println(" days");
      Serial.println("Current shedule of growing is ");
      for(int i = 0; i< plan.length_of_cycle; i++)
      {
        Serial.print("Day ");
        Serial.print(i);
        Serial.print(" : mode ");
        Serial.print(plan.day_modes[i]);
        if(i == plan.current_day)
        {
          Serial.print(" <---- we are here now ");
        }
        Serial.print(" \n");
      }
      Serial.print("\n");

      Serial.println(" 0 is stable mode with nothing works");
      Serial.println(" 1 is stable mode with constant parameters 18 hours of lite,");
      Serial.println(" pump works 2.5 mins every hour, fan works 4 mins every 20 mins");
      Serial.println(" 2 is stable mode with constant parameters 14 hours of lite");
      Serial.println(" pump works 2.5 mins every hour, fan works 4 mins every 20 mins");
      Serial.println(" 3 is stable mode with constant parameters 23 hours of lite");
      Serial.println(" pump works 2.5 mins every hour, fan works 4 mins every 20 mins");

      if(manual)
      {
        Serial.println("Current mode is manual");
      }
      else
      {
        Serial.print("Current mode is ");
        Serial.println(plan.day_modes[plan.current_day]);
        Serial.println("State of perifery");
        Serial.print("Pump ");
        Serial.println(plan.current_timer->get_pump_state());
        Serial.print("Fans ");
        Serial.println(plan.current_timer->get_fan_state());
        Serial.print("Led ");
        Serial.println(plan.current_timer->get_led_state());
      }
      Serial.println("State of sensors");
      Serial.println("DHT22");
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      if (isnan(h) || isnan(t)) 
      {
        Serial.println("DHT22 ERROR");
      }
      else
      {
        Serial.print("Humidity is ");
        Serial.println(h);
        Serial.print("Temperature is ");
        Serial.println(t);
      }
      Serial.print("Whats in EEPROM ");
      unsigned long data = plan.read_EEPROM();
      Serial.println(data) ;
      Serial.println("End of sending info from growbox \n");
    }
  }
}
