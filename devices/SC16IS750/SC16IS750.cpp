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
#include    "mbed.h"
#include    "SC16IS750.h"

#define ENABLE_BULK_TRANSFERS             1
#define BULK_BLOCK_LEN                   16

/** Abstract class SC16IS750 for converter between either SPI or I2C and a Serial port
  * Constructor for this Abstract Class is protected  
  * Supports both SPI and I2C interfaces through derived classes
  *
  * @code
  *
  * @endcode
  */
SC16IS750::SC16IS750() { 
//SC16IS750::SC16IS750() : Serial(NC, NC) {   //Fout, mag geen NC zijn
//SC16IS750::SC16IS750() : SerialBase(NC, NC) { //Fout, mag geen NC zijn
// Dont call _init() here since the SPI or I2C port have not yet been configured...
  //_init();  // initialise UART registers
}


/** Set baudrate of the serial port.    
  *  @param  baud integer baudrate (4800, 9600 etc)
  *  @return none
  */
void SC16IS750::baud(int baudrate) {
  unsigned long divisor = SC16IS750_BAUDRATE_DIVISOR(baudrate);
  char lcr_tmp;
  
  _config.baudrate = baudrate;               // Save baudrate

  lcr_tmp = this->readRegister(LCR);                            // Read current LCR register
  this->writeRegister(LCR, lcr_tmp | LCR_ENABLE_DIV);           // Enable Divisor registers
  this->writeRegister(DLL, ( divisor       & 0xFF));            //   write divisor LSB
  this->writeRegister(DLH, ((divisor >> 8) & 0xFF));            //   write divisor MSB
  this->writeRegister(LCR, lcr_tmp);                            // Restore LCR register, activate regular RBR, THR and IER registers  
  
}


/** Set the transmission format used by the serial port.   
  *   @param bits      The number of bits in a word (5-8; default = 8)
  *   @param parity    The parity used (Serial::None, Serial::Odd, Serial::Even, Serial::Forced1, Serial::Forced0; default = Serial::None)
  *   @param stop_bits The number of stop bits (1 or 2; default = 1) 
  *   @return none   
  */
void SC16IS750::format(int bits, SerialBase::Parity parity, int stop_bits) {
  char lcr_tmp = 0x00;
  
  switch (bits) {
    case 5:  lcr_tmp |= LCR_BITS5;
             break;
    case 6:  lcr_tmp |= LCR_BITS6;
             break;
    case 7:  lcr_tmp |= LCR_BITS7;
             break;
    case 8:  lcr_tmp |= LCR_BITS8;
             break;
    default: lcr_tmp |= LCR_BITS8;     
  }

  switch (parity) {
    case SerialBase::None:    lcr_tmp |= LCR_NONE;
                          break;
    case SerialBase::Odd:     lcr_tmp |= LCR_ODD;
                          break;
    case SerialBase::Even:    lcr_tmp |= LCR_EVEN;
                          break;
    case SerialBase::Forced1: lcr_tmp |= LCR_FORCED1;
                          break;
    case SerialBase::Forced0: lcr_tmp |= LCR_FORCED0;
                          break;                      
    default:              lcr_tmp |= LCR_NONE;     
  }

  switch (stop_bits) {
    case 1:  lcr_tmp |= LCR_BITS1;
             break;
    case 2:  lcr_tmp |= LCR_BITS2;
             break;
    default: lcr_tmp |= LCR_BITS1;     
  }

  _config.dataformat = lcr_tmp;      // Save dataformat   

  this->writeRegister(LCR, lcr_tmp); // Set LCR register, activate regular RBR, THR and IER registers  

};

/** Generate a break condition on the serial line
  *  @return none 
  */
void SC16IS750::send_break() {
  // Wait for 1.5 frames before clearing the break condition
  // This will have different effects on our platforms, but should
  // ensure that we keep the break active for at least one frame.
  // We consider a full frame (1 start bit + 8 data bits bits + 
  // 1 parity bit + 2 stop bits = 12 bits) for computation.
  // One bit time (in us) = 1000000/_baud
  // Twelve bits: 12000000/baud delay
  // 1.5 frames: 18000000/baud delay
  set_break(true);
  wait_us(18000000/_config.baudrate);
  set_break(false);    
};
    
/** Set a break condition on the serial line
  *  @param enable  break condition
  *  @return none   
  */
void SC16IS750::set_break(bool enable) {

  if (enable) {
    _config.dataformat |= LCR_BRK_ENA;      // Save dataformat         
  }
  else {
    _config.dataformat &= ~LCR_BRK_ENA;     // Save dataformat           
  }

  this->writeRegister(LCR, _config.dataformat); // Set LCR register
}

/** Set the flow control type on the serial port
  *  Added for compatibility with Serial Class.
  *  SC16IS750 supports only Flow, Pins can not be selected.
  *  This method sets hardware flow control. SC16IS750 supports XON/XOFF, but this is not implemented.  
  *
  *  @param type the flow control type (Disabled, RTS, CTS, RTSCTS)     
  *  @param flow1 the first flow control pin (RTS for RTS or RTSCTS, CTS for CTS) - NOT USED
  *  @param flow2 the second flow control pin (CTS for RTSCTS) - NOT USED
  *  @return none   
  */
void SC16IS750::set_flow_control(Flow type, PinName flow1, PinName flow2) {
  char lcr_tmp; 
  char efr_tmp = 0x00;
  
  // We need to enable flow control to prevent overflow of buffers and
  // lose data when used with fast devices like the WiFly.

  switch (type) {
     case Disabled : 
                     break;
     case RTS:       efr_tmp = EFR_ENABLE_RTS;
                     break;     
     case CTS:       efr_tmp = EFR_ENABLE_CTS;                     
                     break;     
     case RTSCTS:    efr_tmp = EFR_ENABLE_RTS | EFR_ENABLE_CTS;
                     break;
     default:             ;    

  }

  //Save flowcontrol mode and enable enhanced functions  
  _config.flowctrl = efr_tmp | EFR_ENABLE_ENHANCED_FUNCTIONS;

  lcr_tmp = this->readRegister(LCR);                       // save LRC register
  this->writeRegister(LCR, LCR_ENABLE_ENHANCED_FUNCTIONS); // write magic number 0xBF to enable access to EFR register
  this->writeRegister(EFR, _config.flowctrl);              // set flow and enable enhanced functions
  this->writeRegister(LCR, lcr_tmp);                       // restore LCR register
}  

/** Set the RX FIFO flow control levels
  *  This method sets hardware flow control levels. SC16IS750 supports XON/XOFF, but this is not implemented.
  *  Should be called BEFORE Auto RTS is enabled.  
  *
  *  @param resume trigger level to resume transmission (0..15, meaning 0-60 with a granularity of 4)     
  *  @param halt trigger level to resume transmission (0..15, meaning 0-60 with granularity of 4)       
  *  @return none   
  */
void SC16IS750::set_flow_triggers(int resume, int halt) {

  // sanity checks
  halt = halt & 0x0F;
  resume = resume & 0x0F;  
  if (halt <= resume) {
    halt   = TCR_HALT_DEFAULT;
    resume = TCR_RESUME_DEFAULT;  
  }

  // Note: TCR accessible only when EFR[4]=1 and MCR[2]=1
  this->writeRegister(TCR, (resume << 4) | halt);          // set TCR register
}


/** Set the Modem Control register
  *  This method sets prescaler, enables TCR and TLR
  *
  *  @param none 
  *  @return none 
  */
void SC16IS750::set_modem_control() {

  //Note MCR[7:4] and MCR[2] only accessible when EFR[4] is set
  if (SC16IS750_PRESCALER == SC16IS750_PRESCALER_1) { // Default prescaler after reset
    this->writeRegister(MCR, MCR_PRESCALE_1 | MCR_ENABLE_TCR_TLR);
  }  
  else { 
    this->writeRegister(MCR, MCR_PRESCALE_4 | MCR_ENABLE_TCR_TLR);
  }
}  

 

/** Initialise internal registers
  * Should be in protection section. Public for testing purposes
  * If initialisation fails this method does not return.    
  *   @param none
  *   @return none 
  */
void SC16IS750::_init() {

  // Initialise SC16IS750

  // Hardware reset, assuming there is a HW Reset pin
//  this->hwReset();  

  // Software reset, assuming there is no access to the HW Reset pin
  swReset();
  
  // Set default baudrate (depends on prescaler) and save in _config
  // DLL/DLH  
  baud();

  // Set default dataformat and save in _config
  // LCR 
  format();  

  // Set dataflow mode and Enables enhanced functions
  // Save in _config
  // EFR
  set_flow_control(); 
  

  // FIFO control, sets TX and RX IRQ trigger levels and enables FIFO and save in _config
  // Note FCR[5:4] only accessible when EFR[4] is set (enhanced functions enable)
  // FCR, TLR
  set_fifo_control();
  flush();

  // Modem control, sets prescaler, enable TCR and TLR
  // Note MCR[7:4] and MCR[2] only accessible when EFR[4] is set (enhanced functions enable)
  set_modem_control();

  // Set RTS trigger levels
  // Note TCR only accessible when EFR[4] is set (enhanced functions enable) and MCR[2] is set
  set_flow_triggers();


  // Set default break condition and save in _config
  // LCR   
  //set_break();
 
  // The UART bridge should now be successfully initialised.

  // Test if UART bridge is present and initialised
  if(!connected()){ 
#if(0)  
    // Lock up if we fail to initialise UART bridge.
    while(1) {};
#else    
    //printf("Failed to initialise UART bridge\r\n");    
#endif    
  }
  else {
    //printf("Initialised UART bridge!\r\n");      
  }

}


/** FIFO control, sets TX and RX trigger levels and enables FIFO and save in _config
  *  Note FCR[5:4] (=TX_IRQ_LVL) only accessible when EFR[4] is set (enhanced functions enable)
  *  Note TLR only accessible when EFR[4] is set (enhanced functions enable) and MCR[2] is set 
  *   @param  none
  *   @return none
  */
void SC16IS750::set_fifo_control() {

  // Set default fifoformat 
  // FCR
  _config.fifoenable = true;  

  // Note FCR[5:4] (=TX_IRQ_LVL) only accessible when EFR[4] is set (enhanced functions enable)
//  _config.fifoformat = FCR_RX_IRQ_8 | FCR_TX_IRQ_56;
  _config.fifoformat = FCR_RX_IRQ_8 | FCR_TX_IRQ_8;  //Default

  if (_config.fifoenable)
    // enable FIFO mode and set FIFO control values  
    this->writeRegister(FCR, _config.fifoformat | FCR_ENABLE_FIFO);
  else
    // disable FIFO mode and set FIFO control values  
    this->writeRegister(FCR, _config.fifoformat);

  // Set Trigger level register TLR for RX and TX interrupt generation
  // Note TLR only accessible when EFR[4] is set (enhanced functions enable) and MCR[2] is set
  //   TRL Trigger levels for RX and TX are 0..15, meaning 0-60 with a granularity of 4 chars    
  // When TLR for RX or TX are 'Zero' the corresponding values in FCR are used. The FCR settings
  // have less resolution (only 4 levels) so TLR is considered an enhanced function.
  this->writeRegister(TLR, 0x00);                                     // Use FCR Levels
//  this->writeRegister(TLR, (TLR_RX_DEFAULT << 4) | TLR_TX_DEFAULT);   // Use Default enhanced levels

}    


/**
  * Flush the UART FIFOs while maintaining current FIFO mode.
  *   @param  none
  *   @return none
  */
void SC16IS750::flush() {
  // FCR is Write Only, use saved _config

  // reset TXFIFO, reset RXFIFO, non FIFO mode  
  this->writeRegister(FCR, FCR_TX_FIFO_RST | FCR_RX_FIFO_RST);
  
  if (_config.fifoenable)
    // enable FIFO mode and set FIFO control values  
    this->writeRegister(FCR, _config.fifoformat | FCR_ENABLE_FIFO);
  else
    // disable FIFO mode and set FIFO control values  
    this->writeRegister(FCR, _config.fifoformat);
  
#if(0)
//original
 /*
   * Flush characters from SC16IS750 receive buffer.
   */

  // Note: This may not be the most appropriate flush approach.
  //       It might be better to just flush the UART's buffer
  //       rather than the buffer of the connected device
  //       which is essentially what this does.
  while(readable() > 0) {
    getc();
  }
#endif

}


/**
  * Check that UART is connected and operational.
  *  @param  none
  *  @return bool true when connected, false otherwise
  */
bool SC16IS750::connected() {
  // Perform read/write test to check if UART is working
  const char TEST_CHARACTER = 'H';

  this->writeRegister(SPR, TEST_CHARACTER);

  return (this->readRegister(SPR) == TEST_CHARACTER);
}


/** Determine if there is a character available to read.
  * This is data that's already arrived and stored in the receive
  * buffer (which holds 64 chars).
  *
  *   @return 1 if there is a character available to read, 0 otherwise
  */
int SC16IS750::readable() {
  
//  if (this->readableCount() > 0) { // Check count
  if (this->readRegister(LSR) & LSR_DR) { // Data in Receiver Bit, at least one character waiting
    return 1;
  }
  else {
    return 0; 
  }
}

/** Determine how many characters are available to read.
  * This is data that's already arrived and stored in the receive
  * buffer (which holds 64 chars).
  *
  *   @return int Characters available to read
  */
int SC16IS750::readableCount() {

  return (this->readRegister(RXLVL));
}

/** Determine if there is space available to write a character.    
  *   @return 1 if there is a space for a character to write, 0 otherwise
  */
int SC16IS750::writable() {
 
//  if ((this->writableCount() > 0) { // Check count
  if (this->readRegister(LSR) & LSR_THRE) { // THR Empty, space for at least one character
    return 1;
  }
  else {
    return 0;  
  }
}

/** Determine how much space available for writing characters.
  * This considers data that's already stored in the transmit
  * buffer (which holds 64 chars).
  *
  *   @return int character space available to write
  */
int SC16IS750::writableCount() {

  return (this->readRegister(TXLVL));  // TX Level
}


/**
  * Read char from UART Bridge.
  * Acts in the same manner as 'Serial.read()'.  
  *   @param none    
  *   @return char read or -1 if no data available. 
  */ 
int SC16IS750::getc() {

  if (!readable()) {
    return -1;
  }

  return this->readRegister(RHR);
}


/**
  * Write char to UART Bridge. Blocking when no free space in FIFO
  *   @param value char to be written    
  *   @return value written  
  */ 
int SC16IS750::putc(int value) {

  while (this->readRegister(TXLVL) == 0) {
    // Wait for space in TX buffer
    wait_us(10);
  };
  this->writeRegister(THR, value);
  
  return value;
}


/**
  * Write char string to UART Bridge. Blocking when no free space in FIFO
  *   @param *str char string to be written    
  *   @return none  
  */
void SC16IS750::writeString(const char *str) {

#if ENABLE_BULK_TRANSFERS
  int len, idx;
  
  len = strlen(str);  

  // Write blocks of BULK_BLOCK_LEN  
  while (len > BULK_BLOCK_LEN) {
    while(this->readRegister(TXLVL) < BULK_BLOCK_LEN) {
      // Wait for space in TX buffer
      wait_us(10);
    };  
  
    // Write a block of BULK_BLOCK_LEN bytes
#if (0)    
    // Note: can be optimized by writing registeraddress once and then repeatedly write the bytes.
    for (idx=0; idx<BULK_BLOCK_LEN; idx++) {
      this->writeRegister(THR, str[idx]);
    };
#else
    // optimized
    this->writeDataBlock(str, BULK_BLOCK_LEN);    
#endif
              
    len -= BULK_BLOCK_LEN;
    str += BULK_BLOCK_LEN;
  }
  
  // Write remaining bytes 
  // Note: can be optimized by writing registeraddress once and then repeatedly write the bytes.  
  for (idx=0; idx<len; idx++) {
    while (this->readRegister(TXLVL) == 0) {
      // Wait for space in TX buffer
      wait_us(10);
    };
    this->writeRegister(THR, str[idx]);
  }  


#else
  // Single writes instead of bulktransfer
  int len, idx;
  
  len = strlen(str);
  for (idx=0; idx<len; idx++) {
    while (this->readRegister(TXLVL) == 0) {
      // Wait for space in TX buffer
      wait_us(10);
    };
    this->writeRegister(THR, str[idx]);
  }  
#endif  
}


/**
  * Write byte array to UART Bridge. Blocking when no free space in FIFO
  *   @param *data byte array to be written    
  *   @param len   number of bytes to write  
  *   @return none  
  */
void SC16IS750::writeBytes(const char *data, int len) {

#if ENABLE_BULK_TRANSFERS
  int idx;
  
  // Write blocks of BULK_BLOCK_LEN  
  while (len > BULK_BLOCK_LEN) {
    while(this->readRegister(TXLVL) < BULK_BLOCK_LEN) {
      // Wait for space in TX buffer
      wait_us(10);
    };  
  
    // Write a block of BULK_BLOCK_LEN bytes
#if (0)    
    // Note: can be optimized by writing registeraddress once and then repeatedly write the bytes.
    for (idx=0; idx<BULK_BLOCK_LEN; idx++) {
      this->writeRegister(THR, data[idx]);
    };
#else
    // optimized
    this->writeDataBlock(data, BULK_BLOCK_LEN);    
#endif
              
    len  -= BULK_BLOCK_LEN;
    data += BULK_BLOCK_LEN;
  }
  
  // Write remaining bytes 
  // Note: can be optimized by writing registeraddress once and then repeatedly write the bytes.  
  for (idx=0; idx<len; idx++) {
    while (this->readRegister(TXLVL) == 0) {
      // Wait for space in TX buffer
      wait_us(10);
    };
    this->writeRegister(THR, data[idx]);
  }  


#else
  // Single writes instead of bulktransfer
  int idx;
  
  for (idx=0; idx<len; idx++) {
    while (this->readRegister(TXLVL) == 0) {
      // Wait for space in TX buffer
      wait_us(10);
    };
    this->writeRegister(THR, str[idx]);
  }  
#endif  
}


/** Set direction of I/O port pins.
  * This method is specific to the SPI-I2C UART and not found on the 16750
  * Note: The SC16IS752 does not have separate GPIOs for Channel_A and Channel_B.
  *   @param  bits Bitpattern for I/O (1=output, 0=input)
  *   @return none
  */
void SC16IS750::ioSetDirection(unsigned char bits) {
  this->writeRegister(IODIR, bits);
}

/** Set bits of I/O port pins.
  * This method is specific to the SPI-I2C UART and not found on the 16750
  * Note: The SC16IS752 does not have separate GPIOs for Channel_A and Channel_B.  
  *   @param  bits Bitpattern for I/O (1= set output bit, 0 = clear output bit)
  *   @return none
  */
void SC16IS750::ioSetState(unsigned char bits) {
  this->writeRegister(IOSTATE, bits);
}

/** Get bits of I/O port pins.
  * This method is specific to the SPI-I2C UART and not found on the 16750
  * Note: The SC16IS752 does not have separate GPIOs for Channel_A and Channel_B.  
  *   @param  none
  *   @return bits Bitpattern for I/O (1= bit set, 0 = bit cleared)
  */
unsigned char SC16IS750::ioGetState() {
  return this->readRegister(IOSTATE) ;
}


/** Software Reset SC16IS750 device.
  * This method is specific to the SPI-I2C UART and not found on the 16750
  * Note: The SC16IS752 does not have separate Reset for Channel_A and Channel_B.  
  *   @param  none
  *   @return none
  */
void SC16IS750::swReset() {
  this->writeRegister(IOCTRL, IOC_SW_RST);     
}


//
// End Abstract Class Implementation
//



//
// Begin SPI Class Implementation
//


/** Create an SC16IS750_SPI object using a specified SPI bus and CS
  *
  * @param SPI &spi the SPI port to connect to 
  * @param cs  Pinname of the CS pin (active low)
  * @param rst Pinname for Reset pin (active low) Optional, Default = NC 
  */    
SC16IS750_SPI::SC16IS750_SPI (SPI *spi, PinName cs, PinName rst) : _spi(spi), _cs(cs)  {
  _cs = 1;  // deselect
  
  _spi->format(8, 0);          
  _spi->frequency(1000000);

  // The hardware Reset pin is optional. Test and make sure whether it exists or not to prevent illegal access.
  if (rst != NC) {
    _reset = new DigitalOut(rst);   //Construct new pin 
    _reset->write(1);               //Deactivate    
  }
  else {
    // No Hardware Reset pin       
    _reset = NULL;                  //Construct dummy pin     
  }  


  // Dont call _init() until SPI port has been configured.
  // That is why _init() is not called in parent Constructor 
  _init();

};


/** Destruct SC16IS750_SPI bridge object
  *
  * @param  none
  * @return none
  */ 
SC16IS750_SPI::~SC16IS750_SPI() {
   if (_reset != NULL) {delete _reset;}  // Reset pin
}

/** Write value to internal register.
  * Pure virtual, must be declared in derived class.   
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @param data              The 8bit value to write
  *   @return none 
  */
void SC16IS750_SPI::writeRegister(RegisterName registerAddress, char data) {

  _cs = 0; //  select;
  _spi->write(registerAddress);
  _spi->write(data);
  _cs = 1; //  deselect;

}


/** Read value from internal register.
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @return char             The 8bit value read from the register
  */
char SC16IS750_SPI::readRegister(RegisterName registerAddress) {

  // Used in SPI read operations to flush slave's shift register
  const char SPI_DUMMY_CHAR = 0xFF; 

  char result;

  _cs = 0; //  select;
  _spi->write(SPI_READ_MODE_FLAG | registerAddress);
  result = _spi->write(SPI_DUMMY_CHAR);
  _cs = 1; //  deselect;

  return result;  
}


/** Write multiple datavalues to Transmitregister.
  * More Efficient implementation than writing individual bytes
  * Assume that previous check confirmed that the FIFO has sufficient free space to store the data  
  * Pure virtual, must be declared in derived class.   
  *   @param char* databytes   The pointer to the block of data
  *   @param len               The number of bytes to write
  *   @return none 
  */
void SC16IS750_SPI::writeDataBlock (const char *data, int len) {
  int i;
  
  _cs = 0; //  select;
  
  // Select the Transmit Holding Register
  // Assume that previous check confirmed that the FIFO has sufficient free space to store the data
  _spi->write(THR);
  
  for (i=0; i<len; i++, data++)
    _spi->write(*data);
    
  _cs = 1; //  deselect;
}


/** Hardware Reset SC16IS750 device.
  * This method is only available when the Reset pin has been declared and is also connected
  *   @param  none
  *   @return none
  */
void SC16IS750_SPI::hwReset() {
  
  if (_reset != NULL){
    _reset->write(0);  //activate
//    wait_ms(100);
#if MBED_MAJOR_VERSION == 5
    rtos::ThisThread::sleep_for(1000); //test only  
#else
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(1000)); //test only  
#endif        
    _reset->write(1);  //deactivate
  }  
  else {
    printf("Hardware Reset pin is not available...\n\r");     
  }

}

//
// End SPI Implementation
//



//
// Begin I2C Implementation
//

/** Create a SC16IS750_I2C object for a bridge between I2C and a Serial port
  *
  * @param I2C &i2c the I2C port to connect to 
  * @param char deviceAddress the I2C slave address of the SC16IS750
  * @param rst Pinname for Reset pin (active low) Optional, Default = NC   
  *  
  */  
SC16IS750_I2C::SC16IS750_I2C(I2C *i2c, uint8_t deviceAddress, PinName rst) : _i2c(i2c), _slaveAddress(deviceAddress & 0xFE) {

//  _i2c->frequency(400000);
  _i2c->frequency(100000);  
 
  // The hardware Reset pin is optional. Test and make sure whether it exists or not to prevent illegal access.
  if (rst != NC) {
    _reset = new DigitalOut(rst);   //Construct new pin 
    _reset->write(1);               //Deactivate    
  }
  else {
    // No Hardware Reset pin       
    _reset = NULL;                  //Construct dummy pin     
  }  
  
  // Dont call _init() until I2C port has been configured.
  // That is why _init() is not called in parent Constructor 
  _init();
}


/** Destruct SC16IS750_I2C bridge object
  *
  * @param  none
  * @return none
  */ 
SC16IS750_I2C::~SC16IS750_I2C() {
   if (_reset != NULL) {delete _reset;}  // Reset pin
}


/** Write value to internal register.
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @param data              The 8bit value to write
  *   @return none 
  */
void SC16IS750_I2C::writeRegister(RegisterName registerAddress, char data) {
  char w[2];

  w[0] = registerAddress;
  w[1] = data;

  _i2c->write( _slaveAddress, w, 2 );
}


/** Read value from internal register.
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @return char             The 8bit value read from the register
  */
char SC16IS750_I2C::readRegister(RegisterName registerAddress) {
  /*
   * Read char from SC16IS750 register at <registerAddress>.
   */
   char w[1];
   char r[1];
    
   w[0] = registerAddress;
    
//   _i2c->write( _slaveAddress, w, 1 );
   _i2c->write(_slaveAddress, w, 1, true);   //Repeated Start   
   _i2c->read( _slaveAddress, r, 1 );

   return ( r[0] );
}


/** Write multiple datavalues to Transmitregister.
  * More Efficient implementation than writing individual bytes
  * Assume that previous check confirmed that the FIFO has sufficient free space to store the data
  * Pure virtual, must be declared in derived class.   
  *   @param char* databytes   The pointer to the block of data
  *   @param len               The number of bytes to write
  *   @return none 
  */
void SC16IS750_I2C::writeDataBlock (const char *data, int len) {
  
#if(0)  
  int i;
  char w[BULK_BLOCK_LEN];
  
  // Select the Transmit Holding Register
  // Assume that previous check confirmed that the FIFO has sufficient free space to store the data
  w[0] = THR;

  // copy the data..
  for (i=0; i<len; i++)
    w[i+1] = data[i];
    
  _i2c->write( _slaveAddress, w, len + 1);  
#else
  int i;
  
  _i2c->start();
  _i2c->write(_slaveAddress); 

  // Select the Transmit Holding Register
  // Assume that previous check confirmed that the FIFO has sufficient free space to store the data
  _i2c->write(THR);

  // send the data..
  for (i=0; i<len; i++)
    _i2c->write(data[i]);
    
  _i2c->stop();  
#endif
}


/** Hardware Reset SC16IS750 device.
  * This method is only available when the Reset pin has been declared and is also connected
  *   @param  none
  *   @return none
  */
void SC16IS750_I2C::hwReset() {
  
  if (_reset != NULL){
    _reset->write(0);  //activate
//    wait_ms(100);
#if MBED_MAJOR_VERSION == 5
    rtos::ThisThread::sleep_for(1000); //test only  
#else
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(1000)); //test only  
#endif             
    _reset->write(1);  //deactivate
  }  
  else {
    printf("Hardware Reset pin is not available...\n\r");     
  }
}


//
// End I2C Implementation
//



//
// Begin SPI Class Implementation for 16SCIS752 dual UART
//


/** Create an SC16IS752_SPI object using a specified SPI bus and CS
  * Note: The SC16IS752 does not have separate GPIOs for Channel_A and Channel_B. 
  * Note: The SC16IS752 does not have separate Reset for Channel_A and Channel_B.     
  *
  * @param SPI &spi the SPI port to connect to 
  * @param cs  Pinname of the CS pin (active low)
  * @param rst Pinname for Reset pin (active low) Optional, Default = NC 
  * @param channel UART ChannelName, Default = Channel_A      
  */    
SC16IS752_SPI::SC16IS752_SPI (SPI *spi, PinName cs, PinName rst, ChannelName channel) : _spi(spi), _cs(cs), _channel(channel)  {
  _cs = 1;  // deselect
  
  _spi->format(8, 0);          
  _spi->frequency(1000000);

  // The hardware Reset pin is optional. Test and make sure whether it exists or not to prevent illegal access.
  if (rst != NC) {
    _reset = new DigitalOut(rst);   //Construct new pin 
    _reset->write(1);               //Deactivate    
  }
  else {
    // No Hardware Reset pin       
    _reset = NULL;                  //Construct dummy pin     
  }  


  // Dont call _init() until SPI port has been configured.
  // That is why _init() is not called in parent Constructor 
  _init();

};


/** Destruct SC16IS750_SPI bridge object
  *
  * @param  none
  * @return none
  */ 
SC16IS752_SPI::~SC16IS752_SPI() {
   if (_reset != NULL) {delete _reset;}  // Reset pin
}


/** Write value to internal register.
  * Pure virtual, must be declared in derived class.   
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @param data              The 8bit value to write
  *   @return none 
  */
void SC16IS752_SPI::writeRegister(RegisterName registerAddress, char data) {

  _cs = 0; //  select;
  _spi->write(registerAddress | _channel);
  _spi->write(data);
  _cs = 1; //  deselect;

}


/** Read value from internal register.
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @return char             The 8bit value read from the register
  */
char SC16IS752_SPI::readRegister(RegisterName registerAddress) {

  // Used in SPI read operations to flush slave's shift register
  const char SPI_DUMMY_CHAR = 0xFF; 

  char result;

  _cs = 0; //  select;
  _spi->write(SPI_READ_MODE_FLAG | registerAddress | _channel);
  result = _spi->write(SPI_DUMMY_CHAR);
  _cs = 1; //  deselect;

  return result;  
}


/** Write multiple datavalues to Transmitregister.
  * More Efficient implementation than writing individual bytes
  * Assume that previous check confirmed that the FIFO has sufficient free space to store the data  
  * Pure virtual, must be declared in derived class.   
  *   @param char* databytes   The pointer to the block of data
  *   @param len               The number of bytes to write
  *   @return none 
  */
void SC16IS752_SPI::writeDataBlock (const char *data, int len) {
  int i;
  
  _cs = 0; //  select;
  
  // Select the Transmit Holding Register
  // Assume that previous check confirmed that the FIFO has sufficient free space to store the data
  _spi->write(THR | _channel);
  
  for (i=0; i<len; i++, data++)
    _spi->write(*data);
    
  _cs = 1; //  deselect;
}


/** Hardware Reset SC16IS752 device.
  * This method is only available when the Reset pin has been declared and is also connected
  *   @param  none
  *   @return none
  */
void SC16IS752_SPI::hwReset() {
  
  if (_reset != NULL){
    _reset->write(0);  //activate
//    wait_ms(100);
#if MBED_MAJOR_VERSION == 5
    rtos::ThisThread::sleep_for(1000); //test only  
#else
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(1000)); //test only  
#endif             
    _reset->write(1);  //deactivate
  }  
  else {
    printf("Hardware Reset pin is not available...\n\r");     
  }

}

//
// End SPI Implementation
//



//
// Begin I2C Implementation for 16SCIS752 dual UART
//

/** Create a SC16IS752_I2C object for a bridge between I2C and a Serial port
  * Note: The SC16IS752 does not have separate GPIOs for Channel_A and Channel_B. 
  * Note: The SC16IS752 does not have separate Reset for Channel_A and Channel_B.     
  *
  * @param I2C &i2c the I2C port to connect to 
  * @param char deviceAddress the I2C slave address of the SC16IS750
  * @param rst Pinname for Reset pin (active low) Optional, Default = NC   
  * @param channel UART Channel, Default = Channel_A    
  */  
SC16IS752_I2C::SC16IS752_I2C(I2C *i2c, uint8_t deviceAddress, PinName rst, ChannelName channel) : _i2c(i2c), _slaveAddress(deviceAddress & 0xFE), _channel(channel) {

//  _i2c->frequency(400000);
  _i2c->frequency(100000);
   
  // The hardware Reset pin is optional. Test and make sure whether it exists or not to prevent illegal access.
  if (rst != NC) {
    _reset = new DigitalOut(rst);   //Construct new pin 
    _reset->write(1);               //Deactivate    
  }
  else {
    // No Hardware Reset pin       
    _reset = NULL;                  //Construct dummy pin     
  }  
  
  // Dont call _init() until I2C port has been configured.
  // That is why _init() is not called in parent Constructor 
  _init();
}


/** Destruct SC16IS752_I2C bridge object
  *
  * @param  none
  * @return none
  */ 
SC16IS752_I2C::~SC16IS752_I2C() {
   if (_reset != NULL) {delete _reset;}  // Reset pin
}


/** Write value to internal register.
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @param data              The 8bit value to write
  *   @return none 
  */
void SC16IS752_I2C::writeRegister(RegisterName registerAddress, char data) {
  char w[2];

  w[0] = registerAddress | _channel;
  w[1] = data;

  _i2c->write( _slaveAddress, w, 2 );
}


/** Read value from internal register.
  *   @param registerAddress   The address of the Register (enum RegisterName)
  *   @return char             The 8bit value read from the register
  */
char SC16IS752_I2C::readRegister(RegisterName registerAddress) {
  /*
   * Read char from SC16IS752 register at <registerAddress>.
   */
   char w[1];
   char r[1];
    
   w[0] = registerAddress | _channel;
    
//   _i2c->write( _slaveAddress, w, 1 );
   _i2c->write(_slaveAddress, w, 1, true);   //Repeated Start
   _i2c->read(_slaveAddress, r, 1 );

   return ( r[0] );
}


/** Write multiple datavalues to Transmitregister.
  * More Efficient implementation than writing individual bytes
  * Assume that previous check confirmed that the FIFO has sufficient free space to store the data
  * Pure virtual, must be declared in derived class.   
  *   @param char* databytes   The pointer to the block of data
  *   @param len               The number of bytes to write
  *   @return none 
  */
void SC16IS752_I2C::writeDataBlock (const char *data, int len) {
  
#if(0)  
  int i;
  char w[BULK_BLOCK_LEN];
  
  // Select the Transmit Holding Register
  // Assume that previous check confirmed that the FIFO has sufficient free space to store the data
  w[0] = THR | _channel;

  // copy the data..
  for (i=0; i<len; i++)
    w[i+1] = data[i];
    
  _i2c->write( _slaveAddress, w, len + 1);  
#else
  int i;
  
  _i2c->start();
  _i2c->write(_slaveAddress); 

  // Select the Transmit Holding Register
  // Assume that previous check confirmed that the FIFO has sufficient free space to store the data
  _i2c->write(THR | _channel);

  // send the data..
  for (i=0; i<len; i++)
    _i2c->write(data[i]);
    
  _i2c->stop();  
#endif
}


/** Hardware Reset SC16IS752 device.
  * This method is only available when the Reset pin has been declared and is also connected
  *   @param  none
  *   @return none
  */
void SC16IS752_I2C::hwReset() {
  
  if (_reset != NULL){
    _reset->write(0);  //activate
//    wait_ms(100);
#if MBED_MAJOR_VERSION == 5
    rtos::ThisThread::sleep_for(1000); //test only  
#else
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(1000)); //test only  
#endif              
    _reset->write(1);  //deactivate
  }  
  else {
    printf("Hardware Reset pin is not available...\n\r");     
  }
}


//
// End I2C Implementation
//



