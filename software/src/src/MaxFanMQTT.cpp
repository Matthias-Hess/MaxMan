#include "MaxFanMQTT.h"
#include "MaxFanConfig.h"
#include <Arduino.h>

// PubSubClient requires a client reference; we'll set callback to static function
static MqttController* instanceForCallback = nullptr;

MqttController::MqttController()
    : _mqtt(_wifiClient), _onCommandReceived(nullptr), _connected(false),
      _lastSentState(), _forceUpdate(true), _lastConnectAttemptMs(0), _reconnectIntervalMs(RECONNECT_BASE_MS)
{
    instanceForCallback = this;
    Serial.println("MqttController: constructed");
}

void MqttController::begin(const char* deviceName) {
    Serial.println("MqttController: begin");

    (void)deviceName;
    _mqtt.setCallback(MqttController::mqttCallbackStatic);
    _forceUpdate = true;
    ensureConnected();
}

void MqttController::setCommandCallback(FanController::CommandCallback callback) {
    _onCommandReceived = callback;
    Serial.println("MqttController: command callback registered");
}

void MqttController::ensureConnected() {
    if (_mqtt.connected()) {
        _connected = true;
        Serial.println("connected = true");
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        _connected = false;
        Serial.println("connected = false (WiFi not connected)");
        return;
    }

    const char* host = GlobalConfig.mqttHost;
    int port = GlobalConfig.mqttPort;
    if (!host || strlen(host) == 0) {
        _connected = false;
        Serial.println("connected = false (invalid host)");
        return;
    }

    uint32_t now = millis();
    // Backoff: only attempt if interval elapsed
    if (now - _lastConnectAttemptMs < _reconnectIntervalMs) {
        uint32_t remaining = _reconnectIntervalMs - (now - _lastConnectAttemptMs);
        Serial.printf("MQTT: Skipping connect attempt (backoff), wait %u ms\n", remaining);
        _connected = false;
        return;
    }
    _lastConnectAttemptMs = now;

    // Validate topics
    if (!isValidTopic(GlobalConfig.mqttCommandTopic) || !isValidTopic(GlobalConfig.mqttStateTopic)) {
        _connected = false;
        // increase backoff
        uint32_t next = _reconnectIntervalMs * 2;
        _reconnectIntervalMs = (next > RECONNECT_MAX_MS) ? RECONNECT_MAX_MS : next;
        return;
    }

    Serial.printf("MQTT: Connecting to %s:%d\n", host, port);
    _mqtt.setServer(host, port);

    String clientId = String(GlobalConfig.mqttClientId);
    if (clientId.length() == 0) {
        clientId = "MaxFan-" + WiFi.macAddress();
    }

    bool ok;
    if (strlen(GlobalConfig.mqttUsername) > 0) {
        Serial.printf("MQTT: Using auth user='%s'\n", GlobalConfig.mqttUsername);
        ok = _mqtt.connect(clientId.c_str(), GlobalConfig.mqttUsername, GlobalConfig.mqttPassword);
    } else {
        ok = _mqtt.connect(clientId.c_str());
    }
    if (ok) {
        _connected = true;
        _reconnectIntervalMs = RECONNECT_BASE_MS; // reset backoff
        bool subOk = _mqtt.subscribe(GlobalConfig.mqttCommandTopic);
        Serial.printf("MQTT: Connected (clientId=%s), subscribed=%d\n", clientId.c_str(), (int)subOk);
        _forceUpdate = true;
    } else {
        _connected = false;
        int state = _mqtt.state();
        Serial.printf("MQTT: Connect failed, state=%d\n", state);
        uint32_t next = _reconnectIntervalMs * 2;
        _reconnectIntervalMs = (next > RECONNECT_MAX_MS) ? RECONNECT_MAX_MS : next;
    }
}

void MqttController::notifyStatus(const MaxFanState& currentState) {
    



    if (!_forceUpdate && (currentState == _lastSentState)) return;

    if (!_mqtt.connected()) {
        Serial.println("MQTT: Not connected, cannot publish");
        return;
    }

    String payload = currentState.ToJson();
    Serial.printf("MQTT: Publishing to %s payload=%s\n", GlobalConfig.mqttStateTopic, payload.c_str());
    bool ok = _mqtt.publish(GlobalConfig.mqttStateTopic, payload.c_str());
    if (ok) {
        _lastSentState = currentState;
        _forceUpdate = false;
        Serial.println("MQTT: Publish OK");
    } else {
        int st = _mqtt.state();
        Serial.printf("MQTT: Publish FAILED, state=%d\n", st);
        Serial.println("MQTT: Forcing disconnect and reconnect after publish failure");
        // Properly disconnect PubSubClient first
        _mqtt.disconnect();
        // Stop underlying client socket
        _wifiClient.stop();
        _connected = false;
        _forceUpdate = true; // try again after reconnect
        // Reset backoff so a reconnect attempt happens quickly
        _reconnectIntervalMs = RECONNECT_BASE_MS;
        _lastConnectAttemptMs = 0;
    }
}

void MqttController::loop() {
    ensureConnected();
    if (_mqtt.connected()) {
        _mqtt.loop();
    }
}

bool MqttController::isConnected() {
    return _mqtt.connected();
}

char MqttController::getIndicatorLetter() {
    // WiFi not connected -> W
    if (WiFi.status() != WL_CONNECTED) return 'W';

    // If WiFi connected but MQTT not connected, inspect state
    if (!_mqtt.connected()) {
        int st = _mqtt.state();
        // PubSubClient state mapping (connect error reasons):
        // 3 -> MQTT_CONNECT_UNAVAILABLE (host unreachable)
        // 4 -> MQTT_CONNECT_BAD_CREDENTIALS
        // 5 -> MQTT_CONNECT_UNAUTHORIZED
        if (st == 3) return 'R';
        if (st == 4 || st == 5) return 'C';
        // Fallback for other errors: show 'R' (remote unreachable)
        return 'R';
    }

    return '\0';
}

void MqttController::mqttCallbackStatic(char* topic, byte* payload, unsigned int length) {
    if (instanceForCallback) instanceForCallback->mqttCallback(topic, payload, length);
}

void MqttController::mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.printf("MQTT: Message arrived topic=%s len=%u\n", topic, length);
    String s;
    for (unsigned int i = 0; i < length; i++) s += (char)payload[i];
    Serial.printf("MQTT: Payload=%s\n", s.c_str());
    if (_onCommandReceived == nullptr) {
        Serial.println("MQTT: No command callback registered");
        return;
    }
    _onCommandReceived(s);
}

bool MqttController::isValidTopic(const char* topic) {
    if (!topic) return false;
    size_t len = strlen(topic);
    if (len == 0) return false;
    for (size_t i = 0; i < len; i++) {
        char c = topic[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') return false;
    }
    return true;
}
