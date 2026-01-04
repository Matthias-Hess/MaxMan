#ifndef MAXFANMQTT_H
#define MAXFANMQTT_H

#include "RemoteAccess.h"
#include <PubSubClient.h>
#include <WiFi.h>

class MaxFanMQTT : public RemoteAccess {
public:
    MaxFanMQTT();
    void begin(const char* deviceName = nullptr) override;
    void setCommandCallback(CommandCallback callback) override;
    void notifyStatus(const MaxFanState& currentState) override;
    void loop() override;
    bool isConnected() override;
    char getIndicatorLetter() override;
    RemoteAccess::Icon getIcon() override { return RemoteAccess::ICON_MQTT; }

private:
    WiFiClient _wifiClient;
    PubSubClient _mqtt;
    CommandCallback _onCommandReceived;
    bool _connected;

    // State publish dedupe
    MaxFanState _lastSentState;
    bool _forceUpdate;

    // Reconnect/backoff
    uint32_t _lastConnectAttemptMs;
    uint32_t _reconnectIntervalMs;
    static constexpr uint32_t RECONNECT_BASE_MS = 1000;
    static constexpr uint32_t RECONNECT_MAX_MS = 60000;

    bool isValidTopic(const char* topic);

    void ensureConnected();
    static void mqttCallbackStatic(char* topic, byte* payload, unsigned int length);
    void mqttCallback(char* topic, byte* payload, unsigned int length);
};

#endif
