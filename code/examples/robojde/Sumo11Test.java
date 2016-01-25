/*
 * Sumo11Test
 *
 * Copyright 2003 by RidgeSoft, LLC., PO Box 482, Pleasanton, CA  94566, U.S.A.
 * www.ridgesoft.com
 *
 * RidgeSoft grants you the right to use, modify, make derivative works and
 * redistribute this source file provided you do not remove this copyright notice.
 *
 * 8/5/2004 - PLS, Noetic Design, Inc. - added WheelWatcher WW-01 tests
 */

import com.ridgesoft.handyboard.HandyBoard;
import com.ridgesoft.robotics.PushButton;
import com.ridgesoft.ui.*;

/**
 * This class implements tests of the Sumo11 robot controller.  Most of the
 * test cases are inherited from the HandyBoardTest class.
 *
 * The START button, STOP button and thumbwheel are used to control the tests.
 * -> Choose a test by using the thumbwheel to scroll through the list of tests.
 * -> Start the test by pressing the START button.
 * -> Most tests use the thumbwheel to vary the test.  For example, for
 *    motor tests, the thumbwheel varies the motor speed.
 * -> Stop the test by pressing the STOP button. 
 */
public class Sumo11Test extends HandyBoardTest {

    // Override main method from the parent class.
    public static void main(String args[]) {
        try {
            System.out.println("Sumo11Test");

            // Set board type to Sumo11
            HandyBoard.setBoardType(HandyBoard.TYPE_SUMO11);

            // Tell virtual machine to ignore the stop button
            HandyBoard.setTerminateOnStop(false);

            // Create the main test menu
            MenuItemList mainMenu = new BasicMenuItemList("Select Category", new MenuItem[] {
                                                                        new ThumbTest("Thunbwheel"), 
                                                                        new BuzzerTest("Buzzer"),
                                                                        new AnalogTests("Analogs", 0, 7),
                                                                        new Digital8OutputTest("Digital 8 Output"),
                                                                        new Digital9OutputTest("Digital 9 Output"),
                                                                        new DigitalInTest("Digital Inputs"),
                                                                        new MotorTestList("Motors", 2),
                                                                        new ServoTestList("Servos", 4),
                                                                        new IRRxTest("IR Receiver"),
                                                                        new IRTxTest("IR Transmitter"),
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
     * Test for digital output 8, the only dedicated digital output on the Sumo11.
     */
    public static class Digital8OutputTest extends Test {
        public Digital8OutputTest(String name) {
            super(name);
        }

        public void select(PushButton selectButton, PushButton exitButton, MenuController menuController) {
            while (!exitButton.isPressed()) {
                switch (menuController.getScrollIndex(3)) {
                case 0:
                    HandyBoard.clearDigitalOutput(8);
                    menuController.printStatus("0");
                    break;
                case 1:
                    HandyBoard.setDigitalOutput(8);
                    menuController.printStatus("1");
                    break;
                case 2:
                    HandyBoard.toggleDigitalOutput(8);
                    menuController.printStatus("toggle");
                    try {
                        Thread.sleep(500);
                    }
                    catch (InterruptedException e) {}
                    break;
                }
            }
            HandyBoard.clearDigitalOutput(8);
            menuController.buttonPressed();
        }
    }
}