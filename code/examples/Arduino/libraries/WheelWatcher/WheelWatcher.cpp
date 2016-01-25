/******************************************************************************/
/* WW-11 / WW-12 WheelWatcher Library for Arduino 0022 - 1.0                  */
/*                                                                            */
/* Copyright 2009-2012, Noetic Design, Inc.                                   */
/*                                                                            */
/******************************************************************************/

#if defined(ARDUINO) && ARDUINO >= 100
  #include <Arduino.h>
#else
  #include <WProgram.h>
#endif
#include <HardwareSerial.h>
#include <Wire.h>
#include "WheelWatcher.h"

#define DEFAULT_TIMEOUT 500

#if defined(UBRR3H)
#define NUM_SERIAL_PORTS 4
#elif defined(UBRR2H)
#define NUM_SERIAL_PORTS 3
#elif defined(UBRR1H)
#define NUM_SERIAL_PORTS 2
#elif defined(UBRRH) || defined(UBRR0H)
#define NUM_SERIAL_PORTS 1
#else
#define NUM_SERIAL_PORTS 0
#endif

WheelWatcher::WheelWatcher()
{
   ready = FALSE;
   pSerial = NULL;
   port = 0;
   velocity = 0;
   distance = 0;
}

BOOL WheelWatcher::InitSerial(long baud, int serial_port)
{
   char name[3] = "  ";
   int version;

   pSerial = NULL;
   i2c = FALSE;
   little_endian = FALSE;
   if ((serial_port < 0) || (serial_port >= NUM_SERIAL_PORTS))
   {
      return FALSE;
   }
   switch(serial_port)
   {
#if defined(UBRRH) || defined(UBRR0H)
      case 0:
         pSerial = &Serial;      // support single serial port on Uno, Duemilanove, etc.
         break;
#endif
#if defined(UBRR1H)
      case 1:
         pSerial = &Serial1;     // support additional serial ports on Arduinos that offer them, such as the Mega 2560
         break;
#endif
#if defined(UBRR2H)
      case 2:
         pSerial = &Serial2;
         break;
#endif
#if defined(UBRR3H)
      case 3:
         pSerial = &Serial3;
         break;
#endif
      default:
         return FALSE;
   }
   port = serial_port;
   pSerial->begin(baud);
   flush();
   if (!Sync())
      return FALSE;
   return ready = TRUE;
}

// WARNING: not supported with currently shipping WW-11 or WW-12!
BOOL WheelWatcher::InitI2C(unsigned int slave_address)
{
   char name[3] = "  ";
   int version;

   i2c = TRUE;
   little_endian = TRUE;
   ready = TRUE;
   slave_addr = slave_address;
   Wire.begin();
   if (!Sync())
      return FALSE;
   return TRUE;
}

/**************************************************/
/* start of device-independent transport routines */

void WheelWatcher::start_read(int num)
{
   delayMicroseconds(100);
   if (i2c)
      Wire.requestFrom(slave_addr, num);
}

void WheelWatcher::end_read()
{
   flush();
}

void WheelWatcher::start_write(void)
{
   delayMicroseconds(100);
   if (i2c)
      Wire.beginTransmission(slave_addr);
}

void WheelWatcher::end_write(void)
{
   if (i2c)
      Wire.endTransmission();
   else
   {
      pSerial->write('\n');
   }
}

int WheelWatcher::available(void)
{
   if (i2c)
      return (int)Wire.available();
   else
      return pSerial->available();
}

int WheelWatcher::read(void)
{
   if (i2c)
#if defined(ARDUINO) && ARDUINO >= 100
      return (int)Wire.read();
#else
      return (int)Wire.receive();
#endif
   else
   {
      int data = pSerial->read();
      return data;
   }
}

void WheelWatcher::write(uint8_t val)
{
   if (i2c)
#if defined(ARDUINO) && ARDUINO >= 100
      Wire.write(val);
#else
      Wire.send(val);
#endif
   else
   {
      pSerial->write(val);
   }
}

void WheelWatcher::flush(void)
{
   if (i2c)
   {
      while (Wire.available())
#if defined(ARDUINO) && ARDUINO >= 100
         Wire.read();
#else
         Wire.receive();
#endif
   }
   else
      pSerial->flush();
}

int WheelWatcher::calc_num(uint8_t command_bytes, uint8_t data_bytes)
{
   if (i2c)
      return command_bytes + data_bytes;
   else
      return command_bytes + data_bytes * 2 + 1; // serial takes an extra terminator character
}

/* end of device-independent transport routines   */
/**************************************************/

/**************************************************/
/* start of helper functions                      */

BOOL WheelWatcher::waitFor(int timeout, int chars)
{
   unsigned long timeToStop = (unsigned long)timeout + millis();
   while (available() < chars)
   {
      if (millis() > timeToStop)
         return FALSE;
   }
   return TRUE;
}

BOOL WheelWatcher::checkEOL(void)
{
   // assume the character is already there
   int val;

   if (i2c)
      return !available(); // there should be no more pending characters

   val = read();
   if ((val == '\n') || (val == '\r') || (val == '\0'))
      return TRUE;
   else
      return FALSE;
}

BOOL WheelWatcher::checkAck(int timeout)
{
   int num = i2c ? 1 : 2;
   uint8_t val;

   // Arduino Wire library can't do RESTART, so can't get ack byte correctly; have to delay to allow for turnaround time in WC firmware
   // when doing a two-phase transfer like this
   delayMicroseconds(875);
   start_read(num);
   if (waitFor(timeout, num))
   {
      if ((val = read()) == WW_ACK)
      {
         if (checkEOL())
         {
            end_read();
            return TRUE;
         }
      }
   }
   end_read();
   return FALSE;
}

uint8_t WheelWatcher::getASCIIHexByte(void)
{
   char msn = (char)read() - '0';
   char lsn = (char)read() - '0';

   if (msn > 9)
      msn -= 7;
   if (lsn > 9)
      lsn -= 7;
   return (uint8_t)(((msn << 4) & 0xf0) | (lsn & 0x0f));
}

uint8_t WheelWatcher::getByte()
{
   if (i2c)
      return (uint8_t)read();    // i2c is natively byte-oriented
   else
      return getASCIIHexByte();  // serial is ASCII hex formatted, so each byte received represents a nibble, most significant first
}

uint16_t WheelWatcher::getWord()
{
   uint16_t val, val1, val2;

   val1 = (uint16_t)getByte();
   val2 = (uint16_t)getByte();

   if (little_endian)
   {
      val = val2;
      val <<= 8;
      val += val1;
   }
   else
   {
      val = val1;
      val <<= 8;
      val += val2;
   }
   return val;
}

uint32_t WheelWatcher::getDword()
{
   uint32_t val, val1, val2;

   val1 = (uint32_t)getWord();
   val2 = (uint32_t)getWord();

   if (little_endian)
   {
      val = val2;
      val <<= 16;
      val += val1;
   }
   else
   {
      val = val1;
      val <<= 16;
      val += val2;
   }
   return val;
}

void WheelWatcher::putByte(uint8_t val)
{
   char msn; 
   char lsn;

   if (i2c)
   {
      write(val); // i2c is natively byte-oriented
      return;
   }

   msn = (val >> 4) & 0x0f; // serial is nibble / ASCII hex oriented
   lsn = val & 0x0f;
   if (msn > 9)
      msn += 7;
   msn += '0';
   if (lsn > 9)
      lsn += 7;
   lsn += '0';

   write(msn);
   write(lsn);
}

void WheelWatcher::putWord(uint16_t val)
{
   uint8_t val1, val2;

   val1 = (uint8_t)(val >> 8);
   val2 = (uint8_t)(val & 0x0ff);

   if (little_endian)
   {
      putByte(val2);
      putByte(val1);
   }
   else
   {
      putByte(val1);
      putByte(val2);
   }
}

void WheelWatcher::putDword(uint32_t val)
{
   uint16_t val1, val2;

   val1 = (uint16_t)(val >> 16);
   val2 = (uint16_t)(val & 0x0ffff);
   if (little_endian)
   {
      putWord(val2);
      putWord(val1);
   }
   else
   {
      putWord(val1);
      putWord(val2);
   }
}

BOOL WheelWatcher::doWordCommand(char command, uint16_t &val, int timeout)
{
   int num = calc_num(1, 2);
   char cmd;

   start_write();
   write(command);
   end_write();

   start_read(num);
   if (!waitFor(timeout, num))
   {
      end_read();
      return FALSE;
   }
   if ((cmd = read()) != command)
   {
      end_read();
      return FALSE;
   }
   val = getWord();
   if (!checkEOL())
   {
      end_read();
      return FALSE;
   }
   end_read();
   return TRUE;
}


BOOL WheelWatcher::doDwordCommand(char command, uint32_t &val, int timeout)
{
   int num = calc_num(1, 4);
   char cmd;

   start_write();
   write(command);
   end_write();

   start_read(num);
   if (!waitFor(timeout, num))
   {
      end_read();
      return FALSE;
   }
   if ((cmd = read()) != command)
   {
      end_read();
      return FALSE;
   }
   val = getDword();
   if (!checkEOL())
   {
      end_read();
      return FALSE;
   }
   end_read();
   return TRUE;
}

/* end of helper functions                        */
/**************************************************/

/**************************************************/
/* start of command handlers                      */

BOOL WheelWatcher::Sync()
{
   for (int i = 0; i < 10; i++)
   {
      start_write();
      write(WW_SYNC);
      end_write();

      start_read(1);
      if (waitFor(50 * (i + 1), 1))
      {
         int val = read();
         end_read();
         if (val == WW_SYNC)
         {
            delay(20);
            flush();
            return TRUE;
         }
      }
      end_read();
   }
   return FALSE;
}

BOOL WheelWatcher::GetName(char *name, int &version, int timeout)
{
   int cmd;
   int num = calc_num(5, 0);

   start_write();
   write(WW_NAME_CMD);
   end_write();

   start_read(num);
   if (!waitFor(timeout, num))
   {
      end_read();
      return FALSE;
   }
   if ((cmd = read()) != WW_NAME_CMD)
   {
      end_read();
      return FALSE;
   }
   if (name)
   {
      name[0] = read();
      name[1] = read();
      name[2] = '\0';
   }
   version = (int)getASCIIHexByte();
   if (!checkEOL())
   {
      end_read();
      return FALSE;
   }
   end_read();
   return TRUE;
}

BOOL WheelWatcher::Reset()
{
   start_write();
   write(WW_RESET_CMD);
   end_write();
   return checkAck();
}

BOOL WheelWatcher::CommTest(uint8_t first, uint8_t last, uint8_t step, int timeout)
{
   int num = calc_num(1, 1);
   uint8_t echo;
   uint8_t swapped_echo;

   for (int val = first; val <= last; val += step)
   {
      start_write();
      write(WW_ECHO_CMD);
      putByte(val);
      end_write();

      start_read(num);
      if (!waitFor(timeout, num))
      {
         end_read();
         return FALSE;
      }
      if (read() != WW_ECHO_CMD)
      {
         end_read();
         return FALSE;
      }
      echo = getByte();
      if (!checkEOL())
      {
         end_read();
         return FALSE;
      }
      end_read();
      swapped_echo = ((val & 0x0f) << 4) | ((val >> 4) & 0x0f);
      if (swapped_echo != echo)
         return FALSE;
   }
   return TRUE;
}

BOOL WheelWatcher::GetStatus(STAT &stat)
{
   return doWordCommand(WW_STAT_CMD, (uint16_t &)stat);
}

//
// NOTE: you cannot override the operating mode bits that select quadrature vs. sign/magnitude vs. serial vs. i2c -- these are set using the mode pins
//
BOOL WheelWatcher::SetMode(WW_MODE mode)
{
   return SetConstant(A_Mode, mode.data);
}

BOOL WheelWatcher::SetI2C7BitAddr(uint8_t i2c_7bit_address)
{
   start_write();
   write(WW_I2C_ADDR_CMD);
   write(i2c_7bit_address * 2);
   end_write();
   return checkAck();
}

BOOL WheelWatcher::GetI2C7BitAddr(uint8_t &i7bitadr)
{
   uint8_t i8bitadr;
   if (GetConstant(A_I2CAddr, i8bitadr))
   {
      i7bitadr = i8bitadr / 2;
      return TRUE;
   }
   return FALSE;
}

BOOL WheelWatcher::GetEncoderProperties(WW_OPMODE &opmode, uint16_t &ticks_per_rotation)
{
   WW_MODE mode;

   if (GetConstant(A_Mode, mode.data))
   {
      opmode = (WW_OPMODE)mode.flags.mode;
      ticks_per_rotation = 128;
      return TRUE;
   }
   return FALSE;
}


BOOL WheelWatcher::GetDistance(long &dist, int timeout)
{
   if (i2c)
      return doDwordCommand(WW_DST_CMD, (uint32_t &)dist, timeout);
   else
   {
      dist = distance;
      return TRUE;
   }
}

BOOL WheelWatcher::ToggleDistance(void)
{
   if (!i2c)
   {
      start_write();
      write(WW_DST_CMD);
      end_write();
      return checkAck();
   }
   else
      return TRUE;
}

BOOL WheelWatcher::GetVelocity(int &vel, int timeout)
{
   if (i2c)
      return doWordCommand(WW_VEL_CMD, (uint16_t &)vel, timeout);
   else
   {
      vel = velocity;
      return TRUE;
   }
}

BOOL WheelWatcher::ToggleVelocity(void)
{
   if (!i2c)
   {
      start_write();
      write(WW_VEL_CMD);
      end_write();
      return checkAck();
   }
   else
      return TRUE;
}

BOOL WheelWatcher::GetAcceleration(int &accel, int timeout)
{
   return doWordCommand(WW_ACCEL_CMD, (uint16_t &)accel, timeout);
}

//
// change the WheelWatcher's internal distance counter
//
BOOL WheelWatcher::SetDistance(long int dist, int timeout)
{
   start_write();
   write(WW_DST_CMD);
   putDword(dist);
   end_write();
   return checkAck(timeout);
}


//
// NOTE: you cannot override the operating mode or the baud rate -- the mode is set by the mode pins, and the baud rate is fixed at compile time to 38400
// you can, however, change the slow_comms, notify, and short_mode bits
//
BOOL WheelWatcher::SetConstant(EEADDR addr, uint8_t const_value)
{
   start_write();
   write(WW_CONSTS_CMD);
   putByte(addr);
   putByte(const_value);
   end_write();

   if (addr != A_Baud)
      return checkAck(DEFAULT_TIMEOUT);
   else
      return TRUE;
}

BOOL WheelWatcher::GetConstant(EEADDR addr, uint8_t &const_value)
{
   uint8_t raddr;
   uint8_t rconst;
   int num = calc_num(1, 2);

   start_write();
   write(WW_CONSTS_CMD);
   putByte(addr);
   end_write();

   start_read(num);
   if (!waitFor(DEFAULT_TIMEOUT, num))
   {
      end_read();
      return FALSE;
   }
   if (read() != WW_CONSTS_CMD)
   {
      end_read();
      return FALSE;
   }
   raddr = getByte();
   rconst = getByte();
   if (!checkEOL())
   {
      end_read();
      return FALSE;
   }
   end_read();
   const_value = rconst;
   if (addr != raddr)
      return FALSE;
   return TRUE;
}

BOOL WheelWatcher::Process(void)
{
   char cmd;

   if (i2c)
      return FALSE;
   if (available())
   {
      start_read(1);
      cmd = read();
      if (cmd == 'V')
      {
         if (waitFor(DEFAULT_TIMEOUT, calc_num(0, 2)))
            velocity = (int)getWord();
      }
      else if (cmd == 'D')
      {
         if (waitFor(DEFAULT_TIMEOUT, calc_num(0, 4)))
            distance = (long)getDword();
      }
      end_read();
      return TRUE;
   }
   return FALSE;
}

/* end of command handlers                        */
/**************************************************/

