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
#include <LiquidCrystal.h>
#include <EEPROM.h>
#define DELAY_TIME 100
#define NUM_KEYS   4
#define PIN_LENGTH 5
#define ALLOWED_PIN_ATTEMPTS 3
#define MENU_OPTIONS 2 //might be able to determine this automatically
#define EEPROM_ADDR  0

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

const char* menu_options[] = {"RESET KEY", "EXIT"}; 
char* master_pin =  "00000";
char* pins[] = {"11111", "22222", "33333", "44444"}; //should save to eeprom
char pin[PIN_LENGTH + 1];
char key;
int num_pin_attempts = 0;

void flush_serial();
char get_menu_option();
void get_pin(char* pin);
char get_key();
void generate_pin(char key, char* pins[]);
char check_key_pin(char key, char* pins[]);
void save_pins(char* pins[]); //save and get from eeprom
void get_pins(char* pins[]);
void print_pins(char* pins[]);

//lcd display functions
void lcd_print_enter_pin()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PRESS * TO ENTER");
  lcd.setCursor(0,1);
  lcd.print("PIN");

}
void lcd_print_enter_master_pin()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ENTER MASTER");
  lcd.setCursor(0,1);
  lcd.print("PIN");

}
void lcd_print_incorrect_pin()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("INCORRECT ENTRY");
  lcd.setCursor(0, 1);
  lcd.print("RE-ENTER PIN");
}
void lcd_print_three_incorrect_attempts()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("THREE INCORRECT");
  lcd.setCursor(0, 1);
  lcd.print("ATTEMPTS");
}
void lcd_print_releasing_key()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RELEASING KEY");
  //lcd.setCursor(0, 1);
  //lcd.print("ATTEMPTS");
}
void lcd_print_enter_key_to_reset()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ENTER KEY TO");
  lcd.setCursor(0, 1);
  lcd.print("RESET");
}
void lcd_print_invalid_key()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("INVALID KEY");
  //lcd.setCursor(0, 1);
  //lcd.print("RESET");
}
void lcd_print_new_pin(char* pin)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("NEW PIN:");
  lcd.setCursor(0, 1);
  lcd.print(pin);
}
void setup() 
{
  Serial.begin(9600); 
  randomSeed(analogRead(0)); //an obvious exploit, Be careful with this. 
  lcd.begin(16, 2);
  //save_pins(pins);         //this code can be uncommented to initialise the eeprom
  //get_pins(pins);
  lcd_print_enter_pin();
}
void loop() 
{
  while(Serial.available() > 0) // Input related code
  {
    switch(Serial.read())       //Input related code
    {
      flush_serial();
      case '*': //enter key
    lcd.clear();        
        for(num_pin_attempts = 0; num_pin_attempts < ALLOWED_PIN_ATTEMPTS; num_pin_attempts++)
        {
        get_pin(pin);         //function takes input
        key = check_key_pin(pin,pins);
          if(key == -1)
          {
            if(num_pin_attempts < ALLOWED_PIN_ATTEMPTS - 1) //don't print this for the last incorrect entry
            {
              Serial.println("Pin incorrect!");
              Serial.println("Renter Pin!");
              lcd_print_incorrect_pin();
              delay(DELAY_TIME);

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
          lcd_print_three_incorrect_attempts();
          delay(10*DELAY_TIME);
          Serial.println("Press * to enter pin.");
          lcd_print_enter_pin();
          break; //break from switch when three incorrect pins are entered
        }

        Serial.print("Releasing key ");
        lcd_print_releasing_key();
        Serial.println((int)key + 1);
        delay(4*DELAY_TIME);
        Serial.println("Press * to enter pin.");
        lcd_print_enter_pin();

        //^^delay
        //^^clear screen
        break;
     
      case '#': //menu
          Serial.println("Enter Master Pin.");
          lcd_print_enter_master_pin();
          delay(DELAY_TIME);

          for(num_pin_attempts = 0; num_pin_attempts < ALLOWED_PIN_ATTEMPTS; num_pin_attempts++)
          {
            get_pin(pin);          //function takes input
            if(strcmp(master_pin,pin) != 0)
            {
              if(num_pin_attempts < ALLOWED_PIN_ATTEMPTS - 1)
              {
                Serial.println("Pin incorrect!");
                Serial.println("Renter Pin!");
                lcd_print_incorrect_pin();
                delay(DELAY_TIME);
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
            lcd_print_three_incorrect_attempts();
            delay(10*DELAY_TIME);
            Serial.println("Press * to enter pin.");
            lcd_print_enter_pin();
            break;
          }
          
          switch(get_menu_option())    //function takes input
          {
            case 0: //Get Key Selection
              Serial.println("Enter the key to reset.");
              lcd_print_enter_key_to_reset();
              key = get_key();        //function takes input
              if(key >0 && key <= NUM_KEYS)
              {
                key--;
                generate_pin(key, pins);
                Serial.print("New pin: ");
                Serial.println(pins[key]);
                lcd_print_new_pin(pins[key]);
                delay(DELAY_TIME*4);
                //delay for a few seconds to let the admin not the key
              }
              else
              {
                Serial.println("Invalid key.");
                lcd_print_invalid_key();
              }
              break;
            case 1: //Exit (Do Nothing)
              break;
          }
           Serial.println("Press * to enter pin.");
           lcd_print_enter_pin();
          //clear the screen before returning to default state
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
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("MENU");
  delay(DELAY_TIME);
  Serial.println("USE 1 and 3 to navigate");
  Serial.println("* Star to confirm option");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("1,3 TO NAVIGATE");
  lcd.setCursor(0,1);
  lcd.print("* TO CONFIRM");
  delay(DELAY_TIME);

  while(1)
  {
    if(!shown)
    {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(menu_options[option]);
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
  lcd.clear();
  lcd.setCursor(0,0);
  while(1)
  {
    while(Serial.available() > 0 && (pin_index + 1) <= PIN_LENGTH)
    {
      pin[pin_index] = Serial.read();
      pin_index++;
      print_pin(pin, pin_index);
      lcd.print("*");
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
 
