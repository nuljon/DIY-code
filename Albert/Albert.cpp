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

#include <avr/pgmspace.h>
#include <Arduino.h>
#include "Albert.h"

// written by David A. Mellis. Based on code by Rob Faludi http://www.faludi.com
int availableRAM() 
{ int size = 2048; 
  byte *buf;
  while ((buf = (byte *) malloc(--size)) == NULL);
  free(buf);
  return size;
}

void openDrain(byte pin, bool value)
{ if(value) pinMode(pin, INPUT);
  else pinMode(pin, OUTPUT); 
  digitalWrite(pin, LOW);  
}

void blinkLed(byte pin, int n)
{ pinMode(pin, OUTPUT); 
  for(byte i=0; i<n; i++)
  { digitalWrite(pin, HIGH);       
    delay(300);                 
    digitalWrite(pin, LOW);   
    delay(300);
  }
}

void maxLoops(const unsigned long loops)
{ static unsigned long i=0;
  if(++i > loops) while(1);
}


