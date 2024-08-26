/*
 Arduino Controlled GPS Corrected Generator
 
 Permission is granted to use, copy, modify, and distribute this software
 and documentation for non-commercial purposes.

 Based on the projects: 
 W3PM (http://www.knology.net/~gmarcus/) &
 SQ1GU (http://sq1gu.tobis.com.pl/pl/syntezery-dds/44-generator-si5351a)

 Updates from SQ5NRY:
 - removed display and buttons
 - exposed the initial freq. correction delta
 - added a duplicated PPS output via GPIO
 - minor refactoring
*/

#include <TinyGPS++.h>
#include <string.h>
#include <ctype.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <Wire.h>
#include <si5351.h>

TinyGPSPlus gps;
Si5351 si5351;

#define LED_PPS 12
#define LED_ACC 11
#define ppsPin 2

unsigned long XtalFreq = 100000000;
long correction = 124000;
unsigned long mult = 0;
unsigned long Freq = 40000000;  //main 40MHz output
unsigned int tcount = 0;
unsigned int tcount2 = 0;
int validGPSflag = false;
boolean GPSstatus = true;
byte new_freq = 1;
unsigned long pps_correct;
byte pps_valid = 1;
float stab_float = 1000;

void setup() {

  pinMode(LED_PPS, OUTPUT);
  pinMode(LED_ACC, OUTPUT);
  digitalWrite(LED_ACC, HIGH);  //make a quick flash on boot to say I'm alive
  digitalWrite(LED_PPS, HIGH);
  delay(500);
  digitalWrite(LED_ACC, LOW);
  digitalWrite(LED_PPS, LOW);

  TCCR1B = 0;              //disable Timer5 during setup
  TCCR1A = 0;              //reset
  TCNT1 = 0;               //reset counter to zero
  TIFR1 = 1;               //reset overflow
  TIMSK1 = 1;              //turn on overflow flag
  pinMode(ppsPin, INPUT);  //inititalize GPS 1pps input
  digitalWrite(ppsPin, HIGH);

  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, correction);
  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_8MA);

  Serial.begin(9600);

  //set CLK0 to output 2.5MHz
  si5351.set_ms_source(SI5351_CLK0, SI5351_PLLA);
  si5351.set_freq(250000000ULL, SI5351_CLK0);
  si5351.set_ms_source(SI5351_CLK1, SI5351_PLLB);
  si5351.set_freq(Freq * SI5351_FREQ_MULT, SI5351_CLK1);
  si5351.update_status();

  delay(1000);
  GPSproces(6000);

  if (millis() > 5000 && gps.charsProcessed() < 10) {
    delay(5000);
    GPSstatus = false;
  }

  if (GPSstatus == true) {
    do {
      GPSproces(1000);
    } while (gps.satellites.value() == 0);

    attachInterrupt(0, PPSinterrupt, RISING);
    TCCR1B = 0;
    tcount = 0;
    mult = 0;
    validGPSflag = 1;
  }
}

void loop() {
  if (tcount2 != tcount) {
    tcount2 = tcount;
    pps_correct = millis();
  }
  if (tcount < 4) {
    GPSproces(0);
  }

  if (new_freq == 1) {
    correct_si5351a();
    new_freq = 0;
    if (abs(stab_float) < 1) {
      digitalWrite(LED_ACC, HIGH);
    }
    if (abs(stab_float) > 1) {
      digitalWrite(LED_ACC, LOW);
    }
  }

  if (new_freq == 2) {
    update_si5351a();
    new_freq = 0;
  }

  if (millis() > pps_correct + 1200) {
    pps_valid = 0;
    pps_correct = millis();
  }
}

byte stab_count = 44;
void PPSinterrupt() {
  togglePpsLed();
  tcount++;
  stab_count--;

  if (tcount == 4)  //start counting the 2.5MHz signal from Si5351A CLK0
  {
    TCCR1B = 7;  //clock on rising edge of pin 5
  }

  if (tcount == 44)  //the 40 second gate time elapsed - stop counting
  {
    TCCR1B = 0;  //turn off counter
    if (pps_valid == 1) {
      XtalFreq = mult * 0x10000 + TCNT1;  //calculate correction factor
      new_freq = 1;
    }
    TCNT1 = 0;  //reset count to zero
    mult = 0;
    tcount = 0;  //reset the seconds counter
    pps_valid = 1;
    Serial.begin(9600);
    stab_count = 44;
    calculateCorrection();
  }
}

bool ppsLed = true;
void togglePpsLed() {
  if (ppsLed) {
    digitalWrite(LED_PPS, HIGH);
  } else {
    digitalWrite(LED_PPS, LOW);
  }
  ppsLed = !ppsLed;
}

ISR(TIMER1_OVF_vect) {
  mult++; 
  TIFR1 = (1 << TOV1);  //clear overlow flag
}

void calculateCorrection() {
  long stab = XtalFreq - 100000000;
  stab = stab * 10;
  if (stab > 100 || stab < -100) {
    correction = correction + stab;
  } else if (stab > 20 || stab < -20) {
    correction = correction + stab / 2;
  } else correction = correction + stab / 4;

  long pomocna = 10000 / (Freq / 1000000);
  stab *= 100;
  stab /= pomocna;
  stab_float = float(stab) / 10;
}

//set new frequency
void update_si5351a() {
  si5351.set_freq(Freq * SI5351_FREQ_MULT, SI5351_CLK1);
}

//apply frequency correction
void correct_si5351a() {
  si5351.set_correction(correction, SI5351_PLL_INPUT_XO);
}

//read and process NMEA data from GPS
static void GPSproces(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (Serial.available())
      gps.encode(Serial.read());
  } while (millis() - start < ms);
}
