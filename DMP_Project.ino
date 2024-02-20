/*Keypad*/




#include <Keypad.h>
#include <Servo.h>
#include <LedControl.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 11
#define SS_PIN 10

MFRC522 mfrc522(SS_PIN, RST_PIN);

const int ROWS = 4;
const int COLS = 4;
char keys[ROWS][COLS] =
{
  {'D','C','B','A'},
  {'#','9','6','3'},
  {'0','8','5','2'},
  {'*','7','4','1'}
};

byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

Servo servo;
int motorPin = 43;

const int trigPin = 38;
const int echoPin = 39;
float duration, distance;

const int DIN = 46;
const int CS = 47;
const int CLK = 42;
LedControl lcdMatrix = LedControl(DIN, CLK, CS, 0);

byte Lock[8] = {B00000000, B00000000, B01111000, B01001110, B01001010, B01001110, B01111000, B00000000};

byte Anim[6][8] = {{B00000000, B00000000, B01111000, B01001110, B01001010, B01001110, B01111000, B00000000},
                   {B00000000, B00000000, B01111000, B01001111, B01001001, B01001011, B01111000, B00000000},
                   {B00000000, B00000000, B01111000, B01001111, B01001011, B01001000, B01111000, B00000000},
                   {B00000000, B00000000, B01111000, B01001111, B01001000, B01001000, B01111000, B00000000},
                   {B00000000, B00000000, B01111011, B01001111, B01001000, B01001000, B01111000, B00000000},
                   {B00000000, B00000011, B01111001, B01001111, B01001000, B01001000, B01111000, B00000000}};

LiquidCrystal_I2C lcd(0x27, 16, 2);

int buzzerPin = 34;

int motionPin = 12;
int motionState = LOW;
int motionVal = 0;

int numberOfFailedTries = 0;

void setup()
{
  Serial.begin(9600);
  while (!Serial);
  SPI.begin();
  mfrc522.PCD_Init();
  delay(4);

  servo.attach(motorPin);
  servo.write(0);
  delay(15);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(buzzerPin, OUTPUT);

  pinMode(motionPin, INPUT);

  lcd.init();
  lcd.setCursor(0, 0);

  lcdMatrix.shutdown(0, false);
  lcdMatrix.setIntensity(0, 0);
  lcdMatrix.clearDisplay(0);
}

void lcdMatrixStartupAnimation()
{
  int i;
  for (i = 0; i < 8; ++i)
  {
    lcdMatrix.setRow(0, i, Lock[i]);
    delay(50);
  }
}

void lcdMatrixIncorrectCodeAnimation()
{
  int i;
  for (i = 0; i < 3; ++i)
  {
    delay(200);
    lcdMatrix.setIntensity(0, 2);
    delay(200);
    lcdMatrix.setIntensity(0, 0);
  }
}

void lcdMatrixUnlockAnimation()
{
  int i, j;
  for (i = 0; i < 6; ++i)
  {
    for (j = 0; j < 8; ++j)
    {
      lcdMatrix.setRow(0, j, Anim[i][j]);
    }
    delay(50);
  }
}

bool codesMatch(char enteredCode[], char predefinedCode[])
{
  int i;
  for (i = 0; i < 4; ++i)
  {
    if (enteredCode[i] != predefinedCode[i])
    {
      return false;
    }
  }

  return true;
}

bool userIsPresent = false;
bool userIsClose = false;
bool correctCodeWasEntered = false;
bool correctKeyfobWasUsed = false;
bool enteringTheCode = false;
char correctCodeTudor[4] = {'2', '0', '0', '2'};
char correctCodeProfessor[4] = {'9', '9', '1', '0'};
char enteredCode[4];
int enteredCodeIndex = 0;

bool enteringTheResetCode = false;
char correctResetCode[4] = {'A', 'B', 'C', 'D'};
char enteredResetCode[4];
int enteredResetCodeIndex = 0;

unsigned long lastMillis = 0;
bool buzzerState = LOW;
int numberOfSounds1 = 0;
int numberOfSounds2 = 0;

void triggerAlarm()
{
  while(1)
  {
    unsigned long currentMillis = millis();

    // Output the first frequency
    if (currentMillis - lastMillis >= 1 && numberOfSounds1 < 160) 
    {
      lastMillis = currentMillis;

      buzzerState = !buzzerState;
      digitalWrite(buzzerPin, buzzerState);
      ++numberOfSounds1;
    }

    // Output the second frequency after the first one completes
    if (currentMillis - lastMillis >= 2 && numberOfSounds1 == 160 && numberOfSounds2 < 200) 
    {
      lastMillis = currentMillis;

      buzzerState = !buzzerState;
      digitalWrite(buzzerPin, buzzerState);
      ++numberOfSounds2;
    }

    if (numberOfSounds1 == 160 && numberOfSounds2 == 200)
    {
      numberOfSounds1 = 0;
      numberOfSounds2 = 0;
    }

    char key = keypad.getKey();

    if (key == '*')
    {
      enteringTheResetCode = true;
      lcd.setCursor(0, 1);
      lcd.print("*               ");
      enteredResetCode[3] = 0;
    }
    else if (enteringTheResetCode == true && key == '#')
    {
      if ((enteredResetCode[3] >= '0' && enteredResetCode[3] <= '9') || (enteredResetCode[3] >= 'A' && enteredResetCode[3] <= 'D'))
      {
        if (codesMatch(enteredResetCode, correctResetCode))
        {
          break;
        }
        else
        {
          lcd.setCursor(0, 1);
          lcd.print("Incorrect code!");

          digitalWrite(buzzerPin, LOW);
          lcdMatrixIncorrectCodeAnimation();

          lcd.setCursor(0, 1);
          lcd.print("Try again       ");
        }
        enteringTheResetCode = false;
        enteredResetCodeIndex = 0;
      }
    }
    else if (enteringTheResetCode == true && key)
    {
      enteredResetCode[enteredResetCodeIndex] = key;

      lcd.setCursor(enteredResetCodeIndex + 1, 1);
      char placeholder[16];
      placeholder[0] = key;
      strcpy(placeholder + 1, "               ");
      lcd.print(placeholder);

      enteredResetCodeIndex = (enteredResetCodeIndex + 1) % 4;
    }
  }
}

// void reset()
// {
//   userIsPresent = false;
//   userIsClose = false;
//   correctCodeWasEntered = false;
//   correctKeyfobWasUsed = false;
//   enteringTheCode = false;
//   enteredCodeIndex = 0;
//   motionState = LOW;
//   lcd.clear();
//   lcd.noBacklight();
//   lcdMatrix.clearDisplay(0);
// }

//int lastTime;

void loop()
{
  // if (userIsPresent == true && millis() - lastTime > 10000)
  // {
  //   digitalWrite(trigPin, LOW);
  //   delayMicroseconds(2);
  //   digitalWrite(trigPin, HIGH);
  //   delayMicroseconds(10);
  //   digitalWrite(trigPin, LOW);

  //   duration = pulseIn(echoPin, HIGH);
  //   distance = (duration * 0.0343) / 2;
  //   if (distance > 1000)
  //   {
  //     reset();
  //   }

  //   lastTime = millis();
  // }

  if (userIsPresent == false)
  {
    motionVal = digitalRead(motionPin);

    if (motionVal == HIGH)
    {
      if (motionState == LOW) 
      {
        //Serial.println("Motion detected!");
        motionState = HIGH;
        lcd.backlight();
        lcd.print("Hello, human!");
        lcdMatrixStartupAnimation();
        userIsPresent = true;
        //lastTime = millis();
      }
    }
  }
  else
  {
    if (userIsClose == false)
    {
      digitalWrite(trigPin, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);

      duration = pulseIn(echoPin, HIGH);
      distance = (duration * 0.0343) / 2;
      if (distance < 50)
      {
        //Serial.print("Distance: ");
        //Serial.println(distance);
        userIsClose = true;
        lcd.setCursor(0, 1);
        lcd.print("Enter the code");
      }
    }
    else
    {
      if (correctCodeWasEntered == false)
      {
        char key = keypad.getKey();
  
        if (enteringTheCode == false && key == '*')
        {
          enteringTheCode = true;
          lcd.setCursor(0, 1);
          lcd.print("*               ");
          enteredCode[3] = 0;
        }
        else if (enteringTheCode == true && key == '#')
        {
          if ((enteredCode[3] >= '0' && enteredCode[3] <= '9') || (enteredCode[3] >= 'A' && enteredCode[3] <= 'D'))
          {
            if (codesMatch(enteredCode, correctCodeTudor))
            {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Hello, Tudor!");
              lcd.setCursor(0, 1);
              lcd.print("Validate key fob");
              correctCodeWasEntered = true;
            }
            else if (codesMatch(enteredCode, correctCodeProfessor))
            {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Hello, professor!");
              lcd.setCursor(0, 1);
              lcd.print("Validate key fob");
              correctCodeWasEntered = true;
            }
            else
            {
              lcd.setCursor(0, 1);
              lcd.print("Incorrect code!");

              lcdMatrixIncorrectCodeAnimation();

              lcd.setCursor(0, 1);
              lcd.print("Try again       ");

              ++numberOfFailedTries;
              if (numberOfFailedTries == 3)
              {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Alarm is on!");
                lcd.setCursor(0, 1);
                lcd.print("Enter reset code");

                triggerAlarm();

                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Hello, human!");
                lcd.setCursor(0, 1);
                lcd.print("Enter the code");

                numberOfFailedTries = 0;
              }
            }
            enteringTheCode = false;
            enteredCodeIndex = 0;
          }
        }
        else if (enteringTheCode == true && key && key != '*' && key != '#')
        {
          enteredCode[enteredCodeIndex] = key;

          lcd.setCursor(enteredCodeIndex + 1, 1);
          char placeholder[16];
          placeholder[0] = key;
          strcpy(placeholder + 1, "               ");
          lcd.print(placeholder);

          enteredCodeIndex = (enteredCodeIndex + 1) % 4;
        }
      }
      else
      {
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial() && correctKeyfobWasUsed == false)
        {
          if (mfrc522.uid.uidByte[0] == 0x69 && mfrc522.uid.uidByte[1] == 0x08 && mfrc522.uid.uidByte[2] == 0x67 && mfrc522.uid.uidByte[3] == 0xB3)
          {
            correctKeyfobWasUsed = true;
            lcdMatrixUnlockAnimation();
            servo.write(90);
            delay(15);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Welcome home!");
          }
        }
      }
    }
  }
}




/*RFID*/





// #include <SPI.h>
// #include <MFRC522.h>

// #define RST_PIN 11          // Configurable, see typical pin layout above
// #define SS_PIN 10         // Configurable, see typical pin layout above

// MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

// void setup() 
// {
//   Serial.begin(9600);                       // Initialize serial communications with the PC
//   while (!Serial);                       // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
//   SPI.begin();                             // Init SPI bus
//   mfrc522.PCD_Init();              // Init MFRC522
//   delay(4);                                             // Optional delay. Some board do need more time after init to be ready, see Readme
//   mfrc522.PCD_DumpVersionToSerial();      // Show details of PCD - MFRC522 Card Reader details
//   Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
// }

// void loop() 
// {
//   // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
//   if ( ! mfrc522.PICC_IsNewCardPresent())
//   {
//     return;
//   }
//   // Select one of the cards
//   if ( ! mfrc522.PICC_ReadCardSerial()) 
//   {
//     return;
//   }
//   // Dump debug info about the card; PICC_HaltA() is automatically called
//   mfrc522.PICC_DumpToSerial(&(mfrc522.uid));

//   // if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
//   // {
//   //   Serial.print(mfrc522.uid.uidByte[0]); Serial.print(" ");
//   //   Serial.print(mfrc522.uid.uidByte[1]); Serial.print(" ");
//   //   Serial.print(mfrc522.uid.uidByte[2]); Serial.print(" ");
//   //   Serial.print(mfrc522.uid.uidByte[3]); Serial.print("\n");
//   // }
// }




/*Servo Motor*/




// #include <Servo.h>

// Servo servo;
// int angle = 10;

// void setup() 
// {
//   servo.attach(43);
//   servo.write(angle);
// }

// void loop() 
// { 
//   // scan from 0 to 180 degrees
//   for(angle = 10; angle < 180; ++angle)  
//   {
//     servo.write(angle);
//     delay(15);
//   } 
//   // now scan back from 180 to 0 degrees
//   for(angle = 180; angle > 10; --angle)
//   {
//     servo.write(angle);
//     delay(15);
//   }
// }



/*Ultrasonic Sensor*/




// const int trigPin = 9;
// const int echoPin = 10;

// float duration, distance;

// void setup() 
// {
//   pinMode(trigPin, OUTPUT);
//   pinMode(echoPin, INPUT);
//   Serial.begin(9600);
// }

// void loop() 
// {
//   digitalWrite(trigPin, LOW);
//   delayMicroseconds(2);
//   digitalWrite(trigPin, HIGH);
//   delayMicroseconds(10);
//   digitalWrite(trigPin, LOW);

//   duration = pulseIn(echoPin, HIGH);
//   distance = (duration*.0343)/2;
//   Serial.print("Distance: ");
//   Serial.println(distance);
//   delay(100);
// }





/*LED Matrix*/




// #include <LedControl.h>

// int DIN = 11;
// int CS = 7;
// int CLK = 13;
// LedControl lcdMatrix = LedControl(DIN, CLK, CS, 0);

// byte Lock[8] = {B00000000, B00000000, B01111000, B01001110, B01001010, B01001110, B01111000, B00000000};

// byte Anim[6][8] = {{B00000000, B00000000, B01111000, B01001110, B01001010, B01001110, B01111000, B00000000},
//                    {B00000000, B00000000, B01111000, B01001111, B01001001, B01001011, B01111000, B00000000},
//                    {B00000000, B00000000, B01111000, B01001111, B01001011, B01001000, B01111000, B00000000},
//                    {B00000000, B00000000, B01111000, B01001111, B01001000, B01001000, B01111000, B00000000},
//                    {B00000000, B00000000, B01111011, B01001111, B01001000, B01001000, B01111000, B00000000},
//                    {B00000000, B00000011, B01111001, B01001111, B01001000, B01001000, B01111000, B00000000}};

// void setup() 
// {
//   lcdMatrix.shutdown(0, false);
//   lcdMatrix.setIntensity(0, 0);
//   lcdMatrix.clearDisplay(0);

//   int i;
//   for (i = 0; i < 8; ++i)
//   {
//     lcdMatrix.setRow(0, i, Lock[i]);
//     delay(50);
//   }

//   for (i = 0; i < 3; ++i)
//   {
//     delay(200);
//     lcdMatrix.setIntensity(0, 2);
//     delay(200);
//     lcdMatrix.setIntensity(0, 0);
//   }

//   delay(3000);

//   int j;
//   for (i = 0; i < 6; ++i)
//   {
//     for (j = 0; j < 8; ++j)
//     {
//       lcdMatrix.setRow(0, j, Anim[i][j]);
//     }
//     delay(50);
//   }
// }

  
// void loop()
// {
  
// }




/*Buzzer*/




// int buzzer = 12;

// void setup()

// {
//   pinMode(buzzer, OUTPUT);
// }

// void loop()
// {
//   int i;
//   while(1)
//   {
//     //output a frequency
//     for(i = 0; i < 80; ++i)
//     {
//       digitalWrite(buzzer, HIGH);
//       delay(1);
//       digitalWrite(buzzer, LOW);
//       delay(1);
//     }

//     //output another frequency
//     for(i = 0; i < 100; ++i)
//     {
//       digitalWrite(buzzer,HIGH);
//       delay(2);
//       digitalWrite(buzzer,LOW);
//       delay(2);
//     }
//   }
// }



/*Motion Sensor*/





// int inputPin = 8;
// int pirState = LOW;
// int val = 0;
 
// void setup() 
// {
//   pinMode(inputPin, INPUT);
 
//   Serial.begin(9600);
// }
 
// void loop()
// {
//   val = digitalRead(inputPin);
  
//   if (val == HIGH)
//   {
//     if (pirState == LOW) 
// 	  {
//       Serial.println("Motion detected!");
//       pirState = HIGH;
//     }
//   }
//   else 
//   {
//     if (pirState == HIGH)
// 	  {
//       Serial.println("Motion ended!");
//       pirState = LOW;
//     }
//   }
// }




/*LCD*/





// #include <LiquidCrystal_I2C.h>

// LiquidCrystal_I2C lcd(0x27, 16, 2);

// void setup() 
// {
//   lcd.init();
//   lcd.backlight();
//   lcd.setCursor(0, 0);
//   lcd.print("Hello, World!");
// }

// void loop() 
// {
  
// }