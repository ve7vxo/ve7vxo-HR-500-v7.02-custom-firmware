

void Tune_button(void){                                             // Tune has been requested.
  if (SCREEN == 0){
    Tft.LCD_SEL = 1;
    Tft.lcd_fill_rect(121, 142, 74, 21, BG_col);                      // Erase stale text form ATU message windows
    Tft.lcd_fill_rect(121, 199, 74, 21, BG_col);
  }
  
  if (!TUNING){                                                     // Initiate tuning if not in progress already
    if (ATU == 0){
      ATU = 1;
      if (SCREEN == 0) DrawATU();
    }
    if (SCREEN == 0){                                       
      Tft.drawString((uint8_t*)"STOP", 122, 199,  3, FG_col);       // Display main page ATU text while tuning
      Tft.drawString((uint8_t*)"TUNING", 122, 142,  2, WARN_LED);
    }
    else if (SCREEN == 2){                                          // Display appropriate for ATU data page 
      Tft.LCD_SEL = 1;
      Tft.drawString((uint8_t*)"TUNE", 130, 213,  2, BG_col);       // Erase TUNE button and show TUNING for 1 sec.
      Tft.drawString((uint8_t*)"TUNING", 120, 213,  2, WARN_LED);            
      delay (1000);
      Tft.drawString((uint8_t*)"TUNING", 120, 213,  2, BG_col);
      Tft.drawString((uint8_t*)"STOP", 130, 213,  2, FG_col);       // Display new ATU text while tuning
    }
    Tft.LCD_SEL = 0;
    Tft.lcd_fill_rect(66, 199, 76, 26, BG_col);                     // Erase stale SWR display
    delay (100);
    OVSWR = 10;
    VSWR = 10;
    strcpy(RL_TXT, "    ");
    strcpy(ORL_TXT, "    ");
    TUNING = 1;                                                     // Tuning state is active
    ATUT[BAND] = 0;                                                    // Matching solution not yet valid for current antenna
    if (TX){                                                        // If the amp is on, turn it off
      PTT = 0;
      BIAS_OFF
      TX = 0;
      byte SRSend_RLY = SR_DATA;
      RELAY_CS_LOW
      SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE3));
      SPI.transfer(SRSend_RLY);
      SPI.endTransaction();
      RELAY_CS_HIGH    
      RF_BYPASS
    }
    ATU_TUNE_LOW                                                    // Activate the tune line
    delay(5);                                                       // Tells tuner board to go to work
  }
  else Tune_End();
}




void Tune_End(void){                                                // Handle termination of matching attempt.
  size_t len;
  Tft.LCD_SEL = 1;
  if (SCREEN == 0){
    Tft.lcd_fill_rect(121, 142, 74, 21, BG_col);                      // Erase stale ATU status text
    Tft.lcd_fill_rect(121, 199, 74, 21, BG_col);
    Tft.drawString((uint8_t*)"TUNE", 122, 199,  3, ACT_TXT);          // display TUNE button
  }
  TUNING = 0;
  ATU_TUNE_HIGH                                                     // Signal ATU board to stop
  delay(10);
  ATU_buff[0] = 0;
  Serial3.setTimeout(100);
  Serial3.println("*S");                                            // Request status update from ATU board
  Serial3.readBytesUntil(0x0D, ATU_buff, 10);                                      
  ATU_STAT = ATU_buff[0];                                           // state depending on status
                                                                    // Reply and save to EEPROM afterward
  switch (ATU_STAT){                                                
    case 'F': 
      if (SCREEN == 0){                                             // Main page display    
        Tft.drawString((uint8_t*)"FAILED", 122, 142,  2, ALRM_LED); 
      }
      else if (SCREEN == 2) {                                       // ATU data page display
        Tft.lcd_fill_rect(105, 210, 115, 25, BG_col);               
        Tft.drawString((uint8_t*)"FAILED", 120, 213,  2, ALRM_LED);
      }
      ATUB[BAND] = 0;
      ATU = 0;
      DrawATU(); 
      Wait_RF();
    break;
    
    case 'E': 
      if (SCREEN == 0){                                             // Main page display     
        Tft.drawString((uint8_t*)"HI SWR", 122, 142,  2, ALRM_LED); 
      }
      else if (SCREEN == 2){                                       // ATU data page display
        Tft.lcd_fill_rect(105, 210, 115, 25, BG_col);               
        Tft.drawString((uint8_t*)"HI SWR", 120, 213,  2, ALRM_LED); 
      }
      ATUB[BAND] = 0;
      ATU = 0;
      DrawATU(); 
      Wait_RF();
    break;
    
    case 'H': 
      if (SCREEN == 0){                                             // Main page display     
        Tft.drawString((uint8_t*)"HI PWR", 122, 142,  2, WARN_LED); 
      }
      else if (SCREEN == 2){                                       // ATU data page display
        Tft.lcd_fill_rect(105, 210, 115, 25, BG_col);                 
        Tft.drawString((uint8_t*)"HI PWR", 120, 213,  2, WARN_LED);
      }
      ATUB[BAND] = 0; 
      ATU = 0;
      DrawATU(); 
     Wait_RF();
    break;
    
    case 'L': 
      if (SCREEN == 0){                                             // Main page display     
        Tft.drawString((uint8_t*)"LO PWR", 122, 142,  2, WARN_LED); 
      }
      else if (SCREEN == 2){                                       // ATU data page display
        Tft.lcd_fill_rect(105, 210, 115, 25, BG_col);                 
        Tft.drawString((uint8_t*)"LO PWR", 120, 213,  2, WARN_LED);
      }
      ATUB[BAND] = 0; 
      ATU = 0;
      DrawATU(); 
      Wait_RF();
    break;
      
    case 'A': 
      if (SCREEN == 0){                                             // Main page display     
        Tft.drawString((uint8_t*)"CANCEL", 122, 142,  2, GOOD_LED); 
      }
      else if (SCREEN == 2){                                       // ATU data page display
        Tft.lcd_fill_rect(105, 210, 115, 25, BG_col);                 
        Tft.drawString((uint8_t*)"CANCEL", 120, 213,  2, GOOD_LED);
      }
      ATUB[BAND] = 0; 
      ATU = 0;
      DrawATU(); 
      Wait_RF();
    break;
    
    case 'T':
    case 'S':
      if (SCREEN == 0){                                             // Main page display 
        Tft.drawString((uint8_t*)"TUNED", 128, 142,  2, GOOD_LED);
      }
      else if (SCREEN == 2){                                        // ATU data page display
        Tft.lcd_fill_rect(105, 210, 115, 25, BG_col);                 
        Tft.drawString((uint8_t*)"TUNED", 120, 213,  2, GOOD_LED);
      }
      ATUT[BAND] = ANTSEL[BAND];                                    // Matching solution is valid for current antenna selection
      EEPROM.update(eeatut+BAND,ATUT[BAND]);                        // and save in EEPROM
      ATUB[BAND] = 1;                                               // Update per-band array of ATU button state
      ATU = 1;
      DrawATU();                                                    // Update ATU status display      
      Serial3.setTimeout(50);
      Serial3.println("*F");
      Serial3.readBytesUntil(0x0D, ATU_rsp, 8);
      Serial3.println("*F");
      Serial3.readBytesUntil(0x0D, ATU_rsp, 8);                     // Discard junk from first read and read again

//******************************************************************************
#ifdef DEBUG_ATU        
      Serial.println(ATU_rsp);               // Send result to USB for debugging
//      strcpy(ATU_rsp, "064");                // Fake result for SWR of 2.8
//      strcpy(ATU_rsp, "128");                // Fake result for SWR of 1.5
//      strcpy(ATU_rsp, "192");                // Fake result for SWR of 1.2
#endif
//******************************************************************************        
      
      unsigned char RL_CH = (ATU_rsp[0] - 48) * 100 + (ATU_rsp[1] - 48) * 10 + (ATU_rsp[2] - 48);
      int SWR = swr[RL_CH]/10;
      if (SWR < 18) vswr_col = GOOD_LED;                                            // VSWR color green at 1:1
      if (SWR >= 18) vswr_col = WARN_LED;                                           // Above 1.8:1 swr display yellow
      if (SWR >= 25) vswr_col = ALRM_LED;                                           // Above 2.5:1 swr display red      
      snprintf(RL_TXT, sizeof(RL_TXT), "%2d.%d", SWR/10, SWR % 10);
      Tft.LCD_SEL = 0;
      Tft.drawString((uint8_t*)RL_TXT, 70, 203, 2, vswr_col);
      strcpy(ORL_TXT, RL_TXT);
      delay(300);                                                                   // Display previous text momentarily
      Wait_RF();                                                                    // Wait for drive to be removed      
    break;
  }
  EEPROM.update(eeatub+BAND, ATUB[BAND]);                                           // Update array in EEPROM
  Switch_to_RX();                                                                   // Return amp to RX state
  if (SCREEN == 2) DrawATU_Data();
  
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Request all final values from ATU after auto tune

void ATU_GetAll(void){
  int p_idx;
  strcpy(ATU_cmd, "*P?");
  p_idx = ATU_exch;
  Serial.println(ATU_buff);                             
  if(p_idx){                                      // Convert ascii to int
    ATUL[BAND] = (ATU_buff[p_idx - 25] - 48) * 100 + (ATU_buff[p_idx - 24] - 48) * 10 + (ATU_buff[p_idx - 23] - 48); 
    ATUC[BAND] = (ATU_buff[p_idx - 20] - 48) * 100 + (ATU_buff[p_idx - 19] - 48) * 10 + (ATU_buff[p_idx - 18] - 48);
    ATUH[BAND] = (ATU_buff[p_idx - 15] - 48);
    ATUS[BAND] = (ATU_buff[p_idx - 12] - 48);
  }
  EEPROM.update(eeatuc+BAND, ATUC[BAND]);         // Store tuned ATU parameters in EEPROM
  EEPROM.update(eeatul+BAND, ATUL[BAND]);
  EEPROM.update(eeatuh+BAND, ATUH[BAND]);
  EEPROM.update(eeatus+BAND, ATUS[BAND]);     
  
}




//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Request position of relays for L, C, Bypass and Capacitor position
// Store the values in EEPROM arrays on a per band basis

void ATU_GetValues(void){
  
  strcpy(ATU_cmd, "*C?");                         // Request ATU capacitance
  ATU_param();
//******************************************************************************              
//  strcpy(ATU_val, "127");                         // fake reponse for max capacitance
//******************************************************************************              
  ATUC[BAND] = atoi(ATU_val);
  EEPROM.update(eeatuc+BAND, ATUC[BAND]);         // Store in EEPROM


  strcpy(ATU_cmd, "*L?");                         // Request ATU inductance
  ATU_param(); 
//******************************************************************************              
//  strcpy(ATU_val, " 63");                         // fake reponse for max inductance
//******************************************************************************              
  ATUL[BAND] = atoi(ATU_val);
  EEPROM.update(eeatul+BAND, ATUL[BAND]);         // Store in EEPROM


  strcpy(ATU_cmd, "*H?");                        // Request ATU capacitor position
  ATU_param();
  if(ATU_val[0] == '0') ATUH[BAND] = 0;
  if(ATU_val[0] == '1') ATUH[BAND] = 1;
 
//******************************************************************************              
//  strcpy(ATU_val, "  0");                         // fake reponse for low Z
//  ATUH[BAND] = atoi(ATU_val);
//******************************************************************************              

  ATUH[BAND] = atoi(ATU_val);
  EEPROM.update(eeatuh+BAND, ATUH[BAND]);         // Store in EEPROM

  strcpy(ATU_cmd, "*Y?");                         // Request ATU bypass switch state
  ATU_param();
  if(ATU_val[0] == '0') ATUS[BAND] = 0;
  if(ATU_val[0] == '1') ATUS[BAND] = 1;
 
//******************************************************************************              
//  strcpy(ATU_val, "  0");                         // fake reponse for ATU bypass
//  ATUS[BAND] = atoi(ATU_val);
//******************************************************************************              

  ATUS[BAND] = atoi(ATU_val);
  EEPROM.update(eeatus+BAND, ATUS[BAND]);         // Store in EEPROM     
  
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Wait for RF drive to end.

void Wait_RF(void){
  while (analogRead(INPUT_RF) > 1){                                 // Wait for RF drive removed
    if (SCREEN == 0){                                               // When on main screen
      DrawTxPanel(ALRM_LED);                                        // Flash red TX status to indicate drive ON
      delay(200);
      DrawTxPanel(BG_col);
      delay(400);
    }
    else if (SCREEN == 2){                                          // When on ATU data screen
      Tft.LCD_SEL = 1;
      Tft.lcd_fill_rect(105, 210, 115, 25, BG_col);                 // Erase stale text
      Tft.drawString((uint8_t*)"DRIVE", 130, 213,  2, ALRM_LED);    // Display flashing red warning to remove drive
      delay(200);
      Tft.drawString((uint8_t*)"DRIVE", 130, 213,  2, BG_col);          
      delay(400);     
    }
  } 
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ATU serial data exchange
int ATU_exch(void){                                                 // Exchange serial data with ATU board
  size_t b_len;  
  Serial3.setTimeout(100);                                           // Don't get hung waiting for ATU serial responses
  Serial3.println(ATU_cmd);
  b_len = Serial3.readBytesUntil(0x0D, ATU_buff, 39);
  ATU_buff[b_len] = 0;                                              // Terminate recieved string
  
//******************************************************************************
#ifdef DEBUG_ATU  
  Serial.println(ATU_buff);                                           // Send result to USB for debugging
#endif
//******************************************************************************  

  return b_len; 
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Alternate ATU data exchange with alternate buffer for testing
void ATU_param(void){                                               // Exchange serial data between control and ATU boards
  size_t b_len;  
  Serial3.setTimeout(50);                                           // Don't get hung waiting for ATU serial responses
  Serial3.println(ATU_cmd);
  b_len = Serial3.readBytesUntil(0x0D, ATU_val, 5);
  ATU_val[b_len] = 0;                                              // Terminate recieved string

//******************************************************************************  
#ifdef DEBUG_ATU  
  Serial.println(ATU_val);                                           // Send result to USB for debugging
#endif
//******************************************************************************  
  
  return b_len; 
}
