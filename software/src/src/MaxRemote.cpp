#include "MaxRemote.h"
#include "MaxReceiver.h"
#include <stdlib.h>  // for strtol
using namespace MaxFan;
#include <vector>

const uint16_t TICK = 400; 
const uint16_t FREQ = 38000; // 38kHz

MaxRemote::MaxRemote(uint8_t irPin) : irsend(irPin) {
  
 }

// Initialize the IR transmitter.
void MaxRemote::begin() {
  irsend.begin();
}

void MaxRemote::send(MaxFanState& state) {
  // Gibt Mikrosekunden seit dem Start zurück (uint64_t)
  int64_t now = esp_timer_get_time();

  // Umrechnung in Millis, falls gewünscht:
  unsigned long uptimeMilliseconds = now / 1000;

  if(uptimeMilliseconds < 5000)
    return; // während der ersten paar Sekunden die Updates noch zurückhalten. Der MaxFan ist vielleicht noch nicht bereit

  if((uptimeMilliseconds - lastSentAtUptimeMilliseconds )<2000)
    return; // IR updates throtteln
  
  lastSentAtUptimeMilliseconds = uptimeMilliseconds;

  if(this->lastSentState==state){
    return;
  }

  

  // zustand merken, damit derselbe Zustand nicht x-fach übermittelt wird
  lastSentState.SetBytes(state.GetStateByte(), state.GetSpeedByte(), state.GetTempByte());

  Serial.println("Changes detected, sending via IR...");

  // Daten zusammenstellen in einen Array
   uint8_t data[17];

   for (int i=0; i<10; i++){
    data[i] = HEADER[i];
   }

   data[10]=lastSentState.GetStateByte();
   data[11]=lastSentState.GetSpeedByte();
   data[12]=lastSentState.GetTempByte();

   data[13]=FOOTER[0];
   data[14]=FOOTER[1];

   data[15] = data[10] ^ data[11] ^ data[12] ^ data[13] ^ data[14] ;

   data[16]=0xFF;

  // Aus dem Array die Durations berechnen
  std::vector<uint16_t> durations;
    
  bool currentLevel = false; // Wir starten mit dem Startbit (0 / Space)
  uint32_t currentDuration = 0;

  for (uint16_t i = 0; i < 17; i++) {
      uint8_t b = data[i];

      // Wir bauen den Frame für dieses Byte: 1 Start(0), 8 Daten(LSB), 2 Stop(1)
      for (int bitIdx = 0; bitIdx < 11; bitIdx++) {
          bool bit;

          if (bitIdx == 0)      bit = false;      // Startbit
          else if (bitIdx < 9)  bit = (b >> (bitIdx - 1)) & 0x01; // Daten LSB
          else                  bit = true;       // 2 Stoppbits

          // Wenn das Bit den gleichen Pegel hat wie das vorherige, verlängern
          if (bit == currentLevel) {
              currentDuration += TICK;
          } else {
              // Pegelwechsel: Alten Puls wegschreiben und neuen starten
              durations.push_back(currentDuration);
              currentLevel = bit;
              currentDuration = TICK;
          }
      }
  }
  // Den letzten Puls noch hinzufügen
  durations.push_back(currentDuration);
  unsigned long start = millis();

  irsend.sendRaw(durations.data(), durations.size(), FREQ / 1000);
  unsigned long end = millis();

  //Serial.print("\nsending durations: ");
  //for (int i=0; i<durations.size();i++) {
  //    Serial.print(durations.at(i));
  //    Serial.print(" ");
  //}
  //Serial.print("\nsent in ");
  //Serial.print(end-start);
  //Serial.println();

  Serial.println(lastSentState.ToJson());
}







