/* DIY Spotwelder

by Jon Nuljon June 2016

A resistance welder with these Functions:
  - 2 welding modes:
	Conitnuous mode - welding current maintained until release of actuator (button, foot pedal, etc.)
	Dual Pulse mode - user configurable timing for pre-weld pulse, pause, main weld pulse (can save 3 configs)
  - Overheat protection via temperature controled cooling fan and weld current cutoff at pre defined temperatures 
  - LCD display of status and menu controls.
  - Menu control interface ... coming soon
  - Powered by AC current at approximately 2 volts / 500 amperes. 
         This is achieved by rewinding the core of a microwave oven transformer and connecting it to 2 opposed 
         SCR's to control the AC weld current, various switches for mode select and menu control, a thermistor 
         with resistors to detect the temperarure, transistor to control a DC fan motor for cooling, 
         a Grove RGB backlight LCD display to give visual signals (resitance welders should not throw an arc),
         and an Arduino Pro-mini micro controler to rule the the whole pile and run the code that follows.

heavily modified from original code by Albert van Dalen, Arduino spot welder controller
http://www.avdweb.nl/arduino/hardware-interfacing/spot-welder-controller.html

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License at http://www.gnu.org/licenses .

works with the circuit, SpotWelder v3.0, design by Jon Nuljon & Norman Olds Jr. (inspired by Albert van Dalen)
https://www.dropbox.com/s/4k4nrjpkoesbhgz/SpotWelder%20v3.0.pdf?dl=0

For pulse welding, we want to start welding at peak power. Since our welder is represents an inductive load
running on alternating current, the amperage over time will follow the voltage by 90 degress. So to avoid inrush
currents we need to calcualte the weld start time based on mains voltage zero crossing points.

these 15 lines = my attempt to graph the voltage, amperage, and preWeld pulse in ascii art

MAINS POWER where I live is delivered by 60Hz sinus cycle 
         or we could say 16.667ms <---------->
                                              <->  sinusMax_us 4167us (zero cross until peak amps)
                                   ____        ____        ____        ____        ____
VOLTS zeroCross-->                /____\ ____ /____\ ____ /____\ ____ /____\ ____ /____\ __
                                        \____/      \____/      \____/      \____/      \__
                                       ____        ____        ____        ____        ____
AMPS lag 90 degrees->                 /____\ ____ /____\ ____ /____\ ____ /____\ ____ /____\ __
                                            \____/      \____/      \____/      \____/      \__
                                                    _______________________
begin weld at max power				   |                       |
                                   ________________|     preWeld pulse     |__________________
                                                   <----------50ms---------><----- weldPause ...
											
*/

/*include library dependencies */
#include <Arduino.h>	//Arduino core library
#include <Streaming.h>  // library for concatenating serial output code
#include <Switch.h>		// library to debounce and read switche by Albert van Dalen
#include <Albert.h>		// library for blinking LED on power up by Albert van Dalen
#include <Wire.h>		// display uses I2C aka Wire 
#include <rgb_lcd.h>	// library for the Grove RGB Backlight LCD Display - 16 columns 2 rows

/*Hardware setup for I/O connections (pins) on the Arduino pro-mini board  */
const byte weldPin = A1;  // output connected to A7 via air wire for SCR gate control (since A7 was bad on my board)
const byte weldButtonPin = 10;  // input connected to type N barrel connector
const byte thermPin = A3;	// analog input connected to thermistor
const byte ledPin = A2;		// analog output connected to anode for green LED
const byte fanPin = 11;		// analog output connected to base of NPN transistor that drives the fan
const byte zeroCrossPin = A0;   // input connected to 1M ohm resistor to read AC signal
const byte menu1Pin = 8;	// input connected to momentary switch 1
const byte menu2Pin = 9;	// input connected to momentary switch 2
const byte menu3Pin = 7;        // input connected to A6 via air wire for momentary switch 3 (since A6 was bad on my board)

/*more ascii art of Hardware for the MODE selector. The pins wired to a rotary switch, configured as active low

	     OFF (nc  or Rswitch0)
	        |    
	       /    _Rswitch1
	      /    
	 GND_/       _Rswitch2
	 
            	 _Rswitch3
			

	       Rswitch0         rotary switch OFF position has no connection */
const byte Rswitch1 = 2;  //rotary switch 1st position I/O pin
const byte Rswitch2 = 3;  //rotary switch 2nd position I/O pin
const byte Rswitch3 = 4;  //rotary switch 3rd position I/O pin

/* TEMPERATURE CONTROL RANGE
5v vcc from the arduino is supplied to a voltage divider circuit via a 1k thermistor (NTC) and a 1k ohm resistor
which provides a voltage used for calculating temperature of the welding transformer core. The overheat protection
will cut off the weldign current at 350 degrees farenheit, set by MAXtemp. However, we want to avoid this by
cooling the core with increasing fan speed beginning at 150 degress, set by MINtemp, with the fan full on by 300
degrees (2xMINtemp).
*/

const int MAXtemp = 350; // CRITICAL overheat protection current cut off point
const int MINtemp = 150; // overheat protection activation threshold to begin cooling

/*other declared constants*/
const int sinusMax_us = 4167; //1000ms divided by 60Hz divided by 4SinusPeriods equals 4167us
const int step_ms = 50;        //increment weld pulse by this amount in miliseconds
const byte MAXfan = 255;	//assign max PWM value for fan speed must be less than 256

/* construct a weld button */
Switch weldButton(weldButtonPin); //switch.h library call

/* instantiate a display object*/
rgb_lcd lcd; //Grove RGB Backlight library call

typedef struct {				// define a data structure to hold welder status data 
	byte mode;                  // for mode 
	int temperature;		  //for temperature 
	boolean sinus;			 // for zerocross 
	boolean fire;		    // for weld actuator 
	byte fanspeed;		   // for PWM value to drive fan motor
}inputData;				 // label it inputData

/*  declare an array for each of the weld timers and store an initial value for each of the 4 modes.
	The first postion is always null since mode 0 is continuous and does not use timed pulses */
int preWeldPulse[4] = { 0, 50, 50, 50 };     // array for duration of the pre-weld pulse time for each mode 
int weldPause[4] = { 0, 500, 500, 500 };	// array for the pause between pulses for each mode
int weldPulse[4] = { 0, 100, 200, 400 };		// array for duratrion of the main weld pulse for each mode
/*	declare an array to hold the mode names as strings for disoplay purposes*/
char* modeString[] = { "Continuous Weld" , " Pulse Weld 1" , " Pulse Weld 2" , " Pulse Weld 3" };

/* declare and initialize other global vars */
//int preWeld_ms = 50;	 //variable for first weld pulse in miliseconds
//int weldPause_ms = 500; //pause between pulses in miliseconds
byte colorR = 0;	   // variable for Red component of RGB backlight display
byte colorG = 0;      // variable for Green component of RGB backlight display
byte colorB = 255;   // variable for Blue component of RGB backlight display
inputData myData;   // variable to hold status data


//bool continuously = 0;	// global flag to indicate whether user desires continuous weld mode option

void setup()
{
	Serial.begin(9600);
	lcd.begin(16, 2);

	// set backlight color
	lcd.setRGB(colorR, colorG, colorB);

	/* setup pins*/
	
	pinMode(Rswitch1, INPUT_PULLUP); //rotory switch first click to the right
	pinMode(Rswitch2, INPUT_PULLUP); //rotory switch second click
	pinMode(Rswitch3, INPUT_PULLUP); //rotory switch third click

	/*weld button configured active low 
	uses a barrel jack connector for easy change of hardware
	(i.e thumb swith, foot pedal, limit switch, etc.) */
	pinMode(weldButtonPin, INPUT_PULLUP);

	/* for output to SCRs that control AC to the MOT
	(microwave oven transformer) modified with
	rewound secondary for high amperage output */
	pinMode(weldPin, OUTPUT);

	/* for power on indicator */
	pinMode(ledPin, OUTPUT);

	/* for reading zero cross of Alternating Current Sinus */
	pinMode(zeroCrossPin, INPUT);
	
	/* for the fan*/
	pinMode(fanPin, OUTPUT);

	/* END setup pins */
	
	//signal power up/reset
	blinkLed(ledPin, 2); // albert.h library call 
	digitalWrite(ledPin, 1); // power on indication
	
	/* check for continuous welding mode
	weldButton.poll(); //switch.h library call
	if (weldButton.on() & !Rswitch()) { //if weldButton is fired at switch position 0
		continuously = 1; // enable continuous welding - WARNING overheat danger!
		colorR = 255; colorG = 0; colorB = 0; // make backlight red
	}
	*/
	
	lcd.setRGB(colorR, colorG, colorB); // light the display

	//analogWrite(fanPin, MAXfan); //start pwm on fan at max

	/* populate the modeData structures with initial values*/

}
void loop()
{
	getStatus();								// read our sensors and switches
	while (myData.temperature >= MAXtemp){		// while OVERHEATED is true 
		myData.fanspeed = MAXfan;				// set fan speed go max
		analogWrite(fanPin, myData.fanspeed);   // run fan to cool the core
		shutDown();								// shutdown the weldeing transformer
		getStatus();							// read sensors again
	}
	if (myData.temperature < MINtemp){			// if temperature is below minimum threshold
		myData.fanspeed = 0;					// set fan speed to 0
		analogWrite(fanPin, myData.fanspeed);	// turn off the fan
	}
	if (myData.temperature >= MINtemp){			// if temperature is above minimum threshold
		myData.fanspeed = strainMap(myData.temperature, MINtemp, 300L, 50L, MAXfan); // set fan speed accoprding to temperature level
		analogWrite(fanPin, myData.fanspeed);	// run the fan to cool the core
	}
	
	switch (myData.mode)					//check what mode we aere in
	{
		case 0: {					// if mode is 0
					colorR = 255;	// make RGB backlight color red
					colorG = 0;
					colorB = 0;
					display(myData.mode);
					weld(weldButton.on()); // continuous welding 				
					break;
		}
		case 1: {
					colorR = 0;	
					colorG = 255;	// make RGB backlight color green
					colorB = 0;
					display(myData.mode);
					if (weldButton.pushed())
					{
						weldCyclus(myData.mode); // use timerz 
					}
					break;
		}
		case 2: {
					colorR = 0;
					colorG = 0;	
					colorB = 255;		// make RGB backlight color blue
					display(myData.mode);
					if (weldButton.pushed())
					{
						weldCyclus(myData.mode); // use timerz 
					}
					break;
		}
		case 3: {
					colorR = 255;
					colorG = 0;
					colorB = 255;		// make RGB backlight color purple
					display(myData.mode);
					if (weldButton.pushed())
					{
						weldCyclus(myData.mode); // use timerz 
					}
					break;
		}
	}
}
void shutDown()
{
	colorR = 155; //some red
	colorG = 165; //more green
	colorB = 2;  //and hardly any blue
	lcd.setRGB(colorR, colorG, colorB); //sets RGB backlight yellow
	lcd.clear();  //clear screen 
	lcd.setCursor(0, 0); //from top left corner of screen
	lcd.print("OVERHEATED "); //write the message
	lcd.print(myData.temperature); //write the temoerature
	lcd.print("*F"); //in Farenheight
	delay(1000); //wait a second
	lcd.setCursor(0, 4); //on the next row indented 4 spaces
	lcd.print("waiting..."); //write the message
	lcd.blink(); //lets blink for added attention
	delay(2000); //wait a couple seconds
}
void weldCyclus(int mode)
{
	sinusMax();
	pulseWeld(preWeldPulse[mode]);
	delay(weldPause[mode]);
	sinusMax();
	pulseWeld(weldPulse[mode]);
}
int Rswitch()
{
	byte mode;
	if (!digitalRead(Rswitch1)) //switched 1st position equals 1 
		mode = 1;
	else if (!digitalRead(Rswitch2)) //switched 2nd position equals 2
		mode = 2;
	else if (!digitalRead(Rswitch3)) //switched 3rd position equals 3
		mode = 3;
	else mode = 0;					// mode to 0 (off position)
	return mode;					//return the byte as mode
}
void pulseWeld(int ms)
{
	weld(1);
	delay(ms);
	weld(0);
	Serial << "            weld: " << ms << "ms" << endl;
}
void weld(bool b)
{
	digitalWrite(weldPin, b);  //send weld signal to SCRs or not
	lcd.setRGB(255, 255, 255); //flash white backlight while welding
	if (!b) lcd.setRGB(colorR, colorG, colorB); //revert backlight color
		
}
void sinusMax()
{
	                                     //to prevent inrush current, turn-on at the sinus max
	while (digitalRead(zeroCrossPin)); //AC sinus is above zero do nothing
	while (!digitalRead(zeroCrossPin)); //AC sinus is below zero do nothing
	delayMicroseconds(sinusMax_us); // just crossed zero at beginning of sinus wait until max
}
long strainMap(long strain, long inmin, long inmax, long outmin, long outmax) //constrains input before mapping
{
	if (strain <= inmin) return outmin;  // if input too low then return minimum range
	if (strain >= inmax) return outmax;  // if input too high then returm maximum range
	return (strain - inmin) * (outmax - outmin) / (inmax - inmin) + outmin; // return calculated output
}
int readTemperature()
{
	float temp = analogRead(thermPin); // read the voltage (0 to 1023 = 0 to 5v)
	Serial << "analog: " << temp;  // print the value
	// convert the value to the resistance of the thermistor (R1)
	// R1 = R2 * (Vin / Vout - 1)
	temp = 1023.0 / temp - 1; // (Vin / Vout - 1)
	temp =  1000.0 * temp; // R2 * (Vin / Vout -1)
	Serial << " resistance: " << temp << " Ohms"; //print the resistance
	// to solve for temperature use steinhart B parameter equation
	// 1 / T = 1 / To + 1 / B * ln(R / Ro) where To is room temperature in Kelvin
	// and B = 3986 for the 1k thermistor (marked 102)
	float steinhart; // we need another variable
	steinhart =  temp/1000.0; // (R / Ro)
	steinhart = log(steinhart); // ln(R / Ro)
	steinhart /= 3986.0; // 1/B * ln(R / Ro)
	steinhart += 1.0 / (25.0 + 273.15); // + (1 / To)
	steinhart = 1.0 / steinhart; // Invert
	steinhart -= 273.15; // convert to Celsius
	Serial << " / " << steinhart << "*C"; //print temperature in degrees celsius
	steinhart = (steinhart * 1.8) + 32.0; // convert to farenheit
	Serial << " / " << steinhart <<"*F"; // print temperature in degrees farenheit
	temp = int(steinhart); // cast as an integer
	Serial << " / " << temp << endl; // print the result we will return
	return (temp); // return the temperature (farenheit)
//	return (200); // for debug fan
//	return (351); // for debug testing shutDown()

}
void getStatus() {					// populate the myData status data structure
	weldButton.poll();
	myData.temperature = readTemperature();		// with the temperature
	myData.mode = Rswitch();			// with the mode
	myData.fire = weldButton.on();			// state of the weld actuator
	myData.sinus = digitalRead(zeroCrossPin);	// state of the MAINS sinus							
}

void displayStatus()
{
	getStatus(); //get the status data
	lcd.clear();					// clear lcd screen 
	lcd.setCursor(0, 0);			// from top left of LCD screen
	lcd.print("mode:");			// print message
	switch (myData.mode)
	{
		case 0:	// except if mode 0 which is continuos weld 
			lcd.print("Continuous ");   // print Continuous
			break;
		default:						// otherwise
		{
			lcd.print("Pulse (");		// print Pulse (
			lcd.print(preWeldPulse[myData.mode]);		// print preWeld pulse time
			lcd.print("/");				// print /
			lcd.print(weldPause[myData.mode]);	// print pause time
			lcd.print("/");				// print /
			lcd.print(weldPulse[myData.mode]);		// print weld pulse time
			lcd.print(") ");			// print )
			break;
		}
	}
	lcd.print("weld:");				// print message
	lcd.print(myData.fire);			// print weld actuator status
	lcd.print(" zc:");				// print message
	lcd.print(myData.sinus);		// print zerocross status
	lcd.setCursor(0, 1);			// on the next line
	lcd.print("Temperature:");			// print message
	lcd.print(myData.temperature);	// print temperature
	lcd.print("* Fan:");				// print message
	Serial << "fan speed:" << myData.fanspeed << endl; // print the value of fanspeed to serial window
	byte speed = map(myData.fanspeed, 0, 255, 0, 100); //convert the fan speed PWM value to percent
	lcd.print(speed);				//print the fanspeed
	lcd.print("%");					// as percentage
	delay(500);						// wait a half second
									// scroll to the left to read rest of nessage
	for (int positionCounter = 0; positionCounter < 15; positionCounter++) { //loop needed positions total
		lcd.scrollDisplayLeft(); // scroll left one position at a time
		delay(150);				 // wait a bit so its not a blur
	}
	delay(300);					// wait a litte so user can read screen
	Serial << "free ram: " << freeRam() << endl; // wrie available ram to serial window
}
void display(int mode) {
	lcd.setRGB(colorR, colorG, colorB); // light the display
	lcd.clear();						// clear the screen
	lcd.setCursor(0, 0);					// from top left
	lcd.print(modeString[mode]);					// print the mode
	lcd.setCursor(0, 1);				// on thenext line
	if (!mode) {
		lcd.print("Core Temp: ");
		lcd.print(myData.temperature);
		lcd.print("*F");
	}
	else {
		lcd.print(preWeldPulse[mode]);
		lcd.print(" / ");
		lcd.print(weldPause[mode]);
		lcd.print(" / "); 
		lcd.print(weldPulse[mode]);
	}
}
int freeRam() {
	extern int __heap_start, *__brkval;
	int v;
	return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}
