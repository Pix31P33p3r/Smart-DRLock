/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FT_UTILS.ino                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ababdelo <ababdelo@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/08/27 10:07:06 by ababdelo          #+#    #+#             */
/*   Updated: 2023/08/27 10:07:08 by ababdelo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RFID_DRLOCK.h"

/************************************** Count the uid lenght ***************************************/

uint32_t digit_count(uint32_t nbr) {
  int count = 0;
  if (nbr == 0)
    return 1;
  while (nbr) {
    count++;
    nbr = nbr / 10;
  }
  return count;
}
/***************************************************************************************************/

/******************************************** GetID ************************************************/

int getID() {
  if (rdm6300.get_new_tag_id()) {
    uint32_t newTagId = rdm6300.get_tag_id();
    uid = String(newTagId, HEX);
    uid.toUpperCase();
    // Serial.print("UID: ");
    // Serial.println(newTagId);

    int nbrcntr = digit_count(newTagId);
    readCard = new uint32_t[nbrcntr];
    for (int i = nbrcntr - 1; i >= 0; i--) {
      readCard[i] = newTagId % 10;
      newTagId /= 10;
    }
    return 1;
  }
  return 0;
}
/***************************************************************************************************/

/******************************** Check readCard IF is masterCard***********************************/

// Check to see if the ID passed is the master programing card
boolean isMaster(uint32_t test[]) {
  return checkTwo(test, masterCard);
}
/***************************************************************************************************/

/*****************************************Find ID From EEPROM***************************************/

boolean findID(uint32_t find[]) {
  int count = EEPROM.read(0);          // Read the first Byte of EEPROM that
  for (int i = 1; i <= count; i++) {   // Loop once for each EEPROM entry
    readID(i);                         // Read an ID from EEPROM, it is stored in storedCard[4]
    if (checkTwo(find, storedCard)) {  // Check to see if the storedCard read from EEPROM
      return true;
      //break;  // Stop looking we found it
    } else {
    }  // If not, return false
  }
  return false;
}
/***************************************************************************************************/

/*****************************************Remove ID from EEPROM*************************************/

void deleteID(uint32_t a[]) {
  if (!findID(a)) {
    failedWrite();  // If ID not found, indicate failure and return
    return;
  }

  int numCards = EEPROM.read(0);  // Read the number of stored cards from EEPROM
  int slot = findIDSLOT(a);  // Find the slot number of the card to delete
  int start = (slot * 4) + 2;  // Calculate the starting address of the card to delete
  int numBytesToMove = (numCards - slot) * 4;  // Calculate the number of bytes to shift

  numCards--;  // Decrement the number of stored cards
  EEPROM.write(0, numCards);  // Update the number of stored cards in EEPROM

  // Shift the remaining cards in EEPROM to fill the gap left by the deleted card
  for (int j = 0; j < numBytesToMove; j++) {
    EEPROM.write(start + j, EEPROM.read(start + 4 + j));
  }

  // Clear the bytes occupied by the deleted card
  for (int k = 0; k < 4; k++) {
    EEPROM.write(start + numBytesToMove + k, 0);
  }

  successRemove();  // Indicate successful removal
}
/***************************************************************************************************/

/**************************************** Read an ID from EEPROM************************************/

void readID(int number) {
  int start = (number * 4) + 2;              // Figure out starting position
  for (int i = 0; i < 4; i++) {              // Loop 4 times to get the 4 Bytes
    storedCard[i] = EEPROM.read(start + i);  // Assign values read from EEPROM to array
  }
}
/***************************************************************************************************/

/****************************************Add ID to EEPROM*******************************************/

void writeID(uint32_t a[]) {
  if (!findID(a)) {                   // Before we write to the EEPROM, check to see if we have seen this card before!
    int num = EEPROM.read(0);         // Get the numer of used spaces, position 0 stores the number of ID cards
    int start = (num * 4) + 6;        // Figure out where the next slot starts
    num++;                            // Increment the counter by one
    EEPROM.write(0, num);             // Write the new count to the counter
    for (int j = 0; j < 4; j++) {     // Loop 4 times
      EEPROM.write(start + j, a[j]);  // Write the array values to EEPROM in the right position
    }
    successWrite();
    // Serial.println(F("Succesfully added ID record to EEPROM"));
  } else {
    failedWrite();
    // Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
}
/***************************************************************************************************/

/*****************************************Check Bytes***********************************************/

boolean checkTwo(uint32_t a[], uint32_t b[]) {
  if (a[0] == 0)  // Check if the first element of a is zero
    return false; // If it's zero, return false immediately
  
  for (int k = 0; k < 4; k++) {  // Loop 4 times
    if (a[k] != b[k])            // If a != b, set ismatch = false and return false
      return false;
  }
  
  // If the loop completes without returning false, it means all elements match
  return true; // Return true
}
/***************************************************************************************************/

/*****************************************Find Slot*************************************************/

int findIDSLOT(uint32_t find[]) {
  int count = EEPROM.read(0);          // Read the first Byte of EEPROM that
  for (int i = 1; i <= count; i++) {   // Loop once for each EEPROM entry
    readID(i);                         // Read an ID from EEPROM, it is stored in storedCard[4]
    if (checkTwo(find, storedCard)) {  // Check to see if the storedCard read from EEPROM is the same as the find[] ID card passed
      return i;                        // The slot number of the card
      //break;                           // Stop looking we found it
    }
  }
}
/***************************************************************************************************/

/******************************************Access Granted*******************************************/
void granted(int setDelay, String permission) {
  // Determine LED colors based on permission
  if (permission == "YES") {
    analogWrite(BLUELED, LED_OFF);   // Turn off blue LED
    analogWrite(REDLED, LED_OFF);    // Turn off red LED
    analogWrite(GREENLED, LED_ON);   // Turn on green LED
  } else {
    analogWrite(BLUELED, LED_ON);    // Turn on blue LED
    analogWrite(REDLED, LED_OFF);    // Turn off red LED
    analogWrite(GREENLED, LED_OFF);  // Turn off green LED
  }

  digitalWrite(RELAYDOOR, LOW);   // Unlock the door
  delay(setDelay);                // Hold the door unlocked for the given seconds
  digitalWrite(RELAYDOOR, HIGH);  // Relock the door
}
/***************************************************************************************************/

/*****************************************Access Denied*********************************************/
void denied() {
  analogWrite(GREENLED, LED_OFF);  // Make sure green LED is off
  analogWrite(BLUELED, LED_OFF);   // Make sure blue LED is off
  analogWrite(REDLED, LED_ON);     // Turn on red LED
  delay(1000);
}
/***************************************************************************************************/

/*************************************Cycle Leds (Program Mode)*************************************/
void cycleLeds() {
  analogWrite(REDLED, LED_OFF);   // Make sure red LED is off
  analogWrite(GREENLED, LED_ON);  // Make sure green LED is on
  analogWrite(BLUELED, LED_OFF);  // Make sure blue LED is off
  delay(200);
  analogWrite(REDLED, LED_OFF);    // Make sure red LED is off
  analogWrite(GREENLED, LED_OFF);  // Make sure green LED is off
  analogWrite(BLUELED, LED_ON);    // Make sure blue LED is on
  delay(200);
  analogWrite(REDLED, LED_ON);     // Make sure red LED is on
  analogWrite(GREENLED, LED_OFF);  // Make sure green LED is off
  analogWrite(BLUELED, LED_OFF);   // Make sure blue LED is off
  delay(200);
}
/***************************************************************************************************/

/***************************************Normal Mode Led*********************************************/
void normalModeOn() {
  analogWrite(BLUELED, LED_ON);    // Blue LED ON and ready to read card
  analogWrite(REDLED, LED_OFF);    // Make sure Red LED is off
  analogWrite(GREENLED, LED_OFF);  // Make sure Green LED is off
  digitalWrite(RELAYDOOR, HIGH);   // Make sure Door is Locked
}
/***************************************************************************************************/

/**************************************Success Write to EEPROM***************************************/
// Flashes the green LED 3 times to indicate a successful write to EEPROM
void successWrite() {
  analogWrite(BLUELED, LED_OFF);   // Make sure blue LED is off
  analogWrite(REDLED, LED_OFF);    // Make sure red LED is off

  for (int i = 0; i < 3; i++) {
    analogWrite(GREENLED, LED_ON);  // Turn on green LED
    delay(200);
    analogWrite(GREENLED, LED_OFF); // Turn off green LED
    delay(200);
  }
}
/***************************************************************************************************/

/*************************************Failed Write to EEPROM****************************************/
// Flashes the red LED 3 times to indicate a failed write to EEPROM
void failedWrite() {
  analogWrite(BLUELED, LED_OFF);   // Make sure blue LED is off
  analogWrite(REDLED, LED_OFF);    // Make sure red LED is off
  analogWrite(GREENLED, LED_OFF);  // Make sure green LED is off
  delay(200);
  for (int x = 0; x < 3; x++) {
    digitalWrite(REDLED, LED_ON);  // Make sure blue LED is on
    delay(200);
    digitalWrite(REDLED, LED_OFF);  // Make sure red LED is off
    delay(200);
  }
  analogWrite(REDLED, LED_OFF);  // just making sure the Red LED is OFF
  analogWrite(BLUELED, LED_ON);  // Make sure blue LED is on
}
/***************************************************************************************************/

/*********************************Success Remove UID From EEPROM************************************/
// Flashes the blue LED 3 times to indicate a success delete to EEPROM
void successRemove() {
  analogWrite(BLUELED, LED_OFF);   // Make sure blue LED is off
  analogWrite(REDLED, LED_OFF);    // Make sure red LED is off
  analogWrite(GREENLED, LED_OFF);  // Make sure green LED is off
  delay(200);
  analogWrite(REDLED, LED_ON);  // Make sure red LED is on
  delay(200);
  analogWrite(REDLED, LED_OFF);  // Make sure red LED is off
  delay(200);
  analogWrite(REDLED, LED_ON);  // Make sure red LED is on
  delay(200);
  analogWrite(REDLED, LED_OFF);  // Make sure red LED is off
  delay(200);
  analogWrite(REDLED, LED_ON);  // Make sure red LED is on
  delay(200);
}
/***************************************************************************************************/

/******************************************Print 2 LCD**********************************************/
void print2lcd(String str1, int c1, String str2, int c2) {
  lcd.clear();
  lcd.setCursor(c1, 0);
  lcd.print(str1);
  if (str2.length() > 0) {
    lcd.setCursor(c2, 1);
    lcd.print(str2);
  }
}
/***************************************************************************************************/
