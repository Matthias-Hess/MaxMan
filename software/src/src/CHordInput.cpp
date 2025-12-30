#include "ChordInput.h"

// =========================================================
// Implementierung: KeyEvent Struct
// =========================================================

KeyEvent::KeyEvent(uint16_t m, const std::vector<int>* pMap) 
    : mask(m), pinMap(pMap) {}

KeyEvent::KeyEvent() 
    : mask(0), pinMap(nullptr) {}

int KeyEvent::getBitIndex(int pinId) const {
    if (!pinMap) return -1;
    // Suche den Pin im Vector
    for (size_t i = 0; i < pinMap->size(); ++i) {
        if ((*pinMap)[i] == pinId) return (int)i;
    }
    return -1; // Nicht gefunden
}

bool KeyEvent::IsSingle(int pinId) const {
    int idx = getBitIndex(pinId);
    if (idx < 0) return false;
    // Prüfen ob NUR dieses eine Bit gesetzt ist
    return mask == (uint16_t)(1 << idx);
}

bool KeyEvent::IsChord(int pinIdA, int pinIdB) const {
    int idxA = getBitIndex(pinIdA);
    int idxB = getBitIndex(pinIdB);
    if (idxA < 0 || idxB < 0) return false;
    
    // Prüfen ob GENAU diese zwei Bits gesetzt sind
    return mask == (uint16_t)((1 << idxA) | (1 << idxB));
}


// =========================================================
// Implementierung: ChordInput Klasse
// =========================================================

ChordInput::ChordInput(std::initializer_list<int> pins) 
    : _pins(pins) 
{
    _currentSequence = 0;
    _isRecording = false;

    // Hardware initialisieren (Active Low angenommen -> INPUT_PULLUP)
    for (int pin : _pins) {
        pinMode(pin, INPUT_PULLUP);
    }
}

uint16_t ChordInput::readHardware() {
    uint16_t inputMask = 0;
    
    for (size_t i = 0; i < _pins.size(); ++i) {
        // Logik: Active LOW (Taste gedrückt = LOW)
        if (digitalRead(_pins[i]) == LOW) {
            inputMask |= (1 << i);
        }
    }
    return inputMask;
}
void ChordInput::CancelCurrentChord() {
    if(!_isRecording) return;
    
    _currentChordIsCancelled = true;

}

bool ChordInput::IsKeyDown(int pin)
{
    uint16_t currentState = readHardware();
    if (currentState==0) return false;
    for (size_t i = 0; i < _pins.size(); ++i) {
        if ((_pins[i]) == pin) {
            return (currentState & (1 << i)) != 0;
        }
    }
    return false;
}

void ChordInput::tick() {
    if (millis() - lastButtonCheck < 20) {
      return;
    }

    lastButtonCheck = millis();


    uint16_t liveInput = readHardware();

    if (liveInput > 0) {
        // FALL: Tasten sind gedrückt -> Aufnehmen/Erweitern
        _isRecording = true;
        _currentSequence |= liveInput; 
    } 
    else {
        // FALL: Alle Tasten losgelassen
        if (_isRecording && ! _currentChordIsCancelled) {
            // Sequenz ist beendet -> Speichern
            if (_currentSequence > 0) {
                // Event erstellen und Map-Pointer übergeben
                KeyEvent evt(_currentSequence, &_pins);
                _eventQueue.push(evt);
            }
        }
        _currentChordIsCancelled = false;
        _currentSequence = 0;
        _isRecording = false;
    }
}

bool ChordInput::hasEvent() {
    return !_eventQueue.empty();
}

KeyEvent ChordInput::popEvent() {
    if (_eventQueue.empty()) {
        return KeyEvent(); // Leeres Event
    }
    
    KeyEvent evt = _eventQueue.front();
    _eventQueue.pop();
    return evt;
}