////////////////////////////////////////////////////////////////////////////////////////////////////
/// \file	WheelWatcher.h
///
/// \brief	Declares the WheelWatcher class
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _WHEEL_COMMANDER_H_
#define _WHEEL_COMMANDER_H_

#if defined(ARDUINO) && ARDUINO >= 100
  #include <Arduino.h>
#else
  #include <WProgram.h>
#endif
#include <HardwareSerial.h>
#include <inttypes.h>

#define TRUE 1
#define FALSE 0
typedef unsigned char BOOL;

#define WW_SYNC '.'
#define WW_ACCEL_CMD 'A'
#define WW_VEL_CMD 'V'
#define WW_DST_CMD 'D'
#define WW_STAT_CMD 'S'
#define WW_ECHO_CMD 'E'
#define WW_RESET_CMD 'R'
#define WW_NAME_CMD 'N'
#define WW_I2C_ADDR_CMD 'T'
#define WW_CONSTS_CMD 'F'
#define WW_UPDATE_CMD 'U'

#define WW_ACK 'a'
#define WW_NACK 'n'

#define WW_SLOW_COMMS 0x01  // -- user settable mode -- access via bytes[1]
#define WW_NOTIFY 0x02
#define WW_SHORT_MODE 0x04

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \class  WheelWatcher : public NdiMover
///
/// \author Pete Skeggs
/// \date 3/2/2012
///
/// \brief 
////////////////////////////////////////////////////////////////////////////////////////////////////

class WheelWatcher
{
public:

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// \brief Constructor. 
	///
	/// \remarks Pete Skeggs, 3/2/2012
	///
	////////////////////////////////////////////////////////////////////////////////////////////////////

	WheelWatcher();


   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Initializes the WheelWatcher to use the serial port. 
   /// 
   /// \param	baud - the number of bits per second the WheelWatcher is already configured for.
   /// \param  serial_port - 0 for most Arduino boards; Mega can take 0-3.
   ///
   /// \retval true if it succeeds, false if it fails. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   BOOL InitSerial(long baud, int serial_port = 0);

   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Initializes the WheelWatcher to use I2C using the specified 7 bit slave address.
   ///
   /// \param slave_address - 7 bit I2C address of WheelWatcher board.
   ///
   /// \retval true if it succeeds, false if it fails. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   BOOL InitI2C(unsigned int slave_address);


#pragma pack(1)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \struct ww_stat  
///
/// \author Pete Skeggs
/// \date 3/2/2012
///
/// \brief . 
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct ww_stat {
   uint8_t cmd_overflow : 1;
   uint8_t port_overflow : 1;
   uint8_t dir : 1;
   uint8_t cha : 1;
   uint8_t chb : 1;
} STAT;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief the ww mode union. 
///
/// \retval The ww mode union. 
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef union _ww_mode_union
{
	struct _sm_mode {
		uint8_t slow_comms : 1;
		uint8_t notify : 1;
		uint8_t short_mode : 1;
		uint8_t mode : 3;
	} flags;
	uint8_t data;
} WW_MODE;

typedef enum _ww_operating_mode_
{
	WM_QUADRATURE, WM_SIGNMAG, WM_SERIAL, WM_I2C
} WW_OPMODE;


#pragma pack()


   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Gets the status. 
   ///
   /// \remarks Pete Skeggs, 3/2/2012
   ///
   /// \param	stat  - [in/out] a stat. 
   ///
   /// \retval true if it succeeds, false if it fails. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   BOOL GetStatus(STAT &stat);

   // eeprom addresses

   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \enum _eeprom_addrs
   ///
   /// \author Pete Skeggs
   /// \date 3/2/2012
   ///
   /// \brief Values that represent _eeprom_addrs. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   typedef enum _eeprom_addrs 
   {
         A_skip, A_Written, A_Mode, A_Baud, A_I2CAddr, 
         A_End
   } EEADDR;


   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Sets a mode. 
   ///
   /// \remarks Pete Skeggs, 3/2/2012
   ///
   /// \param	mode  - A mode. 
   ///
   /// \retval true if it succeeds, false if it fails. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   BOOL SetMode(WW_MODE mode);

   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Sets a 2 c 7 bit addr. 
   ///
   /// \remarks Pete Skeggs, 3/2/2012
   ///
   /// \param	i2c_7bit_address  - A 2c 7bit address. 
   ///
   /// \retval true if it succeeds, false if it fails. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   BOOL SetI2C7BitAddr(uint8_t i2c_7bit_address); // this takes a 7 bit address

   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Gets a 2 c 7 bit addr. 
   ///
   /// \remarks Pete Skeggs, 3/2/2012
   ///
   /// \param	i7bitadr  - [in/out] a 7bitadr. 
   ///
   /// \retval true if it succeeds, false if it fails. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   BOOL GetI2C7BitAddr(uint8_t &i7bitadr);

   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Gets an encoder properties. 
   ///
   /// \remarks Pete Skeggs, 3/2/2012
   ///
   /// \param	quadrature          - [in/out] a quadrature. 
   /// \param	ticks_per_rotation  - [in/out] the ticks per rotation. 
   ///
   /// \retval true if it succeeds, false if it fails. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   BOOL GetEncoderProperties(WW_OPMODE &opmode, uint16_t &ticks_per_rotation);

   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Sets a constant. 
   ///
   /// \remarks Pete Skeggs, 3/2/2012
   ///
   /// \param	addr          - An addr. 
   /// \param	const_value  - A const value. 
   ///
   /// \retval true if it succeeds, false if it fails. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   BOOL SetConstant(EEADDR addr, uint8_t const_value);

   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Gets a constant. 
   ///
   /// \remarks Pete Skeggs, 3/2/2012
   ///
   /// \param	addr          - An addr. 
   /// \param	const_value  - [in/out] a const value. 
   ///
   /// \retval true if it succeeds, false if it fails. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   BOOL GetConstant(EEADDR addr, uint8_t &const_value);

   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Gets a distance. 
   ///
   /// \remarks Pete Skeggs, 3/2/2012
   ///
   /// \param	dist      - [in/out] a dist. 
   /// \param	timeout  - A timeout. 
   ///
   /// \retval true if it succeeds, false if it fails. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   BOOL GetDistance(long &dist, int timeout = 5000);
   BOOL ToggleDistance(void);

   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Gets a velocity. 
   ///
   /// \remarks Pete Skeggs, 3/2/2012
   ///
   /// \param	vel      - [in/out] a vel. 
   /// \param	timeout  - A timeout. 
   ///
   /// \retval true if it succeeds, false if it fails. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   BOOL GetVelocity(int &vel, int timeout = 5000);
   BOOL ToggleVelocity(void);

   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Gets an acceleration. 
   ///
   /// \remarks Pete Skeggs, 3/2/2012
   ///
   /// \param	accel  - [in/out] an accel. 
   /// \param	timeout  - A timeout. 
   ///
   /// \retval true if it succeeds, false if it fails. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   BOOL GetAcceleration(int &accel, int timeout = 5000);

   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Sets a distance. 
   ///
   /// \remarks Pete Skeggs, 3/2/2012
   ///
   /// \param	dist      - [in/out] a dist. 
   /// \param	timeout  - A timeout. 
   ///
   /// \retval true if it succeeds, false if it fails. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   BOOL SetDistance(long int dist, int timeout = 5000);

   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Gets a name. 
   ///
   /// \remarks Pete Skeggs, 3/2/2012
   ///
   /// \param	name      - If non-null, a name. 
   /// \param	version  - [in/out] a version. 
   /// \param	timeout  - A timeout. 
   ///
   /// \retval true if it succeeds, false if it fails. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   BOOL GetName(char *name, int &version, int timeout = 10000);

   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Resets this object. 
   ///
   /// \remarks Pete Skeggs, 3/2/2012
   ///
   /// \retval true if it succeeds, false if it fails. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   BOOL Reset();

   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Synchronises this object. 
   ///
   /// \remarks Pete Skeggs, 3/2/2012
   ///
   /// \retval true if it succeeds, false if it fails. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   BOOL Sync();

   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Tests comm. 
   ///
   /// \remarks Pete Skeggs, 3/2/2012
   ///
   /// \param	first - The first byte value to test using the Echo command. 
   /// \param	last  - The last byte value to test. 
   /// \param	step  - Amount to increment by starting at first and ending at last. 
   ///
   /// \retval true if the test passes, false if the test fails. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////

   BOOL CommTest(uint8_t first = 0, uint8_t last = 255, uint8_t step = 1, int timeout = 1000);

   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /// \brief Periodic update task; needs to be called from loop(). 
   ///
   /// \remarks Pete Skeggs, 3/2/2012
   ///
   /// \retval true if output is available, false if not. 
   ////////////////////////////////////////////////////////////////////////////////////////////////////
   BOOL Process(void);


   WW_MODE curmode;
   BOOL ready;
   BOOL i2c;
   HardwareSerial *pSerial;
   int port;
   int slave_addr;
   int little_endian; // i2c buffers are little-endian, serial are big-endian

   private:
      long distance;
      int velocity;

      int available(void);
      void start_read(int num);
      void end_read();
      void start_write(void);
      void end_write(void);
      int calc_num(uint8_t command_bytes, uint8_t data_bytes);
      int read(void);
      void flush(void);
      void write(uint8_t val);
      BOOL checkAck(int timeout = 5000);
      BOOL waitFor(int timeout, int chars);
      BOOL checkEOL(void);
      uint8_t getASCIIHexByte(void);
      uint8_t getByte(void);
      uint16_t getWord(void);
      uint32_t getDword(void);
      void putByte(uint8_t val);
      void putWord(uint16_t val);
      void putDword(uint32_t val);
      BOOL doWordCommand(char command, uint16_t &val, int timeout = 5000);
      BOOL doDwordCommand(char command, uint32_t &val, int timeout = 5000);
};

#endif
