/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Smart-DR-Lock.ino                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ababdelo <ababdelo@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/08/27 10:06:55 by ababdelo          #+#    #+#             */
/*   Updated: 2023/08/27 10:06:58 by ababdelo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RFID_DRLOCK.h"

/****************************************Setup Function*********************************************/
void setup() {
  digitalWrite(RST, HIGH);        // first thing to do is to set rst pin to HIGH to make sure arduino won't reset itself from the start
  digitalWrite(RELAYDOOR, HIGH);  // and Make sure that the door is locked

  //Arduino Pin Configuration
  pinMode(REDLED, OUTPUT);
  pinMode(GREENLED, OUTPUT);
  pinMode(BLUELED, OUTPUT);
  pinMode(BTNERASE, INPUT_PULLUP);  // Enable pin's pull up resistor
  pinMode(BTNDOOR, INPUT_PULLUP);   // Enable pin's pull up resistor
  pinMode(RELAYDOOR, OUTPUT);       //Be careful how relay circuit behave on while resetting or power-cycling your Arduino
  pinMode(RST, OUTPUT);             // Pin that'ill reset arduino after an erasing of memory

  // digitalWrite(RELAYLAMP, LOW);     // Make sure lamp is turned off
  analogWrite(REDLED, LED_OFF);    // Make sure led is off
  analogWrite(GREENLED, LED_OFF);  // Make sure led is off
  analogWrite(BLUELED, LED_OFF);   // Make sure led is off

  Serial.begin(9600);  // Initialize serial communications with PC
  rdm6300.begin(RDM6300_RX_PIN);
  lcd.init();  //Initialize LCD_I2C

  lcd.begin(16, 2);  // Initialize the interface to LCD_I2C screen
  lcd.backlight();   // Turn on LCD_I2C Backlight
  // Serial.println(F("Access Control"));  // For debugging purposes
  print2lcd(" Access Control", 0, " Scan Your Tag!", 0);

  // Check if master card defined, if not let user choose a master card
  // This also useful to just redefine Master Card
  // You can keep other EEPROM records just write other than 42 to EEPROM address 1
  // EEPROM address 1 should hold magical number which is '42'
  if (EEPROM.read(1) != 42) {
    print2lcd(" Define  Master ", 0, "", 0);
    // Serial.println(F("No Master Card Defined"));
    // Serial.println(F("Scan A PICC to Define as Master Card"));
    do {
      successRead = getID();          // sets successRead to 1 when we get read from reader otherwise 0
      digitalWrite(BLUELED, LED_ON);  // Visualize Master Card need to be defined
      delay(200);
      digitalWrite(BLUELED, LED_OFF);
      delay(200);
    } while (!successRead);              // Program will not go further while you did not got a successful read
    for (int j = 0; j < 4; j++) {        // Loop 4 times
      EEPROM.write(2 + j, readCard[j]);  // Write scanned PICC's UID to EEPROM, start from address 3
    }
    EEPROM.write(1, 42);  // Write to EEPROM we defined Master Card.
    // Serial.println(F("Master Card Defined"));
    print2lcd(" Master Defined ", 0, "", 0);
    // Serial.print("Master Defined !");
    // Serial.print("\n");
    delay(1000);
  }
  // Serial.println(F("-------------------"));
  // Serial.println(F("Master Card's UID"));
  for (int i = 0; i < 4; i++) {          // Read Master Card's UID from EEPROM
    masterCard[i] = EEPROM.read(2 + i);  // Write it to masterCard
    //    Serial.print(masterCard[i], HEX);
  }
  // Serial.println();
  // Serial.println(F("-------------------"));
  // Serial.println(F("Everything is Ready"));
  // Serial.println(F("Waiting PICCs to be scanned"));
  print2lcd(" Access Control", 0, " Scan Your Tag!", 0);
  cycleLeds();  // Everything ready lets give user some feedback by cycling leds
}
/***************************************************************************************************/

/*****************************************Loop Function*********************************************/
void loop() {
  // Continuously read RFID tags
  do {
    successRead = getID();  // sets successRead to 1 when we get read from reader otherwise 0

    // If not in program mode and door button is pressed, grant access
    if (!programMode && digitalRead(BTNDOOR) == LOW)
      granted(2000, "NO");

    // Wipe EEPROM if erase button is pressed
    while (digitalRead(BTNERASE) == LOW && programMode && counter > 0) {
      print2lcd("Erasing EEPROM", 1, ("Cancel before: " + String(cntr)), 0);
      digitalWrite(REDLED, LED_ON);    // Indicate the start of EEPROM erasure
      digitalWrite(BLUELED, LED_OFF);
      delay(1000);
      cntr--;

      if (cntr == 0) {  // Check if the erase process should proceed
        eraseEEPROM();
      }
    }

    counter = cntr;
    if (programMode) cycleLeds();
    else normalModeOn();
  } while (!successRead);

  // Handle RFID tag scanning
  if (programMode) {
    if (isMaster(readCard)) {
      print2lcd("Exited", 5, "Program Mode", 2);
      programMode = false;
      delay(1000);
      print2lcd(" Access Control", 0, " Scan Your Tag!", 0);
      return;
    } else {
      if (findID(readCard)) {
        print2lcd("Removed Existing", 0, " ID from EEPROM ", 0);
        Serial.print("|     Role :     ' Master '          Action : ' Removed Existing UID: " + uid + " '     \n");
        deleteID(readCard);
      } else {
        print2lcd(" Added New ID ", 1, "to  EEPROM", 3);
        Serial.print("|     Role :     ' Master '          Action : ' Added New UID: " + uid + " '     \n");
        writeID(readCard);
      }
    }
    delay(1000);
    print2lcd("Entered", 4, "Program Mode", 2);
  } else {
    if (isMaster(readCard)) {
      programMode = true;
      print2lcd("Entered", 4, "Program Mode", 2);
      int count = EEPROM.read(0);
    } else {
      if (findID(readCard)) {
        print2lcd("Welcome Back", 2, "", 0);
        Serial.print("|     UID :     " + uid + "          Action :     ' Entered the Club '     \n");
        granted(2000, "YES");
        print2lcd(" Access Control", 0, " Scan Your Tag!", 0);
      } else {
        print2lcd(" Access  Denied ", 0, "Unknown :)", 3);
        Serial.print("|     UID :     " + uid + "           Action :     ' Attempted entering the Club '     \n");
        denied();
        print2lcd(" Access Control", 0, " Scan Your Tag!", 0);
      }
    }
  }
}
/***************************************************************************************************/

/****************************************EEPROM Erase Function*********************************************/
void eraseEEPROM() {
  // Iterate through each address in EEPROM and clear if data exists
  for (int x = 0; x < EEPROM.length(); x++) {
    if (EEPROM.read(x) != 0) {
      EEPROM.write(x, 0);
    }
  }

  // Indicate successful erasure on LCD
  print2lcd("EEPROM  Erased", 1, "Successfully", 2);
  // Print action to serial monitor
  Serial.print("|     Role : ' Master'          Action : ' Erased EEPROM '     \n");

  // Visualize successful wipe with LED
  for (int i = 0; i < 4; i++) {
    analogWrite(REDLED, LED_OFF);
    delay(200);
    analogWrite(REDLED, LED_ON);
    delay(200);
  }

  // Reset Arduino to apply wiping
  digitalWrite(RST, LOW);
  delay(50);
  digitalWrite(RST, HIGH);
}
/***************************************************************************************************/
