/* SC16IS750 I2C or SPI to UART bridge 
 *   v0.1 WH, Nov 2013, Sparkfun WiFly Shield code library alpha 0 used as example, Added I2C I/F and many more methods.
 *                      https://forum.sparkfun.com/viewtopic.php?f=13&t=21846
 *   v0.2 WH, Feb 2014, Added Doxygen Documentation, Added Hardware Reset pin methods.
 *   v0.3 WH, Dec 2014, Added support for SC16IS752 dual UART. 
 *   v0.4 WH, Dec 2014, Added Repeated Start for I2C readRegister(). Set I2C clock at 100kb/s. Fixed and added some comments. 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef _SC16IS750_H
#define _SC16IS750_H


#include "Stream.h"
#include "SerialBase.h"

//I2C Slaveaddresses                     A1  A0 
#define SC16IS750_SA0            0x90  /* VDD VDD */
#define SC16IS750_SA1            0x92  /* VDD VSS */
#define SC16IS750_SA2            0x94  /* VDD SCL */
#define SC16IS750_SA3            0x95  /* VDD SDA */
#define SC16IS750_SA4            0x98  /* VSS VDD */
#define SC16IS750_SA5            0x9A  /* VSS VSS */
#define SC16IS750_SA6            0x9C  /* VSS SCL */
#define SC16IS750_SA7            0x9E  /* VSS SDA */
#define SC16IS750_SA8            0xA0  /* SCL VDD */
#define SC16IS750_SA9            0xA2  /* SCL VSS */
#define SC16IS750_SA10           0xA4  /* SCL SCL */
#define SC16IS750_SA11           0xA6  /* SCL SDA */
#define SC16IS750_SA12           0xA8  /* SDA VDD */
#define SC16IS750_SA13           0xAA  /* SDA VSS */
#define SC16IS750_SA14           0xAC  /* SDA SCL */
#define SC16IS750_SA15           0xAE  /* SDA SDA */

//Default I2C Slaveaddress
#define SC16IS750_DEFAULT_ADDR   SC16IS750_SA0


/** See datasheet section 7.8 for configuring the
  * "Programmable baud rate generator"
  */
#define SC16IS750_XTAL_FREQ              14745600UL /* On-board crystal (New mid-2010 Version) */
#define SC16IS750_PRESCALER_1                   1   /* Default prescaler after reset           */
#define SC16IS750_PRESCALER_4                   4   /* Selectable by setting MCR[7]            */
#define SC16IS750_PRESCALER                      SC16IS750_PRESCALER_1  
#define SC16IS750_BAUDRATE_DIVISOR(baud)       ((SC16IS750_XTAL_FREQ/SC16IS750_PRESCALER)/(baud*16UL))

//Default baudrate
#define SC16IS750_DEFAULT_BAUDRATE           9600


/** See section 8.3 of the datasheet for definitions
  * of bits in the FIFO Control Register (FCR)
  */
#define FCR_RX_IRQ_60                 (3 << 6)
#define FCR_RX_IRQ_56                 (2 << 6)
#define FCR_RX_IRQ_16                 (1 << 6)
#define FCR_RX_IRQ_8                  (0 << 6)
//TX Level only accessible when EFR[4] is set
#define FCR_TX_IRQ_56                 (3 << 4)
#define FCR_TX_IRQ_32                 (2 << 4)
#define FCR_TX_IRQ_16                 (1 << 4)
#define FCR_TX_IRQ_8                  (0 << 4)
//#define FCR_RESERVED                  (1 << 3)
#define FCR_TX_FIFO_RST               (1 << 2)
#define FCR_RX_FIFO_RST               (1 << 1)
#define FCR_ENABLE_FIFO               (1 << 0)

//FIFO size
#define SC16IS750_FIFO_RX                  64
#define SC16IS750_FIFO_TX                  64


/** See section 8.4 of the datasheet for definitions
  * of bits in the Line Control Register (LCR)
  */
#define LCR_BITS5                      0x00
#define LCR_BITS6                      0x01
#define LCR_BITS7                      0x02
#define LCR_BITS8                      0x03

#define LCR_BITS1                      0x00
#define LCR_BITS2                      0x04

#define LCR_NONE                       0x00
#define LCR_ODD                        0x08
#define LCR_EVEN                       0x18
#define LCR_FORCED1                    0x28
#define LCR_FORCED0                    0x38

#define LCR_BRK_ENA                    0x40
#define LCR_BRK_DIS                    0x00

#define LCR_ENABLE_DIV                 0x80
#define LCR_DISABLE_DIV                0x00

#define LCR_ENABLE_ENHANCED_FUNCTIONS (0xBF)


/** See section 8.5 of the datasheet for definitions
  * of bits in the Line status register (LSR)
  */
#define LSR_DR   (0x01) /* Data ready in RX FIFO                       */
#define LSR_OE   (0x02) /* Overrun error                               */
#define LSR_PE   (0x04) /* Parity error                                */
#define LSR_FE   (0x08) /* Framing error                               */
#define LSR_BI   (0x10) /* Break interrupt                             */
#define LSR_THRE (0x20) /* Transmitter holding register (FIFO empty)   */
#define LSR_TEMT (0x40) /* Transmitter empty (FIFO and TSR both empty) */
#define LSR_FFE  (0x80) /* At least one PE, FE or BI in FIFO           */


/** See section 8.6 of the datasheet for definitions
  * of bits in the Modem control register (MCR)
  */
#define MCR_MDTR                      (1 << 0) /* Data Terminal Ready pin control. */
#define MCR_MRTS                      (1 << 1) /* Request to Send pin control when not in Auto RTS mode.*/
//MCR[2] only accessible when EFR[4] is set
#define MCR_ENABLE_TCR_TLR            (1 << 2)
#define MCR_ENABLE_LOOPBACK           (1 << 4)
//MCR[7:5] only accessible when EFR[4] is set
#define MCR_ENABLE_XON_ANY_CHAR       (1 << 5)
#define MCR_ENABLE_IRDA               (1 << 6)
#define MCR_PRESCALE_1                (0 << 7)
#define MCR_PRESCALE_4                (1 << 7)


/** See section 8.7 of the datasheet for definitions
  * of bits in the Modem status register (MSR)
  */
#define MSR_DCTS                      (1 << 0) /* Delta CTS - CTS Changed State      */
#define MSR_DDSR                      (1 << 1) /* Delta DSR - DSR Changed State      */
#define MSR_DDI                       (1 << 2) /* Delta DI  - DI  Changed State      */
#define MSR_DCD                       (1 << 3) /* Delta CD  - CD  Changed State      */
#define MSR_CTS                       (1 << 4) /* CTS State - Complement of NCTS pin */
//MSR[7:5] only accessible when GPIO[7:4] are set as modem pin
#define MSR_DSR                       (1 << 5) /* DSR State - Complement of NDSR pin */
#define MSR_RI                        (1 << 6) /* RI State  - Complement of  NRI pin */
#define MSR_CD                        (1 << 7) /* CD State  - Complement of  NCD pin */


/** See section 8.8 of the datasheet for definitions
  * of bits in the Interrupt enable register (IER)
  */
#define IER_ERHRI (0x01) /* Enable received data available interrupt            */
#define IER_ETHRI (0x02) /* Enable transmitter holding register empty interrupt */
#define IER_ELSI  (0x04) /* Enable receiver line status interrupt               */
#define IER_EMSI  (0x08) /* Enable modem status interrupt                       */
//IER[7:5] only accessible when EFR[4] is set
#define IER_SLEEP (0x10) /* Enable sleep mode                                   */
#define IER_XOFFI (0x20) /* Enable XOFF interrupt                               */
#define IER_RTSI  (0x40) /* Enable RTS interrupt                                */
#define IER_CTSI  (0x80) /* Enable CTS interrupt                                */


/** See section 8.9 of the datasheet for definitions
  * of bits in the Interrupt identification register (IIR)
  * Bit 0 is set to 0 if an IRQ is pending.
  * Bits 1..5 are used to identify the IRQ source.
  */
#define IIR_IRQ_NOT_PENDING             (0x01)  /* IRQ Not Pending              */
#define IIR_TX_EMPTY                    (0x02)  /* THR Interrupt                */
#define IIR_RX_DATA                     (0x04)  /* RHR Interrupt                */
#define IIR_RX_ERROR                    (0x06)  /* Line Status Error Interrupt  */
#define IIR_RX_TIMEOUT                  (0x0B)  /* RX Timeout Interrupt         */
#define IIR_RX_XOFF                     (0x10)  /* RX XOff Interrupt            */
#define IIR_DCTS_DRTS                   (0x20)  /* Delta CTS or RTS Interrupt   */
#define IIR_DIO                         (0x30)  /* Delta GPIO pin Interrupt     */

#define IIR_BITS_USED                   (0x07)


/** See section 8.10 of the datasheet for definitions
  * of bits in the Enhanced Features Register (EFR)
  */
#define EFR_ENABLE_CTS                  (1 << 7)
#define EFR_ENABLE_RTS                  (1 << 6)
#define EFR_ENABLE_XOFF2_CHAR_DETECT    (1 << 5)
#define EFR_ENABLE_ENHANCED_FUNCTIONS   (1 << 4)
// EFR[3:0] are used to define Software Flow Control mode
// See section 7.3
#define EFR_DISABLE_TX_FLOW_CTRL        (0x0 << 2)
#define EFR_TX_XON2_XOFF2               (0x1 << 2)
#define EFR_TX_XON1_XOFF1               (0x2 << 2)
#define EFR_TX_XON2_1_XOFF2_1           (0x3 << 2)

#define EFR_DISABLE_RX_FLOW_CTRL        (0x0 << 0)
#define EFR_RX_XON2_XOFF2               (0x1 << 0)
#define EFR_RX_XON1_XOFF1               (0x2 << 0)
#define EFR_RX_XON2_1_XOFF2_1           (0x3 << 0)

#define EFR_TX_XON2_XOFF2_RX_FLOW       (0x1 << 2) | (0x3 << 0)
#define EFR_TX_XON1_XOFF1_RX_FLOW       (0x2 << 2) | (0x3 << 0)
#define EFR_TX_XON2_1_XOFF2_1_RX_FLOW   (0x3 << 2) | (0x3 << 0)



/** See section 8.12 of the datasheet for definitions
  * of bits in the Transmission Control Register (TCR)
  * These levels control when RTS is asserted or de-asserted and auto RTS is enabled. Note that XON/XOFF is not supported in this lib.
  *   Trigger level to halt transmission to the device   : 0..15 (meaning 0-60 with a granularity of 4) 
  *     RTS is de-asserted when RX FIFO is above the set trigger level (i.e. buffer is getting full)  
  *   Trigger level to resume transmission to the device : 0..15 (meaning 0-60 with a granularity of 4) 
  *     RTS is asserted again when RX FIFO drops below the set trigger level (i.e. buffer has room again)    
  */
#define TCR_HALT_DEFAULT                (0x0E)
#define TCR_RESUME_DEFAULT              (0x08)  

/** See section 8.12 of the datasheet for definitions
  * Note: The device will stop transmissions from the TX FIFO when CTS is de-asserted by external receiver and 
  *       auto CTS is enabled. Note that XON/XOFF is not supported in this lib.
  */
  
    
/** See section 7.5 and 8.13 of the datasheet for definitions
  * of bits in the Trigger Level Register (TLR) control when an IRQ is generated.
  *   Trigger level for TX interrupt: 0..15 (meaning 0-60 with a granularity of 4) 
  *     IRQ when TX FIFO is above the set trigger level (i.e. buffer is getting full)
  *   Trigger level for RX interrupt: 0..15 (meaning 0-60 with a granularity of 4) 
  *     IRQ when RX FIFO is above the set trigger level (i.e. data is waiting to be read)
  */
#define TLR_TX_DEFAULT                  (0x0E)
#define TLR_RX_DEFAULT                  (0x04)  


/**
  * See section 8.16, 8.17, 8.18 of the datasheet for definitions
  * of bits in the IO Direction (IODIR), IO State (IOSTATE) and IO Interrupt Enable register (IOINTENA)
  * 
  * Basically a direct mapping of register bits to GPIO pin.
  */


/**
  * See section 8.19 of the datasheet for definitions
  * of bits in the IO Control register (IOC)
  * 
  * Bit 0 is set to 0 to enable latch of IO inputs.
  * Bit 1 is set to enable GPIO[7-4] as /RI, /CD, /DTR, /DST.
  * Bit 2 is set to enable software reset.
  */
#define IOC_ENA_LATCH                   (0x01)
#define IOC_ENA_MODEM                   (0x02) /* Set GPIO[7:4] pins to modem functions */
#define IOC_SW_RST                      (0x04) 


/**
  * See section 8.20 of the datasheet for definitions
  * of bits in the Extra Features Control register (EFCR)
  * 
  */
#define EFCR_ENA_RS485                  (0x01)  
#define EFCR_DIS_RX                     (0x02)    
#define EFCR_DIS_TX                     (0x04)    
#define EFCR_ENA_TX_RTS                 (0x10)    
#define EFCR_INV_RTS_RS485              (0x20)    
#define EFCR_ENA_IRDA                   (0x80)    

// See Chapter 11 of datasheet
#define SPI_READ_MODE_FLAG              (0x80)


/** Abstract class SC16IS750 for a bridge between either SPI or I2C and a Serial port
  *
  * Supports both SPI and I2C interfaces through derived classes
  *
  * @code
  *
  * @endcode
  */
//class SC16IS750 {
//class SC16IS750 : public SerialBase, public Stream {    // Wrong, Serialbase can not be constructed for NC,NC
class SC16IS750 : public Stream {    
public:

//  SC16IS750 Register definitions (shifted to align)
    enum RegisterName { 
/*
 * 16750 addresses. Registers accessed when LCR[7] = 0.
 */   
        RHR         = 0x00 << 3, /* Rx buffer register     - Read access  */
        THR         = 0x00 << 3, /* Tx holding register    - Write access */
        IER         = 0x01 << 3, /* Interrupt enable reg   - RD/WR access */

/*
 * 16750 addresses. Registers accessed when LCR[7] = 1.
 */       
        DLL         = 0x00 << 3, /* Divisor latch (LSB)    - RD/WR access */
        DLH         = 0x01 << 3, /* Divisor latch (MSB)    - RD/WR access */

/*
 * 16750 addresses. IIR/FCR is accessed when LCR[7:0] <> 0xBF.
 *                  Bit 5 of the FCR register is accessed when LCR[7] = 1.
 */       
        IIR         = 0x02 << 3, /* Interrupt id. register - Read only    */
        FCR         = 0x02 << 3, /* FIFO control register  - Write only   */
/*
 * 16750 addresses. EFR is accessed when LCR[7:0] = 0xBF.
 */       
        EFR         = 0x02 << 3, /* Enhanced features reg  - RD/WR access */     

/*
 * 16750 addresses.
 */       
        LCR         = 0x03 << 3, /* Line control register  - RD/WR access */
/*
 * 16750 addresses. MCR/LSR is accessed when LCR[7:0] <> 0xBF.
 *                  Bit 7 of the MCR register is accessed when EFR[4] = 1.
 */       
        MCR         = 0x04 << 3, /* Modem control register - RD/WR access */
        LSR         = 0x05 << 3, /* Line status register   - Read only    */
 
/*
 * 16750 addresses. MSR/SPR is accessed when LCR[7:0] <> 0xBF.
 *                  MSR, SPR register is accessed when EFR[1]=0 and MCR[2]=0.
 */       
        MSR         = 0x06 << 3, /* Modem status register  - Read only    */
        SPR         = 0x07 << 3, /* Scratchpad register    - RD/WR access */
/*
 * 16750 addresses. TCR/TLR is accessed when LCR[7:0] <> 0xBF.
 *                  TCR, TLR register is accessed when EFR[1]=1 and MCR[2]=1.
 */       
        TCR         = 0x06 << 3, /* Transmission control register - RD/WR access */
        TLR         = 0x07 << 3, /* Trigger level register        - RD/WR access */

/*
 * 16750 addresses. XON, XOFF is accessed when LCR[7:0] = 0xBF.
 */       
        XON1        = 0x04 << 3, /* XON1 register          - RD/WR access */
        XON2        = 0x05 << 3, /* XON2 register          - RD/WR access */
        XOFF1       = 0x06 << 3, /* XOFF1 register         - RD/WR access */
        XOFF2       = 0x07 << 3, /* XOFF2 register         - RD/WR access */

/*
 * 16750 addresses.
 */       
        TXLVL       = 0x08 << 3, /* TX FIFO Level register - Read only    */
        RXLVL       = 0x09 << 3, /* RX FIFO Level register - Read only    */
        IODIR       = 0x0A << 3, /* IO Pin Direction reg   - RD/WR access */
        IOSTATE     = 0x0B << 3, /* IO Pin State reg       - RD/WR access */
        IOINTENA    = 0x0C << 3, /* IO Interrupt Enable    - RD/WR access */
//        reserved    = 0x0D << 3,
        IOCTRL      = 0x0E << 3, /* IO Control register    - RD/WR access */
        EFCR        = 0x0F << 3, /* Extra features reg     - RD/WR access */

    } ;


 // This enum used to be part of SerialBase class (access via SerialBase.h).
 //  It seems not be supported anymore. The enums for Parity have moved to Serial now..  
    enum Flow {
        Disabled = 0,
        RTS,
        CTS,
        RTSCTS
    };
 
//  SC16IS752 Channel definitions (shifted to align)
    enum ChannelName { 
      Channel_A     = 0x00 << 1,   
      Channel_B     = 0x01 << 1         
    };
  
// SC16IS750 configuration register values
// Several configuration registers are write-only. Need to save values to allow restoring.
struct SC16IS750_cfg {
  char baudrate;
  char dataformat;  
  char flowctrl;  
  char fifoformat;
  bool fifoenable;      
};


/** Determine if there is a character available to read.
  * This is data that's already arrived and stored in the receive
  * buffer (which holds 64 chars).
  *
  *   @return 1 if there is a character available to read, 0 otherwise
  */
  int readable();

/** Determine how many characters are available to read.
  * This is data that's already arrived and stored in the receive
  * buffer (which holds 64 chars).
  *
  *   @return int Characters available to read
  */ 
  int readableCount();

/** Determine if there is space available to write a character.    
  *   @return 1 if there is a space for a character to write, 0 otherwise
  */
  int writable();

  
/** Determine how much space available for writing characters.
  * This considers data that's already stored in the transmit
  * buffer (which holds 64 chars).
  *
  *   @return int character space available to write
  */  
  int writableCount();

/**
  * Read char from UART Bridge.
  * Acts in the same manner as 'Serial.read()'.  
  *   @param none    
  *   @return char read or -1 if no data available. 
  */ 
  int getc();  
  
/**
  * Write char to UART Bridge. Blocking when no free space in FIFO
  *   @param value char to be written    
  *   @return value written  
  */
  int putc(int c);
 

#if DOXYGEN_ONLY
  /** Write a formatted string to the UART Bridge. Blocking when no free space in FIFO
    *
    * @param format A printf-style format string, followed by the
    *               variables to use in formatting the string.
    */
   int printf(const char* format, ...);
#endif


/**
  * Write char string to UART Bridge. Blocking when no free space in FIFO
  *   @param *str char string to be written    
  *   @return none  
  */
  void writeString(const char *str);


/**
  * Write byte array to UART Bridge. Blocking when no free space in FIFO
  *   @param *data byte array to be written    
  *   @param len   number of bytes to write  
  *   @return none  
  */
  void writeBytes(const char *data, int len);
    
/** Set baudrate of the serial port.    
  *  @param  baud integer baudrate (4800, 9600 etc)
  *  @return none
  */
  void baud(int baudrate = SC16IS750_DEFAULT_BAUDRATE);   

/** Set the transmission format used by the serial port.   
  *   @param bits      The number of bits in a word (5-8; default = 8)
  *   @param parity    The parity used (Serial::None, Serial::Odd, Serial::Even, Serial::Forced1, Serial::Forced0; default = Serial::None)
  *   @param stop_bits The number of stop bits (1 or 2; default = 1) 
  *   @return none   
  */
  void format(int bits=8, SerialBase::Parity parity=SerialBase::None, int stop_bits=1);
  
#if(0)
/** Attach a function to call whenever a serial interrupt is generated
  *
  *  @param fptr A pointer to a void function, or 0 to set as none
  *  @param type Which serial interrupt to attach the member function to (Seriall::RxIrq for receive, TxIrq for transmit buffer empty)
  */
  void attach(void (*fptr)(void), IrqType type=RxIrq);
 
/** Attach a member function to call whenever a serial interrupt is generated
  *
  *  @param tptr pointer to the object to call the member function on
  *  @param mptr pointer to the member function to be called
  *  @param type Which serial interrupt to attach the member function to (Seriall::RxIrq for receive, TxIrq for transmit buffer empty)
  *  @return none   
  */
  template<typename T>
    void attach(T* tptr, void (T::*mptr)(void), IrqType type=RxIrq) {
      if((mptr != NULL) && (tptr != NULL)) {
          _irq[type].attach(tptr, mptr);
          serial_irq_set(&_serial, (SerialIrq)type, 1);
      }
  }
#endif
 
/** Generate a break condition on the serial line
  *  @param none
  *  @return none 
  */
  void send_break();


/** Set a break condition on the serial line
  *  @param enable  break condition
  *  @return none   
  */
  void set_break(bool enable=false);
    

/** Set the flow control type on the serial port
  *  Added for compatibility with Serial Class.
  *  SC16IS750 supports only Flow, Pins can not be selected. 
  *  This method sets hardware flow control levels. SC16IS750 supports XON/XOFF, but this is not implemented.  
  *
  *  @param type the flow control type (Disabled, RTS, CTS, RTSCTS)     
  *  @param flow1 the first flow control pin (RTS for RTS or RTSCTS, CTS for CTS)
  *  @param flow2 the second flow control pin (CTS for RTSCTS)
  *  @return none   
  */
  void set_flow_control(Flow type=Disabled, PinName flow1=NC, PinName flow2=NC);


/** Set the RX FIFO flow control levels
  *  This method sets hardware flow control levels. SC16IS750 supports XON/XOFF, but this is not implemented.   
  *  Should be called BEFORE Auto RTS is enabled.    
  *
  *  @param resume trigger level to resume transmission (0..15, meaning 0-60 with a granularity of 4)     
  *  @param halt trigger level to resume transmission (0..15, meaning 0-60 with granularity of 4)           
  *  @return none   
  */
  void set_flow_triggers(int resume = TCR_RESUME_DEFAULT, int halt = TCR_HALT_DEFAULT);


/** Set the Modem Control register
  *  This method sets prescaler, enables TCR and TLR
  *
  *  @param none 
  *  @return none 
  */
  void set_modem_control();

 
/**
  * Check that UART is connected and operational.
  *   @param  none
  *   @return bool true when connected, false otherwise
  */
  bool connected();

         

/** FIFO control, sets TX and RX IRQ trigger levels and enables FIFO and save in _config
  *  Note FCR[5:4] (=TX_IRQ_LVL) only accessible when EFR[4] is set (enhanced functions enable)
  *  Note TLR only accessible when EFR[4] is set (enhanced functions enable) and MCR[2] is set 
  *   @param  none
  *   @return none
  */
  void set_fifo_control();


/** Flush the UART FIFOs while maintaining current FIFO mode.
  *   @param  none
  *   @return none
  */
  void flush();


/** Set direction of I/O port pins.
  * This method is specific to the SPI-I2C UART and not found on the 16750
  * Note: The SC16IS752 does not have separate GPIOs for Channel_A and Channel_B.    
  *   @param  bits Bitpattern for I/O (1=output, 0=input)
  *   @return none
  */
  void ioSetDirection(unsigned char bits);

/** Set bits of I/O port pins.
  * This method is specific to the SPI-I2C UART and not found on the 16750
  * Note: The SC16IS752 does not have separate GPIOs for Channel_A and Channel_B.    
  *   @param  bits Bitpattern for I/O (1= set output bit, 0 = clear output bit)
  *   @return none
  */
  void ioSetState(unsigned char bits);

/** Get bits of I/O port pins.
  * This method is specific to the SPI-I2C UART and not found on the 16750
  * Note: The SC16IS752 does not have separate GPIOs for Channel_A and Channel_B.    
  *   @param  none
  *   @return bits Bitpattern for I/O (1= bit set, 0 = bit cleared)
  */
  unsigned char ioGetState();


/** Software Reset SC16IS750 device.
  * This method is specific to the SPI-I2C UART and not found on the 16750
  * Note: The SC16IS752 does not have separate Reset for Channel_A and Channel_B.    
  *   @param  none
  *   @return none
  */
  void swReset();


/** Hardware Reset SC16IS750 device.
  * Pure virtual, must be declared in derived class.   
  * This method is only functional when the Reset pin has been declared and is also connected
  *   @param  none
  *   @return none
  */
  virtual void hwReset() =0;

/** Write value to internal register.
  * Pure virtual, must be declared in derived class.   
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @param data              The 8bit value to write
  *   @return none 
  */
  virtual void writeRegister (RegisterName register_address, char data ) =0;

/** Read value from internal register.
  * Pure virtual, must be declared in derived class.   
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @return char             The 8bit value read from the register
  */
  virtual char readRegister (RegisterName register_address ) =0;

/** Write multiple datavalues to Transmitregister.
  * More Efficient implementation than writing individual bytes
  * Pure virtual, must be declared in derived class.   
  *   @param char* databytes   The pointer to the block of data
  *   @param len               The number of bytes to write
  *   @return none 
  */
  virtual void writeDataBlock (const char *data, int len ) =0;


/** Initialise internal registers
  * Should be in protection section. Public for testing purposes
  * If initialisation fails this method does not return.    
  *   @param none
  *   @return none 
  */
  void _init();
  
protected:
//protected is accessible to derived classes, but not to external users


/** Constructor is protected for this abstract Class
  *  
  */
  SC16IS750();  

/** Needed to implement Stream
  *
  * Read char from UART Bridge.
  * Acts in the same manner as 'Serial.read()'.  
  *   @param none    
  *   @return char read or -1 if no data available. 
  */  
  virtual int _getc() {
    return getc();
  }


/** Needed to implement Stream
  *
  * Write char to UART Bridge. Blocking when no free space in FIFO
  *   @param value char to be written    
  *   @return value written  
  */
  virtual int _putc(int c) {
    return putc(c); 
  }
  
/** Needed to implement Stream
  *
  */
  virtual int peek() {return 0;};
  

// Save config settings
SC16IS750_cfg _config;

private:
//private is not accessible to derived classes, nor external users

};



/** Class SC16IS750_SPI for a bridge between SPI and a Serial port
 *
 * @code
 * #include "mbed.h"
 * #include "SC16IS750.h"
 *
 * SPI spi(PTD2, PTD3, PTD1); //MOSI, MISO, SCK
 * SC16IS750_SPI serial_spi(&spi, PTD0);
 * 
 * Serial pc(USBTX,USBRX);
 *
 * int main() {
 *   pc.printf("\nHello World!\n");
 *
 *   while(1) { 
 *     serial_spi.ioSetState(0x00);
 *     wait(0.5);
 *     serial_spi.ioSetState(0xFF); 
 *     wait(0.5); 
 *     serial_spi.putc('*');  
 *     pc.putc('*');                
 *   }
 * }
 *
 * @endcode
 */
class SC16IS750_SPI : public SC16IS750 {
public:

/** Create an SC16IS750_SPI object using a specified SPI bus and CS
  *
  * @param SPI &spi the SPI port to connect to 
  * @param cs  Pinname of the CS pin (active low)
  * @param rst Pinname for Reset pin (active low) Optional, Default = NC 
  */  
  SC16IS750_SPI(SPI *spi, PinName cs, PinName rst = NC);

/** Destruct SC16IS750_SPI bridge object
  *
  * @param  none
  * @return none
  */ 
  virtual ~SC16IS750_SPI();


/** Write value to internal register.
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @param data              The 8bit value to write
  *   @return none 
  */
  virtual void writeRegister(SC16IS750::RegisterName registerAddress, char data);

/** Read value from internal register.
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @return char             The 8bit value read from the register
  */
  virtual char readRegister(SC16IS750::RegisterName registerAddress);

/** Write multiple datavalues to Transmitregister.
  * More Efficient implementation than writing individual bytes
  * Assume that previous check confirmed that the FIFO has sufficient free space to store the data 
  * 
  *   @param char* databytes   The pointer to the block of data
  *   @param len               The number of bytes to write
  *   @return none 
  */
  virtual void writeDataBlock (const char *data, int len );

/** Hardware Reset SC16IS750 device.
  * This method is only functional when the Reset pin has been declared and is also connected
  *   @param  none
  *   @return none
  */
  virtual void hwReset();


protected:
//protected is accessible to derived classes, but not to external users


private:
  SPI *_spi;          //SPI bus reference
  DigitalOut _cs;     //CS of SPI device (active low)

/** Optional Hardware Reset pin for the bridge device (active low)
  * Default PinName value is NC
  */
  DigitalOut* _reset; //Reset the Bridge device (active low)

};



/** Class SC16IS750_I2C for a bridge between I2C and a Serial port
 *
 * @code
 * #include "mbed.h"
 * #include "SC16IS750.h"
 *
 * I2C i2c(PTE0, PTE1);       //SDA, SCL
 * SC16IS750_I2C serial_i2c(&i2c, SC16IS750_DEFAULT_ADDR);
 *
 * Serial pc(USBTX,USBRX);
 *
 * int main() {
 *   pc.printf("\nHello World!\n");
 *
 *   while(1) { 
 *     serial_i2c.ioSetState(0x00);
 *     wait(0.5);
 *     serial_i2c.ioSetState(0xFF); 
 *     wait(0.5); 
 *     serial_i2c.putc('*');   
 *     pc.putc('*');                
 *   }
 * }
 *
 * @endcode
 */
class SC16IS750_I2C : public SC16IS750 {
public:

/** Create an SC16IS750_I2C object using a specified I2C bus and slaveaddress
  *
  * @param I2C &i2c the I2C port to connect to 
  * @param char deviceAddress the address of the SC16IS750
  * @param rst Pinname for Reset pin (active low) Optional, Default = NC   
  */  
  SC16IS750_I2C(I2C *i2c, uint8_t deviceAddress = SC16IS750_DEFAULT_ADDR, PinName rst = NC);


/** Destruct SC16IS750_I2C bridge object
  *
  * @param  none
  * @return none
  */ 
  virtual ~SC16IS750_I2C();


/** Write value to internal register.
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @param data              The 8bit value to write
  *   @return none 
  */
  virtual void writeRegister(SC16IS750::RegisterName register_address, char data );

/** Read value from internal register.
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @return char             The 8bit value read from the register
  */
  virtual char readRegister(SC16IS750::RegisterName register_address );


/** Write multiple datavalues to Transmitregister.
  * More Efficient implementation than writing individual bytes
  * Assume that previous check confirmed that the FIFO has sufficient free space to store the data 
  * Pure virtual, must be declared in derived class.   
  *   @param char* databytes   The pointer to the block of data
  *   @param len               The number of bytes to write
  *   @return none 
  */
  virtual void writeDataBlock (const char *data, int len );


/** Hardware Reset SC16IS750 device.
  * This method is only functional when the Reset pin has been declared and is also connected
  *   @param  none
  *   @return none
  */
  virtual void hwReset();


protected:
//protected is accessible to derived classes, but not to external users


private:
  I2C *_i2c;                    //I2C bus reference
  uint8_t _slaveAddress;        //I2C Slave address of device

/** Optional Hardware Reset pin for the bridge device (active low)
  * Default PinName value is NC
  */
  DigitalOut* _reset;           //Reset the Bridge device (active low)

};



/** Class SC16IS752_SPI for a bridge between SPI and a Serial port
 *
 * @code
 * #include "mbed.h"
 * #include "SC16IS750.h"
 *
 * SPI spi(PTD2, PTD3, PTD1); //MOSI, MISO, SCK
 * SC16IS750_SPI serial_spi(&spi, PTD0, NC, SC16IS750::Channel_B);
 * 
 * Serial pc(USBTX,USBRX);
 *
 * int main() {
 *   pc.printf("\nHello World!\n");
 *
 *   while(1) { 
 *     serial_spi.ioSetState(0x00);
 *     wait(0.5);
 *     serial_spi.ioSetState(0xFF); 
 *     wait(0.5); 
 *     serial_spi.putc('*');  
 *     pc.putc('*');                
 *   }
 * }
 *
 * @endcode
 */
class SC16IS752_SPI : public SC16IS750 {
public:

/** Create an SC16IS752_SPI object using a specified SPI bus and CS
  * Note: The SC16IS752 does not have separate GPIOs for Channel_A and Channel_B.  
  * Note: The SC16IS752 does not have separate Reset for Channel_A and Channel_B.    
  *
  * @param SPI &spi the SPI port to connect to 
  * @param cs  Pinname of the CS pin (active low)
  * @param rst Pinname for Reset pin (active low) Optional, Default = NC 
  * @param channel UART ChannelName, Default = Channel_A    
  */  
  SC16IS752_SPI(SPI *spi, PinName cs, PinName rst = NC, ChannelName channel = SC16IS750::Channel_A );

/** Destruct SC16IS752_SPI bridge object
  *
  * @param  none
  * @return none
  */ 
  virtual ~SC16IS752_SPI();


/** Write value to internal register.
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @param data              The 8bit value to write
  *   @return none 
  */
  virtual void writeRegister(SC16IS750::RegisterName registerAddress, char data);

/** Read value from internal register.
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @return char             The 8bit value read from the register
  */
  virtual char readRegister(SC16IS750::RegisterName registerAddress);

/** Write multiple datavalues to Transmitregister.
  * More Efficient implementation than writing individual bytes
  * Assume that previous check confirmed that the FIFO has sufficient free space to store the data 
  * 
  *   @param char* databytes   The pointer to the block of data
  *   @param len               The number of bytes to write
  *   @return none 
  */
  virtual void writeDataBlock (const char *data, int len );

/** Hardware Reset SC16IS750 device.
  * This method is only functional when the Reset pin has been declared and is also connected
  *   @param  none
  *   @return none
  */
  virtual void hwReset();


protected:
//protected is accessible to derived classes, but not to external users


private:
  SPI *_spi;          //SPI bus reference
  DigitalOut _cs;     //CS of SPI device (active low)

/** Optional Hardware Reset pin for the bridge device (active low)
  * Default PinName value is NC
  */
  DigitalOut* _reset; //Reset the Bridge device (active low)

// Save Channel setting
  ChannelName _channel; 
};



/** Class SC16IS752_I2C for a bridge between I2C and a Serial port
 *
 * @code
 * #include "mbed.h"
 * #include "SC16IS750.h"
 *
 * I2C i2c(PTE0, PTE1);       //SDA, SCL
 * SC16IS752_I2C serial_i2c(&i2c, SC16IS750_DEFAULT_ADDR, NC, SC16IS750::Channel_A);
 *
 * Serial pc(USBTX,USBRX);
 *
 * int main() {
 *   pc.printf("\nHello World!\n");
 *
 *   while(1) { 
 *     serial_i2c.ioSetState(0x00);
 *     wait(0.5);
 *     serial_i2c.ioSetState(0xFF); 
 *     wait(0.5); 
 *     serial_i2c.putc('*');   
 *     pc.putc('*');                
 *   }
 * }
 *
 * @endcode
 */
class SC16IS752_I2C : public SC16IS750 {
public:

/** Create an SC16IS752_I2C object using a specified I2C bus, slaveaddress and Channel
  * Note: The SC16IS752 does not have separate GPIOs for Channel_A and Channel_B. 
  * Note: The SC16IS752 does not have separate Reset for Channel_A and Channel_B.     
  *
  * @param I2C &i2c the I2C port to connect to 
  * @param char deviceAddress the address of the SC16IS750
  * @param rst Pinname for Reset pin (active low) Optional, Default = NC
  * @param channel UART ChannelName, Default = Channel_A  
  */  
  SC16IS752_I2C(I2C *i2c, uint8_t deviceAddress = SC16IS750_DEFAULT_ADDR, PinName rst = NC, ChannelName channel = SC16IS750::Channel_A);


/** Destruct SC16IS752_I2C bridge object
  *
  * @param  none
  * @return none
  */ 
  virtual ~SC16IS752_I2C();


/** Write value to internal register.
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @param data              The 8bit value to write
  *   @return none 
  */
  virtual void writeRegister(SC16IS750::RegisterName register_address, char data );

/** Read value from internal register.
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @return char             The 8bit value read from the register
  */
  virtual char readRegister(SC16IS750::RegisterName register_address );


/** Write multiple datavalues to Transmitregister.
  * More Efficient implementation than writing individual bytes
  * Assume that previous check confirmed that the FIFO has sufficient free space to store the data 
  * Pure virtual, must be declared in derived class.   
  *   @param char* databytes   The pointer to the block of data
  *   @param len               The number of bytes to write
  *   @return none 
  */
  virtual void writeDataBlock (const char *data, int len );


/** Hardware Reset SC16IS752 device.
  * This method is only functional when the Reset pin has been declared and is also connected
  *   @param  none
  *   @return none
  */
  virtual void hwReset();


protected:
//protected is accessible to derived classes, but not to external users


private:
  I2C *_i2c;                    //I2C bus reference
  uint8_t _slaveAddress;        //I2C Slave address of device

/** Optional Hardware Reset pin for the bridge device (active low)
  * Default PinName value is NC
  */
  DigitalOut* _reset;           //Reset the Bridge device (active low)

// Save Channel setting
  ChannelName _channel; 

};


#endif  //  _SC16IS750_H
