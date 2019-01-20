#include <Wire.h>
#include <Time.h>
#include <DS1307RTC.h>
#include <EEPROM.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

void check_commands();
void show_on_LCD(String S);
void do_reset_epoch();
void do_show_time();
void do_send_info();
void do_show_temp();
void print2digits(int number);
void check_shedule();

const float grow_version = 0.02; 

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
unsigned long previousMillis = 0;
const unsigned long interval = 100; //timer interval in ms

//flags for main loop
boolean show_time = false;
boolean show_temp = false;
boolean led = false;
boolean fans = false;
boolean cooler = false;
boolean pump = false;
boolean manual_mode = false;

const int relay_pin1 = 4; //12v dc pump here
const int relay_pin2 = 5; //LED Lamp cooler here
const int relay_pin3 = 6; //LED Lamp here
const int relay_pin4 = 7; //12v Fans here
const byte hardcoded_sign = 101;
int length_of_cycle = 50; 
tmElements_t tm;
byte *day_modes;
tmElements_t start_time;

LiquidCrystal_I2C lcd(0x3F,16,2);   // Задаем адрес и размерность дисплея
DHT dht(2, DHT11);

//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {8, 9, 10, 11}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {14, 15, 16, 17}; //connect to the column pinouts of the keypad

// Set list of commands, that we can get from matrix keyboard
const char SetOn1[5] = "A001";
const char SetOn2[5] = "A002";
const char SetOn3[5] = "A003";
const char SetOn4[5] = "A004";
const char SetOff1[5] = "B001";
const char SetOff2[5] = "B002";
const char SetOff3[5] = "B003";
const char SetOff4[5] = "B004";
const char SetOffAll[5] = "B100";
const char GetInfo[5] = "C001";
const char ShowTime[5] = "C002";
const char ShowTemp[5] = "C003";
const char Reset[5] = "D001";
// variable for current command
String command;

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

void setup()
{
  Serial.begin(9600);
  //Serial.println("Start of init");
  dht.begin();
  //Serial.println("Start of dht");
  lcd.init(); 
  //Serial.println("Start of lcd");           
  lcd.backlight();
  //check in what day we started growing cycle
  byte sign = 0;
  //read sign byte from 0 address
  EEPROM.get(0, sign);
  if(sign != hardcoded_sign)
  {
    //it means that we have problems
    //just rewrite it 
    do_reset_epoch(); //Potential cause of errors, if RTC doesnt work
  }
  else
  {
    //read start cycle time from 1 address
    EEPROM.get(1, start_time);
  }
  // create shedule for growing
  // in future we need to set this shedule from pc by serial commands
  // but now it is hardcoded
  // allocate memory to the standart array of modes for all days
  day_modes = new byte[length_of_cycle];
  //fill it with HARDCODED shedule
  for(int i=0; i<(length_of_cycle*0.75); i++)
  {
    day_modes[i] = 3;
  }
  for(int i=(length_of_cycle*0.75); i<length_of_cycle; i++)
  {
    day_modes[i] = 2;
  }

  pinMode(relay_pin1, OUTPUT);
  pinMode(relay_pin2, OUTPUT);
  pinMode(relay_pin3, OUTPUT);
  pinMode(relay_pin4, OUTPUT);
  digitalWrite(relay_pin1, HIGH);
  digitalWrite(relay_pin2, HIGH);
  digitalWrite(relay_pin3, HIGH);
  digitalWrite(relay_pin4, HIGH);

  command = String();
  Serial.println("End of init");
}
  
void loop()
{
  // it is the main loop
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    // we have to check flags
    if(show_time == true)
    {
      do_show_time();
    }
    if(show_temp == true)
    {
      do_show_temp();
    }
    if(!manual_mode)
    {
      if(led == true)
      {
        digitalWrite(relay_pin3, LOW);
      }
      else
      {
        digitalWrite(relay_pin3, HIGH);
      }
      if(pump == true)
      {
        digitalWrite(relay_pin1, LOW);
      }
      else
      {
        digitalWrite(relay_pin1, HIGH);
      }
      if(fans == true)
      {
        digitalWrite(relay_pin4, LOW);
      }
      else
      {
        digitalWrite(relay_pin4, HIGH);
      }
      if(cooler == true)
      {
        digitalWrite(relay_pin2, LOW);
      }
      else
      {
        digitalWrite(relay_pin2, HIGH);
      }
    }
    check_commands();
    check_shedule();
  }
}

void check_commands()
{
  char customKey = customKeypad.getKey();
  if (customKey)
  {
    show_time = false;
    show_temp = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("# - manual mode"); 
    lcd.setCursor(0, 1);
    lcd.print("0 - auto mode"); 
    
    Serial.println(customKey);
    if (customKey == '0')
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Auto mode!"); 
      lcd.setCursor(0, 1);
      lcd.print("# to manual"); 
      manual_mode = false;
    }
    if (customKey == '#')
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Command mode!"); 
      lcd.setCursor(0, 1);
      lcd.print("* to cancel"); 
      
      //Serial.println("we start to read the command!");
      //start read new command
      //clear old values in command string
      command = String();
      //go to read symbol loop while it not ends with * symbol
      while(true)
      {
        customKey = customKeypad.getKey();
        if(customKey)
        {
          if (customKey == '*')
          {
            //Serial.println("we got end of command");
            //Serial.print("now we have : ");
            //Serial.println(command);
            break;
          }
          else
          {
            command += customKey;
            //Serial.print("now we have : ");
            //Serial.println(command);
            show_on_LCD(command);
          }
        }
        //delay(10);
      }
      //After that loop we thinks that command was received and we need to know what command is it and do that command
        if(command == String(SetOn1))
        {
          //Serial.println("SetOn1 command was received!");
          //digitalWrite(relay_pin1, LOW);
          pump = true;
          manual_mode = true;
          return;
        }
        if(command == String(SetOn2))
        {
          //Serial.println("SetOn2 command was received!");
          //digitalWrite(relay_pin2, LOW);
          cooler = true;
          manual_mode = true;
          return;
        }
        if(command == String(SetOn3))
        {
          //Serial.println("SetOn3 command was received!");
          //digitalWrite(relay_pin3, LOW);
          led = true;
          manual_mode = true;
          return;
        }
        if(command == String(SetOn4))
        {
          //Serial.println("SetOn4 command was received!");
          //digitalWrite(relay_pin4, LOW);
          fans = true;
          manual_mode = true;
          return;
        }
        if(command == String(SetOff1))
        {
          //Serial.println("SetOff1 command was received!");
          //digitalWrite(relay_pin1, HIGH);
          cooler = false;
          manual_mode = true;
          return;
        }
        if(command == String(SetOff2))
        {
          //Serial.println("SetOff2 command was received!");
          //digitalWrite(relay_pin2, HIGH);
          cooler = false;
          manual_mode = true;
          return;
        }
        if(command == String(SetOff3))
        {
          //Serial.println("SetOff3 command was received!");
          //digitalWrite(relay_pin3, HIGH);
          led = false;
          manual_mode = true;
          return;
        }
        if(command == String(SetOff4))
        {
          //Serial.println("SetOff4 command was received!");
          fans = false;
          manual_mode = true;
          //digitalWrite(relay_pin4, HIGH);
          return;
        }
        if(command == String(SetOffAll))
        {
          //Serial.println("SetOffAll command was received!");
          //digitalWrite(relay_pin4, HIGH);
          led = false;
          fans = false;
          cooler = false;
          pump = false;
          manual_mode = true;
          return;
        }
        if(command == String(ShowTemp))
        {
          //Serial.println("ShowTemp command was received!");
          show_time = false;
          show_temp = true;
          lcd.clear();
          do_show_temp();
          return;
        }
        if(command == String(ShowTime))
        {
          //Serial.println("ShowTime command was received!");
          show_time = true;
          show_temp = false;
          lcd.clear();
          do_show_time();
          return;
        }
        if(command == String(GetInfo))
        {
          //Serial.println("GetInfo command was received!");
          do_send_info();
          return;
        }
        if(command == String(Reset))
        {
          //Serial.println("Reset command was received!");
          do_reset_epoch();
          return;
        }
        else
        {
          //Serial.println("That is not a command, i dont know what to do");
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Not a command"); 
          lcd.setCursor(0, 1);
          lcd.print("Press any key"); 
          return;
        }
        
    }
  }
}
void show_on_LCD(String S)
{
  //Serial.println("Start of showing on LCD!");
  //Serial.print("we got: ");
  //Serial.println(S);
  
  char charBuf[S.length()+1];
  S.toCharArray(charBuf, S.length()+1);
  //Serial.print("we do with it: ");
  //Serial.println(charBuf);
  lcd.clear(); 
  lcd.setCursor(0, 0);              // Устанавливаем курсор в начало 1 строки
  lcd.print(charBuf);     // Выводим текст
  //Serial.println("End of showing on LCD!");
}
void do_reset_epoch()
{
  //set current time as start time
  //and rewrite it into EEPROM
  if (RTC.read(tm)) 
  {

    lcd.setCursor(2, 0);
    lcd.print("Reboot!");
    EEPROM.put(0, hardcoded_sign);
    EEPROM.put(1, tm);
    //Serial.println("We just have rebooted our system");
    Serial.println("New cycle of growing starts now");
  }
  else
  {
    //Serial.println("We cant restart growing cycle");
    Serial.println("Problems with RTC");
    lcd.setCursor(0, 0);
    lcd.print("RTC problem");
    lcd.setCursor(1, 0);
    lcd.print("No reboot");
  }
}
void do_show_time()
{
  //Show time and date, received from RTC, on LCD
  

  if (RTC.read(tm)) 
  {
    /*// send to serial port
    Serial.print("Ok, Time = ");
    print2digits(tm.Hour);
    Serial.write(':');
    print2digits(tm.Minute);
    Serial.write(':');
    print2digits(tm.Second);
    Serial.print(", Date (D/M/Y) = ");
    Serial.print(tm.Day);
    Serial.write('/');
    Serial.print(tm.Month);
    Serial.write('/');
    Serial.print(tmYearToCalendar(tm.Year));
    Serial.println();
    */
    // show on LCD
    //lcd.clear(); 
    printLCD2digits(tm.Hour, 0, 0);
    lcd.setCursor(2, 0);
    lcd.print(":");
    printLCD2digits(tm.Minute, 3, 0);
    lcd.setCursor(5, 0);
    lcd.print(":");
    printLCD2digits(tm.Second, 6, 0);

    printLCD2digits(tm.Day, 0, 1);
    lcd.setCursor(2, 1);
    lcd.print(":");
    printLCD2digits(tm.Month, 3, 1);
    lcd.setCursor(5, 1);
    lcd.print(":");
    printLCD2digits(tmYearToCalendar(tm.Year), 6, 1);
    
  } 
  else 
  {
    //lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RTC Error!");
    
    if (RTC.chipPresent()) 
    {
      Serial.println("The DS1307 is stopped.  Please run the SetTime");
      //Serial.println("example to initialize the time and begin running.");
      //Serial.println();
    } 
    else 
    {
      Serial.println("DS1307 read error!  Please check the circuitry.");
      //Serial.println();
    }
  }
}

void print2digits(int number) 
{
  if (number >= 0 && number < 10) 
  {
    Serial.write('0');
  }
  Serial.print(number);
}

void printLCD2digits(int number, int pos, int str) 
{
  if (number >= 0 && number < 10) 
  {
    lcd.setCursor(pos, str);
    lcd.print("0");
    lcd.setCursor(pos+1, str);
    lcd.print(number);
  }
  else
  {
    lcd.setCursor(pos, str);
    lcd.print(number);
  }

}

void do_send_info()
{
  // Send all info about system to UART through Serial 
  //send info to the Serial
      //RTC.read(tm);
      int current_day = tm.Day - start_time.Day;
      Serial.println("Sending info from growbox");
      Serial.print("Version of software is ");
      Serial.println(grow_version);
      Serial.print("Current Time = ");
      print2digits(tm.Hour);
      Serial.write(':');
      print2digits(tm.Minute);
      Serial.write(':');
      print2digits(tm.Second);
      Serial.print(", Date (D/M/Y) = ");
      Serial.print(tm.Day);
      Serial.write('/');
      Serial.print(tm.Month);
      Serial.write('/');
      Serial.print(tmYearToCalendar(tm.Year));
      Serial.println();
      Serial.print("Start cycle time = ");
      print2digits(start_time.Hour);
      Serial.write(':');
      print2digits(start_time.Minute);
      Serial.write(':');
      print2digits(start_time.Second);
      Serial.print(", Date (D/M/Y) = ");
      Serial.print(start_time.Day);
      Serial.write('/');
      Serial.print(start_time.Month);
      Serial.write('/');
      Serial.print(tmYearToCalendar(start_time.Year));
      Serial.println();
      Serial.print("Current cycle of growing is ");
      Serial.print(length_of_cycle);
      Serial.println(" days");
      Serial.println("Current shedule of growing is ");
      for(int i = 0; i< length_of_cycle; i++)
      {
        Serial.print("Day ");
        Serial.print(i);
        Serial.print(" : mode ");
        Serial.print(day_modes[i]);
        if(i == current_day)
        {
          Serial.print(" <---- we are here now ");
        }
        Serial.print(" \n");
      }
      Serial.print("\n");
/*
      Serial.println(" 0 is stable mode with nothing works");
      Serial.println(" 1 is stable mode with constant parameters 14 hours of lite,");
      Serial.println(" pump works 3 mins every hour, fan works 5 mins every 20 mins");
      Serial.println(" 2 is stable mode with constant parameters 18 hours of lite");
      Serial.println(" pump works 3 mins every hour, fan works 5 mins every 20 mins");
      Serial.println(" 3 is stable mode with constant parameters 24 hours of lite");
      Serial.println(" pump works 3 mins every hour, fan works 5 mins every 20 mins");
*/
      if(manual_mode)
      {
        Serial.println("Current mode is manual_mode");
      }
      else
      {
        Serial.println("Current mode is auto");
      }
      Serial.println(day_modes[current_day]);
      Serial.println("State of perifery");
      Serial.print("Pump ");
      Serial.println(pump);
      Serial.print("Fans ");
      Serial.println(fans);
      Serial.print("Led ");
      Serial.println(led);
      Serial.print("Cooler ");
      Serial.println(cooler);
/*      Serial.println("State of sensors");
      Serial.println("DHT11");
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      if (isnan(h) || isnan(t)) 
      {
        Serial.println("DHT11 ERROR");
      }
      else
      {
        Serial.print("Humidity is ");
        Serial.println(h);
        Serial.print("Temperature is ");
        Serial.println(t);
      }*/
/*
      Serial.println("List of keyboard commands :");
      Serial.println("Set On Pump = A001");
      Serial.println("Set On Cooler = A002");
      Serial.println("Set On Lamp = A003");
      Serial.println("SetOn Fans = A004");
      Serial.println("SetOff Pump = B001");
      Serial.println("SetOff Cooler = B002");
      Serial.println("SetOff Lamp = B003");
      Serial.println("SetOff Fans = B004");
      Serial.println("SetOff All = B100");
      Serial.println("Get Info to Serial = C001");
      Serial.println("Show Time on LCD = C002");
      Serial.println("Show Temp on LCD = C003");
      Serial.println("Restart growing cycle = D001");
      Serial.println("End of sending info from growbox \n");
   */ 
}
void do_show_temp()
{
  // Show humidity and temperature, received from DHT11, on LCD
  
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) 
  {
    lcd.clear(); 
    lcd.print("Time out error of DHT11 module");
    return;
  }
  // Выводим показания влажности и температуры
  //lcd.clear();
  lcd.setCursor(0, 0);              // Устанавливаем курсор в начало 1 строки
  lcd.print("Humidity =    ");     // Выводим текст
  lcd.setCursor(11, 0); 
  lcd.print(h, 1);
  lcd.setCursor(0, 1);              // Устанавливаем курсор в начало 2 строки
  lcd.print("Temp     =    ");    // Выводим текст, \1 - значок градуса
  lcd.setCursor(11, 1);             
  lcd.print(t,1);  
  //delay(10);
}

void check_shedule()
{
  if(manual_mode == true)
  {
    return;
  }
  else{
  RTC.read(tm);
  int current_day = tm.Day - start_time.Day;
  switch(day_modes[current_day])
  {
      // 0 is stable mode with nothing works
      // 1 is stable mode with constant parameters 14 hours of lite,
      // pump works 2 mins every hour, fan works 5 mins every 20 mins.
      // 2 is stable mode with constant parameters 18 hours of lite
      // pump works 2 mins every hour, fan works 5 mins every 20 mins.
      // 3 is stable mode with constant parameters 24 hours of lite
      // pump works 2 mins every hour, fan works 5 mins every 20 mins.
    case 0:
      led = false;
      fans = false;
      cooler = false;
      pump = false;
      break;
    case 1:
      //check led
      if(tm.Hour<=14)
      {
        led = true;
        cooler = true;
      }
      else
      {
        led = false;
        cooler = false;
      }
      //check pump
      if(tm.Minute >= 58)
      {
        pump = true;
      }
      else
      {
        pump = false;
      }
      //check fans
       if(tm.Minute<=5 || (tm.Minute > 20 && tm.Minute < 25) || (tm.Minute > 40 && tm.Minute < 45))
      {
        fans = true;
      }
      else
      {
        fans = false;
      }
    break;
    case 2:
      //check led
      if(tm.Hour<=18)
      {
        led = true;
        cooler = true;
      }
      else
      {
        led = false;
        cooler = false;
      }
      //check pump
      if(tm.Minute >= 58)
      {
        pump = true;
      }
      else
      {
        pump = false;
      }
      //check fans
       if(tm.Minute<=5 || (tm.Minute > 20 && tm.Minute < 25) || (tm.Minute > 40 && tm.Minute < 45))
      {
        fans = true;
      }
      else
      {
        fans = false;
      }
    break;
    case 3:
          //check led
      if(tm.Hour<=24)
      {
        led = true;
        cooler = true;
      }
      else
      {
        led = false;
        cooler = false;
      }
      //check pump
      if(tm.Minute >= 58)
      {
        pump = true;
      }
      else
      {
        pump = false;
      }
      //check fans
       if(tm.Minute<=5 || (tm.Minute >= 20 && tm.Minute < 25) || (tm.Minute >= 40 && tm.Minute < 45))
      {
        fans = true;
      }
      else
      {
        fans = false;
      }
    break;
    }
  }
}

