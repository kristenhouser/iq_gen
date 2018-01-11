/******************************************************************************
IQ GEN TEST SCRIPT

1/10/2018
******************************************************************************/
// Due to limitations in the Arduino environment, SPI.h must be included both
//  in the library which uses it *and* any sketch using that library.
#include <SPI.h>
#include <SparkFun_MiniGen.h>
#include <SoftwareSerial.h>


#define NINETY_D_SHIFT    (3.14 / 2)  / .00153
#define ONE_D_SHIFT       (0.0174533) / .00153
#define FIVE_D_SHIFT      (0.0872665) / .00153 

// Create an instance of the MiniGen device; note that this has no provision for
//  alternate CLK and MOSI pins, but you *can* pass it a different CS (or FSYNC,
//  as it's referred to elsewhere) pin number.
MiniGen i(10), q(9);    //check pins

uint16_t i_phase = 0, q_phase = 0;

static float frequency = 455000.0;   //455 kHz - goal frequency
unsigned long freqReg;
SoftwareSerial mySerial(0, 1); // RX, TX
char serialData = 'x';

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // Clear the registers in the AD9837 chip, so we're starting from a known
  //  location. Note that since the AD9837 has no DOUT, we can't use the
  //  read-modify-write method of control. At power up, the output frequency
  //  will be 100Hz.

  i.reset();
  q.reset();
  delay(2000);
  
  i.setMode(MiniGen::SINE);
  q.setMode(MiniGen::SINE);
  delay(3000);
    
  // This needs a little explanation. The choices are FULL, COARSE, and FINE.
  //  a FULL write takes longer but writes the entire frequency word, so you
  //  can change from any frequency to any other frequency. COARSE only allows
  //  you to change the upper bits; the lower bits remain unchanged, so you
  //  can do a fast write of a large step size. FINE is the opposite; quick
  //  writes but smaller steps.
  i.setFreqAdjustMode(MiniGen::FULL);
  q.setFreqAdjustMode(MiniGen::FULL);

  freqReg = i.freqCalc(frequency);
  i.adjustFreq(MiniGen::FREQ0, freqReg);
  q.adjustFreq(MiniGen::FREQ0, freqReg);

  delay(100);
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
        incrementPhase(ONE_D_SHIFT);   
        break;
      }
      case 'd':
      {
        decrementPhase(ONE_D_SHIFT);  
        break;
      }
      case 'U':
      {
        incrementPhase(FIVE_D_SHIFT);   
        break;
      }
      case 'D':
      {
        decrementPhase(FIVE_D_SHIFT);   
        break;
      }
      default:
        break;
    }

  } 
}

//TODO: phase_reg
//TODO: error check
void setPhase(int new_i_phase, int new_q_phase) {
  i_phase = new_i_phase;
  q_phase = new_q_phase;

  // chip select i
  i.selectPhaseReg(MiniGen::PHASE0);
  i.adjustPhaseShift(MiniGen::PHASE0, i_phase);  

  i.selectPhaseReg(MiniGen::PHASE1);
  i.adjustPhaseShift(MiniGen::PHASE1, i_phase);

  // chip select q
  q.selectPhaseReg(MiniGen::PHASE0);
  q.adjustPhaseShift(MiniGen::PHASE0, q_phase);

  q.selectPhaseReg(MiniGen::PHASE1);
  q.adjustPhaseShift(MiniGen::PHASE1, q_phase);
}

//TODO: error check
void incrementPhase(uint16_t shift) {
  // TODO: shouldn't fail quietly, just do the math to wrap around
  if (q_phase + shift < 4096)
    q_phase += shift;

  Serial.println(q_phase * .00153 , DEC);

  q.selectPhaseReg(MiniGen::PHASE0);
  q.adjustPhaseShift(MiniGen::PHASE0, q_phase);  

  q.selectPhaseReg(MiniGen::PHASE1);
  q.adjustPhaseShift(MiniGen::PHASE1, q_phase);  
}

//TODO: error check
void decrementPhase(uint16_t shift) {
  // TODO: shouldn't fail quietly, just do the math to wrap around
  if (q_phase - shift >= 0)
    q_phase -= shift;

  Serial.println(q_phase * .00153 , DEC);

  q.selectPhaseReg(MiniGen::PHASE0);
  q.adjustPhaseShift(MiniGen::PHASE0, q_phase);  

  q.selectPhaseReg(MiniGen::PHASE1);
  q.adjustPhaseShift(MiniGen::PHASE1, q_phase);  
}

