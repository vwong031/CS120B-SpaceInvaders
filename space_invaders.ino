#include <LiquidCrystal.h>
#include <SPI.h>
#include <Wire.h>
#define EEPROM_I2C_ADDRESS 0x50
#define BLACK 0x0000
#define WHITE 0xFFFF
#define CLOCK_PIN 13  // Clock
#define DATA_PIN  11  // Serial Data Input (MOSI)
#define CS_PIN  10    // Chip Select
#define DC_PIN  8     // Data/Command
#define RESET_PIN 9   // Reset
const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

typedef struct task {
  int state;
  unsigned long period;
  unsigned long elapsedTime;
  int (*TickFct)(int);

} task;

const unsigned short tasksNum = 7;
task tasks[tasksNum];

// SNES Controller Pins
const unsigned int DATA_CLOCK = A1,
                   DATA_LATCH = A2,
                   DATA_SERIAL = A3;

// Variables
unsigned char score = 0;
unsigned char high_score = score;
bool isOFF;
unsigned char reset_btn;
unsigned char shoot_btn;
unsigned char right_btn;
unsigned char left_btn;
int readVal = 0;
int myAddress = 100;
signed char x = 0;
signed char alien_movement = 0;
signed char y = 0;
signed char y_pos = 84;
signed char offset = 0;
signed char alien1_offset = 0;
signed char alien2_offset = 0;
unsigned char shoot;
bool x_pos;
unsigned char lives = 3;  
unsigned char alien_lives1, alien_lives2 = 2;
unsigned char killAlien1, killAlien2 = 0;    // 0 - don't kill; 1 - kill left alien; 2 - kill right alien
unsigned char killTank = 0;
unsigned char gameOver = 0;

// Helper Functions
void setupControllerPins() {
  // Set DATA_CLOCK normally HIGH
  pinMode(DATA_CLOCK, OUTPUT);
  digitalWrite(DATA_CLOCK, HIGH);

  // Set DATA_LATCH normally LOW
  pinMode(DATA_LATCH, OUTPUT);
  digitalWrite(DATA_LATCH, LOW);

  // Set DATA_SERIAL normally HIGH
  pinMode(DATA_SERIAL, OUTPUT);
  digitalWrite(DATA_SERIAL, HIGH);
  pinMode(DATA_SERIAL, INPUT);
}

void cmd(byte m) {
  digitalWrite(DC_PIN, LOW);  // cmd

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(m);
//  if (m == 0x01) {
//    delay(100);
//  }
  digitalWrite(CS_PIN, HIGH);
}

void data(byte m) {
  digitalWrite(DC_PIN, HIGH); //  data  

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(m);
  digitalWrite(CS_PIN, HIGH);
}

void readController(unsigned char &system_btn) {
  unsigned char buttons[12];
  
  digitalWrite(DATA_LATCH, HIGH);
  digitalWrite(DATA_LATCH, LOW);

  // Retrieve button presses from shift register
  for (int i = 0; i < 16; i++) {
    digitalWrite(DATA_CLOCK, LOW);

    if (i <= 11) {
      buttons[i] = digitalRead(DATA_SERIAL);
    }

    digitalWrite(DATA_CLOCK, HIGH);
  }

  system_btn = buttons[3];
  reset_btn = buttons[2];
  shoot_btn = buttons[0];
  right_btn = buttons[7];
  left_btn = buttons[6];

  if (shoot_btn == 0) {
    Serial.println("SHOOT");
  }
  else if (reset_btn == 0) {
    Serial.println("RESET");
  }
  else if (system_btn == 0) {
    Serial.println("START");
  }
  else if (left_btn == 0) {
    Serial.println("LEFT");
  }
  else if (right_btn == 0) {
    Serial.println("RIGHT");
  }
}

// Function to write to EEPROOM
void writeEEPROM(int address, byte val, int i2c_address) {
  // Begin transmission to I2C EEPROM
  Wire.beginTransmission(i2c_address);
 
  // Send memory address as two 8-bit bytes
  Wire.write((int)(address >> 8));   // MSB
  Wire.write((int)(address & 0xFF)); // LSB
 
  // Send data to be stored
  Wire.write(val);
 
  // End the transmission
  Wire.endTransmission();
 
  // Add 5ms delay for EEPROM
  delay(5);
}

// Function to read from EEPROM
byte readEEPROM(int address, int i2c_address) {
  // Define byte for received data
  byte readData = 0xFF;
 
  // Begin transmission to I2C EEPROM
  Wire.beginTransmission(i2c_address);
 
  // Send memory address as two 8-bit bytes
  Wire.write((int)(address >> 8));   // MSB
  Wire.write((int)(address & 0xFF)); // LSB
 
  // End the transmission
  Wire.endTransmission();
 
  // Request one byte of data at current memory address
  Wire.requestFrom(i2c_address, 1);
 
  // Read the data and assign to variable
  if (Wire.available()) {
    readData =  Wire.read(); 
    Serial.println("read");
  }
 
  // Return the data as function output
  return readData;
}

void WhiteBackground() {
  cmd(0x2A);    // CASET
  data(0x00);
  data(0x02);   // START = 2
  data(0x00); 
  data(0x81);   // END = 129
  cmd(0x2B);    // RASET
  data(0x00);
  data(0x01);   // START = 1
  data(0x00);
  data(0x81);   // END = 129

  // White Background
  cmd(0x2C);
  unsigned char j, k;
  for (k = 0; k < 129; ++k) {
    for (j = 0; j < 129; ++j) {
      data(0xFF);   // WHITE
      data(0xFF);   // WHITE
    }
  } 
}

void DisplayStart() {
  unsigned char row, column;

  // "PRESS"
  cmd(0x2A);
  data(0x00);
  data(0x0A);   // min = 10
  data(0x00);
  data(0x75);   // max = 117

  cmd(0x2B);
  data(0x00);
  data(0x19);   // min = 25
  data(0x00);
  data(0x5B);   // max = 91

  cmd(0x2C);
  for (row = 25; row < 92; ++row) {
    for (column = 10; column < 118; ++column) {
      if (row >= 25 && row <= 28) {
        // Top Line
        if (column >= 10 && column <= 29 || 
            column >= 32 && column <= 51 ||
            column >= 54 && column <= 73 ||
            column >= 76 && column <= 95 ||
            column >= 98 && column <= 117) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // Left & Right Line
      else if (row > 28 && row <= 36) {
        if (column >= 10 && column <= 13 || column >= 26 && column <= 29 || 
            column >= 32 && column <= 35 || column >= 48 && column <= 51 ||
            column >= 54 && column <= 57 ||
            column >= 76 && column <= 79 || 
            column >= 98 && column <= 101) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // Middle Line
      else if (row > 36 && row <= 40) {
        if (column >= 10 && column <= 29 ||
            column >= 32 && column <= 51 ||
            column >= 54 && column <= 70 ||
            column >= 76 && column <= 95 ||
            column >= 98 && column <= 117) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // Tail
      else if (row > 40 && row <= 53) {
        if (column >= 10 && column <= 13 ||
            column >= 32 && column <= 35 || (column >= (row + 0) && column <= (row + 5) && row <= 51) ||
            (column >= 54 && column <= 57 && row <= 50) ||
            (column >= 92 && column <= 95 && row <= 50) ||
            (column >= 114 && column <= 117 && row <= 50) ) {
          data(0x00);
          data(0x00);
        }
        else if (column >= 53 && column <= 72 && row >= 50) {
          data(0x00);
          data(0x00);
        }
        else if (column >= 76 && column <= 95 && row >= 50) {
          data(0x00);
          data(0x00);
        }
        else if (column >= 98 && column <= 117 && (row >= 50)) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // "START"
      // Top Line
      else if (row >= 63 && row <= 66) {
        if (column >= 10 && column <= 29 ||
            column >= 32 && column <= 51 ||
            column >= 54 && column <= 73 ||
            column >= 76 && column <= 95 ||
            column >= 98 && column <= 117) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // Left & Right Line
      else if (row > 66 && row <= 74) {
        if (column >= 10 && column <= 13 ||
            column >= 41 && column <= 44 ||
            column >= 54 && column <= 57 || column >= 70 && column <= 73 ||
            column >= 76 && column <= 79 || column >= 92 && column <= 95 ||
            column >= 106 && column <= 109) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // Middle Line
      else if (row > 74 && row <= 78) {
        if (column >= 10 && column <= 29 ||
            column >= 41 && column <= 44 ||
            column >= 54 && column <= 73 ||
            column >= 76 && column <= 95 ||
            column >= 106 && column <= 109) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // Tail
      else if (row > 78 && row <= 91) {
        if (column >= 26 && column <= 29 && row <= 88) {
          data(0x00);
          data(0x00);
        }
        else if (column >= 10 && column <= 29 && row >= 88) {
          data(0x00);
          data(0x00);
        }
        else if (column >= 41 && column <= 44 ||
                 column >= 54 && column <= 57 || 
                 column >= 70 && column <= 73 ||
                 column >= 76 && column <= 79 || (column >= (row + 3) && column <= (row + 8) && row <= 90) ||
                 column >= 106 && column <= 109) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else {
        data(0xFF);
        data(0xFF);
      }
    }
  }
}

void YouWin() {
  unsigned char row, column;

  cmd(0x2A);
  data(0x00);
  data(0x19);   // min = 25
  data(0x00);
  data(0x75);   // max = 117

  cmd(0x2B);
  data(0x00);
  data(0x19);   // min = 25
  data(0x00);
  data(0x81);   // max = 129

  cmd(0x2C);
  for (row = 25; row < 128; ++row) {
    for (column = 25; column < 118; ++column) {
      // Top Line
      if (row >= 25 && row <= 28) {
        if (column >= 34 && column <= 38 || column >= 48 && column <= 52 || 
            column >= 56 && column <= 74 ||
            column >= 78 && column <= 82 || column >= 92 && column <= 96) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else if (row >= 28 && row <= 40) {
        // Left & Right Line
        if (column >= 34 && column <= 38 || column >= 48 && column <= 52 || 
            column >= 56 && column <= 60 || column >= 70 && column <= 74 ||
            column >= 78 && column <= 82 || column >= 92 && column <= 96) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // Middle Line
      else if (row > 40 && row <= 44) {
        if (column >= 34 && column <= 52 ||
            column >= 56 && column <= 60 || column >= 70 && column <= 74 ||
            column >= 78 && column <= 82 || column >= 92 && column <= 96) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // Tail
      else if (row >= 44 && row <= 54) {
        if (column >= 41 && column <= 45 || 
            column >= 56 && column <= 60 || column >= 70 && column <= 74 ||
            column >= 78 && column <= 82 || column >= 92 && column <= 96) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // Bottom Line
      else if (row >= 54 && row <= 58) {
        if (column >= 41 && column <= 45 || 
            column >= 56 && column <= 74 ||
            column >= 78 && column <= 96) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // "WIN"
      // Top Line
      else if (row >= 63 && row <= 66) {
        if (column >= 28 && column <= 32 || column >= 48 && column <= 52 || 
            column >= 63 && column <= 67 ||
            column >= 78 && column <= 82 || column >= 83 && column <= 91 || column >= 95 && column <= 99) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else if (row >= 66 && row <= 79) {
        if (column >= 28 && column <= 32 || column >= 48 && column <= 52 || 
            column >= 63 && column <= 67 ||
            column >= 78 && column <= 82 || column >= 87 && column <= 91 || column >= 95 && column <= 99) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else if (row >= 79 && row <= 90) {
        if (column >= 28 && column <= 32 || column >= 38 && column <= 42 || column >= 48 && column <= 52 || 
            column >= 63 && column <= 67 ||
            column >= 78 && column <= 82 || column >= 87 && column <= 91 || column >= 95 && column <= 99) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // BOTTOM LINE
      else if (row >= 90 && row <= 94) {
        if (column >= 28 && column <= 37 || column >= 43 && column <= 52 ||
            column >= 63 && column <= 67 ||
            column >= 78 && column <= 82 || column >= 87 && column <= 99) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else {
        data(0xFF);
        data(0xFF);
      }
    }
  }
}

void YouLose() {
  unsigned char row, column;

  cmd(0x2A);
  data(0x00);
  data(0x19);   // min = 25
  data(0x00);
  data(0x75);   // max = 117

  cmd(0x2B);
  data(0x00);
  data(0x19);   // min = 25
  data(0x00);
  data(0x81);   // max = 129

  cmd(0x2C);
  for (row = 25; row < 128; ++row) {
    for (column = 25; column < 118; ++column) {
      // Top Line
      if (row >= 25 && row <= 28) {
        if (column >= 34 && column <= 38 || column >= 48 && column <= 52 || 
            column >= 56 && column <= 74 ||
            column >= 78 && column <= 82 || column >= 92 && column <= 96) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else if (row >= 28 && row <= 40) {
        // Left & Right Line
        if (column >= 34 && column <= 38 || column >= 48 && column <= 52 || 
            column >= 56 && column <= 60 || column >= 70 && column <= 74 ||
            column >= 78 && column <= 82 || column >= 92 && column <= 96) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // Middle Line
      else if (row > 40 && row <= 44) {
        if (column >= 34 && column <= 52 ||
            column >= 56 && column <= 60 || column >= 70 && column <= 74 ||
            column >= 78 && column <= 82 || column >= 92 && column <= 96) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // Tail
      else if (row >= 44 && row <= 54) {
        if (column >= 41 && column <= 45 || 
            column >= 56 && column <= 60 || column >= 70 && column <= 74 ||
            column >= 78 && column <= 82 || column >= 92 && column <= 96) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // Bottom Line
      else if (row >= 54 && row <= 58) {
        if (column >= 41 && column <= 45 || 
            column >= 56 && column <= 74 ||
            column >= 78 && column <= 96) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // "LOSE"
      // Top Line
      else if (row >= 63 && row <= 66) {
        if (column >= 28 && column <= 32 ||
            column >= 44 && column <= 62 ||
            column >= 66 && column <= 84 ||
            column >= 88 && column <= 106) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // Left & Right Line
      else if (row >= 66 && row <= 74) {
        if (column >= 28 && column <= 32 ||
            column >= 44 && column <= 48 || column >= 58 && column <= 62 ||
            column >= 66 && column <= 70 ||
            column >= 88 && column <= 92) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // Middle Line
      else if (row >= 74 && row <= 78) {
        if (column >= 28 && column <= 32 ||
            column >= 44 && column <= 48 || column >= 58 && column <= 62 ||
            column >= 66 && column <= 84 ||
            column >= 88 && column <= 102) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // Left & Right Line
      else if (row >= 78 && row <= 86) {
        if (column >= 28 && column <= 32 ||
            column >= 44 && column <= 48 || column >= 58 && column <= 62 ||
            column >= 80 && column <= 84 ||
            column >= 88 && column <= 92) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      // Bottom Line
      else if (row >= 86 && row <= 90) {
        if (column >= 28 && column <= 42 ||
            column >= 44 && column <= 62 ||
            column >= 66 && column <= 84 ||
            column >= 88 && column <= 106) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else {
        data(0xFF);
        data(0xFF);
      }
    }
  }
}

void Kill_Alien() {  
  if (108 + y - 1 == 22) {
    if (offset >= 11 + alien_movement && offset <= 28 + alien_movement) {
      --alien_lives1;
      
      if (alien_lives1 == 0) {
        killAlien1 = 1;
      }
      else {
        killAlien1 = 0;
      }
    }
    else if (offset >= 64 + alien_movement && offset <= 81 + alien_movement) {
      --alien_lives2;

      if (alien_lives2 == 0) {
        killAlien2 = 1;
      }
      else {
        killAlien2 = 0;
      }
    }
  }
}

void Kill_Tank() {
  if (30 + y_pos == 115) {
    if ((alien1_offset >= 56 + x && alien1_offset <= 72 + x) || (alien2_offset >= 56 + x && alien2_offset <= 72 + x)) {
      --lives;
      Serial.print("lives: "); Serial.println(lives);

      if (lives == 0) {
        killTank = 1;
      }
    }
  }
}

void Alien_Left() {
  unsigned char row, column;

  if (x_pos && alien_movement <= 40) {
    alien_movement += 5;

    if (alien_movement == 40) {
      x_pos = false;
    }
  }
  else if (!x_pos && alien_movement >= 0) {
    alien_movement -= 5;

    if (alien_movement == 0) {
      x_pos = true;
    }
  }

  cmd(0x2A);
  data(0x00);
  data(0x02);   // min = 0 
  data(0x00);
  data(0x45);   // max = 69

  cmd(0x2B);
  data(0x00);
  data(0x07);   // min = 7
  data(0x00);
  data(0x16);   // max = 22

  cmd(0x2C);
  for(row = 7; row < 22; ++row) {
    for(column = 0; column < 68; ++column) {
      if (row >= 7 && row <= 9) {
        if (column >= 17 + alien_movement && column <= 22 + alien_movement && !killAlien1) {
          data(0x00);
          data(0x00); 
        }
        else if (column >= 17 + alien_movement && column <= 22 + alien_movement && killAlien1) {
          data(0xFF);
          data(0xFF);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else if (row > 9 && row <= 12) {
        if (column >= 13 + alien_movement && column <= 26 + alien_movement && !killAlien1) {
          data(0x00);
          data(0x00);
        }
        else if (column >= 13 + alien_movement && column <= 26 + alien_movement && killAlien1) {
          data(0xFF);
          data(0xFF);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else if (row > 12 && row <= 15) {
        if ((column >= 11 + alien_movement && column <= 15 + alien_movement || column >= 18 + alien_movement && column <= 20 + alien_movement || 
            column >= 23 + alien_movement && column <= 28 + alien_movement) && !killAlien1) {
          data(0x00);
          data(0x00);
        }
        else if ((column >= 11 + alien_movement && column <= 15 + alien_movement || column >= 18 + alien_movement && column <= 20 + alien_movement || 
                  column >= 23 + alien_movement && column <= 28 + alien_movement) && killAlien1) {
          data(0xFF);
          data(0xFF);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else if (row > 15 && row <= 18) {
        if (column >= 11 + alien_movement && column <= 28 + alien_movement && !killAlien1) {
          data(0x00);
          data(0x00);
        }
        else if (column >= 11 + alien_movement && column <= 28 + alien_movement && killAlien1) {
          data(0xFF);
          data(0xFF);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else if (row > 18 && row <= 21) {
        if ((column >= 15 + alien_movement && column <= 18 + alien_movement || column >= 20 + alien_movement && column <= 23 + alien_movement) && !killAlien1) {
          data(0x00);
          data(0x00);
        }
        else if ((column >= 15 + alien_movement && column <= 18 + alien_movement || column >= 20 + alien_movement && column <= 23 + alien_movement) && killAlien1) {
          data(0xFF);
          data(0xFF);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else {
        data(0xFF);
        data(0xFF);
      }
    }
  }
}

void Alien_Right() {
  unsigned char row, column;

  if (x_pos && alien_movement <= 40) {
    alien_movement += 5;

    if (alien_movement == 40) {
      x_pos = false;
    }
  }
  else if (!x_pos && alien_movement >= 0) {
    alien_movement -= 5;

    if (alien_movement == 0) {
      x_pos = true;
    }
  }

  cmd(0x2A);
  data(0x00);
  data(0x41);   // min = 65
  data(0x00);
  data(0x81);   // max = 129

  cmd(0x2B);
  data(0x00);
  data(0x07);   // min = 7
  data(0x00);
  data(0x16);   // max = 22

  cmd(0x2C);
  for(row = 7; row < 22; ++row) {
    for(column = 65; column < 130; ++column) {
      if (row >= 7 && row <= 9) {
        if (column >= 70 + alien_movement && column <= 75 + alien_movement && !killAlien2) {
          data(0x00);
          data(0x00); 
        }
        else if (column >= 70 + alien_movement && column <= 75 + alien_movement && killAlien2) {
          data(0xFF);
          data(0xFF);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else if (row > 9 && row <= 12) {
        if (column >= 66 + alien_movement && column <= 79 + alien_movement && !killAlien2) {
          data(0x00);
          data(0x00);
        }
        else if (column >= 66 + alien_movement && column <= 79 + alien_movement && killAlien2) {
          data(0xFF);
          data(0xFF);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else if (row > 12 && row <= 15) {
        if ((column >= 64 + alien_movement && column <= 68 + alien_movement || column >= 71 + alien_movement && column <= 73 + alien_movement || 
            column >= 76 + alien_movement && column <= 81 + alien_movement) && !killAlien2) {
          data(0x00);
          data(0x00);
        }
        else if ((column >= 64 + alien_movement && column <= 68 + alien_movement || column >= 71 + alien_movement && column <= 73 + alien_movement || 
                  column >= 76 + alien_movement && column <= 81 + alien_movement) && killAlien2) {
          data(0xFF);
          data(0xFF);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else if (row > 15 && row <= 18) {
        if (column >= 64 + alien_movement && column <= 81 + alien_movement && !killAlien2) {
          data(0x00);
          data(0x00);
        }
        else if (column >= 64 + alien_movement && column <= 81 + alien_movement && killAlien2) {
          data(0xFF);
          data(0xFF);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else if (row > 18 && row <= 21) {
        if ((column >= 68 + alien_movement && column <= 71 + alien_movement || column >= 73 + alien_movement && column <= 76 + alien_movement) && !killAlien2) {
          data(0x00);
          data(0x00);
        }
        else if ((column >= 68 + alien_movement && column <= 71 + alien_movement || column >= 73 + alien_movement && column <= 76 + alien_movement) && killAlien2) {
          data(0xFF);
          data(0xFF);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else {
        data(0xFF);
        data(0xFF);
      }
    }
  }
}

void Tank() {
  unsigned char row, column;

  if (!right_btn && left_btn && x <= 50) {
    x += 5;
  }
  else if (right_btn && !left_btn && x >= -46) {
    x -= 5;
  }
  
  cmd(0x2A);
  data(0x00);
  data(0x02);   // min = 0 
  data(0x00);
  data(0x81);   // max = 129

  cmd(0x2B);
  data(0x00);
  data(0x71);   // min = 113
  data(0x00);
  data(0x7D);   // max = 125

  cmd(0x2C);
  for (row = 113; row < 125; ++row) {
    for (column = 0; column < 128; ++column) {
      if (row >= 113 && row <= 115) {
        if (column >= 63 + x && column <= 65 + x && !killTank) {
          data(0x00);
          data(0x00);
        }
        else if (column >= 63 + x && column <= 65 + x && killTank) {
          data(0xFF);
          data(0xFF);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else if (row >= 115 && row <= 117) {
        if (column >= 61 + x && column <= 67 + x && !killTank) {
          data(0x00);
          data(0x00);
        }
        else if (column >= 61 + x && column <= 67 + x && killTank) {
          data(0xFF);
          data(0xFF);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else if (row >= 117 && row <= 119) {
        if (column >= 58 + x && column <= 70 + x && !killTank) {
          data(0x00);
          data(0x00);
        }
        else if (column >= 58 + x && column <= 70 + x && killTank) {
          data(0xFF);
          data(0xFF);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else if (row >= 119 && row <= 124) {
        if (column >= 56 + x && column <= 72 + x && !killTank) {
          data(0x00);
          data(0x00);
        }
        else if (column >= 56 + x && column <= 72 + x && killTank) {
          data(0xFF);
          data(0xFF);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else {
        data(0xFF);
        data(0xFF);
      }
    }
  }
}

void GoodbyeLaser() {
  unsigned char row, column;
  
  cmd(0x2A);
  data(0x00);
  data(0x02);   // min = 17 
  data(0x00);
  data(0x81);   // max = 116

  cmd(0x2B);
  data(0x00);
  data(0x17);   // min = 23
  data(0x00);
  data(0x1E);   // max = 30

  cmd(0x2C);
  for (row = 23; row < 30; ++row) {
    for (column = 0; column < 128; ++column) {
      data(0xFF);
      data(0xFF);
    }
  }
}

void Laser(signed char &x) {
  unsigned char row, column;

  cmd(0x2A);
  data(0x00);
  data(0x02);   // min = 2
  data(0x00);
  data(0x81);   // max = 116

  cmd(0x2B);
  data(0x00);
  data(0x16);   // min = 22
  data(0x00);
  data(0x71);   // max = 113

  cmd(0x2C);
  for (row = 22; row < 112; ++row) {
    for (column = 0; column < 128; ++column) {
      if (row >= 22 + y_pos && row <= 30 + y_pos && (22 + y_pos == 108 + y) && (30 + y_pos == 115 + y) && shoot) {
        if (!killAlien1 && !killAlien2) {
          if (column == alien1_offset || column == alien2_offset || column == offset) {
            data(0x00);
            data(0x00);
          }
          else {
            data(0xFF);
            data(0xFF);
          }
        }
        else if (killAlien1 && !killAlien2) {
          if (column == alien2_offset || column == offset) {
            data(0x00);
            data(0x00);
          }
          else {
            data(0xFF);
            data(0xFF);
          }
        }
        else if (!killAlien1 && killAlien2) {
          if (column == alien1_offset || column == offset) {
            data(0x00);
            data(0x00);
          }
          else {
            data(0xFF);
            data(0xFF);
          }
        }
        else if (killAlien1 && killAlien2) {
          if (column == offset) {
            data(0xFF);
            data(0xFF);
          }
        }
      }
      else if (row >= 22 + y_pos && row <= 30 + y_pos) {
        if (!killAlien1 && !killAlien2) {
          if (column == alien1_offset || column == alien2_offset) {
            data(0x00);
            data(0x00);
          }
          else {
            data(0xFF);
            data(0xFF);
          }
        }
        else if (killAlien1 && !killAlien2) {
          if (column == alien2_offset) {
            data(0x00);
            data(0x00);
          }
          else {
            data(0xFF);
            data(0xFF);
          }
        }
        else if (!killAlien1 && killAlien2) {
          if (column == alien1_offset) {
            data(0x00);
            data(0x00);
          }
          else {
            data(0xFF);
            data(0xFF);
          }
        }
        else if (killAlien1 && killAlien2) {
          if (column == alien1_offset || column == alien2_offset) {
            data(0xFF);
            data(0xFF);
          }
        }
      }
      else if (row >= 108 + y && row <= 115 + y && shoot) {
        if (column == offset) {
          data(0x00);
          data(0x00);
        }
        else {
          data(0xFF);
          data(0xFF);
        }
      }
      else {
        data(0xFF);
        data(0xFF);
      }
    }
  }
}

void IncrementScore() {
  if (108 + y - 1 == 22) {
    if (offset >= 11 + alien_movement && offset <= 28 + alien_movement) {
      ++score;
    }
    else if (offset >= 64 + alien_movement && offset <= 81 + alien_movement) {
      ++score;
    }
  }
}

enum System_States { START, OFF, ON, RESET, WAIT };
int System_Tick(int state1) {
  static unsigned char system_btn;
  
  switch (state1) {
    case START:
      state1 = OFF;
      isOFF = true;
      shoot = 0;
      break;

    case OFF:
      if (system_btn) {
        state1 = OFF;
      }
      else {
        state1 = WAIT;
      }
      break;

    case WAIT:
      if (!system_btn) {
        state1 = WAIT;
      }
      else if (system_btn && isOFF) {
        state1 = ON;
        isOFF = false;
      }
      else if (system_btn && !isOFF) {
        state1 = OFF;
        isOFF = true;
      }
      break;

    case ON:
      if (!reset_btn) {
        state1 = RESET;
      }
      else if (!system_btn && gameOver) {
        state1 = WAIT;
      }
      else if (system_btn && !killAlien1 && !killAlien2 && !killTank && reset_btn) {
        state1 = ON;
      }
      break;

    case RESET:
      state1 = OFF;
      break;
  }

  switch (state1) {
    case START:
      break;

    case OFF:
      readController(system_btn);
      Serial.println("OFF");
      break;

    case WAIT:
      readController(system_btn);
      Serial.println("WAIT");
      break;

    case ON:
      readController(system_btn);
      
      if (!shoot_btn) {
        shoot = 1;
      }

      Serial.println("ON");
      break;

    case RESET:
      isOFF = true;
      WhiteBackground();
      Serial.println("RESET");
      break;
  }

  return state1;
}

enum LCD_System { LCD_START, OFF2, LEVEL1, WIN, LOSE };
int LCD_Screen(int state2) {
  switch (state2) {
    case LCD_START:
      state2 = OFF2;
      gameOver = 0;
      break;

    case OFF2:
      if (isOFF) {
        state2 = OFF2;
      }
      else {
        state2 = LEVEL1;
        WhiteBackground();
      }
      break;

    case LEVEL1:
      if (!reset_btn) {
        state2 = OFF2;
        WhiteBackground();
      }
      else if (!isOFF && (!killAlien1 || !killAlien2) && !killTank) {
        state2 = LEVEL1;
      }
      else if (!isOFF && killAlien1 && killAlien2 && !killTank) {
        state2 = WIN;
        gameOver = 1;
        WhiteBackground();
      }
      else if (!isOFF && killTank) {
        state2 = LOSE;
        gameOver = 1;
        WhiteBackground();
      }
      break;

    case WIN:
      if (isOFF) {
        state2 = OFF2;
        WhiteBackground();
      }
      else if (!reset_btn) {
        state2 = OFF2;
        WhiteBackground();
      }
      else {
        state2 = WIN;
      }
      break;

    case LOSE:
      if (!reset_btn || isOFF) {
        state2 = OFF2;
        WhiteBackground();
      }
      else {
        state2 = LOSE;
      }
      break;
  }

  switch (state2) {
    case LCD_START:
      break;

    case OFF2:
      DisplayStart();
      gameOver = 0;
      break;

    case LEVEL1:
      gameOver = 0;
      break;

    case WIN:
      YouWin();
      break;

    case LOSE:
      YouLose();
      break;
  }

  return state2;
}

enum Alien_States { ALIEN_START, ALIEN_OFF, ALIEN_1, GAME_OVER };
int Alien_Display(int state3) {
  switch(state3) {
    case ALIEN_START:
      state3 = ALIEN_OFF;
      break;

    case ALIEN_OFF:
      if (isOFF) {
        state3 = ALIEN_OFF;
      }
      else {
        state3 = ALIEN_1;
      }
      break;

    case ALIEN_1:
      if (!isOFF && !gameOver) {
        state3 = ALIEN_1;
      }
      else if (!reset_btn) {
        state3 = ALIEN_OFF;
      }
      else if (!isOFF && gameOver) {
        state3 = GAME_OVER;
      }
      break;

    case GAME_OVER:
      if (isOFF) {
        state3 = ALIEN_OFF;
      }
      else if (!reset_btn) {
        state3 = ALIEN_OFF;
      }
      break;
  }

  switch(state3) {
    case ALIEN_START:
      break;

    case ALIEN_OFF:
      x_pos = true;
      killAlien1 = 0;
      killAlien2 = 0;
      alien_lives1 = 2;
      alien_lives2 = 2;
      break;

    case ALIEN_1:
      Alien_Left();
      Alien_Right();
      Kill_Alien();
      break;

    case GAME_OVER:
      break;
  }

  return state3;
}

enum Display_Score_States { SCORE_START, OFF3, UPDATE };
int Display_Score(int state4) {
  switch (state4) {
    case SCORE_START:
      state4 = OFF3;
      break;

    case OFF3:
      if (isOFF) {
        state4 = OFF3;
      }
      else {
        state4 = UPDATE;
      }
      break;

    case UPDATE:
      if (!isOFF) {
        state4 = UPDATE;
      }
      else {
        state4 = OFF3;
      }
      break;
  }

  switch (state4) {
    case SCORE_START:
      break;

    case OFF3:
      score = 0;
      lcd.noDisplay();
      break;

    case UPDATE:
      if (score > high_score) {
        high_score = score;
        writeEEPROM(myAddress, high_score, EEPROM_I2C_ADDRESS);
      }

      if (!reset_btn) {
        score = 0;
        high_score = 0;
        writeEEPROM(myAddress, high_score, EEPROM_I2C_ADDRESS);
        high_score = readEEPROM(myAddress, EEPROM_I2C_ADDRESS);
      }

      IncrementScore();
      
      lcd.display();
      lcd.setCursor(0, 0);
      lcd.print("High Score: "); lcd.print(high_score);
      lcd.setCursor(0, 1);
      lcd.print("Score: "); lcd.print(score);
      break;
  }
  
  return state4;
}

enum Tank_States { TANK_START, TANK_OFF, TANK_ON, GAME_END };
int Tank_Display(int state5) {
  switch(state5) {
    case TANK_START:
      state5 = TANK_OFF;
      break;

    case TANK_OFF:
      if (isOFF) {
        state5 = TANK_OFF;
      }
      else {
        state5 = TANK_ON;
      }
      break;

    case TANK_ON:
      if (!reset_btn) {
        state5 = TANK_OFF;
      }
      else if (!isOFF && !gameOver) {
        state5 = TANK_ON;
      }
      else if (!isOFF && gameOver) {
        state5 = GAME_END;
      }
      break;

    case GAME_END:
      if (isOFF) {
        state5 = TANK_OFF;
      }
      else if (!reset_btn) {
        state5 = TANK_OFF;
      }
      break;
  }

  switch(state5) {
    case TANK_START:
      break;

    case TANK_OFF:
      killTank = 0;
      lives = 3;
      break;

    case TANK_ON:
      Tank();
      Kill_Tank();
      break;

    case GAME_END:
      break;
  }

  return state5;
}

enum Laser_States { LASER_START, LASER_OFF, LASER_ON, STOP };
int Laser_Display(int state6) {
  static signed char x = 0;
  
  switch(state6) {
    case LASER_START:
      state6 = LASER_OFF;
      break;

    case LASER_OFF:
      if (isOFF) {
        state6 = LASER_OFF;
      }
      else {
        state6 = LASER_ON;
      }
      break;

    case LASER_ON:
      if (!reset_btn) {
        state6 = LASER_OFF;
      }
      else if (!isOFF && !gameOver) {
        state6 = LASER_ON;
      }
      else if (!isOFF && gameOver) {
        state6 = STOP;
      }
      break;

    case STOP:
      if (isOFF) {
        state6 = LASER_OFF;
      }
      else if (!reset_btn) {
        state6 = LASER_OFF; 
      }
      break;
  }

  switch(state6) {
    case LASER_START:
      break;

    case LASER_OFF:
      y = 0;
      break;

    case LASER_ON:
      if (!shoot_btn) {
        offset = ((63 + x) + (65 + x)) / 2;
      }

      Laser(x);

      if (shoot) {
        if (y >= -85) {
          y -= 5;
        }
        else if (y < -85 || y == 0) {
          y = 0;
          shoot = 0;
        }
      }
      else if (!shoot) {
        GoodbyeLaser();
      }
    break;

    case STOP:
      break;
  }

  return state6;
}

enum Alien_Laser_States { ALIEN_LASER_START, ALIEN_LASER_OFF, ALIEN_LASER_ON, END_GAME };
int Alien_Laser(int state7) {
  switch(state7) {
    case ALIEN_LASER_START:
      state7 = ALIEN_LASER_OFF;
      break;

    case ALIEN_LASER_OFF:
      if (isOFF) {
        state7 = ALIEN_LASER_OFF;
      }
      else {
        state7 = ALIEN_LASER_ON;
      }
      break;

    case ALIEN_LASER_ON:
      if (!reset_btn) {
        state7 = ALIEN_LASER_OFF;
      }
      else if (!isOFF && !gameOver) {
        state7 = ALIEN_LASER_ON;
      }
      else if (!isOFF && gameOver) {
        state7 = END_GAME;
      }
      break;

    case END_GAME:
      if (isOFF) {
        state7 = ALIEN_LASER_OFF;
      }
      else if (!reset_btn) {
        state7 = ALIEN_LASER_OFF;
      }
      break;
  }

  switch(state7) {
    case ALIEN_LASER_START:
      break;

    case ALIEN_LASER_OFF:
      y_pos = 84;
      break;

    case ALIEN_LASER_ON:
      if (y_pos <= 84) {
          y_pos += 5;
      }
      else if (y_pos > 84) {
        y_pos = 0;
        alien1_offset = ((11 + alien_movement) + (28 + alien_movement)) / 2;
        alien2_offset = ((64 + alien_movement) + (81 + alien_movement)) / 2;
      }
      break;

    case END_GAME:
      break;
  }

  return state7;
}

void setup() {
  // put your setup code here, to run once:
  setupControllerPins();
  pinMode(CS_PIN, OUTPUT);
  pinMode(DC_PIN, OUTPUT);
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  delay(5);
  digitalWrite(RESET_PIN, LOW);
  delay(20);
  digitalWrite(RESET_PIN, HIGH);
  delay(150);
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  cmd(0x01);    // Software reset
  delay(150);
  cmd(0x11);    // Sleep out & booster on
  cmd(0x3A);    // Set color mode
  data(0x05);   // Bit 0x05
  cmd(0x29);    // Display ON  
  
  WhiteBackground();

  unsigned char i = 0;
  tasks[i].state = START;
  tasks[i].period = 100;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &System_Tick;
  ++i;
  tasks[i].state = LCD_START;
  tasks[i].period = 500;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &LCD_Screen;
  ++i;
  tasks[i].state = ALIEN_START;
  tasks[i].period = 100;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &Alien_Display;
  ++i;
  tasks[i].state = SCORE_START;
  tasks[i].period = 50;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &Display_Score;
  ++i;
  tasks[i].state = TANK_START;
  tasks[i].period = 100;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &Tank_Display;
  ++i;
  tasks[i].state = LASER_START;
  tasks[i].period = 5;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &Laser_Display;
  ++i;
  tasks[i].state = ALIEN_LASER_START;
  tasks[i].period = 10;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &Alien_Laser;
  ++i;

  Wire.begin();
  high_score = readEEPROM(myAddress, EEPROM_I2C_ADDRESS);
  lcd.begin(16, 2);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned char i;
  for (i = 0; i < tasksNum; ++i) {
    if ( (millis() - tasks[i].elapsedTime) >= tasks[i].period) {
      tasks[i].state = tasks[i].TickFct(tasks[i].state);
      tasks[i].elapsedTime = millis(); // Last time this task was ran
    }
  }
}
