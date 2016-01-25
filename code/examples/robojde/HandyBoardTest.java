/*
 * HandyBoardTest
 *
 * Copyright 2003 by RidgeSoft, LLC., PO Box 482, Pleasanton, CA  94566, U.S.A.
 * www.ridgesoft.com
 *
 * RidgeSoft grants you the right to use, modify, make derivative works and
 * redistribute this source file provided you do not remove this copyright notice.
 *
 * 8/5/2004 - PLS, Noetic Design, Inc. - added WheelWatcher WW-01 tests
 * NOTE: the HandyBoard class's ShaftEncoders were designed by RidgeSoft to
 * connect encoder 0 to pins 10 and 11, and encoder 1 to 12 and 13.
 * So, wire the WW-01s up like this:
 * (left WW-01) 
 * pin 11 = ChB/Blue; pin 10 = ChA/Yellow; pin 8 = DIR/Orange; pin 7 = CLK/Purple
 * (right WW-01)
 * pin 13 = ChA/Yellow; pin 12 = ChB/Blue; pin 14 = DIR/Orange; pin 15 = CLK/Purple
 */

import com.ridgesoft.io.Display;
import com.ridgesoft.robotics.AnalogInput;
import com.ridgesoft.robotics.PushButton;
import com.ridgesoft.robotics.Motor;
import com.ridgesoft.robotics.Servo;
import com.ridgesoft.io.Speaker;
import com.ridgesoft.handyboard.HandyBoard;
import com.ridgesoft.vm.VM;
import com.ridgesoft.ui.*;

/**
 * This class implements tests of the Handy Board.  
 *
 * The START button, STOP button and thumbwheel are used to control the tests.
 * -> Choose a test by using the thumbwheel to scroll through the list of tests.
 * -> Start the test by pressing the START button.
 * -> Most tests use the thumbwheel to vary the test.  For example, for
 *    motor tests, the thumbwheel varies the motor speed.
 * -> Stop the test by pressing the STOP button. 
 */
public class HandyBoardTest {
    public static void main(String args[]) {
        try {
            System.out.println("HandyBoardTest");

            // Tell virtual machine to ignore the stop button
            HandyBoard.setTerminateOnStop(false);

            // Create the main test menu
            MenuItemList mainMenu = new BasicMenuItemList("Select Category", new MenuItem[] {
                                                                        new ThumbTest("Thunbwheel"), 
                                                                        new BuzzerTest("Buzzer"),
                                                                        new AnalogTests("Analogs", 0, 7),
                                                                        new DigitalInTest("Digital Inputs"),
                                                                        new Digital9OutputTest("Digital 9 Out"),
                                                                        new MotorTestList("Motors", 4),
                                                                        new IRRxTest("IR Receiver"),
                                                                        new IRTxTest("IR Transmitter"),
                                                                        new AnalogTests("Exp. Analogs", 16, 16),
                                                                        new DigitalOutTest("Exp. Dig. Outs"),
                                                                        new ServoTestList("Exp. Servos", 6),
                                                                        new WW01Test("WheelWatcher")
                                                                        });

            // Create a MenuController
            TwoLineDisplayMenuController menuController = new TwoLineDisplayMenuController(
                                                                        HandyBoard.getLcdDisplay(),
                                                                        HandyBoard.getStartButton(),
                                                                        HandyBoard.getStopButton(),
                                                                        HandyBoard.getThumbWheel(),
                                                                        HandyBoard.getBuzzer(),
                                                                        mainMenu);

            // Show the menus
            menuController.show();
        }
        catch (Throwable t) {
            // Catch all exceptions and print a stack trace.
            t.printStackTrace();
        }
    }

    /**
     * Abstract base class for all tests, which implements the MenuItem interface
     * because test objects are used as menu choices.
     */
    public abstract static class Test implements MenuItem {
        private String mName;

        protected Test(String name) {
            mName = name;
        }

        public String toString() {
            return mName;
        }

        public abstract void select(PushButton selectButton, PushButton exitButton, MenuController menuController);
    }

    /**
     * Thumbwheel test.
     */
    public static class ThumbTest extends Test {
        public ThumbTest(String name) {
            super(name);
        }

        public void select(PushButton selectButton, PushButton exitButton, MenuController menuController) {
            while (!exitButton.isPressed())
                menuController.printStatus(Integer.toString(HandyBoard.readThumbWheel())); 

            menuController.buttonPressed();
        }
    }

    /**
     * Analog inputs test.
     */
    public static class AnalogTests extends Test {
        private int mBaseIndex;
        private int mCount;

        public AnalogTests(String name, int baseIndex, int count) {
            super(name);
            mBaseIndex = baseIndex;
            mCount = count;
        }

        public void select(PushButton selectButton, PushButton exitButton, MenuController menuController) {
            while (!exitButton.isPressed()) {
                int i = menuController.getScrollIndex(mCount) + mBaseIndex;
                menuController.printStatus(Integer.toString(i) + ": " + HandyBoard.analog(i)); 
            }
            menuController.buttonPressed();
        }
    }

    /**
     * Motor selection menu.
     */
    public static class MotorTestList extends MenuItemList {
        private byte mNumberOfMotors;
        private MotorTest[] mMotorTests;

        public MotorTestList(String name, int numberOfMotors) {
            super(name);
            mNumberOfMotors = (byte)numberOfMotors;
        }

        /**
         * Gets the list of menu items in this list.
         * @return      array of MenuItems
         */
        public MenuItem[] getMenuItems() {
            if (mMotorTests == null) {
                mMotorTests = new MotorTest[mNumberOfMotors];
                for (int i = 0; i < mNumberOfMotors; ++i)
                    mMotorTests[i] = new MotorTest("Motor " + i, HandyBoard.getMotor(i));
            }
            return mMotorTests;
        }

        /**
         * Frees menuItems.
         */
        public void freeMenuItems() {
            mMotorTests = null;
        }
    }

    /**
     * Motor test.
     */
    public static class MotorTest extends Test {
        private Motor mMotor;

        public MotorTest(String name, Motor motor) {
            super(name);
            mMotor = motor;
        }

        public void select(PushButton selectButton, PushButton exitButton, MenuController menuController) {
            while (!exitButton.isPressed()) {
                // Vary the power according to the thumbwheel position
                int power = menuController.getScrollIndex(Motor.MAX_FORWARD - Motor.MAX_REVERSE + 1) + Motor.MAX_REVERSE;
                mMotor.setPower(power);
                menuController.printStatus("power:  " + power); 
            }
            menuController.buttonPressed();
            mMotor.setPower(0);
        }
    }

    /**
     * Digital inputs test.
     */
    public static class DigitalInTest extends Test {
        public DigitalInTest(String name) {
            super(name);
        }

        public void select(PushButton selectButton, PushButton exitButton, MenuController menuController) {
            while (!exitButton.isPressed()) {
                int id = menuController.getScrollIndex(10) + 6;
                if (id == 6) {
                    StringBuffer bitStringBuffer = new StringBuffer();
                    for (int i = 15; i > 6; --i) {
                        if (HandyBoard.isDigitalInputSet(i))
                            bitStringBuffer.append('1');
                        else
                            bitStringBuffer.append('0');
                    }   
                    menuController.printStatus("15-7: " + bitStringBuffer.toString()); 
                }
                else {
                    char value;
                    if (HandyBoard.isDigitalInputSet(id))
                        value = '1';
                    else
                        value = '0';
                    menuController.printStatus(Integer.toString(id) + ": " + value); 
                }
            }
            menuController.buttonPressed();
        }
    }

    /**
     * Buzzer test.
     */
    public static class BuzzerTest extends Test {
        public BuzzerTest(String name) {
            super(name);
        }

        public void select(PushButton selectButton, PushButton exitButton, MenuController menuController) {
            menuController.printStatus(""); 
            int duration = 250;
            HandyBoard.play(262, duration);     // C
            HandyBoard.play(277, duration);     // C sharp
            HandyBoard.play(294, duration);     // D
            HandyBoard.play(311, duration);     // D sharp
            HandyBoard.play(330, duration);     // E
            HandyBoard.play(349, duration);     // F
            HandyBoard.play(370, duration);     // F sharp
            HandyBoard.play(392, duration);     // G
            HandyBoard.play(415, duration);     // G sharp
            HandyBoard.play(440, duration);     // A
            HandyBoard.play(466, duration);     // A sharp
            HandyBoard.play(494, duration);     // B
        }
    }

    /**
     * Native method for reading the count of IR pulses seen by the
     * IR demodulator.  Pointing an IR remote control at the demodulator
     * and pressing buttons should cause the count to increase if the
     * remote control transmits at the frequency of the demodulator ~40 Hz.
     */
    public static native int irPulseCount();

    /**
     * IR demodulator test.  Use an IR remote control to send IR pulses to the
     * demodulator or use another Handy Board running the IR transmitter test
     * to generate pulses.
     */
    public static class IRRxTest extends Test {
        public IRRxTest(String name) {
            super(name);
        }

        public void select(PushButton selectButton, PushButton exitButton, MenuController menuController) {
            int lastValue = -1;
            while (!exitButton.isPressed()) {
                int pulseCount = irPulseCount();
                if (pulseCount != lastValue) {
                    menuController.printStatus("Pulses: " + Integer.toString(pulseCount));
                    lastValue = pulseCount;
                }
            }
            menuController.buttonPressed();
        }
    }

    /**
     * Modulated IR transmitter test.  You must connect an IR LED to pins 1 and 3 of the IR OUT port
     * on the Handy Board with a current limiting resistor (~100-400 ohms) in the circuit.  Using another
     * Handy Board running the IR receive test you will see the pulse count increment if the transmitter
     * and receiver work.
     */
    public static class IRTxTest extends Test {
        public IRTxTest(String name) {
            super(name);
        }

        public void select(PushButton selectButton, PushButton exitButton, MenuController menuController) {
            menuController.printStatus("Running");
            boolean setIt = true;
            while (!exitButton.isPressed()) {
                if (setIt) {
                    VM.setBit(HandyBoard.PORTA, 6);
                    setIt = false;
                }
                else {
                    VM.clearBit(HandyBoard.PORTA, 6);
                    setIt = true;
                }
            }
            VM.clearBit(HandyBoard.PORTA, 6);
            menuController.buttonPressed();
        }
    }

    /**
     * Digital output 9 test.  Digital input #9 can be configured to be a digital output.  This
     * tests digital 9 as an output.
     */
    public static class Digital9OutputTest extends Test {
        public Digital9OutputTest(String name) {
            super(name);
        }

        public void select(PushButton selectButton, PushButton exitButton, MenuController menuController) {
            menuController.printStatus("Running");
            HandyBoard.setDigital9Direction(true);
            while (!exitButton.isPressed()) {
                switch (menuController.getScrollIndex(3)) {
                case 0:
                    HandyBoard.clearDigitalOutput(9);
                    menuController.printStatus("0");
                    break;
                case 1:
                    HandyBoard.setDigitalOutput(9);
                    menuController.printStatus("1");
                    break;
                case 2:
                    HandyBoard.toggleDigitalOutput(9);
                    menuController.printStatus("toggle");
                    try {
                        Thread.sleep(500);
                    }
                    catch (InterruptedException e) {}
                    break;
                }
            }
            HandyBoard.clearDigitalOutput(9);
            HandyBoard.setDigital9Direction(false);
            menuController.buttonPressed();
        }
    }

    /**
     * Test the 9 digital outputs (0 - 8) on the expansion board.  Digital 8 is not pinned out
     * to a header.  It is routed to the pin marked dig8 under the LCD screen.
     */
    public static class DigitalOutTest extends Test {
        public DigitalOutTest(String name) {
            super(name);
        }

        public void select(PushButton selectButton, PushButton exitButton, MenuController menuController) {
            while (!exitButton.isPressed()) {
                int id = menuController.getScrollIndex(12) - 1;
                if (id < 0 || id > 8) {
                    if (id < 0) {
                        digitalsOff();
                        menuController.printStatus("8-0: off");
                    }
                    else if (id == 9) {
                        digitalsOn();
                        menuController.printStatus("8-0: on");
                    }
                    else {
                        digitalsToggle();
                        menuController.printStatus("8-0: toggle");
                        try {
                            Thread.sleep(500);
                        }
                        catch (InterruptedException e) {}
                    }
                }
                else {
                    for (int i = 0; i < 9; ++i) {
                        if (i == id)
                            HandyBoard.setDigitalOutput(i);
                        else
                            HandyBoard.clearDigitalOutput(i);
                    }
                    menuController.printStatus(Integer.toString(id) + ": " + "on");
                }
            }
            digitalsOff();  
        }

        private void digitalsOff() {
            for (int i = 0; i < 9; ++i)
                HandyBoard.clearDigitalOutput(i);
        }

        private void digitalsOn() {
            for (int i = 0; i < 9; ++i)
                HandyBoard.setDigitalOutput(i);
        }

        private void digitalsToggle() {
            for (int i = 0; i < 9; ++i)
                HandyBoard.toggleDigitalOutput(i);
        }
    }

    /**
     * Servo selection menu.
     */
    public static class ServoTestList extends MenuItemList {
        private byte mNumberOfServos;
        private ServoTest[] mServoTests;

        public ServoTestList(String name, int numberOfServos) {
            super(name);
            mNumberOfServos = (byte)numberOfServos;
        }

        /**
         * Gets the list of menu items in this list.
         * @return      array of MenuItems
         */
        public MenuItem[] getMenuItems() {
            if (mServoTests == null) {
                mServoTests = new ServoTest[mNumberOfServos];
                for (int i = 0; i < mNumberOfServos; ++i)
                    mServoTests[i] = new ServoTest("Servo " + i, HandyBoard.getServo(i));
            }
            return mServoTests;
        }

        /**
         * Frees menuItems.
         */
        public void freeMenuItems() {
            mServoTests = null;
        }
    }

    /**
     * Servo test.
     */
    public static class ServoTest extends Test {
        private Servo mServo;

        public ServoTest(String name, Servo servo) {
            super(name);
            mServo = servo;
        }

        public void select(PushButton selectButton, PushButton exitButton, MenuController menuController) {
            int oldPosition = -1;
            while (!exitButton.isPressed()) {
                int position = menuController.getScrollIndex(101);
                if (position != oldPosition) {
                    menuController.printStatus("position: " + position); 
                    mServo.setPosition(position);
                    oldPosition = position;
                }
            }
            menuController.buttonPressed();
            mServo.off();
        }
    }

    /**
     * WheelWatcher test.
     */
    public static class WW01Test extends Test {
        public WW01Test(String name) {
            super(name);
        }

        public void select(PushButton selectButton, PushButton exitButton, MenuController menuController) {
            HandyBoard.enableEncoder(0); // left WheelWatcher
            HandyBoard.enableEncoder(1); // right WheelWatcher      
            while (!exitButton.isPressed()) {
                HandyBoard.getLcdDisplay().print(0, "R c:" + HandyBoard.getEncoderCounts(1) + " s:" + HandyBoard.getEncoderRate(1) + " d:" + (HandyBoard.isDigitalInputSet(14) ? 'F' : 'B'));
                HandyBoard.getLcdDisplay().print(1, "L c:" + HandyBoard.getEncoderCounts(0) + " s:" + HandyBoard.getEncoderRate(0) + " d:" + (HandyBoard.isDigitalInputSet(8) ? 'B' : 'F')); 
            }
            menuController.buttonPressed();
        }
    }


}