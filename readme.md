# Yet Another DIY SpotWelder
created in June of 2016 by Jon Nuljon & Norman Olds Jr.

The hardware is inspired by Albert van Dalen's designs. The software is a heavily modified adaptation of his original code: [Arduino spot welder controller / solid state relay][bb3b1a05]

  [bb3b1a05]: http://www.avdweb.nl/arduino/hardware-interfacing/spot-welder-controller.html "van Dalen's Welder"

Website with project details of our build is [here.][ad17f465]

  [ad17f465]: http://nuljon.com/wp/index.php/projects-for-living/spot-welder/ "MOT Spot Welder Project"

  ## This software controls a resistance welder with these functions:
- Conitnuous mode - the welding current is maintained until release of the weld actuator, which could be a button, foot pedal, limit switch, or whatever.
- Dual Pulse mode - two weld pulses for each actuation that have user configurable timing for:
  1.   pre-weld pulse
  2.   pause between pulses
  3. main weld pulse
- Zero-cross detection for weld pulse start time calculation that begins at peak power and avoids unnecessary inrush currents.
- Overheat protection upon transformer core reaching user defined temperatures with intitial default values:
  1. fan begins to spin at 150^ farenheit and is full on at 300^
  2. weld current cutoff happens at 350^ farenheit
- LCD display of mode and status
  - []  Menu control program interface ... coming soon
- onboard conversion of MAINS Powere
  1. 110V / 10A to approx. 2v / 500A weld current AC
  2. 110V AC to 12V and 5V DC for control and display

## Hardware Description
 [Our welder][4a1f0bcd] is built by constructing a form with locking arms in a pincer fashion which provide for electrodes to be placed either adjacent (for battery tab welding) or in opposition (for welding sheet metal enclosures). This mechanism is welded to a solid base that can support the arms and the heavy core of a microwave oven transformer with rewound secondary coil. The welder is rigged with a power cord, switch, and fuse suitable for mains power connection to the primary coil. The secondary coil is a single wind of heavy copper cable and connected to the control circuitry consisting of:
  1. two opposed SCRs that pass the AC weld current
  2. various switches for mode select and menu control
  3. a thermistor and resistor acting as a voltage divider to detect the temperarure
  4. a transistor to drive a DC fan motor for cooling
  5. a Grove RGB backlight LCD display to give visual signals (resitance welders should not throw an arc)
  6. a barrel connector to receive preferred actuator switch
  7. a transformer and rectifier circuit to provide DC power
  8. a 1 megaohm resistor to detect AC zero-crossings
  9. an Arduino Pro-mini micro controler board to rule the the whole pile and run the code.

  [4a1f0bcd]: http://nuljon.com/wp/index.php/projects-for-living/spot-welder/spot-welder-v3-0-build/ "DIY_SpotWelder_v3-0_build"

You can find a schematic for the control circuit [here.][4871cd44]

  [4871cd44]: https://www.dropbox.com/s/4k4nrjpkoesbhgz/SpotWelder%20v3.0.pdf?dl=0 "DIY Spotwelder v3.0"


The program, DIY_SpotWelder.ino, is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License at http://www.gnu.org/licenses .
