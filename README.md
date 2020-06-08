# ICOM IC-7300 Arduino CW memory pad

**Arduino CW memory pad for IC7300, v. 1.0**
-----------------------------------------
Copyright (c) 2020 Adrian Petrila, YO3GFH
TNX for original idea by Adrian Florescu, YO3HJV

http://yo3hjv.blogspot.com/2020/04/cw-and-voice-memory-keyer-for-icom-ic.html

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

**Features**
--------
* 10 memory banks (0-9) , 60 bytes each, saved in EEPROM.
  You can extend these, but don't forget you only have 1k of EEPROM :-)
  Send any mem bank by pressing the associate key (0-9) on the keypad.
  Press 'C' at any time to abort current message.
  To program the mem banks, use any terminal app (putty, teraterm, etc.) to connect
  to device.

* Power on (`*` key) and power off (`#` key, hold for 1.5 sec.).

* Switch VFOs ( set VFO A with 'A' key, set VFO B with 'B' key ).

* Frequency input mode - hold 'D' key for 1.5 sec. to enter this mode,
  hold it again for 1.5 sec. to return to mem pad mode.
  This mode allows for changing the operating frequency on the transceiver.
  Enter the desired freq. as 8 digits (mandatory), such as 03 51 02 03 (3510.203 kHz)
  or 10 10 12 12 (10101.212 kHz) followed by the '#' key.

This was done with an Arduino Uno board and a 4x4 keypad; if you wish to add a display
(and other stuff), a Mega would be needed probably, since you'll need more GPIO pins.

It's taylored to my own needs, modify it to suit your own. I'm not a professional programmer,
so this isn't the best code you'll find on the web, you have been warned :-))

All the bugs are guaranteed to be genuine, and are exclusively mine =)
