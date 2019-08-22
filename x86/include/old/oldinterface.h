/*
 * 
 * Filename: interface.h
 * 
 * Description: Some general definitions used for the user interface 
 * 
 * Author(s): Timothy Stonis, Andrew Bose
 * 
 * Copyright 1997, Cobalt Microserver, Inc.
 * 
 */


#define kBufferLen	0xA0
#define kPromptString	"Cobalt: "
#define kMaxArgs	0x10
#define kMaxArgLen	0x30

struct command_desc {
    char * cmd_help;
    char * cmd_name;
    int (* cmd_func)( int argc, char *argv[] );
}


// Function prototypes

int ParseAndDispatch( unsigned char *, int );
int StripArgs( unsigned char *, unsigned char [kMaxArgs][kMaxArgLen], int );
void UserInterface( void );
void ClearBuffer( unsigned char * );
int ReadInput( unsigned char * );
void ErrorOut( int );
void DumpHelp( void );

void RTCMenu(void);
void FlashMenu(void);
void MemoryMenu(void);
void LCDMenu(void);
void NotImp( void );


#define kByteAccess	0x02
#define kWordAccess	0x04
#define kLongAccess	0x08

#define	kBufOver	0x01
#define kNoCmd		0x02
#define kBadNum		0x03
#define kArgIgnore	0x04
#define kBadArg		0x05
#define kBadFLASH	0x06
#define kBadDate	0x07
#define kRBFail		0x08
#define kMaxErr		kBadDate

// Error code definitions
#define kErrors		{						\
  			"\r\nError:  Unknown error\r\n",		\
  			"\r\nError:  Too many characters\r\n",		\
  			"\r\nError:  Command not found\r\n",		\
			"\r\nError:  Bad number of arguments\r\n",	\
			"\r\nError:  Extra arguments ignored\r\n",	\
			"\r\nError:  Bad argument\r\n",			\
			"\r\nError:  Bad FLASH image\r\n",		\
			"\r\nError:  Bad date string\r\n",		\
			"\r\nError:  Failed readback test\r\n",		\
  			  }


#ifdef _dslkajdflheuisdhfaljksdhfskdf_

// Available commands (make sure the list is terminated with "")
// Outdated commands (such as rfm) are prefixed with "__"
#define kCommands	{ "?",					\
			  "rb", "rw", "rl",			\
			  "wb", "ww", "wl",			\
			  "dm", "readrtc","writertc",		\
			  "loadkernel",				\
			  "boot",				\
			  "rm",					\
			  "wm",					\
			  "wrm",				\
			  "__rfm",				\
			  "__bfr",				\
			  "bfd",				\
 			  "lcd", "lcdr", "lcdw",                \
                          "br",                                 \
                          "exp",                                \
                          "__cop",                              \
			  "1","2","3","4","5","6","7","8","9",  \
			  "10","11","12","13","14","15","16",	\
			  "/",					\
			  "reset",				\
			  "echip","esector","prog","burnit",	\
			  "setbfd","clearbfd",                  \
                          "mfg",                                \
                          "setlcd","clearlcd",                  \
			  "names",				\
			  "lspci",				\
			  "read_pci_cfg",			\
			  "write_pci_cfg",			\
			  "read_pci_cfg_dev",			\
			  "write_pci_cfg_dev",			\
			  ""					\
			}
#define kCommandCnt	57
// Help screen
#define	kHelpScreen	{										\
			"?                       - This menu\r\n",					\
			"r[b,w,l] address        - Read byte, word, or long from address\r\n", 		\
			"w[b,w,l] address data   - Write byte, word, or long data to address\r\n", 	\
			"dm address n            - Display n bytes of memory from address\r\n",    	\
			"readrtc                 - Read time and date from the RTC\r\n",    		\
			"writertc YYMMDDWHHmmSS  - Write time and date to the RTC\r\n",    		\
			"                          YY: Year      MM: Month   W: Day of week (Sunday = 1)\r\n",    \
			"                          HH: Hour (24) mm: Minute SS: Second\r\n",    	\
			"loadkernel              - load kernel image from serial port\r\n",    		\
			"boot                    - jump into loaded kernel\r\n",    			\
			"rm address              - loop read from address (reset to kill)\r\n",    	\
			"wm address data         - loop write to address (reset to kill)\r\n",    	\
			"wrm address data        - loop write/read from address (reset to kill)\r\n",   \
			"bfd                     - boot kernel from DISK image\r\n",    		\
			"lcd [on,off,reset,clear]- lcd on, off, reset, clear \r\n",                     \
                        "lcdr address            - Read DD RAM from address\r\n",                       \
                        "lcdw address data       - Write byte to DD RAM address\r\n",                   \
                        "br                      - Read the buttons\r\n",                               \
                        "exp                     - Test the PCI Expansion Slot\r\n",                    \
			"setbfd, clearbfd        - Set or clear flag to bfd next reset\r\n",            \
			"setlcd, clearlcd        - Set or clear flag to lcd test next reset\r\n",            \
			"lspci                   - list pci devices\r\n",            \
			"read_pci_cfg bus dev reg size (1,2,4) - read a cobfug byte form pci bus\r\n",            \
			"write_pci_cfg bus dev reg val size (1,2,4) - write a cobfug byte form pci bus\r\n",            \
                        "reset                   - Reset Board\r\n"                               \
			}
			
#define kHelpLines	25	






#define kMemScreen     {                                                      				\
			"								\r\n",		\
			"Memory Menu							\r\n",		\
			"								\r\n",		\
                        "r[b,w,l] address        - Read byte, word, or long from address\r\n",		\
                        "w[b,w,l] address data   - Write byte, word, or long data to address\r\n",	\
                        "dm address n            - Display n bytes of memory from address\r\n",		\
                        "rm address              - loop read from address (reset to kill)\r\n",		\
                        "wm address data         - loop write to address (reset to kill)\r\n",		\
                        "wrm address data        - loop write/read from address (reset to kill)\r\n",	\
                        "/                       - run through all 'c' diagnostics\r\n"			\
			 }
                        
#define kMemLines      10      



#define kLCDScreen	{										\
			"								\r\n",		\
			"LCD Menu							\r\n",		\
			"								\r\n",		\
                        "lcd [on,off,reset,clear]- lcd on, off, reset, clear \r\n",			\
                        "lcdr address            - Read DD RAM from address\r\n",			\
                        "lcdw address data       - Write byte to DD RAM address\r\n",			\
                        "br                      - Read the buttons\r\n",  				\
                        "/                       - run through all 'c' diagnostics\r\n"			\
			}

#define kLCDLines	8


#define kRTCScreen	{										\
			"								\r\n",		\
			"RTC Menu							\r\n",		\
			"								\r\n",		\
			"readrtc                 - Read time and date from the RTC\r\n",    		\
			"writertc YYMMDDWHHmmSS  - Write time and date to the RTC\r\n",    		\
			"                          YY: Year      MM: Month    W: Day of week (Sunday = 1)\r\n",    \
			"                          HH: Hour (24) mm: Minute  SS: Second\r\n",    	\
                        "/                       - run through all 'c' diagnostics\r\n"			\
			}
	
#define kRTCLines 	8	


#define kFlashScreen	{										\
			"								\r\n",		\
			"Flash Menu							\r\n",		\
			"								\r\n",		\
			"echip                   - Chip Erase \r\n",    		\
			"esector SA              - Sector Erase sector (SA)\r\n",    		\
			"burnit                  - Burn flash from serial port\r\n",    		\
			"prog address data       - Program byte data at address \r\n",    		\
                        "/                       - run through all 'c' diagnostics\r\n"			\
			}

#define kFlashLines 	8	

#endif


// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
