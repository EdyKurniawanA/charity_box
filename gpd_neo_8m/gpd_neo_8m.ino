#include <TinyGPSPlus.h>

/*
   GPS Neo 8M Diagnostic Code for Arduino Mega 2560
   This version includes detailed diagnostics to troubleshoot connection issues
*/

static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;

void setup()
{
  Serial.begin(115200);
  Serial2.begin(GPSBaud);
 
  Serial.println(F("=== GPS Neo 8M Diagnostic Tool ==="));
  Serial.println(F("Hardware Serial2 connection (TX2-Pin16, RX2-Pin17)"));
  Serial.println();
  
  // Wait a moment for serial to stabilize
  delay(1000);
  
  Serial.println(F("Expected wiring:"));
  Serial.println(F("GPS VCC (Red) -> Arduino 5V"));
  Serial.println(F("GPS GND (Black) -> Arduino GND"));
  Serial.println(F("GPS RX -> Arduino TX2 (Pin 16)"));
  Serial.println(F("GPS TX -> Arduino RX2 (Pin 17)"));
  Serial.println();
  
  Serial.println(F("Starting GPS diagnostics..."));
  Serial.println(F("Listening for raw GPS data..."));
  Serial.println();
}

void loop()
{
  static unsigned long lastCheck = 0;
  static unsigned long rawDataCount = 0;
  static unsigned long validSentences = 0;
  
  // Read and display raw GPS data
  while (Serial2.available() > 0)
  {
    char c = Serial2.read();
    rawDataCount++;
    
    // Display raw characters (first 100 chars only to avoid spam)
    if (rawDataCount <= 100) {
      Serial.print(c);
    }
    
    // Try to encode the character
    if (gps.encode(c))
    {
      validSentences++;
      displayInfo();
    }
  }
  
  // Display diagnostics every 5 seconds
  if (millis() - lastCheck > 5000)
  {
    lastCheck = millis();
    
    Serial.println(F("\n=== DIAGNOSTIC INFO ==="));
    Serial.print(F("Raw characters received: "));
    Serial.println(rawDataCount);
    Serial.print(F("Characters processed by GPS library: "));
    Serial.println(gps.charsProcessed());
    Serial.print(F("Valid sentences decoded: "));
    Serial.println(validSentences);
    Serial.print(F("Failed checksum count: "));
    Serial.println(gps.failedChecksum());
    Serial.print(F("Passed checksum count: "));
    Serial.println(gps.passedChecksum());
    
    if (rawDataCount == 0)
    {
      Serial.println(F("\n*** NO DATA RECEIVED ***"));
      Serial.println(F("Possible issues:"));
      Serial.println(F("1. Check power connections (VCC to 5V, GND to GND)"));
      Serial.println(F("2. Verify TX/RX connections are not swapped"));
      Serial.println(F("3. GPS module may be faulty"));
      Serial.println(F("4. Try different baud rate (38400 or 115200)"));
    }
    else if (gps.charsProcessed() == 0)
    {
      Serial.println(F("\n*** RECEIVING DATA BUT NOT GPS FORMAT ***"));
      Serial.println(F("Data might be at wrong baud rate or corrupted"));
      Serial.println(F("Try changing baud rate to 38400 or 115200"));
    }
    else if (validSentences == 0)
    {
      Serial.println(F("\n*** RECEIVING GPS DATA BUT NO VALID SENTENCES ***"));
      Serial.println(F("GPS is working but may need time to get satellite fix"));
      Serial.println(F("Move to an open area with clear sky view"));
    }
    else
    {
      Serial.println(F("\n*** GPS IS WORKING CORRECTLY ***"));
    }
    
    Serial.println(F("=======================\n"));
  }
  
  // Extended timeout for initial detection
  if (millis() > 30000 && rawDataCount == 0)
  {
    Serial.println(F("\n*** 30 SECONDS - NO DATA RECEIVED ***"));
    Serial.println(F("Please check:"));
    Serial.println(F("1. Power connections"));
    Serial.println(F("2. TX/RX wiring"));
    Serial.println(F("3. Try different baud rates"));
    
    // Try different baud rates
    Serial.println(F("\nTrying different baud rates..."));
    
    uint32_t baudRates[] = {4800, 9600, 38400, 115200};
    for (int i = 0; i < 4; i++)
    {
      Serial.print(F("Testing baud rate: "));
      Serial.println(baudRates[i]);
      Serial2.end();
      delay(100);
      Serial2.begin(baudRates[i]);
      delay(2000);
      
      int testCount = 0;
      while (Serial2.available() > 0 && testCount < 50)
      {
        Serial.print((char)Serial2.read());
        testCount++;
      }
      
      if (testCount > 0)
      {
        Serial.print(F("\n*** FOUND DATA AT BAUD RATE: "));
        Serial.print(baudRates[i]);
        Serial.println(F(" ***"));
        Serial.println(F("Update your code with this baud rate!"));
        while(true); // Stop here
      }
    }
    
    Serial.println(F("\nNo data found at any baud rate."));
    Serial.println(F("Check hardware connections."));
    while(true);
  }
}

void displayInfo()
{
  Serial.println(F("\n=== GPS LOCATION DATA ==="));
  
  // Latitude
  Serial.print(F("Latitude: "));
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.println(F(" degrees"));
  }
  else
  {
    Serial.println(F("INVALID"));
  }
  
  // Longitude
  Serial.print(F("Longitude: "));
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lng(), 6);
    Serial.println(F(" degrees"));
  }
  else
  {
    Serial.println(F("INVALID"));
  }
  
  // Altitude
  Serial.print(F("Altitude: "));
  if (gps.altitude.isValid())
  {
    Serial.print(gps.altitude.meters(), 2);
    Serial.println(F(" meters"));
  }
  else
  {
    Serial.println(F("INVALID"));
  }
  
  // Additional useful information
  Serial.print(F("Satellites: "));
  if (gps.satellites.isValid())
  {
    Serial.println(gps.satellites.value());
  }
  else
  {
    Serial.println(F("INVALID"));
  }
  
  // Date and Time
  Serial.print(F("Date/Time: "));
  if (gps.date.isValid() && gps.time.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
    Serial.print(F(" "));
    
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.println(F(" UTC"));
  }
  else
  {
    Serial.println(F("INVALID"));
  }
  
  // CSV format output for easy data logging
  Serial.print(F("CSV: "));
  if (gps.location.isValid() && gps.altitude.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
    Serial.print(F(","));
    Serial.print(gps.altitude.meters(), 2);
  }
  else
  {
    Serial.print(F("INVALID,INVALID,INVALID"));
  }
  Serial.println();
  
  Serial.println(F("========================"));
}