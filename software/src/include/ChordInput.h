#ifndef CHORD_INPUT_H
#define CHORD_INPUT_H

#include <vector>
#include <queue>
#include <initializer_list>
#include <cstdint>
#include <Arduino.h> 

// ---------------------------------------------------------
// Das Event-Objekt (Ergebnis)
// ---------------------------------------------------------
struct KeyEvent {
    uint16_t mask;                  // Die Bitmaske der gedrückten Tasten
    const std::vector<int>* pinMap; // Referenz auf die Pin-Konfiguration

    // Konstruktoren
    KeyEvent(uint16_t m, const std::vector<int>* pMap);
    KeyEvent();

    // API Methoden
    bool IsSingle(int pinId) const;
    bool IsChord(int pinIdA, int pinIdB) const;


private:
    // Interne Hilfsmethode: Wandelt Pin-ID in Bit-Index um
    int getBitIndex(int pinId) const;
};

// ---------------------------------------------------------
// Die Input-Manager Klasse (Logik)
// ---------------------------------------------------------
class ChordInput {
public:
    // Konstruktor: Erwartet eine Liste von Pins (z.B. {12, 14, 27})
    ChordInput(std::initializer_list<int> pins);

    // Muss regelmäßig im Timer/Loop aufgerufen werden
    void tick();

    // Prüfen ob Events da sind
    bool hasEvent();

    // bricht den gerade pendenten Tastendruck (Chord) ab.
    // er wird dann beim Loslassen der Tasten nicht als Event ausgegeben
    void CancelCurrentChord();

    // Liefert true, wenn der Button momentan aktiv ist.
    bool IsKeyDown(int pinId);

    // Das nächste Event holen
    KeyEvent popEvent();

    // Gibt die Zeit des letzten erkannten Inputs zurück (in Mikrosekunden seit Boot)
    int64_t getLastInputTime() const;

private:
    std::vector<int> _pins;           // Speichert die Pin-IDs
    std::queue<KeyEvent> _eventQueue; // Warteschlange
    uint16_t _currentSequence;        // Aktuell gedrückte Tasten (Bitmaske)
    bool _isRecording;                // Status-Flag
    bool _currentChordIsCancelled;    // true, wenn der aktuell recordete Chrod gecancelled wurde
    long lastButtonCheck =0;          // millis des letzten Checks
    int64_t _lastInputTime = 0;       // Zeitstempel des letzten Inputs (esp_timer_get_time())
    // Hilfsmethode zum Hardware-Lesen
    uint16_t readHardware();
};

#endif