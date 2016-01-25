// WheelWatcher Serial Test

#include <Wire.h> // need to include this, as WheelWatcher.cpp needs it (and it must be included in the Sketch -- just including it in Unicoder.h does not work)
#include <WheelWatcher.h>

//#define MEGA 1
#ifdef MEGA
#define SERIAL_PORT 1
#define debug Serial
#define DEBUG_BAUD 38400
#else
#include <SoftwareSerial.h>
SoftwareSerial mySerial(2, 3);   // you can pick whatever you want here that's compatible with the software serial library
#define SERIAL_PORT 0
#define debug mySerial
#define DEBUG_BAUD 9600
#endif

WheelWatcher ww;

void toggle_led()
{
   static bool toggle = false;
   digitalWrite(13, toggle);
   toggle = !toggle;
}

// fast blink = fail
void fail(int code)
{
   debug.print("Fatal Error ");
   debug.println(code);
   ww.Sync();
   for (;;)
   {
      digitalWrite(13, LOW); // turn LED off
      delay(3000);
      for (int i = 0; i < code; i++)
      {
        toggle_led();
        delay(250);
        toggle_led();
        delay(250);
      }
   }
}

void setup()                                                                   // run once, when the sketch starts
{
   char name[4];
   int ver;

   debug.begin(DEBUG_BAUD);
   debug.println("Mega Serial WheelWatcher Test");

   // slow blink = start
   pinMode(13, OUTPUT);
   toggle_led();
   delay(500);  
   toggle_led();
   delay(500);  
   toggle_led();
   delay(500);  

   if (!ww.InitSerial(38400, SERIAL_PORT))
      fail(1);

   if (!ww.Sync())
      fail(2);

   if (!ww.CommTest())
      fail(3);

   if (!ww.GetName(name, ver))
      fail(4);
   else
   {
      debug.print("Device name: ");
      debug.print(name);
      debug.print(", version: ");
      debug.println(ver);
   }

   if (!ww.ready)
     fail(5);
   if (!ww.Reset())
      fail(6);
}

void loop()                                                                    // run over and over again
{
   long dist;
   int vel;
   int accel;

   if (ww.GetDistance(dist))
   {
      debug.print("Dist = ");
      debug.println(dist);
      toggle_led();
      if (dist > 1280)  // 10 rotations
      {
         digitalWrite(13, HIGH);                                            // LED off = done
         for (;;);
      }
   }
   else
      fail(7);
   if (ww.GetVelocity(vel))
   {
      debug.print("Vel = ");
      debug.println(vel);
   }
   else
      fail(8);
   if (ww.GetAcceleration(vel))
   {
      debug.print("Accel = ");
      debug.println(accel);
   }
   else
      fail(9);
   delay(1000);
} 


