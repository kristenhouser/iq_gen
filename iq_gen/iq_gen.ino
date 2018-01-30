/******************************************************************************
IQ GEN TEST SCRIPT

Commands:

i: set phase of i and q to 0
q: set phase of i to 0 and q to 90
u: increase phase of q one degree
U: increase phase of q five degrees
d: decrease phase of q one degree
U: decrease phase of q five degrees
f float_value: change frequency to float_value

1/18/2018
******************************************************************************/
// Due to limitations in the Arduino environment, SPI.h must be included both
//  in the library which uses it *and* any sketch using that library.
#include <SPI.h>
#include <SparkFun_MiniGen.h>
#include <SoftwareSerial.h>
#include <math.h>


// Phase shifts

#define NINETY_D_SHIFT    (3.14 / 2)  / .00153
#define FIVE_D_SHIFT      (0.0872665) / .00153
#define TWENTY_D_SHIFT    (0.0872665 * 4) / .00153

// Chip Selects
#define MINIGEN_I_CS         (10)
#define MINIGEN_Q_CS         (9)
#define SEVEN_SEG_CS         (8)

MiniGen i(MINIGEN_I_CS), q(MINIGEN_Q_CS);    //check pins

int16_t i_phase = 0, q_phase = 0;

static float frequency = 455000.0;   //455 kHz - goal frequency
unsigned long freqReg;

SoftwareSerial mySerial(0, 1); // RX, TX
char serialData = 'x';

int cycles = 0;

void setup()
{
  // Setup Seven Segment Display
  pinMode(SEVEN_SEG_CS, OUTPUT);
  digitalWrite(SEVEN_SEG_CS, HIGH); //By default, don't be selecting OpenSegment
  
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  Serial.println("Minigen Program");

  i.reset();
  q.reset();
  delay(1000);
  
  i.setMode(MiniGen::SINE);
  q.setMode(MiniGen::SINE);
  delay(1000);

  i.setFreqAdjustMode(MiniGen::FULL);
  q.setFreqAdjustMode(MiniGen::FULL);

  freqReg = i.freqCalc(frequency);
  i.adjustFreq(MiniGen::FREQ0, freqReg);
  q.adjustFreq(MiniGen::FREQ0, freqReg);

  SPI.setDataMode(SPI_MODE0);  // Clock idle high, data capture on falling edge

  SPI.begin(); //Start the SPI hardware
  SPI.setClockDivider(SPI_CLOCK_DIV64); //Slow down the master a bit
  
  //Force Seven Seg Cursor to Beginning of Display
  //digitalWrite(SEVEN_SEG_CS, LOW); //Drive the CS pin low to select OpenSegment
  SPI.transfer('v'); //Reset command

  setDisplayDecimal();
  updateQuadDisplay(0);
  
  //digitalWrite(SEVEN_SEG_CS, HIGH); //Drive the CS pin low to select OpenSegment

}

void loop()
{
  // listen for commands and respond accordingly

   if (Serial.available() > 0) {
    serialData = Serial.read();
    switch (serialData) {
      case 'i':
      {
        Serial.println("i_phase = 0, q_phase = 0");
        setPhase(0, 0);
        break;
      }
      case 'q':
      {
        Serial.println("i_phase = 0, q_phase = pi/2");
        setPhase(0, NINETY_D_SHIFT);
        break;
      }
      case 'u':
      {
        incrementPhase(FIVE_D_SHIFT);   
        break;
      }
      case 'd':
      {
        decrementPhase(FIVE_D_SHIFT);  
        break;
      }
      case 'U':
      {
        incrementPhase(TWENTY_D_SHIFT);   
        break;
      }
      case 'D':
      {
        decrementPhase(TWENTY_D_SHIFT);   
        break;
      }
      case 'f':
      {
        frequency = Serial.parseFloat();
        if (frequency >= 0 && frequency < 3e6)
        {
          freqReg = i.freqCalc(frequency);
          i.adjustFreq(MiniGen::FREQ0, freqReg);
          q.adjustFreq(MiniGen::FREQ0, freqReg);           
        }
        break;
      }
      default:
        break;
    }

    }  
  
    
    //Send the four characters to the display

}


void setPhase(int new_i_phase, int new_q_phase) {
  i_phase = new_i_phase;
  q_phase = new_q_phase;

  SPI.setDataMode(SPI_MODE2);  

  // chip select i
  i.selectPhaseReg(MiniGen::PHASE0);
  i.adjustPhaseShift(MiniGen::PHASE0, i_phase);  

  // chip select q
  q.selectPhaseReg(MiniGen::PHASE0);
  q.adjustPhaseShift(MiniGen::PHASE0, q_phase);

  spiSendValue(q_phase);
}

void incrementPhase(uint16_t shift) {
  // TODO: shouldn't fail quietly, just do the math to wrap around
  if (q_phase + shift < 4096)
    q_phase += shift;
  else {
    q_phase = shift - (4096 - q_phase);
  }

  Serial.println((q_phase * .00153) * (180/3.14), 1);

  q.selectPhaseReg(MiniGen::PHASE0);
  q.adjustPhaseShift(MiniGen::PHASE0, q_phase);  

  updateQuadDisplay((q_phase * .00153) * (180/3.14));
}

void decrementPhase(uint16_t shift) {
  // TODO: shouldn't fail quietly, just do the math to wrap around
  //Serial.println();

  if ((q_phase - shift) >= 0) 
    q_phase -= shift;
  else 
    q_phase = 4096 - (q_phase + shift);

  Serial.println((q_phase * .00153) * (180/3.14), 1);

  q.selectPhaseReg(MiniGen::PHASE0);
  q.adjustPhaseShift(MiniGen::PHASE0, q_phase);   

  updateQuadDisplay(((q_phase * .00153) * (180/3.14)));
}

void updateQuadDisplay(float shift) {
  SPI.setDataMode(SPI_MODE2);  

  spiSendValue(shift);
}

//Given a number, spiSendValue chops up an integer into four values and sends them out over spi
void spiSendValue(float num)
{
  int temp, tens, ones, decimal;
  SPI.setDataMode(SPI_MODE0); 
  digitalWrite(SEVEN_SEG_CS, LOW); //Drive the CS pin low to select OpenSegment
 
  temp = (int) num;

  if (temp >= 0) {
    Serial.println("setting display polarity +");
    setDisplayPolarity(0);
  }
  else {
    Serial.println("setting display polarity -");
    temp *= -1;
    setDisplayPolarity(1);
  }
  
  decimal = (num - temp) * 10;

  tens = temp / 10;
  temp %= 10;
  ones = temp;
  
  // Set SPI mode for seven seg


  SPI.transfer(0x79); // Send the Move Cursor Command
  SPI.transfer(0x01); // Send the data byte, with value 1
  SPI.transfer(tens); //Send the left most digit
  SPI.transfer(ones);
  SPI.transfer(decimal);
  
  setDisplayDecimal();
  digitalWrite(SEVEN_SEG_CS, HIGH); //Release the CS pin to de-select OpenSegment

}

void setDisplayPolarity(int neg) {
  if (neg) {
    SPI.transfer(0x7B);
    delay(1);
    SPI.transfer(64);  
  }
  else {
    SPI.transfer(0x7B);
    SPI.transfer(0);
  }
}

void setDisplayDecimal() { 

  //tempCycles = tempCycles;
  digitalWrite(SEVEN_SEG_CS, LOW); //Drive the CS pin low to select OpenSegment
  SPI.transfer(0x77);     //Decimal Control Command
  delay(1);
  SPI.transfer(4);        // Right most decimal point
}



