#include <SPI.h>

const int SDI = 51;
const int CLK = 52;
const int LE = A15;
const int OE = A14;

const int LVL0 = 22;
const int LVL1 = 23;
const int LVL2 = 24;
const int LVL3 = 25;
const int LVL4 = 26;
const int LVL5 = 27;

const int FREQ0 = A0;
const int FREQ1 = A1;
const int FREQ2 = A2;
const int FREQ3 = A3;
const int FREQ4 = A4;
const int FREQ5 = A5;

const int MODE_BUTTON = 2;
const int ONOFF_BUTTON = 3;

byte RED[] = {1, 0, 0};
byte GREEN[] = {0, 1, 0};
byte BLUE[] = {0, 0, 1};
byte YELLOW[] = {1, 1, 0};
byte PURPLE[] = {1, 0, 1};
byte WHITE[] = {1, 1, 1};

byte red0[36], red1[36], red2[36], red3[36];
byte green0[36], green1[36], green2[36], green3[36];
byte blue0[36], blue1[36], blue2[36], blue3[36];

int mode = 0;
int leds_onoff = 1;
int rounds;

const int audio_ref = 2; // times a second
uint16_t freq[6]; 
uint16_t amps[6];
int audio_mode = 0;

int level = 0;
int tran_level = 0;

int bam_bit = 0;
int bam_counter = 0;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  
  pinMode(SDI, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(LE, OUTPUT);
  pinMode(OE, OUTPUT);

  pinMode(LVL0, OUTPUT);
  pinMode(LVL1, OUTPUT);
  pinMode(LVL2, OUTPUT);
  pinMode(LVL3, OUTPUT);
  pinMode(LVL4, OUTPUT);
  pinMode(LVL5, OUTPUT);

  pinMode(FREQ0, INPUT);
  pinMode(FREQ1, INPUT);
  pinMode(FREQ2, INPUT);
  pinMode(FREQ3, INPUT);
  pinMode(FREQ4, INPUT);
  pinMode(FREQ5, INPUT);

  pinMode(MODE_BUTTON, INPUT); // INT 4
  pinMode(ONOFF_BUTTON, INPUT); // INT 5

  digitalWrite(LVL0, HIGH);
  digitalWrite(LVL1, HIGH);
  digitalWrite(LVL2, HIGH);
  digitalWrite(LVL3, HIGH);
  digitalWrite(LVL4, HIGH);
  digitalWrite(LVL5, HIGH);

  pinMode(FREQ0, INPUT);

  digitalWrite(OE, LOW);

  noInterrupts();
  /*Timer setup***************************/

  // timer4 interrupt at ~150Hz
  // contol register
  TCCR4A = 0;

  // contol register
  TCCR4B = 0;

  // actual counter
  TCNT4 = 0;

  // compare match value
  // 60Hz = 41666, pre 64
  // 150Hz = 1666, pre 64
  // 6kHz = 41, pre 64
  // 12 kHz = 166, pre 8
  // 750 Hz = 332, pre 64
  // 1kHz = 249, pre 64
  OCR4A = 249; // = 16MHz/(12kHz*8) - 1

  // CTC mode ON
  TCCR4B |= (1 << WGM42);

  // Set prescaler to 64
  // 8 = 0 1 0
  // 64 = 0 1 1
  // 1 = 0 0 1
  TCCR4B |= (1 << CS41)|(1 << CS40);

  // enable timer compare interrupt
  TIMSK4 |= (1 << OCIE4A);
  /*Timer setup***************************/

  /*ADC setup*****************************/
  // Set voltage reference to 1,1 V
  ADMUX |= (1<<REFS1);
  /*ADC setup*****************************/

  /*Button interrupt setup****************/
  EICRB |= (1<<ISC51)|(1<<ISC41);
  EIMSK |= (1<<INT5)|(1<<INT4);
  /*Button interrupt setup****************/
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  interrupts();
}

void loop() {
  switch(mode) {
    case 0:
      cross();
      break;
    case 1:
      Snake();
      break;
    case 2:
      cube();
      break;
    case 3:
      Drop(100);
      break;
    case 4:
      Triangle(GREEN);
      break;   
    case 5:
      audio();
    default:
      break;
  }
  
}

void LED(int x, int y, int z, byte rgb[3]) {
  int led_number = x + y*6 + z*36;
  int carry_sum = 0;
  
  if (led_number > 35) {
    carry_sum += 12;
  }if (led_number > 71) {
    carry_sum += 12;
  }if (led_number > 107) {
    carry_sum += 12;
  }if (led_number > 143) {
    carry_sum += 12;
  }if (led_number > 179) {
    carry_sum += 12;
  }

  led_number += carry_sum;
  int byte2write = floor(led_number/8);
  int bit2write = led_number - byte2write*8;

  

  /*WRITING TO THE ARRAYS*/
  bitWrite(red0[byte2write], bit2write, bitRead(rgb[0], 0));
  bitWrite(red1[byte2write], bit2write, bitRead(rgb[0], 1));
  bitWrite(red2[byte2write], bit2write, bitRead(rgb[0], 2));
  bitWrite(red3[byte2write], bit2write, bitRead(rgb[0], 3));

  bitWrite(green0[byte2write], bit2write, bitRead(rgb[1], 0));
  bitWrite(green1[byte2write], bit2write, bitRead(rgb[1], 1));
  bitWrite(green2[byte2write], bit2write, bitRead(rgb[1], 2));
  bitWrite(green3[byte2write], bit2write, bitRead(rgb[1], 3));

  bitWrite(blue0[byte2write], bit2write, bitRead(rgb[2], 0));
  bitWrite(blue1[byte2write], bit2write, bitRead(rgb[2], 1));
  bitWrite(blue2[byte2write], bit2write, bitRead(rgb[2], 2));
  bitWrite(blue3[byte2write], bit2write, bitRead(rgb[2], 3));
  /*WRITING TO THE ARRAYS*/
}

ISR(TIMER4_COMPA_vect){
  
  /*Output enable control*/
  PORTK |= 1<<PK6;
  /*Output enable control*/

  for(int i = level+5; i >= (0+level); i--) {
    SPI.transfer(blue0[i]);  
  }for(int i = level+5; i >= (0+level); i--) {
    SPI.transfer(green0[i]);
  }for(int i = level+5; i >= (0+level); i--) {
    SPI.transfer(red0[i]);
  }

//  if (bam_bit == 0) {
//    Serial.println("IN CASE 0");
//    for(int i = level+5; i >= (0+level); i--) {
//      Serial.println("IN FOR");
//      SPI.transfer(blue0[i]);  
//    }for(int i = level+5; i >= (0+level); i--) {
//      SPI.transfer(green0[i]);
//    }for(int i = level+5; i >= (0+level); i--) {
//      SPI.transfer(red0[i]);
//    }
//  }
//  
//  if (bam_bit == 1) {
//    Serial.println("IN CASE 1");
//    for(int i = level+5; i >= (0+level); i--) {
//      SPI.transfer(blue1[i]);  
//    }for(int i = level+5; i >= (0+level); i--) {
//      SPI.transfer(green1[i]);
//    }for(int i = level+5; i >= (0+level); i--) {
//      SPI.transfer(red1[i]);
//    }
//  }
//  
//  if (bam_bit == 2) {
//    Serial.println("IN CASE 2");
//    for(int i = level+5; i >= (0+level); i--) {
//      SPI.transfer(blue2[i]);  
//    }for(int i = level+5; i >= (0+level); i--) {
//      SPI.transfer(green2[i]);
//    }for(int i = level+5; i >= (0+level); i--) {
//      SPI.transfer(red2[i]);
//    }
//  }
//  
//  if (bam_bit == 3) {
//    Serial.println("IN CASE 3");
//    for(int i = level+5; i >= (0+level); i--) {
//      SPI.transfer(blue3[i]);  
//    }for(int i = level+5; i >= (0+level); i--) {
//      SPI.transfer(green3[i]);
//    }for(int i = level+5; i >= (0+level); i--) {
//      SPI.transfer(red3[i]);
//    }
//  }
    

  /*Transistor level control*/
  PORTA = ~(1<<tran_level);
  /*Transistor level control*/

  /*Latch control*/
  PORTK |= 1<<PK7;
  PORTK &= ~(1<<PK7);
  /*Latch control*/

  /*Output enable control*/
  PORTK &= ~(1<<PK6);
  /*Output enable control*/

  level += 6;
  tran_level++;

  if (tran_level == 6) {
    tran_level = 0;
    level = 0;
    bam_counter++;
  }

//  if (bam_counter == 1) {
//    bam_bit++;
//  }if (bam_counter == 3) {
//    bam_bit++;
//  }if (bam_counter == 7) {
//    bam_bit++;
//  }if (bam_counter == 15) {
//    bam_bit = 0;
//    bam_counter = 0;
//  }

  if (audio_mode == 1) {
    for (int i = 0; i < 6; i++) {
      // Format the register, so that the right pin is chosen
      ADMUX &= B11100000;
  
      // Set which analog pin to read
      ADMUX |= i;
  
      // Enable and start ADC
      ADCSRA |= (1<<ADEN)|(1<<ADSC);
  
      while(ADCSRA & (1<<ADSC));
      
      freq[i] = ADC;
      
      if(freq[i] > amps[i]) {

        ADC = amps[i];
      }
      
    }
    rounds++; 
    if (rounds == 1000/audio_ref) {
      rounds = 0;
    }
  }
}

ISR (INT4_vect) {
  // MODE_BUTTON
  mode++;
  if (mode == 6) {
    mode = 0;   
  }
}

ISR (INT5_vect) {
  // ONOFF_BUTTON
  if (leds_onoff == 0) {
    // turns OE ON and ensables timer interrupts(turns LEDs OFF)
    
    /*Output enable control*/
    PORTK &= ~(1<<PK6);
  
    // disable timer compare interrupt
    TIMSK4 |= (1 << OCIE4A);

    leds_onoff = 1;
    return;
  }
  if (leds_onoff == 1){
    // turns OE OFF and disables timer interrupts(turns LEDs OFF)
    /*Output enable control*/
    PORTK |= 1<<PK6;
  
    // disable timer compare interrupt
    TIMSK4 &= ~(1 << OCIE4A);
    
    leds_onoff = 0;
    return;
  }
  
  
}

void clearLEDs () {

  /*CLEARING THE ARRAYS*/
  /*RED*/
  for (int i = 0; i < 36; i++) {
    red0[i] = B00000000;
  }
  for (int i = 0; i < 36; i++) {
    red1[i] = B00000000;
  }
  for (int i = 0; i < 36; i++) {
    red2[i] = B00000000;
  }
  for (int i = 0; i < 36; i++) {
    red3[i] = B00000000;
  }
  
  /*GREEN*/
  for (int i = 0; i < 36; i++) {
    green0[i] = B00000000;
  }
  for (int i = 0; i < 36; i++) {
    green1[i] = B00000000;
  }
  for (int i = 0; i < 36; i++) {
    green2[i] = B00000000;
  }
  for (int i = 0; i < 36; i++) {
    green3[i] = B00000000;
  }

  /*BLUE*/
  for (int i = 0; i < 36; i++) {
    blue0[i] = B00000000;
  }
  for (int i = 0; i < 36; i++) {
    blue1[i] = B00000000;
  }
  for (int i = 0; i < 36; i++) {
    blue2[i] = B00000000;
  }
  for (int i = 0; i < 36; i++) {
    blue3[i] = B00000000;
  }
  /*CLEARING THE ARRAYS*/
    
}
void audio() {
  audio_mode = 1;
  clearLEDs();
  for (int i = 0; i < 6; i++) {
    int height;
    if ((freq[i] > 876) || (freq[i] == 876))  {
      height = 5;
      //LED(i,0,5, GREEN);
    }if ((freq[i] < 876) && (freq[i] >= 730)) {
      //LED(i,0,4, GREEN);
      height = 4;
    }if ((freq[i] < 730) && (freq[i] >= 584)) {
      //LED(i,0,3, GREEN);
      height = 3;
    }if ((freq[i] < 584) && (freq[i] >= 438)) {
      //LED(i,0,2, GREEN);
      height = 2;
    }if ((freq[i] < 438) && (freq[i] >= 292)) {
      //LED(i,0,1, GREEN);
      height = 1;
    }if ((freq[i] < 292) && (freq[i] >= 143)) {
      //LED(i,0,0, GREEN);
      height = 0;
    }

    // Backside row enabling makes colour errors, speed issue?
    for (int h = 0; h <= height; h++) {
      if (h == 0) {
        LED(i,0,h, GREEN); 
        LED(0,i,h, GREEN);
        LED(5,abs(i-5),h, GREEN);  
        //LED(abs(i-5),5,h, GREEN); 
      }
      if (h == 1) {
        LED(i,0,h, YELLOW);
        LED(0,i,h, YELLOW);
        LED(5,abs(i-5),h, YELLOW); 
        //LED(abs(i-5),5,h, YELLOW); 
      }
      if (h == 2) {
        LED(i,0,h, WHITE);
        LED(0,i,h, WHITE);
        LED(5,abs(i-5),h, WHITE); 
        //LED(abs(i-5),5,h, WHITE); 
      } 
      if (h == 3) {
        LED(i,0,h, BLUE);
        LED(0,i,h, BLUE);
        LED(5,abs(i-5),h, BLUE); 
        //LED(abs(i-5),5,h, BLUE); 
      }  
      if (h == 4) {
        LED(i,0,h, PURPLE);
        LED(0,i,h, PURPLE);
        LED(5,abs(i-5),h, PURPLE); 
        //LED(abs(i-5),5,h, PURPLE); 
      }    
      if (h == 5) {
        LED(i,0,h, RED);
        LED(0,i,h, RED);
        LED(5,abs(i-5),h, RED); 
        //LED(abs(i-5),5,h, RED); 
      }
    }
    
    Serial.println(freq[i]);
  }
  delay(5);
  
}
  
byte randomColor() {
  return random(0,15);
}

void randomSnake() {

  int ms = 100;
  
  while (true) {
    int x1 = random(0, 5);
    int y1 = random(0, 5);
    int z1 = random(0, 5);
  
    int x2 = random(0, 5);
    int y2 = random(0, 5);
    int z2 = random(0, 5);

    int x = x2 - x1;
    int y = y2 - y1;
    int z = z2 - z1; 

    if (x > 0) {
      for (int i = x1; i < x2+1; i++) {
        byte rgb[] = {randomColor(), randomColor(), randomColor()};
        LED(i,y1,z1,rgb);
        delay(ms);
      }
    }if (x < 0) {
      for (int i = x1; i > x2-1; i--) {
        byte rgb[] = {randomColor(), randomColor(), randomColor()};
        LED(i,y1,z1, rgb);
        delay(ms);
      }
    }

    if (y > 0) {
      for (int i = y1; i < y2+1; i++) {
        byte rgb[] = {randomColor(), randomColor(), randomColor()};
        LED(x2,i,z1, rgb);
        delay(ms);
      }
    }if (y < 0) {
      for (int i = y1; i > y2-1; i--){
        byte rgb[] = {randomColor(), randomColor(), randomColor()};
        LED(x2,i,z1, rgb);
        delay(ms);
      }
    }

    if (y > 0) {
      for (int i = z1; i < z2+1; i++) {
        byte rgb[] = {randomColor(), randomColor(), randomColor()};
        LED(x2,y2,i, rgb);
        delay(ms);
      }
    }if (y < 0) {
      for (int i = z1; i > z2-1; i--) {
        byte rgb[] = {randomColor(), randomColor(), randomColor()};
        LED(x2,y2,i, rgb);
        delay(ms);
      }
    }

    delay(100);
    clearLEDs();
  } 
}

void Snake() {

  int ms = 100;
  //int pX[11] = {1,2,3,4,5,0,5,4,3,2,1};
  //int pY[11] = {5,4,3,2,1,0,1,2,3,4,5};
  while (true) {
    //int Xr1 = random(0,12);
    //int Yr1 = random(0,12);
    //int Xr2 = random(0,12);
    //int Yr2 = random(0,12);
    int x1 = random(0, 5);
    int y1 = random(0, 5);
    //int x1 = pX[Xr1];
    //int y1 = pY[Yr1];
    int z1 = random(0, 5);
  
    int x2 = random(0, 5);
    int y2 = random(0, 5);
    //int x2 = pX[Xr2];
    //int y2 = pY[Yr2];
    int z2 = random(0, 5);

    if ((abs(x1 - x2) <= 2) || (abs(y1 - y2) <= 2) || (abs(z1 - z2) <= 2)) {
      break;
    }
 
    int x = x2 - x1;
    int y = y2 - y1;
    int z = z2 - z1; 
    byte rgb[] = {randomColor(), randomColor(), randomColor()};

    if (x > 0) {
      for (int i = x1; i < x2+1; i++) {
        LED(i,y1,z1,rgb);
        delay(ms);
      }
    }if (x < 0) {
      for (int i = x1; i > x2-1; i--) {
        LED(i,y1,z1, rgb);
        delay(ms);
      }
    }

    if (y > 0) {
      for (int i = y1; i < y2+1; i++) {
        LED(x2,i,z1, rgb);
        delay(ms);
      }
    }if (y < 0) {
      for (int i = y1; i > y2-1; i--){
        LED(x2,i,z1, rgb);
        delay(ms);
      }
    }

    if (y > 0) {
      for (int i = z1; i < z2+1; i++) {
        LED(x2,y2,i, rgb);
        delay(ms);
      }
    }if (y < 0) {
      for (int i = z1; i > z2-1; i--) {
        LED(x2,y2,i, rgb);
        delay(ms);
      }
    }

    delay(100);
    clearLEDs();
  } 
}

void cube () {
  clearLEDs();
  for (int j = 0; j < 3; j++) {
    byte color[] = {randomColor(), randomColor(), randomColor()};
    // DOWN LEFT ROW
    for (int i = j; i < 6 - j; i++)
      LED(i,j,j, color);
    // DOWN FRONT ROW
    for (int i = j; i < 6 - j; i++)
      LED(j,i,j, color);
    // DOWN BACK ROW
    for (int i = j; i < 6 - j; i++)
      LED(i,5-j,j, color);
    // DOWN RIGHT ROW
    for (int i = j; i < 6 - j; i++)
      LED(5-j,i,j, color);
  
    /*******************************/
  
    for (int i = j; i < 6 - j; i++)
      LED(i,j,5-j, color);
    for (int i = j; i < 6 - j; i++)
      LED(j,i,5-j, color);
    for (int i = j; i < 6 - j; i++)
      LED(i,5-j,5-j, color);
    for (int i = j; i < 6 - j; i++)
      LED(5-j,i,5-j, color);
  
    /***********************/
  
    for (int i = j; i < 6 - j; i++)
      LED(j,j,i, color);
    for (int i = j; i < 6 - j; i++)
      LED(5-j,j,i, color);
    for (int i = j; i < 6 - j; i++)
      LED(j,5-j,i, color);
    for (int i = j; i < 6 - j; i++)
      LED(5-j,5-j,i, color);

    delay(200);
    clearLEDs();
  }
}

void cross () {

  for (int i = 0; i < 6; i++) {
    LED(0,0,i,PURPLE);
    LED(1,1,i,PURPLE);
    LED(2,2,i,PURPLE);
    LED(3,3,i,PURPLE);
    LED(4,4,i,PURPLE);
    LED(5,5,i,PURPLE);

    LED(5,0,i,PURPLE);
    LED(4,1,i,PURPLE);
    LED(3,2,i,PURPLE);
    LED(2,3,i,PURPLE);
    LED(1,4,i,PURPLE);
    LED(0,5,i,PURPLE);
    delay(100);

    clearLEDs();
  }

  for (int i = 0; i < 6; i++) {
    LED(0,i,0,PURPLE);
    LED(1,i,1,PURPLE);
    LED(2,i,2,PURPLE);
    LED(3,i,3,PURPLE);
    LED(4,i,4,PURPLE);
    LED(5,i,5,PURPLE);

    LED(5,i,0,PURPLE);
    LED(4,i,1,PURPLE);
    LED(3,i,2,PURPLE);
    LED(2,i,3,PURPLE);
    LED(1,i,4,PURPLE);
    LED(0,i,5,PURPLE);
    delay(100);

    clearLEDs();
  }

  for (int i = 5; i >= 0; i--) {
    LED(0,0,i,PURPLE);
    LED(1,1,i,PURPLE);
    LED(2,2,i,PURPLE);
    LED(3,3,i,PURPLE);
    LED(4,4,i,PURPLE);
    LED(5,5,i,PURPLE);

    LED(5,0,i,PURPLE);
    LED(4,1,i,PURPLE);
    LED(3,2,i,PURPLE);
    LED(2,3,i,PURPLE);
    LED(1,4,i,PURPLE);
    LED(0,5,i,PURPLE);
    delay(100);

    clearLEDs();
  }

  for (int i = 5; i >= 0; i--) {
    LED(0,i,0,PURPLE);
    LED(1,i,1,PURPLE);
    LED(2,i,2,PURPLE);
    LED(3,i,3,PURPLE);
    LED(4,i,4,PURPLE);
    LED(5,i,5,PURPLE);

    LED(5,i,0,PURPLE);
    LED(4,i,1,PURPLE);
    LED(3,i,2,PURPLE);
    LED(2,i,3,PURPLE);
    LED(1,i,4,PURPLE);
    LED(0,i,5,PURPLE);
    delay(100);

    clearLEDs();
  }

}

void diag() {
  
  LED(0,0,0,GREEN);
  LED(1,1,1,GREEN);
  LED(2,2,2,GREEN);
  LED(3,3,3,GREEN);
  LED(4,4,4,GREEN);
  LED(5,5,5,GREEN);

  delay(100);

  clearLEDs();

  LED(0,1,0,GREEN);
  LED(1,2,1,GREEN);
  LED(2,3,2,GREEN);
  LED(3,2,3,GREEN);
  LED(4,3,4,GREEN);
  LED(5,4,5,GREEN);

  delay(100);

  clearLEDs();

  LED(0,2,0,GREEN);
  LED(1,3,1,GREEN);
  LED(2,4,2,GREEN);
  LED(3,1,3,GREEN);
  LED(4,2,4,GREEN);
  LED(5,3,5,GREEN);

  delay(100);

  clearLEDs();

  LED(0,3,0,GREEN);
  LED(1,4,1,GREEN);
  LED(2,5,2,GREEN);
  LED(3,0,3,GREEN);
  LED(4,1,4,GREEN);
  LED(5,2,5,GREEN);

  delay(100);

  clearLEDs();

  LED(0,3,0,GREEN);
  LED(1,4,1,GREEN);
  LED(2,5,2,GREEN);
  LED(3,0,3,GREEN);
  LED(4,1,4,GREEN);
  LED(5,2,5,GREEN);

  delay(100);

  clearLEDs();
  
}


void Test() {
 
    for (int k = 0; k < 6; k++) {
      for (int j = 0; j < 6; j++) {
        for (int i = 0; i < 6; i++) {
          LED(i,j,k,RED);
          delay(100);
          clearLEDs();
        }
     }
  }
  clearLEDs();

    for (int k = 0; k < 6; k++) {
      for (int j = 0; j < 6; j++) {
        for (int i = 0; i < 6; i++) {
          LED(i,j,k,GREEN);
          delay(100);
          clearLEDs();
        }
     }
  }
  clearLEDs();

    for (int k = 0; k < 6; k++) {
      for (int j = 0; j < 6; j++) {
        for (int i = 0; i < 6; i++) {
          LED(i,j,k,BLUE);
          delay(100);
          clearLEDs();
        }
     }
  }
  clearLEDs();
}
void test(int z) {
  //digitalWrite(LVL0, LOW);
  PORTA = B11111111;
  PORTA &= ~(1<<z);
  digitalWrite(LE, LOW);
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  
  SPI.transfer(B00000000);
  SPI.transfer(B00000000);
  SPI.transfer(B00000000);
  SPI.transfer(B00000000);
  SPI.transfer(B00000000);
  SPI.transfer(B00000001);

  SPI.transfer(B00000000);
  SPI.transfer(B00000000);
  SPI.transfer(B00000000);
  SPI.transfer(B00000000);
  SPI.transfer(B00000000);
  SPI.transfer(B00000001);

  SPI.transfer(B00000000);
  SPI.transfer(B00000000);
  SPI.transfer(B00000000);
  SPI.transfer(B00000000);
  SPI.transfer(B00000000);
  SPI.transfer(B00000001);


 
  SPI.endTransaction();
  digitalWrite(OE, LOW);
  digitalWrite(LE, HIGH);
}


void Triangle(byte COLOR[])
{
  int del = 75;


  LED(0, 0, 2, COLOR);
  LED(0, 1, 2, COLOR);
  LED(0, 2, 2, COLOR);
  LED(0, 3, 2, COLOR);
  LED(0, 4, 2, COLOR);
  LED(0, 5, 2, COLOR);

  LED(1, 0, 3, COLOR);
  LED(1, 1, 3, COLOR);
  LED(1, 2, 3, COLOR);
  LED(1, 3, 3, COLOR);
  LED(1, 4, 3, COLOR);
  LED(1, 5, 3, COLOR);

  LED(2, 0, 4, COLOR);
  LED(2, 1, 4, COLOR);
  LED(2, 2, 4, COLOR);
  LED(2, 3, 4, COLOR);
  LED(2, 4, 4, COLOR);
  LED(2, 5, 4, COLOR);

  LED(3, 0, 5, COLOR);
  LED(3, 1, 5, COLOR);
  LED(3, 2, 5, COLOR);
  LED(3, 3, 5, COLOR);
  LED(3, 4, 5, COLOR);
  LED(3, 5, 5, COLOR);

  LED(4, 0, 5, COLOR);
  LED(4, 1, 5, COLOR);
  LED(4, 2, 5, COLOR);
  LED(4, 3, 5, COLOR);
  LED(4, 4, 5, COLOR);
  LED(4, 5, 5, COLOR);

  LED(5, 0, 4, COLOR);
  LED(5, 1, 4, COLOR);
  LED(5, 2, 4, COLOR);
  LED(5, 3, 4, COLOR);
  LED(5, 4, 4, COLOR);
  LED(5, 5, 4, COLOR);
  delay(del);
  clearLEDs();

  LED(0, 0, 3, COLOR);
  LED(0, 1, 3, COLOR);
  LED(0, 2, 3, COLOR);
  LED(0, 3, 3, COLOR);
  LED(0, 4, 3, COLOR);
  LED(0, 5, 3, COLOR);

  LED(1, 0, 4, COLOR);
  LED(1, 1, 4, COLOR);
  LED(1, 2, 4, COLOR);
  LED(1, 3, 4, COLOR);
  LED(1, 4, 4, COLOR);
  LED(1, 5, 4, COLOR);

  LED(2, 0, 5, COLOR);
  LED(2, 1, 5, COLOR);
  LED(2, 2, 5, COLOR);
  LED(2, 3, 5, COLOR);
  LED(2, 4, 5, COLOR);
  LED(2, 5, 5, COLOR);

  LED(3, 0, 5, COLOR);
  LED(3, 1, 5, COLOR);
  LED(3, 2, 5, COLOR);
  LED(3, 3, 5, COLOR);
  LED(3, 4, 5, COLOR);
  LED(3, 5, 5, COLOR);

  LED(4, 0, 4, COLOR);
  LED(4, 1, 4, COLOR);
  LED(4, 2, 4, COLOR);
  LED(4, 3, 4, COLOR);
  LED(4, 4, 4, COLOR);
  LED(4, 5, 4, COLOR);

  LED(5, 0, 3, COLOR);
  LED(5, 1, 3, COLOR);
  LED(5, 2, 3, COLOR);
  LED(5, 3, 3, COLOR);
  LED(5, 4, 3, COLOR);
  LED(5, 5, 3, COLOR);
  delay(del);
  clearLEDs();

  LED(0, 0, 4, COLOR);
  LED(0, 1, 4, COLOR);
  LED(0, 2, 4, COLOR);
  LED(0, 3, 4, COLOR);
  LED(0, 4, 4, COLOR);
  LED(0, 5, 4, COLOR);

  LED(1, 0, 5, COLOR);
  LED(1, 1, 5, COLOR);
  LED(1, 2, 5, COLOR);
  LED(1, 3, 5, COLOR);
  LED(1, 4, 5, COLOR);
  LED(1, 5, 5, COLOR);

  LED(2, 0, 5, COLOR);
  LED(2, 1, 5, COLOR);
  LED(2, 2, 5, COLOR);
  LED(2, 3, 5, COLOR);
  LED(2, 4, 5, COLOR);
  LED(2, 5, 5, COLOR);

  LED(3, 0, 4, COLOR);
  LED(3, 1, 4, COLOR);
  LED(3, 2, 4, COLOR);
  LED(3, 3, 4, COLOR);
  LED(3, 4, 4, COLOR);
  LED(3, 5, 4, COLOR);

  LED(4, 0, 3, COLOR);
  LED(4, 1, 3, COLOR);
  LED(4, 2, 3, COLOR);
  LED(4, 3, 3, COLOR);
  LED(4, 4, 3, COLOR);
  LED(4, 5, 3, COLOR);

  LED(5, 0, 2, COLOR);
  LED(5, 1, 2, COLOR);
  LED(5, 2, 2, COLOR);
  LED(5, 3, 2, COLOR);
  LED(5, 4, 2, COLOR);
  LED(5, 5, 2, COLOR);
  delay(del);
  clearLEDs();


  LED(0, 0, 5, COLOR);
  LED(0, 1, 5, COLOR);
  LED(0, 2, 5, COLOR);
  LED(0, 3, 5, COLOR);
  LED(0, 4, 5, COLOR);
  LED(0, 5, 5, COLOR);

  LED(1, 0, 5, COLOR);
  LED(1, 1, 5, COLOR);
  LED(1, 2, 5, COLOR);
  LED(1, 3, 5, COLOR);
  LED(1, 4, 5, COLOR);
  LED(1, 5, 5, COLOR);

  LED(2, 0, 4, COLOR);
  LED(2, 1, 4, COLOR);
  LED(2, 2, 4, COLOR);
  LED(2, 3, 4, COLOR);
  LED(2, 4, 4, COLOR);
  LED(2, 5, 4, COLOR);

  LED(3, 0, 3, COLOR);
  LED(3, 1, 3, COLOR);
  LED(3, 2, 3, COLOR);
  LED(3, 3, 3, COLOR);
  LED(3, 4, 3, COLOR);
  LED(3, 5, 3, COLOR);

  LED(4, 0, 2, COLOR);
  LED(4, 1, 2, COLOR);
  LED(4, 2, 2, COLOR);
  LED(4, 3, 2, COLOR);
  LED(4, 4, 2, COLOR);
  LED(4, 5, 2, COLOR);

  LED(5, 0, 1, COLOR);
  LED(5, 1, 1, COLOR);
  LED(5, 2, 1, COLOR);
  LED(5, 3, 1, COLOR);
  LED(5, 4, 1, COLOR);
  LED(5, 5, 1, COLOR);
  delay(del);
  clearLEDs();

  LED(0, 0, 5, COLOR);
  LED(0, 1, 5, COLOR);
  LED(0, 2, 5, COLOR);
  LED(0, 3, 5, COLOR);
  LED(0, 4, 5, COLOR);
  LED(0, 5, 5, COLOR);

  LED(1, 0, 4, COLOR);
  LED(1, 1, 4, COLOR);
  LED(1, 2, 4, COLOR);
  LED(1, 3, 4, COLOR);
  LED(1, 4, 4, COLOR);
  LED(1, 5, 4, COLOR);

  LED(2, 0, 3, COLOR);
  LED(2, 1, 3, COLOR);
  LED(2, 2, 3, COLOR);
  LED(2, 3, 3, COLOR);
  LED(2, 4, 3, COLOR);
  LED(2, 5, 3, COLOR);

  LED(3, 0, 2, COLOR);
  LED(3, 1, 2, COLOR);
  LED(3, 2, 2, COLOR);
  LED(3, 3, 2, COLOR);
  LED(3, 4, 2, COLOR);
  LED(3, 5, 2, COLOR);

  LED(4, 0, 1, COLOR);
  LED(4, 1, 1, COLOR);
  LED(4, 2, 1, COLOR);
  LED(4, 3, 1, COLOR);
  LED(4, 4, 1, COLOR);
  LED(4, 5, 1, COLOR);

  LED(5, 0, 0, COLOR);
  LED(5, 1, 0, COLOR);
  LED(5, 2, 0, COLOR);
  LED(5, 3, 0, COLOR);
  LED(5, 4, 0, COLOR);
  LED(5, 5, 0, COLOR);
  delay(del);
  clearLEDs();

  LED(0, 0, 4, COLOR);
  LED(0, 1, 4, COLOR);
  LED(0, 2, 4, COLOR);
  LED(0, 3, 4, COLOR);
  LED(0, 4, 4, COLOR);
  LED(0, 5, 4, COLOR);

  LED(1, 0, 3, COLOR);
  LED(1, 1, 3, COLOR);
  LED(1, 2, 3, COLOR);
  LED(1, 3, 3, COLOR);
  LED(1, 4, 3, COLOR);
  LED(1, 5, 3, COLOR);

  LED(2, 0, 2, COLOR);
  LED(2, 1, 2, COLOR);
  LED(2, 2, 2, COLOR);
  LED(2, 3, 2, COLOR);
  LED(2, 4, 2, COLOR);
  LED(2, 5, 2, COLOR);

  LED(3, 0, 1, COLOR);
  LED(3, 1, 1, COLOR);
  LED(3, 2, 1, COLOR);
  LED(3, 3, 1, COLOR);
  LED(3, 4, 1, COLOR);
  LED(3, 5, 1, COLOR);

  LED(4, 0, 0, COLOR);
  LED(4, 1, 0, COLOR);
  LED(4, 2, 0, COLOR);
  LED(4, 3, 0, COLOR);
  LED(4, 4, 0, COLOR);
  LED(4, 5, 0, COLOR);

  LED(5, 0, 0, COLOR);
  LED(5, 1, 0, COLOR);
  LED(5, 2, 0, COLOR);
  LED(5, 3, 0, COLOR);
  LED(5, 4, 0, COLOR);
  LED(5, 5, 0, COLOR);
  delay(del);
  clearLEDs();

  LED(0, 0, 3, COLOR);
  LED(0, 1, 3, COLOR);
  LED(0, 2, 3, COLOR);
  LED(0, 3, 3, COLOR);
  LED(0, 4, 3, COLOR);
  LED(0, 5, 3, COLOR);

  LED(1, 0, 2, COLOR);
  LED(1, 1, 2, COLOR);
  LED(1, 2, 2, COLOR);
  LED(1, 3, 2, COLOR);
  LED(1, 4, 2, COLOR);
  LED(1, 5, 2, COLOR);

  LED(2, 0, 1, COLOR);
  LED(2, 1, 1, COLOR);
  LED(2, 2, 1, COLOR);
  LED(2, 3, 1, COLOR);
  LED(2, 4, 1, COLOR);
  LED(2, 5, 1, COLOR);

  LED(3, 0, 0, COLOR);
  LED(3, 1, 0, COLOR);
  LED(3, 2, 0, COLOR);
  LED(3, 3, 0, COLOR);
  LED(3, 4, 0, COLOR);
  LED(3, 5, 0, COLOR);

  LED(4, 0, 0, COLOR);
  LED(4, 1, 0, COLOR);
  LED(4, 2, 0, COLOR);
  LED(4, 3, 0, COLOR);
  LED(4, 4, 0, COLOR);
  LED(4, 5, 0, COLOR);

  LED(5, 0, 1, COLOR);
  LED(5, 1, 1, COLOR);
  LED(5, 2, 1, COLOR);
  LED(5, 3, 1, COLOR);
  LED(5, 4, 1, COLOR);
  LED(5, 5, 1, COLOR);
  delay(del);
  clearLEDs();

  LED(0, 0, 2, COLOR);
  LED(0, 1, 2, COLOR);
  LED(0, 2, 2, COLOR);
  LED(0, 3, 2, COLOR);
  LED(0, 4, 2, COLOR);
  LED(0, 5, 2, COLOR);

  LED(1, 0, 1, COLOR);
  LED(1, 1, 1, COLOR);
  LED(1, 2, 1, COLOR);
  LED(1, 3, 1, COLOR);
  LED(1, 4, 1, COLOR);
  LED(1, 5, 1, COLOR);

  LED(2, 0, 0, COLOR);
  LED(2, 1, 0, COLOR);
  LED(2, 2, 0, COLOR);
  LED(2, 3, 0, COLOR);
  LED(2, 4, 0, COLOR);
  LED(2, 5, 0, COLOR);

  LED(3, 0, 0, COLOR);
  LED(3, 1, 0, COLOR);
  LED(3, 2, 0, COLOR);
  LED(3, 3, 0, COLOR);
  LED(3, 4, 0, COLOR);
  LED(3, 5, 0, COLOR);

  LED(4, 0, 1, COLOR);
  LED(4, 1, 1, COLOR);
  LED(4, 2, 1, COLOR);
  LED(4, 3, 1, COLOR);
  LED(4, 4, 1, COLOR);
  LED(4, 5, 1, COLOR);

  LED(5, 0, 2, COLOR);
  LED(5, 1, 2, COLOR);
  LED(5, 2, 2, COLOR);
  LED(5, 3, 2, COLOR);
  LED(5, 4, 2, COLOR);
  LED(5, 5, 2, COLOR);
  delay(del);
  clearLEDs();

  LED(0, 0, 1, COLOR);
  LED(0, 1, 1, COLOR);
  LED(0, 2, 1, COLOR);
  LED(0, 3, 1, COLOR);
  LED(0, 4, 1, COLOR);
  LED(0, 5, 1, COLOR);

  LED(1, 0, 0, COLOR);
  LED(1, 1, 0, COLOR);
  LED(1, 2, 0, COLOR);
  LED(1, 3, 0, COLOR);
  LED(1, 4, 0, COLOR);
  LED(1, 5, 0, COLOR);

  LED(2, 0, 0, COLOR);
  LED(2, 1, 0, COLOR);
  LED(2, 2, 0, COLOR);
  LED(2, 3, 0, COLOR);
  LED(2, 4, 0, COLOR);
  LED(2, 5, 0, COLOR);

  LED(3, 0, 1, COLOR);
  LED(3, 1, 1, COLOR);
  LED(3, 2, 1, COLOR);
  LED(3, 3, 1, COLOR);
  LED(3, 4, 1, COLOR);
  LED(3, 5, 1, COLOR);

  LED(4, 0, 2, COLOR);
  LED(4, 1, 2, COLOR);
  LED(4, 2, 2, COLOR);
  LED(4, 3, 2, COLOR);
  LED(4, 4, 2, COLOR);
  LED(4, 5, 2, COLOR);

  LED(5, 0, 3, COLOR);
  LED(5, 1, 3, COLOR);
  LED(5, 2, 3, COLOR);
  LED(5, 3, 3, COLOR);
  LED(5, 4, 3, COLOR);
  LED(5, 5, 3, COLOR);
  delay(del);
  clearLEDs();

  LED(0, 0, 0, COLOR);
  LED(0, 1, 0, COLOR);
  LED(0, 2, 0, COLOR);
  LED(0, 3, 0, COLOR);
  LED(0, 4, 0, COLOR);
  LED(0, 5, 0, COLOR);

  LED(1, 0, 0, COLOR);
  LED(1, 1, 0, COLOR);
  LED(1, 2, 0, COLOR);
  LED(1, 3, 0, COLOR);
  LED(1, 4, 0, COLOR);
  LED(1, 5, 0, COLOR);

  LED(2, 0, 1, COLOR);
  LED(2, 1, 1, COLOR);
  LED(2, 2, 1, COLOR);
  LED(2, 3, 1, COLOR);
  LED(2, 4, 1, COLOR);
  LED(2, 5, 1, COLOR);

  LED(3, 0, 2, COLOR);
  LED(3, 1, 2, COLOR);
  LED(3, 2, 2, COLOR);
  LED(3, 3, 2, COLOR);
  LED(3, 4, 2, COLOR);
  LED(3, 5, 2, COLOR);

  LED(4, 0, 3, COLOR);
  LED(4, 1, 3, COLOR);
  LED(4, 2, 3, COLOR);
  LED(4, 3, 3, COLOR);
  LED(4, 4, 3, COLOR);
  LED(4, 5, 3, COLOR);

  LED(5, 0, 4, COLOR);
  LED(5, 1, 4, COLOR);
  LED(5, 2, 4, COLOR);
  LED(5, 3, 4, COLOR);
  LED(5, 4, 4, COLOR);
  LED(5, 5, 4, COLOR);
  delay(del);
  clearLEDs();

  LED(0, 0, 0, COLOR);
  LED(0, 1, 0, COLOR);
  LED(0, 2, 0, COLOR);
  LED(0, 3, 0, COLOR);
  LED(0, 4, 0, COLOR);
  LED(0, 5, 0, COLOR);

  LED(1, 0, 1, COLOR);
  LED(1, 1, 1, COLOR);
  LED(1, 2, 1, COLOR);
  LED(1, 3, 1, COLOR);
  LED(1, 4, 1, COLOR);
  LED(1, 5, 1, COLOR);

  LED(2, 0, 2, COLOR);
  LED(2, 1, 2, COLOR);
  LED(2, 2, 2, COLOR);
  LED(2, 3, 2, COLOR);
  LED(2, 4, 2, COLOR);
  LED(2, 5, 2, COLOR);

  LED(3, 0, 3, COLOR);
  LED(3, 1, 3, COLOR);
  LED(3, 2, 3, COLOR);
  LED(3, 3, 3, COLOR);
  LED(3, 4, 3, COLOR);
  LED(3, 5, 3, COLOR);

  LED(4, 0, 4, COLOR);
  LED(4, 1, 4, COLOR);
  LED(4, 2, 4, COLOR);
  LED(4, 3, 4, COLOR);
  LED(4, 4, 4, COLOR);
  LED(4, 5, 4, COLOR);

  LED(5, 0, 5, COLOR);
  LED(5, 1, 5, COLOR);
  LED(5, 2, 5, COLOR);
  LED(5, 3, 5, COLOR);
  LED(5, 4, 5, COLOR);
  LED(5, 5, 5, COLOR);
  delay(del);
  clearLEDs();
}

byte RandomCOLOR(int MAX) {

  byte COLOR;

  if (MAX > 5)
  {
    MAX = 5;
  }
  int num = random(0, MAX + 1);

  if (num == 0)
  {
    COLOR = RED;
  }

  if (num == 1)
  {
    COLOR = GREEN;
  }

  if (num == 2)
  {
    COLOR = BLUE;
  }

  if (num == 3)
  {
    COLOR = RED;
  }

  if (num == 4)
  {
    COLOR = GREEN;
  }
  return COLOR;
}

void Random(int numleds, int DELAY) {

  for ( int i = 0; i < numleds; i++) {
    byte COLOR[] = {randomColor(), randomColor(), randomColor()};
    int x;
    int y;
    int z;
    x = random(0, 6);
    y = random(0, 6);
    z = random(0, 6);

    LED(x, y, z, COLOR);
    delay(DELAY / 2);

  }
  clearLEDs();
  delay(DELAY);
}

void Drop(int del)
{

  byte COLOR[] = {randomColor(), randomColor(), randomColor()};
  int numdrop = random(0, 3);
  int x[numdrop];
  int y[numdrop];
  int z = 5;
  for (int m = 0; m < numdrop; m++) {
    x[m] = random(0, 5);
    y[m] = random(0, 5);
    LED(x[m],y[m],5,COLOR);
  }
  for (int j = 0; j < 6; j++) {
    for (int n = 0; n < numdrop; n++) {
      LED(x[n], y[n], z - j, COLOR);
    }
    delay(del / 3);
    clearLEDs();

  }
}

void DropPlane(int del) {

  byte COLOR[] = {randomColor(), randomColor(), randomColor()};
  int oldX[36];
  int oldY[36];
  int Round = 0;
  byte oldColor[36];

  int x;
  int y;
  int z = 5;
  x = random(0, 6);
  y = random(0, 6);
  oldX[Round] = x;
  oldY[Round] = y;
  oldColor[Round] = COLOR;

  for (int j = 0; j < 6; j++) {
    LED(x, y, z - j, COLOR);
    for (int i = 0; i < Round; i++) {
      LED(oldX[i], oldY[i], 0, oldColor[i]);
    }
    delay(del/2);
  }
  
  Round++;
  for (int i = 0; i < Round; i++) {
    LED(oldX[i], oldY[i], 0, oldColor[i]);
  }
}
