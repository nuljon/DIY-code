/* 
Albert.cpp Arduino library Albert
Version 17-12-2015

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program. If not, see http://www.gnu.org/licenses.

Version 17-12-2015 analogReadfast
*/

#ifndef ALBERT_H
#define ALBERT_H

#define arrayLenght(array) sizeof(array)/sizeof(array[0]) // compile-time calculation

int availableRAM(); 
void openDrain(byte pin, bool value);
void blinkLed(byte pin, int n=3);
void maxLoops(const unsigned long loops);

unsigned inline adcIn(bool &overflow, const byte adcPin, const int samples) // inline library functions must be in header
{ unsigned long adcVal = 0;
  for(int i=0; i<samples; i++)
  { unsigned temp = analogRead(adcPin);
    adcVal += temp;
    overflow = ((temp >= 1023) | (temp <= 0)); 
  }
  adcVal /= samples;
  return adcVal;
}

unsigned inline adcIn(const byte adcPin, const int samples) // inline library functions must be in header
{ unsigned long adcVal = 0;
  for(int i=0; i<samples; i++) adcVal += analogRead(adcPin);
  adcVal /= samples;
  return adcVal;
}

template<class T> 
inline Print &operator ,(Print &stream, const T arg) 
{ stream.print(" ");
  stream.print(arg); 
  return stream; 
}

int inline analogReadfast(byte ADCpin, byte prescalerBits=4) // inline library functions must be in header
{ byte ADCSRAoriginal = ADCSRA; 
  ADCSRA = (ADCSRA & B11111000) | prescalerBits; 
  int adc = analogRead(ADCpin);  
  ADCSRA = ADCSRAoriginal;
  return adc;
}

#endif


