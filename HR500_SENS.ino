//===============================================================================================================
// Calculate forward or reflected or drive power, or calculate VSWR depending on passed pType byte
// Based on values updated in ISR every 1 millisecond.
// Return pResult

unsigned int Read_Power(byte pType){
  long pCalc, tCalc, bCalc;
  long pResult;
  
  if (pType == fwd_p){                                                          // for forward power
    if (!TX) return 0;
    if (analogRead(INPUT_RF) < 1) return 0;
    pCalc = long(f_tot) + long(30);
    pResult = (pCalc * pCalc) / long(809);
    if (pResult < 100){                                                         // @ low power estimate output (drive x 10)
    pCalc = long(d_tot) + long(30);
    pResult = (pCalc * pCalc) / long(1086);      
    }
    if (pResult < 75){                                                          // try to estimate down to very low power
    pCalc = long(d_tot) + long(30);
    pResult = (pCalc * pCalc) / long(1200);      
    }
    if (pResult < 50){                                                          
    pCalc = long(d_tot) + long(30);
    pResult = (pCalc * pCalc) / long(1500);      
    }
    if (pResult < 25){                                                          
    pCalc = long(d_tot) + long(30);
    pResult = (pCalc * pCalc) / long(2500);      
    }
  }
  
  if (pType == rfl_p){                                                          // for reflected power
    if (!TX) return 0;
    if (analogRead(INPUT_RF) < 1) return 0;
    pCalc = long(r_tot) + long(140);
    pResult = (pCalc * pCalc) / long(10716);
  }
  
  if (pType == drv_p){                                                          // for drive power
    if (analogRead(INPUT_RF) < 1) return 0;
    pCalc = long(d_tot) + long(30);
    pResult = (pCalc * pCalc) / long(10860);
  }
  
  if (pType == vswr){                                                           // for vswr
    
    if (!TX) return 0;
    if (f_tot < 100) return 0;
    if (TX && r_tot < 5) return 10;                                             // For very low vswr display 1.0
    tCalc = long(100) * long(f_tot) + long(30) * (r_tot) + long(2800);
    bCalc = long(10) * long(f_tot) - long(3) * long(r_tot) - long(280);
    pResult = swr_val.filter(tCalc / bCalc);                                    // Exponentially smooth swr value
    if (pResult < 10) pResult = 10;
    if (pResult > 99) pResult = 99;
  }
  return pResult;
}



//===============================================================================================================
// Returns the voltage in 25mV steps

unsigned int Read_Voltage(void){                                             
byte ADCvinMSB, ADCvinLSB;

  Wire.beginTransmission(LTCADDR);                                          // first get Input Voltage - 80V max
  Wire.write(0x1e);                                                         // Voltage in register address
  Wire.endTransmission(false);
  Wire.requestFrom(LTCADDR, 2, true);                                       // Read two bytes
  delay(1);
  ADCvinMSB = Wire.read();                                                  // Read MSB
  ADCvinLSB = Wire.read();                                                  // Read LSB
  Wire.endTransmission();

  return((unsigned int)(ADCvinMSB) << 4) + ((ADCvinLSB >> 4) & 0x0F);       // formats into 12bit integer  
}



//===============================================================================================================
// Returns the current in 5mA steps

unsigned int Read_Current(void){                                            
byte curSenseMSB, curSenseLSB;
  Wire.beginTransmission(LTCADDR);                                          // Get sense current
  Wire.write(0x14);                                                         // Delta sense register address
  Wire.endTransmission(false);
  Wire.requestFrom(LTCADDR, 2, true);                                       // Read two bytes
  delay(1);
  curSenseMSB = Wire.read();                                                // Read MSB
  curSenseLSB = Wire.read();                                                // Read LSB
  Wire.endTransmission();

  return ((unsigned int)(curSenseMSB) << 4) + ((curSenseLSB >> 4) & 0x0F);  // 12 bit format

}


//===============================================================================================================
// Returns the power from V x I internal LTC based on 5 milliohm shunt

unsigned long Read_DCPwr(void){                                            
byte pwrSenseUSB, pwrSenseMSB, pwrSenseLSB;
  Wire.beginTransmission(LTCADDR);                                                    // Get sense power
  Wire.write(0x5);                                                                    // Power sense register address
  Wire.endTransmission(false);
  Wire.requestFrom(LTCADDR, 3, true);                                                 // Read three bytes
  delay(1);
  pwrSenseUSB = Wire.read();                                                          // Read USB
  pwrSenseMSB = Wire.read();                                                          // Read MSB
  pwrSenseLSB = Wire.read();                                                          // Read LSB
  Wire.endTransmission();
                                                                                      // 5mV/bit and 25mA/bit so div/8000
  return ((unsigned long)((pwrSenseUSB * 65536 + pwrSenseMSB * 256 + pwrSenseLSB)/8000));   

}




//===============================================================================================================
// Clear any existing faults in LTC2945 power monitor chip

void trip_clear(void){
  Wire.beginTransmission(LTCADDR);                                         
  Wire.write(0x04);                                                         // Fault CoR register address
  Wire.endTransmission(false);
  Wire.requestFrom(LTCADDR, 2, true);                                       // Request two bytes (clears any existing fault)
  delay(1);
  Wire.read();
  Wire.read();
  Wire.endTransmission();
}



//===============================================================================================================
// Establish a fault condition which causes LTC2945 to latch out DC power to the PA.

void trip_set(void){
  Wire.beginTransmission(LTCADDR);                                        
  Wire.write(0x03);                                                         // Select fault register
  Wire.write(0x01);                                                         // Write ADIN undervalue fault.
  Wire.endTransmission();
  BIAS_OFF
  Send_RLY(SR_DATA);
  RF_BYPASS
  trip = 1;
  TX = 0;
  DrawMode();
  Tft.LCD_SEL = 0;
}



//===============================================================================================================
// Establish a fault condition which causes LTC2945 to latch out DC power to the PA.

void VDD_OFF(void){
  Wire.beginTransmission(LTCADDR);                                        
  Wire.write(0x03);                                                         // Select fault register
  Wire.write(0x01);                                                         // Write ADIN undervalue fault.
  Wire.endTransmission();
  BIAS_ON                                                                   // discharge VDD supply
}




//===============================================================================================================
// Write max power threshold to LTC2945 to latch out DC power to the PA.

void mxp_set(void){
  Wire.beginTransmission(LTCADDR);                                          // Set control register to test mode
  Wire.write(0x00);
  Wire.write(0x15);                                                         // to allow threshold setting
  Wire.endTransmission();  
  delay(1);
  Wire.beginTransmission(LTCADDR);                                        
  Wire.write(0x0E);                                                         // Select max power register
  Wire.write(MXP_USB);                                                      // Write upper threshold byte
  Wire.write(MXP_MSB);                                                      // Write mid threshold byte
  Wire.write(MXP_LSB);                                                      // Write lower threshold byte
  Wire.endTransmission();
  delay(1);
  Wire.beginTransmission(LTCADDR);                                          // Reset control register to normal mode
  Wire.write(0x00);
  Wire.write(0x05);                                                         
  Wire.endTransmission();  
  delay(1);
}

//===============================================================================================================
// Write max voltage threshold to LTC2945 to latch out DC power to the PA.

void mxv_set(void){
  Wire.beginTransmission(LTCADDR);                                          // Set control register to test mode
  Wire.write(0x00);
  Wire.write(0x15);                                                         // to allow threshold setting
  Wire.endTransmission();  
  delay(1);
  Wire.beginTransmission(LTCADDR);                                        
  Wire.write(0x24);                                                         // Select max voltage register
  Wire.write(MXV_MSB);                                                      // Write upper threshold byte
  Wire.write(MXV_LSB);                                                      // Write lower threshold byte
  Wire.endTransmission();
  delay(1);
  Wire.beginTransmission(LTCADDR);                                          // Reset control register to normal mode
  Wire.write(0x00);
  Wire.write(0x05);                                                         
  Wire.endTransmission();  
  delay(1);
}
//===============================================================================================================
// Write max current threshold to LTC2945 to latch out DC power to the PA.

void mxi_set(void){
  Wire.beginTransmission(LTCADDR);                                          // Set control register to test mode
  Wire.write(0x00);
  Wire.write(0x15);                                                         // to allow threshold setting
  Wire.endTransmission();  
  delay(1);
  Wire.beginTransmission(LTCADDR);                                        
  Wire.write(0x1A);                                                         // Select max power register
  Wire.write(MXI_MSB);                                                      // Write upper threshold byte
  Wire.write(MXI_LSB);                                                      // Write lower threshold byte
  Wire.endTransmission();
  delay(1);
  Wire.beginTransmission(LTCADDR);                                          // Reset control register to normal mode
  Wire.write(0x00);
  Wire.write(0x05);                                                         
  Wire.endTransmission();  
  delay(1);
}

//===============================================================================================================
// Write bogus min ADC threshold to LTC2945 to latch out DC power to the PA.

void minadc_set(void){
  Wire.beginTransmission(LTCADDR);                                          // Set control register to test mode
  Wire.write(0x00);
  Wire.write(0x15);                                                         // to allow threshold setting
  Wire.endTransmission();  
  delay(1);
  Wire.beginTransmission(LTCADDR);                                        
  Wire.write(0x30);                                                         // Select min ADIN register
  Wire.write(0x00);                                                         // Write upper threshold byte
  Wire.write(0x80);                                                         // Write lower threshold byte
  Wire.endTransmission();
  delay(1);
  Wire.beginTransmission(LTCADDR);                                          // Reset control register to normal mode
  Wire.write(0x00);
  Wire.write(0x05);                                                         
  Wire.endTransmission();  
  delay(1);
}


//===============================================================================================================
// Returns the LTC2945 fault log register contents

unsigned int Read_Fault(void){                                             
byte Fault;

  Wire.beginTransmission(LTCADDR);                                          // Communicate with LTC2945 via I2C
  Wire.write(0x03);                                                         // Fault register address
  Wire.endTransmission(false);
  Wire.requestFrom(LTCADDR, 1, true);                                       // Read one byte
  delay(1);
  Fault = Wire.read();                                                      // Read fault log
  Wire.endTransmission();  
  return(Fault);                                                            // Return fault list to calling routine  
}

//===============================================================================================================
// Program fault masks in LTC2945 power monitor chip

void Set_Faults(void){
  Wire.beginTransmission(LTCADDR);                                          // set alert register
  Wire.write(0x01);
  Wire.write(0xA9);                                                         // Enable faults MxV MxI MxP MnADC
  Wire.endTransmission();
  delay(1);
}
