/*
 * Automatic Key Dispenser Firmware
 * Author: Syed Saad Saif
 * Licence: Commercial 
 */

 /*
  * Possible improvements
  * Timer Resets everything
  */
  
  /*
   * Code porting
   * 1) Change the display (currently being displayed on serial monitior)
   * 2) Attach the servos  {Not connected at all}
   * 3) Attach the keypad  (Taken from serial input)
   */
#include "TimerOne.h"
#include <EEPROM.h>

#define NUM_KEYS   4
#define PIN_LENGTH 5
#define ALLOWED_PIN_ATTEMPTS 3
#define MENU_OPTIONS 2 //might be able to determine this automatically
#define EEPROM_ADDR  0
const char* menu_options[] = {"Reset Key", "Exit"}; 
char* master_pin =  "00000";
char* pins[] = {"11111", "22222", "33333", "44444"}; //should save to eeprom
char pin[PIN_LENGTH + 1];
char key;
int num_pin_attempts = 0;

void flush_serial();
void timer_action();
char get_menu_option();
void get_pin(char* pin);
char get_key();
void generate_pin(char* pin, char* pins[]);
char check_key_pin(char key, char* pins[]);
void save_pins(char* pins[]); //save and get from eeprom
void get_pins(char* pins[]);
void print_pins(char* pins[]);

void start_timer();
void setup() 
{
  Serial.begin(9600); 
  randomSeed(analogRead(0)); //an obvious exploit, Be careful with this. 
  //save_pins(pins);         //this code can be uncommented to initialise the eeprom
  //get_pins(pins);
}
void loop() 
{
  while(Serial.available() > 0) // Input related code
  {
    switch(Serial.read())       //Input related code
    {
      flush_serial();
      case '*': //enter key
        Serial.println("Enter Pin.");
        
        for(num_pin_attempts = 0; num_pin_attempts < ALLOWED_PIN_ATTEMPTS; num_pin_attempts++)
        {
        get_pin(pin);         //function takes input
        key = check_key_pin(pin,pins);
          if(key == -1)
          {
            if(num_pin_attempts < ALLOWED_PIN_ATTEMPTS - 1) //don't print this for the last incorrect entry
            {
              Serial.println("Pin incorrect!");
              delay(1000);
              Serial.println("Renter Pin!");
            }                
          }
          else
          {
            break;
          }
        }
        if(num_pin_attempts >= ALLOWED_PIN_ATTEMPTS)
        {
          Serial.println("Three incorrect attempts!");
          delay(10*1000);
          break; //break from switch when three incorrect pins are entered
        }

        Serial.print("Opening key: ");
        Serial.println((int)key + 1);
        //^^delay
        //^^clear screen
        break;
     
      case '#': //menu
          Serial.println("Enter Master Password.");

          for(num_pin_attempts = 0; num_pin_attempts < ALLOWED_PIN_ATTEMPTS; num_pin_attempts++)
          {
            get_pin(pin);          //function takes input
            if(strcmp(master_pin,pin) != 0)
            {
              if(num_pin_attempts < ALLOWED_PIN_ATTEMPTS - 1)
              {
                Serial.println("Master Password incorrect!");
                delay(1000);
                Serial.println("Renter Password!");  
              }              
            }
            else
            {
              break;
            }
          }
          if(num_pin_attempts >= ALLOWED_PIN_ATTEMPTS)
          {
            Serial.println("Three incorrect attempts!");
            delay(10*1000);
            break;
          }
          
          switch(get_menu_option())    //function takes input
          {
            case 0: //Get Key Selection
              Serial.println("Enter the key to reset.");
              key = get_key();        //function takes input
              if(key >0 && key <= NUM_KEYS)
              {
                key--;
                generate_pin(key, pins);
                Serial.print("New pin: ");
                Serial.println(pins[key]);
                //delay for a few seconds to let the admin not the key
              }
              else
              {
                Serial.println("Invalid key number.");
              }
              break;
            case 1: //Exit (Do Nothing)
              break;
          }
          //^^clear the screen before returning to default state
        break;
      default:
        break;
    }
  }
}
char get_menu_option()  //function takes input
{
  bool shown = false;
  unsigned char option = 0;
  char user_input;
  Serial.println("MENU");
  delay(1000);
  Serial.println("USE 1 and 3 to navigate");
  delay(1000);
  Serial.println("* Star to confirm option");
  delay(1000);

  while(1)
  {
    if(!shown)
    {
      Serial.println(menu_options[option]);
      shown = true;
    }
    while(Serial.available() > 0)
    {
      user_input = Serial.read();
      switch(user_input)
      {
        case '1':
          option = option == 0 ? MENU_OPTIONS - 1 : (option - 1);
          shown = false;
          break;
        case '3':
          option = (option + 1)%MENU_OPTIONS;
          shown = false;
        break;
        case '*':
        return option;
      }
      flush_serial();
    }
  }
}
void get_pin(char* pin) //function takes input
{
  int pin_index = 0;
  while(1)
  {
    while(Serial.available() > 0 && (pin_index + 1) <= PIN_LENGTH)
    {
      pin[pin_index] = Serial.read();
      pin_index++;
      print_pin(pin, pin_index);
      //start_timer();
    }
    if(pin_index >= PIN_LENGTH)
    {
      flush_serial();
      pin[pin_index] = '\0';
      return;
    }
  }
}
char get_key()  //function takes input
{
  char key = 0;
  while(1)
  {
    while(Serial.available() > 0)
    {
      key = Serial.read();
      key = key - 48;
      flush_serial();
      return key;
    }
  }
}
void flush_serial()  //code related with input
{
  while(Serial.available() > 0) //Empty the imput buffer
  {
    Serial.read();
  }
}
char check_key_pin(char* pin, char** pins)
{
  get_pins(pins);
  char key = -1;

  for(int i = 0; i < NUM_KEYS; i++)
  {
    if(strcmp(pins[i],pin) == 0)
    {
      key = i;
      break;
    }
  }
  return key;
}

void start_timer()
{
  Timer1.detachInterrupt();
  Timer1.stop();
  Timer1.initialize(5*1000000); // set a timer of length 100000 microseconds (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
  Timer1.attachInterrupt( timer_action ); // attach the service routine here
}
void timer_action()
{
  Serial.println("Everything is reset!");
  Timer1.stop();
}
void print_pin(char* pin, char length)
{
  for(int i = 0; i < length ; i++)
  {
    Serial.print(pin[i]);
  }
  Serial.println();
}
void generate_pin(char key, char* pins[]) //Have to make sure that there are no key collisions.
{
  char i;
  char* pin = "AAAAA";
  get_pins(pins);
  bool key_collision = false;
  do
  {
    for(i = 0; i < PIN_LENGTH; i++)
    {
      pin[i] = random(0,9) + 48;
    }
    for(i = 0; i<NUM_KEYS; i++)
    {
      if(!strcmp(pin, pins[i]))
      {
        key_collision = true;
        break;
      }
    }
  }
  while(key_collision == true);
  strcpy(pins[key],pin);
  save_pins(pins);
}
void save_pins(char* pins[])
{
  char i, j;
  for(i = 0; i<NUM_KEYS; i++)
  {
    for(j = 0; j<PIN_LENGTH; j++)
    {
      EEPROM.update(EEPROM_ADDR + PIN_LENGTH*i + j, pins[i][j]); //save memory cycles
    }
  }
}
void get_pins(char* pins[])
{
  char i, j;
  for(i = 0; i<NUM_KEYS; i++)
  {
    for(j = 0; j<PIN_LENGTH; j++)
    {
      pins[i][j] = EEPROM.read(EEPROM_ADDR + PIN_LENGTH*i + j);
    }
  }
  //print_pins(pins); //debug
}

void print_pins(char* pins[])
{
  char i;
  Serial.println("TEST:::PINS:");
  for(i = 0; i<NUM_KEYS; i++)
  {
    Serial.println(pins[i]);
  }
  Serial.println("***********");
}
