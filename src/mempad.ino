/*
    Arduino CW memory pad for IC7300, v. 1.0
    -----------------------------------------
    Copyright (c) 2020 Adrian Petrila, YO3GFH
    TNX for original idea by Adrian Florescu, YO3HJV - http://yo3hjv.blogspot.com/2020/04/cw-and-voice-memory-keyer-for-icom-ic.html

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

    Features
    --------
    - 10 memory banks (0-9) , 60 bytes each, saved in EEPROM.
      You can extend these, but don't forget you only have 1k of EEPROM :-)
      Send any mem bank by pressing the associate key (0-9) on the keypad.
      Press 'C' at any time to abort current message.
      To program the mem banks, use any terminal app (putty, teraterm, etc.) to connect
      to device.

    - Power on ('*' key) and power off ('#' key, hold for 1.5 sec.).

    - Switch VFOs ( set VFO A with 'A' key, set VFO B with 'B' key ).

    - Frequency input mode - hold 'D' key for 1.5 sec. to enter this mode,
      hold it again for 1.5 sec. to return to mem pad mode.
      This mode allows for changing the operating frequency on the transceiver.
      Enter the desired freq. as 8 digits (mandatory), such as 03 51 02 03 (3510.203 kHz)
      or 10 10 12 12 (10101.212 kHz) followed by the '#' key.

    This was done with an Arduino Uno board and a 4x4 keypad; if you wish to add a display
    (and other stuff), a Mega would be needed probably, since you'll need more GPIO pins.

    It's taylored to my own needs, modify it to suit your own. I'm not a professional programmer,
    so this isn't the best code you'll find on the web, you have been warned :-))

    All the bugs are guaranteed to be genuine, and are exclusively mine =)
*/

#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <Keypad.h>
#include <ctype.h>
#include "mempad.h"

//
// FUNCTION PROTOTYPES (not really needed, just very old habits :-))
//

byte process_input         ( const byte rx, char * buf, const byte len );
byte ascii_to_hex          ( const char c );
byte char_to_num           ( const char c );
byte hex_to_byte           ( const char * buf );

void aq_freq_digits        ( char * buf, KeypadEvent key );

void cw_keypad_event       ( KeypadEvent key );
void process_keypad        ( KeyState state, KeypadEvent key, Stream * ser );
void handle_key_press      ( KeypadEvent key, Stream * ser );
void handle_key_hold       ( KeypadEvent key, Stream * ser );

byte read_banks_from_eprom ( pmem_bank pbank, word nbytes );
void write_banks_to_eprom  ( pmem_bank pbank, word nbytes );
void list_mem_banks        ( pmem_bank pbank, Stream * ser );

void civ_send_prologue     ( const byte trx_addr, const byte ard_addr, Stream * ser );
void civ_send_epilogue     ( Stream * ser );
void civ_send_cw           ( const byte c, Stream * ser );
void civ_start_cw          ( Stream * ser );
void civ_halt_cw           ( Stream * ser );
void civ_trx_pwr_on        ( Stream * ser );
void civ_trx_pwr_off       ( Stream * ser );
void civ_change_freq       ( const char * buf, Stream * ser );
void civ_set_vfo           ( const byte vfo, Stream * ser );

void send_buf_cw           ( const char * buf, const byte len, Stream * ser );
void send_mem_bank         ( pmem_bank pbank, const byte bank_idx, Stream * ser );

void m_wait                ( unsigned long msec );

//
// GLOBALS
//

// this is the serial port for CIV communication
SoftwareSerial civ_port ( SERIAL_RX, SERIAL_TX );

// for ascii_to_hex() and other
byte ascii_table[128] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f
};

// for the 4x4 keypad, change it if using 3x4 or anything else
char keys_table[ROWS][COLS] = {
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' }
};

// pin assignment, change according to your setup
byte row_pins[ROWS]                   = { 5, 4, 3, 2 };
byte col_pins[COLS]                   = { 9, 8, 7, 6 };

Keypad cw_keypad                      = Keypad ( makeKeymap ( keys_table ), row_pins, col_pins, ROWS, COLS );

// 10 membanks
mem_bank messages[MEM_BANKS]          = {};
byte     index                        = 0;

// this will hold the operating frequency
char     freq_buf[FREQ_BUF_SIZE]      = {};
byte     freq_index                   = FREQ_BUF_SIZE;
// we're in mem pad mode by default
bool     freq_mode                    = false;

void setup()
/****************************************************************************/
/* initialization                                                           */
{
  // initialize serial ports
    Serial.begin ( SER_SPEED );                      // USB serial (port 0)
    civ_port.begin ( SER_SPEED );                    // software serial port
    while ( !Serial ) { ; }                          // let it settle
    while ( !civ_port ) { ; }
    m_wait ( 2 );
    civ_port.flush();
    cw_keypad.addEventListener ( cw_keypad_event );  // add keypad event handler
    cw_keypad.setHoldTime ( KEY_HOLD_TIME );         // set the keypad holdtime

    // help banner
    Serial.println ( F("**************************************************************************") );
    Serial.println ( F("***                  Welcome to IC 7300 CW memory pad!                 ***") );
    Serial.println ( F("***              (c) 2020 YO3GFH, original idea by YO3HJV              ***") );
    Serial.println ( F("**************************************************************************") );
    Serial.println ( F("***              There are 10 memory banks (0 to 9) for CW             ***") );
    Serial.println ( F("***    In order to program these, simply enter bank # followed by a    ***") );
    Serial.println ( F("***          space and then the message to send (60 chars max.)        ***") );
    Serial.println ( F("***               Hit Enter to save the message in EEPROM              ***") );
    Serial.println ( F("**************************************************************************") );
    Serial.println ( F("***                  KEYPAD FUNCTIONS (CW mem pad mode)                ***") );
    Serial.println ( F("**************************************************************************") );
    Serial.println ( F("***             Keys '0' to '9': send associated memory bank           ***") );
    Serial.println ( F("***                 Key 'A', 'B': switch between VFO A/B               ***") );
    Serial.println ( F("***                   Key 'C': cancel current message                  ***") );
    Serial.println ( F("***                  Key '*': power on the transceiver                 ***") );
    Serial.println ( F("***        Key '#': power off the transceiver (hold for 1.5 sec.)      ***") );
    Serial.println ( F("**************************************************************************") );
    Serial.println ( F("***      KEYPAD FUNCTIONS (freq. input mode, hold 'D' for 1.5 sec.)    ***") );
    Serial.println ( F("**************************************************************************") );
    Serial.println ( F("***     Keys '0' to '9': enter desired frequency, e.g. 3510.202 kHz    ***") );
    Serial.println ( F("***     would be 03 51 02 02 on the keypad, followed by the '#' key    ***") );
    Serial.println ( F("***       Hold 'D' for 1.5 sec. again to return to CW mem pad mode     ***") );
    Serial.println ( F("**************************************************************************") );
    Serial.println();

    // try to read banks from EEPROM, write empty if first run
    if ( !read_banks_from_eprom ( messages, sizeof ( messages ) ) )
    {
        Serial.println ( F("No valid configuration found, writing new one...") );
        write_banks_to_eprom ( messages, sizeof ( messages ) );
        Serial.println ( F("...done!") );
    }
    else
    {
        Serial.println ( F("Valid configuration found, listing memory banks...") );
        Serial.println();
    }

    // list mem banks
    list_mem_banks ( messages, &Serial );
}

void loop()
/****************************************************************************/
/* main loop                                                                */
{
    const char    *  p;
    byte             i           = 0;
    byte             len         = 0;
    byte             rx_byte     = 0;               // stores received byte
    char             buff        [MAX_BANK_SIZE+1]; // stores string
    byte             msg_ind     = 0;
    char             key;

    // are we connected to the terminal?
    if ( Serial.available() )
    {
        // get byte from USB serial port (terminal)
        rx_byte = Serial.read();

        // process until enter key pressed
        if ( process_input ( rx_byte, buff, MAX_BANK_SIZE ) == DONE_WORKING )
        {
            // a bit of sanity checks
            if ( ( isdigit ( buff[0] ) ) && ( buff[1] == 0x20 ) )
            {
                // make index from ascii char
                msg_ind = ascii_table[( byte )buff[0]] - 0x30;
                p = buff;
                if ( ( msg_ind <= BANK_9 ) && ( msg_ind >= BANK_0 ) )
                {
                     len = strlen ( buff ) - 2;
                     messages[msg_ind].len = len;
                     p += 2;
                     for ( i = 0; i <= len; i++ )
                     {
                         messages[msg_ind].msg[i] = *p++;
                     }
                     // bank all set, now write to EEPROM
                     write_banks_to_eprom ( messages, sizeof ( messages ) );
                     Serial.print ( F("Success! ") );
                     Serial.print ( len );
                     Serial.print ( F(" bytes written to memory bank ") );
                     Serial.print ( msg_ind );
                     Serial.println();
                }
            }
        }
    }
    // check for any keypad event
    key = cw_keypad.getKey();

#ifdef DEBUG_KEYPAD
    Serial.println ( key );
#endif

#ifdef DEBUG_CIV
    // check for data byte on CIV serial port
    if ( civ_port.available() )
    {
        // get a byte from CIV serial port
        rx_byte = civ_port.read();
        // send the byte to the USB serial port
        Serial.print ( rx_byte, HEX );
        Serial.print (" ");
        if ( rx_byte == CIV_STOP )
        {
            Serial.println ();
        }
    }
#endif
}

//
// IMPLEMENTATION SECTION
//

byte process_input ( const byte rx, char * buf, const byte len )
/****************************************************************************/
/* take a byte , stuff it into buf, max len size                            */
{
    byte status = STILL_WORKING;

    switch ( rx )
    {
        case 13:                          // enter key
            buf[index] = 0;               // null terminate buffer
            Serial.println();
            index = 0;
            status = DONE_WORKING;
            break;

        case 127:                        // bkspace
        case 8:
           if ( index > 0 ) { index-- ; }
           Serial.write ( rx );          // echo to terminal
           break;

        default:
           buf[index] = rx;              // copy byte to buffer
           if ( index < len )
           {
               index++ ;
               Serial.write ( rx );      // echo to terminal
           }
           break;
     }
    return status;
}

void write_banks_to_eprom ( pmem_bank pbank, word nbytes )
/*****************************************************************************/
/* write mem banks to EEPROM, nbytes size                                    */
{
    // Arduino EEPROM is accesible byte by byte, so we need a typecast
    const byte * p = ( const byte * )( const void * )pbank;
    word i, ee;

    EEPROM.write ( 0, 0x69 );            // write magic signature
    EEPROM.write ( 1, 0x69 );

    ee = 2;                              // skip signature :-)
    for ( i = 0; i < nbytes; i++ )
    {
        EEPROM.write ( ee++, *p++ );
    }
}

byte read_banks_from_eprom ( pmem_bank pbank, word nbytes )
/*****************************************************************************/
/* read mem banks form EEPROM, nbytes size                                   */
{
    word signature = 0;
    // hack to read 2 bytes into a word
    byte * ptr = ( byte * )( void * )&signature;
    // Arduino EEPROM is accesible byte by byte, so we need a typecast
    byte * p = ( byte * )( void * )pbank;
    word i, ee;

    ptr[0] = EEPROM.read ( 0 );                   // one byte...
    ptr[1] = EEPROM.read ( 1 );                   // and another

  // no good signature found, exit
    if ( signature != EPROM_SIGNATURE ) { return 0; }

    ee = 2;                                       // skip signature
    for ( i = 0; i < nbytes; i++ )
    {
        *p++ = EEPROM.read ( ee++ );
    }
    return 1;
}

void list_mem_banks ( pmem_bank pbank, Stream * ser )
/*****************************************************************************/
/* list the existing mem banks                                               */
{
    if ( ( pbank == 0 ) || ( ser == 0 ) ) { return; }

    ser->println();

    for ( byte i = 0; i < MEM_BANKS; i++ )
    {
        ser->print ( F("Bank #") );
        ser->print ( i );
        ser->print ( F(": ") );

        if ( pbank[i].len != 0 )
        {
            ser->print ( ( const String& ) ( pbank[i].msg ) );
            ser->print ( F(" - ") );
            ser->print ( pbank[i].len );
            ser->print ( F (" of 60 bytes") );
        }
        ser->println();
    }
}

byte ascii_to_hex ( const char c )
/*****************************************************************************/
/* convert ascii char to hex for sending to radio                            */
{
    byte idx;

    idx = c % 128;                           // wrap around just in case :-)
    return ascii_table[idx];                 // index the table by ASCII value
}

byte char_to_num ( const char c )
/*****************************************************************************/
/* convert ascii char to the represented number (including hex)              */
{
    byte num;

    if ( ( c >= '0' ) && ( c <= '9' ) )
    {
        num = c - 0x30;
    }
    else
    {
        switch ( c )
        {
            case 'A': case 'a': num = 10; break;
            case 'B': case 'b': num = 11; break;
            case 'C': case 'c': num = 12; break;
            case 'D': case 'd': num = 13; break;
            case 'E': case 'e': num = 14; break;
            case 'F': case 'f': num = 15; break;
            default: num = 0;
        }
    }
    return num;
}

byte hex_to_byte ( const char * buf )
/*****************************************************************************/
/* convert a 2 digit hex number stored in buf to decimal value               */
{
    byte ln, hn;

    hn = char_to_num ( buf[0] );
    ln = char_to_num ( buf[1] );

    return ( ( hn * 16 ) + ln );
}

void civ_send_prologue ( const byte trx_addr, const byte ard_addr, Stream * ser )
/*****************************************************************************/
/* open CIV communication with the radio                                     */
{
    ser->write ( CIV_START );
    ser->write ( CIV_START );
    ser->write ( trx_addr );
    ser->write ( ard_addr );
}

void civ_send_epilogue ( Stream * ser )
/*****************************************************************************/
/* close CIV communication with the radio                                    */
{
    ser->write ( CIV_STOP );
    ser->flush();
}

void civ_send_cw ( const byte c, Stream * ser )
/*****************************************************************************/
/* send a CW char                                                            */
{
    ser->write ( ascii_to_hex ( c ) );
    m_wait ( 2 );
}

void civ_halt_cw ( Stream * ser )
/*****************************************************************************/
/* halt cw sending                                                           */
{
    civ_start_cw ( ser );
    ser->write ( CIV_HALT_CW );
    civ_send_epilogue ( ser );
}

void civ_start_cw ( Stream * ser )
/*****************************************************************************/
/* tell ICOM we want to send CW                                              */
{
    civ_send_prologue ( TRX_ADDR, ARD_ADDR, ser );
    ser->write ( CIV_SEND_CW );
}

void send_buf_cw ( const char * buf, const byte len, Stream * ser )
/*****************************************************************************/
/* send a buffer of len chars as CW                                          */
{
    byte i;

    if ( len == 0 ) { return ; }

    civ_start_cw ( ser );

    for ( i = 0; i < len; i++ )
    {
        // 30 chars is the max that one can send in a CIV CW command
        // so, if the buffer is longer, stop at 30 and resume right away
        if ( ( i != 0 ) && ( i % 29 == 0 ) )
        {
            civ_send_epilogue ( ser );
            civ_start_cw ( ser );
        }
        civ_send_cw ( buf[i], ser );
    }
    civ_send_epilogue ( ser );
}

void send_mem_bank ( pmem_bank pbank, const byte bank_idx, Stream * ser )
/*****************************************************************************/
/* send a mem bank, as selected by bank_idx                                  */
{
    char * buf;
    byte len;

    if ( bank_idx > MEM_BANKS-1 ) { return; }
    len = pbank[bank_idx].len;
    if ( len == 0 ) { return ; }
    buf = pbank[bank_idx].msg;

    send_buf_cw ( buf, len, ser );
}

void m_wait ( unsigned long msec )
/*****************************************************************************/
/* non-blocking delay                                                        */
{
    unsigned long now;

    now = millis();
    while ( millis() < ( now + msec ) ) { ; }
}

void civ_trx_pwr_on ( Stream * ser )
/*****************************************************************************/
/* power-on the transceiver                                                  */
{
    byte i = 0;

    // send the corect no. of 0xFE, as required by the serial baud rate
    while ( i < CIV_PWR_FES )
    {
        ser->write ( CIV_START );
        i++;
    }

    civ_send_prologue ( TRX_ADDR, ARD_ADDR, ser );
    ser->write ( CIV_PWR_CMD );
    ser->write ( CIV_PWR_ON );
    civ_send_epilogue ( ser );
}

void civ_trx_pwr_off ( Stream * ser )
/*****************************************************************************/
/* power-off the transceiver                                                 */
{
    civ_send_prologue ( TRX_ADDR, ARD_ADDR, ser );
    ser->write ( CIV_PWR_CMD );
    ser->write ( CIV_PWR_OFF );
    civ_send_epilogue ( ser );
}

void civ_change_freq ( const char * buf, Stream * ser )
/*****************************************************************************/
/* change operating freq.                                                    */
/* you need to send freq directly as hex, i.e. 21 MHz would be 0x21          */
/* each element (mhz, khz, hz and fraction) hold 2 bytes in the buffer,      */
/* 8 bytes total. we take each 2 byte element and convert it to decimal      */
{
    byte mhz, khz, hz, f_hz;

    mhz  = hex_to_byte ( buf + 6 );
    khz  = hex_to_byte ( buf + 4 );
    hz   = hex_to_byte ( buf + 2 );
    f_hz = hex_to_byte ( buf );

    civ_send_prologue ( TRX_ADDR, ARD_ADDR, ser );
    ser->write ( CIV_CHG_FREQ );
    ser->write ( f_hz );
    ser->write ( hz );
    ser->write ( khz );
    ser->write ( mhz );
    civ_send_epilogue ( ser );
}

void civ_set_vfo ( const byte vfo, Stream * ser )
/*****************************************************************************/
/* set VFO A/B                                                               */
{
    civ_send_prologue ( TRX_ADDR, ARD_ADDR, ser );
    ser->write ( CIV_SET_VFO );
    ser->write ( vfo );
    civ_send_epilogue ( ser );
}

void cw_keypad_event ( KeypadEvent key )
/*****************************************************************************/
/* event handler for the keypad, installed during setup()                    */
{
    // execute our custom function
    process_keypad ( cw_keypad.getState(), key, &civ_port );
}

void process_keypad ( KeyState state, KeypadEvent key, Stream * ser )
/*****************************************************************************/
/* called by the keypad event handler                                        */
{
    switch ( state )
    {
        case PRESSED:
            handle_key_press ( key, ser ); // process key press
            break;

        case RELEASED:                     // add code for key release
            break;

        case HOLD:
            handle_key_hold ( key, ser );  // process key hold, for poweroff
            break;

        case IDLE:                         // add code for idle :-)
            break;
    }
}

void handle_key_press ( KeypadEvent key, Stream * ser )
/*****************************************************************************/
/* what to do on key press                                                   */
{
    byte msg_ind, i;

    if ( key == NO_KEY ) { return; }

    switch ( key )
    {
        case 'C':
            civ_halt_cw ( ser );          // cancel message
            break;

        case 'A':
            civ_set_vfo ( CIV_SET_VFO_A, ser ); // switch to VFO A
            break;

        case 'B':
            civ_set_vfo ( CIV_SET_VFO_B, ser ); // switch to VFO B
            break;

        case 'D':
            break;

        case '*':
            civ_trx_pwr_on ( ser );       // power on the box
            break;

        case '#':                         // change operating frequency
            if ( ( freq_mode ) && ( freq_index == 0 ) )
            {
                civ_change_freq ( freq_buf, &civ_port );
                freq_index = FREQ_BUF_SIZE;
            }
            break;

        default:
            if ( !freq_mode )             // check what mode we're in
            {
                // wrap around :-)
                i = key % 128;
                // make an index from 0 to 9 directly from ascii value
                msg_ind = ascii_table[i] - 0x30;
                if ( ( msg_ind <= BANK_9 ) && ( msg_ind >= BANK_0 ) )
                {
                    // send the cw message
                    send_mem_bank ( messages, msg_ind, ser );
                }
            }
            else
            {
                // accumulate freq digits in buffer
                aq_freq_digits ( freq_buf, key );
            }
            break;
    }
}

void handle_key_hold ( KeypadEvent key, Stream * ser )
/*****************************************************************************/
/* what to do on keyhold (pressing a key for more than PWR_OFF_HOLD_TIME)    */
{
    switch ( key )
    {
        case '#':
            // power off our box after 1.5 sec. of key press
            civ_trx_pwr_off ( ser );
            break;

        case 'D':
            // toggle frequency input mode
            freq_mode = !freq_mode;
            break;
    }
}

void aq_freq_digits ( char * buf, KeypadEvent key )
/*****************************************************************************/
/* accumulate 8 digits from the keypad into the buffer, from end to start    */
/* 3510.202 kHz -> input 03 51 02 02 -> buffer 02 02 51 03                   */
{
    byte i;

    if ( freq_index == 0 ) { return; }            // are we done?

    if ( freq_index & 1 ) { i = freq_index; }     // odd index
    else { i = freq_index - 2; }                  // even index

    buf[i] = key;                                 // copy to buffer
    freq_index--;                                 // next byte down
}
