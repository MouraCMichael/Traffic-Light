/************************************************************************************************
Author:   Corey Moura
Date:     12/20/2022

Description:  Traffic light sequence program with rotary encoder implementation.  

Notes:

// #include <EEPROM.h>
// EEPROM.read(address)
// EEPROM.write(address, value)
// EEPROM.update(address, value);  // The value is written only if differs from the one already saved at the same address.

// Timer1.setPeriod(microseconds); // Set a new period after the library is already initialized.
// Timer1.restart();               // Restart the timer, from the beginning of a new period.

************************************************************************************************/

// LIBRARIES ******************************

#include <TimerOne.h>
#include <EEPROM.h>

// MACROs **********************************

//Relay Pins
# define RED_PIN 9
# define YEL_PIN 8
# define GRE_PIN 7

//Encoder Pins
#define CLK 2
#define DT 3
#define SW 4

//States
#define CYCLE 0
#define ALL 1
#define CONSTANT_RED 2
#define CONSTANT_YEL 3
#define CONSTANT_GRE 4

//LIGHTs
# define CYCLE_GRE 0
# define CYCLE_YEL 1
# define CYCLE_RED 2

// GLOBAL VARIABLES **********************

int state = 0;

// Interrupt (Timer 1)
int cycleState = CYCLE_GRE;
int intervals = 0;
int holdIntervalVal = 0;
int timeYellow = 5;
int curSeconds = 0;

// Encoder
int clock;
int lastCLK;
int button = 0;
int minInterval = 5;
int maxInterval = 160;
int intervalStep = 5;
unsigned long lastButtonPress = 0;

// FUNCTION PROTOTYPES *******************
void timer_Init(void);
void relay_Init(void);
void encod_Init(void);

void getInterval(void);

void myISR(void);
void startup(void);



/************************************************************************************
* SETUP
************************************************************************************/
void setup() {
  
	Serial.begin(9600);   // Setup Serial Monitor

  // Initializations
  timer_Init();
  relay_Init();
  encod_Init();

  // Test lights for startup
  startup();

  getInterval();
}

/************************************************************************************
* MAIN - Monitor the rotary encoder inputs
************************************************************************************/
void loop() {

  
  // MONITOR ENCODER ---------------------------------------------------------------
  
	clock = digitalRead(CLK);                 // Read the current state of CLK

	// If last and current state of CLK are different, then pulse occurred
	if (clock != lastCLK  && clock == 1){

		// Encoder is rotating CW so increment
		if (digitalRead(DT) != clock) {

      if(intervals + intervalStep <= maxInterval){

        intervals = intervals + intervalStep;         //currentDir ="CW";
			  
      } 
		} 

    // Encoder is rotating CCW so decrement
    else {

      if(intervals - intervalStep >= minInterval){

        intervals = intervals - intervalStep;        //currentDir ="CCW";
			  
      } 
		}

    EEPROM.write(0, intervals);
    curSeconds = 0;                                 // Reset the seconds counter after every update to timer
    
	}

	lastCLK = clock;                   // Remember last CLK state

 

  // MONITOR BUTTON PRESSES -------------------------------------------------------

	button = digitalRead(SW);                   // Read the button state

	//If we detect LOW signal, button is pressed
	if (button == LOW) {

		if (millis() - lastButtonPress > 50) {
      
      if(state == CONSTANT_GRE) state = CYCLE;  
      else state++;

      if(state == CYCLE){
        Timer1.restart();  // Start the timer
        digitalWrite(RED_PIN, HIGH);
        digitalWrite(YEL_PIN, LOW);
        digitalWrite(GRE_PIN, LOW);
        cycleState = CYCLE_GRE;
        curSeconds = 0; 
      }
      else if(state == ALL){
        Timer1.stop();
        digitalWrite(RED_PIN, HIGH);
        digitalWrite(YEL_PIN, HIGH);
        digitalWrite(GRE_PIN, HIGH);
      }      
      else if(state == CONSTANT_RED){
        Timer1.stop();
        digitalWrite(RED_PIN, HIGH);
        digitalWrite(YEL_PIN, LOW);
        digitalWrite(GRE_PIN, LOW);
      }
      else if(state == CONSTANT_YEL){
        Timer1.stop();
        digitalWrite(RED_PIN, LOW);
        digitalWrite(YEL_PIN, HIGH);
        digitalWrite(GRE_PIN, LOW);
      }
      else if(state == CONSTANT_GRE){
        Timer1.stop();
        digitalWrite(RED_PIN, LOW);
        digitalWrite(YEL_PIN, LOW);
        digitalWrite(GRE_PIN, HIGH);
      }
      else{}

		}

		lastButtonPress = millis();                     // Remember last button press event
	
  } 
      
	delay(5);                                       // Slight delay to help debounce the reading
}



/************************************************************************************
* interrupt service routine 
************************************************************************************/
void myISR() {
  
  curSeconds++;

  if(state == CYCLE){
    
    //Serial.print("cycleState "); Serial.println(cycleState);

    if(curSeconds == intervals){

      if(cycleState == CYCLE_RED) {  cycleState = CYCLE_GRE; }
      else cycleState++;


      // Light is GREEN -----------------------------------------
      if(cycleState == CYCLE_GRE){
        digitalWrite(RED_PIN,  LOW);
        digitalWrite(YEL_PIN,  LOW);
        digitalWrite(GRE_PIN,  HIGH);
        } 


      // Light is YELLOW -----------------------------------------
      else if(cycleState == CYCLE_YEL){
        holdIntervalVal = intervals;          // Store the red/green time interval
        intervals = timeYellow;               // Load the yellow light time interval

        digitalWrite(RED_PIN,  LOW);
        digitalWrite(YEL_PIN,  HIGH);
        digitalWrite(GRE_PIN,  LOW);
        }

      // Light is RED --------------------------------------------
      else{
      
        intervals = holdIntervalVal;          // Re-assign the red/green time interval

        digitalWrite(RED_PIN,  HIGH);
        digitalWrite(YEL_PIN,  LOW);
        digitalWrite(GRE_PIN,  LOW);
        } 
      
      curSeconds = 0;   // Restart the time counter

    }
  } 
}

/************************************************************************************
* Start up light sequence for asthetics
************************************************************************************/
void startup(){
  
  unsigned long ms = 1000;
  
  digitalWrite(RED_PIN, HIGH);
  delay(ms);
  
  digitalWrite(YEL_PIN, HIGH);
  delay(ms);

  digitalWrite(GRE_PIN, HIGH);
  delay(ms);

  digitalWrite(RED_PIN, LOW); 
  digitalWrite(YEL_PIN, LOW);
  digitalWrite(GRE_PIN, LOW);

  delay(ms);

  digitalWrite(GRE_PIN, HIGH);

  Timer1.restart();  // Start the timer
}

/************************************************************************************
* GET INTERVAL:  
************************************************************************************/
void getInterval(void){

  intervals = EEPROM.read(0);

  if(intervals < minInterval || intervals > maxInterval){

    intervals = 5;
    EEPROM.write(0, intervals);

  }
  //Serial.print("intervals: ");  Serial.print(intervals);
}

/************************************************************************************
* TIMER 1 INITIALIZATION
************************************************************************************/
void timer_Init(void){

  unsigned long microseconds = 1000000;    // 1000000 microseconds = 1 second

  Timer1.initialize(microseconds);
  Timer1.attachInterrupt(myISR);
  Timer1.stop(); // Stop the timer until startup is complete
}


/************************************************************************************
* RELAY PIN INITIALIZATION
************************************************************************************/
void relay_Init(void){

  pinMode(RED_PIN, OUTPUT);
  digitalWrite(RED_PIN, LOW);

  pinMode(YEL_PIN,OUTPUT);
  digitalWrite(YEL_PIN,  LOW);

  pinMode(GRE_PIN, OUTPUT);
  digitalWrite(GRE_PIN,  LOW);
}

/************************************************************************************
* ROTARY ENCODER INITIALIZATION
************************************************************************************/
void encod_Init(void){

	pinMode(CLK,INPUT);
	pinMode(DT,INPUT);
	pinMode(SW, INPUT_PULLUP);

	// Read the initial state of CLK
	lastCLK = digitalRead(CLK);
}

