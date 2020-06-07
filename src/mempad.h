#ifndef mempad_h
#define mempad_h

//#define DEBUG_KEYPAD
//#define DEBUG_CIV

#define MEM_BANKS         ( byte ) 10     // how many memory banks
#define BANK_0            ( byte ) 0
#define BANK_1            ( byte ) 1
#define BANK_2            ( byte ) 2
#define BANK_3            ( byte ) 3
#define BANK_4            ( byte ) 4
#define BANK_5            ( byte ) 5
#define BANK_6            ( byte ) 6
#define BANK_7            ( byte ) 7
#define BANK_8            ( byte ) 8
#define BANK_9            ( byte ) 9

#define EPROM_SIGNATURE            0x6969 // magic number to write at EEPROM start
#define SER_SPEED                  9600   // serial baud rate
#define MAX_BANK_SIZE     ( byte ) 60+2   // how much we can store in each mem bank
#define STILL_WORKING     ( byte ) 0
#define DONE_WORKING      ( byte ) 1

#define FREQ_BUF_SIZE     ( byte ) 8      // frequency input buffer

#define CIV_START         ( byte ) 0xFE   // CIV codes
#define CIV_STOP          ( byte ) 0xFD
#define CIV_SEND_CW       ( byte ) 0x17
#define CIV_HALT_CW       ( byte ) 0xFF
#define CIV_PWR_OFF       ( byte ) 0x00
#define CIV_PWR_ON        ( byte ) 0x01
#define CIV_PWR_CMD       ( byte ) 0x18
#define CIV_CHG_FREQ      ( byte ) 0x05
#define CIV_SET_VFO       ( byte ) 0x07
#define CIV_SET_VFO_A     ( byte ) 0x00
#define CIV_SET_VFO_B     ( byte ) 0x01

#define ROWS              ( byte ) 4
#define COLS              ( byte ) 4
#define PWR_OFF_HOLD_TIME          1500   // poweroff keypad button hold time

// for pwr on, it's required to send a variable number of 0xFE, proportional to
// the baud rate:
// 115200 150 FEs (115200 / 150 = 768)
// 57600  75  FEs (...=768)
// 38400  50  FEs (...=768)
// 19200  25  FEs (...=768)
// 9600   13  FEs (...=738)
// 4800   7   FEs (...=685)
// we choose the lowest divider to accomodate the lowest baud rate (685)
#define CIV_PWR_FES       ( byte ) ( SER_SPEED / 685 )

#define ARD_ADDR          ( byte ) 0xE0    // default controller addr (can be anything?)
#define TRX_ADDR          ( byte ) 0x94    // radio addr (must match your radio setting)

#define SERIAL_RX         ( byte ) 12      // dig. pin 12 for soft serial RX
#define SERIAL_TX         ( byte ) 13      // dig. pin 13 for soft serial TX

// this is a mem. bank
typedef struct
{
    byte len;                              // in bytes, no null term.
    char msg[MAX_BANK_SIZE];               // CW message
} mem_bank;

typedef mem_bank * pmem_bank;              // pointer to mem_bank


#endif
