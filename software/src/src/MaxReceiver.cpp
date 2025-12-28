#include "MaxReceiver.h"
using namespace MaxFan;



#define RECEIVER_TICK_US 400

// --- Constructor ---
MaxReceiver::MaxReceiver(uint8_t recvPin) : irrecv(recvPin) {
  // Constructor: IRrecv is initialized with the given pin.
}

// --- begin() ---
void MaxReceiver::begin() {
  irrecv.enableIRIn();  // Start the IR receiver.
}



// --- getCommand() ---
// Returns true if a new command was received and parsed successfully
bool MaxReceiver::update(MaxFanState & maxFanState) {
  bool success = false;
  uint8_t data[16];

  while (irrecv.decode(&results)) {
    if (this->parseToBytes(data)) {

      maxFanState.SetBytes(data[10], data[11], data[12]);
      Serial.print("OK");
      success = true;
    } else {
      Serial.print("XX! Typ: ");
      Serial.print(results.decode_type); 
      Serial.print(" Bits: ");
      Serial.println(results.bits);
      // Das ist entscheidend:
      if (results.bits == 0 && results.decode_type == -1) {
        Serial.println("-> Geister-Trigger (Rauschen)");
      }
}
    resume();
  }
  if(success)
    Serial.print("-");
  return success;  
}




bool MaxReceiver::parseToBytes(uint8_t* output16Bytes) {

  memset(output16Bytes, 0, 16);

  int byteCount = 0;
  bool currentBit = false; // Startet bei IR meist mit dem Puls (LOW/0)
  const uint16_t threshold = 250;
  int bitInFrame = 0; 


  

  for (int i = 0; i < results.rawlen; i++) {
    uint16_t duration = results.rawbuf[i];
    
   
    if (duration < threshold) {
      continue;
    }

    

    int ticks = (int)round((float)duration / RECEIVER_TICK_US);
    if (ticks < 1) ticks = 1;

    for (int j = 0; j < ticks; j++) {

      if (bitInFrame == 0 && currentBit) {
        Serial.printf ("startbit != 0 on idx %d", byteCount);
        return false; 
      }
      
      // 2. Datenbits (1-8)
      if (bitInFrame >= 1 && bitInFrame <= 8) {
        if (currentBit) {
          output16Bytes[byteCount] |= (1 << (bitInFrame-1));
        }
      }

      // 3. Stopbits (9-10)
      if ((bitInFrame == 9 || bitInFrame == 10) && !currentBit) {
        Serial.printf("stopbit != 1 on idx %d", byteCount);
        return false;
      }
      bitInFrame++;
      
      if (bitInFrame >= 11) {
        bitInFrame = 0;
        byteCount++;
      }
    }
    
    
    currentBit = !currentBit;
  }

  // Daten gelesen
  if(byteCount<15){
    return false; // zu wenig Daten erhalten
  }
   
  if(byteCount == 15){
    int bitInByte = bitInFrame-2;
    // Letztes Byte mit 1er bits ergänzen wenn nicht gelesen
    for (int bit = bitInByte+1; bit<8; bit++){
      output16Bytes[byteCount] |= (1 << bit);
    }
  }

  // Check if Header is correct
  for (int idx=0; idx<10; idx++)
  {
    if(HEADER[idx] != output16Bytes[idx]){
      Serial.println("header mismatch");
      return false;
    }
  }


  // Daten sollten jetzt korrigiert sein
  uint8_t xorVal = output16Bytes[10] ^ output16Bytes[11] ^ output16Bytes[12] ^ output16Bytes[13] ^ output16Bytes[14];
  if (output16Bytes[15] != xorVal){
    Serial.println("--checksum--");
    Serial.println(output16Bytes[10], BIN);
    Serial.println(output16Bytes[11], BIN);
    Serial.println(output16Bytes[12], BIN);
    Serial.println(output16Bytes[13], BIN);
    Serial.println(output16Bytes[14], BIN);
    Serial.println("-----");
    Serial.println(output16Bytes[15], BIN);
    Serial.println(xorVal, BIN);
    Serial.flush();
    return false;
  } else {
    Serial.println();
    Serial.print("STATE:");
    Serial.print(output16Bytes[10], BIN);
    Serial.printf(" SPEED:");
    Serial.print(output16Bytes[11]);
    Serial.printf(" TEMP:");
    Serial.print(output16Bytes[12]);
    Serial.printf(" F13:");
    Serial.print(output16Bytes[13]);
    Serial.printf(" F14:");
    Serial.print(output16Bytes[14]);

    

    return true;
  }
  Serial.print("fall out");
  return false;

}

// --- resume() --- (private)
// Prepares the IR receiver for the next signal.
void MaxReceiver::resume() {
  irrecv.resume();
}
