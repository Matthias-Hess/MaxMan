#include "ModeConfig.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>

#ifndef APP_VERSION
#define APP_VERSION "v0.0.0-dev"
#endif

ModeConfig* ModeConfig::instance = nullptr;

SelectOptionInt optionsConnection[] = {
    {"None", 0},
    {"BLE",  1},
    {"MQTT", 2}
};

SelectOptionInt optionsTimeout[] = {
    {"Never", 0},
    {"10s",  10},
    {"20s",  20},
    {"30s",  30},
    {"1min", 60},
    {"5min", 300}
};

GEMSelect selectConnection(3, optionsConnection);
GEMSelect selectTimeout(6, optionsTimeout);

// -----------------------------------------------------------
// KONSTRUKTOR
// -----------------------------------------------------------
ModeConfig::ModeConfig(U8G2* display, Encoder* encoder, ChordInput* input)
    : 
     AppMode(*display, *encoder, *input), 
    _menu(*display, GEM_POINTER_ROW, GEM_ITEMS_COUNT_AUTO),
    _mustExit(false),
    
    // --- SEITEN (Statisch) ---
    _pageMain("Settings"),
    _pageRemote("Remote Access"),
    _pageWifi("Wi-Fi Settings"),
    _pageMqtt("MQTT Settings"),
    _pageBle("Bluetooth LE"),
    _pageDisplay("Display"),
    _pageVersionInfo("Firmware Version"),
    _pageExit("Save Changes?"), 
    // _pageVersionsSelect wird im Body initialisiert!

    // --- NAVIGATION ITEMS ---
    _itemNavWifi("Wi-Fi", _pageWifi),
    _itemNavMqtt("MQTT", _pageMqtt),
    _itemNavBle("Bluetooth LE", _pageBle),
    _itemNavRemote("Remote Access", _pageRemote),
    _itemNavDisplay("Display", _pageDisplay),
    _itemNavVersion("Version", _pageVersionInfo),
    _itemNavExit("Exit", callbackCheckExit),

    // --- BACK BUTTONS ---
    _itemBackRemote("Back", callbackGoBackToMain),
    _itemBackWifi("Back", callbackGoBackToMain),
    _itemBackMqtt("Back", callbackGoBackToMain),
    _itemBackBle("Back", callbackGoBackToMain),
    _itemBackDisplay("Back", callbackGoBackToMain),
    _itemBackVersion("Back", callbackGoBackToMain),
    _itemBackFromExit("Back", callbackGoBackToMain),

    // --- ITEMS ---
    _itemConnection("Connection", _editConfig.connection, selectConnection),
    _itemSsid("SSID:", _editConfig.wifiSSID),
    _itemPassword("Password:", _editConfig.wifiPassword),
    _itemMqttHost("Host:", _editConfig.mqttHost),
    _itemMqttPort("Port:", _editConfig.mqttPort),
    _itemMqttClientId("Client ID:", _editConfig.mqttClientId),
    _itemMqttUser("User:", _editConfig.mqttUsername),
    _itemMqttPassword("Password:", _editConfig.mqttPassword),
    _itemMqttCommandTopic("Cmd topic:", _editConfig.mqttCommandTopic),
    _itemMqttStateTopic("State topic:", _editConfig.mqttStateTopic),
    _itemTestWifi("Test Connection", callbackTestWifi),
    _itemTestMqtt("Test Connection", callbackTestMqtt),
    _itemBlePin("PIN:", _editConfig.blePin),
    _itemGenerateNewPIN("Generate new PIN", callbackGenerateNewPIN),
    _itemDisplayTimeoutSeconds("Dim after", _editConfig.displayTimeoutSeconds, selectTimeout),
    _itemCurrentVersion("Installed:", APP_VERSION, true), 
    _itemCheckUpdates("Check for Updates", callbackCheckForUpdates),
    _itemSave("Save Changes", callbackSaveAndRestart),
    _itemDiscard("Discard Changes", callbackDiscardAndRestart)
{
    instance = this;
    snprintf(versionLabel, sizeof(versionLabel), "v%s", APP_VERSION);

    // WICHTIG: Dynamische Page initialisieren
    _pageVersionsSelect = new GEMPage("Select Version");

    // =========================================================
    // MENU STRUKTUR AUFBAUEN
    // =========================================================

    // 1. Main Page
    _pageMain.addMenuItem(_itemNavWifi);
    _pageMain.addMenuItem(_itemNavMqtt);
    _pageMain.addMenuItem(_itemNavBle);
    _pageMain.addMenuItem(_itemNavRemote);
    _pageMain.addMenuItem(_itemNavDisplay);
    _pageMain.addMenuItem(_itemNavVersion);
    _pageMain.addMenuItem(_itemNavExit);

    // 2. Remote Access Page
    _pageRemote.addMenuItem(_itemConnection);
    _pageRemote.addMenuItem(_itemBackRemote); 

    // 3. Wi-Fi Page
    _pageWifi.addMenuItem(_itemSsid);
    _pageWifi.addMenuItem(_itemPassword);
    _pageWifi.addMenuItem(_itemTestWifi);
    _pageWifi.addMenuItem(_itemBackWifi);     

    // 3b. MQTT Page
    _pageMqtt.addMenuItem(_itemMqttHost);
    _pageMqtt.addMenuItem(_itemMqttPort);
    _pageMqtt.addMenuItem(_itemMqttClientId);
    _pageMqtt.addMenuItem(_itemMqttUser);
    _pageMqtt.addMenuItem(_itemMqttPassword);
    _pageMqtt.addMenuItem(_itemMqttCommandTopic);
    _pageMqtt.addMenuItem(_itemMqttStateTopic);
    _pageMqtt.addMenuItem(_itemTestMqtt);
    _pageMqtt.addMenuItem(_itemBackMqtt);

    // 4. BLE Page
    _pageBle.addMenuItem(_itemBlePin);
    _pageBle.addMenuItem(_itemGenerateNewPIN);
    _pageBle.addMenuItem(_itemBackBle);       

    // 5. Display Page
    _pageDisplay.addMenuItem(_itemDisplayTimeoutSeconds);
    _pageDisplay.addMenuItem(_itemBackDisplay); 

    // 6. Version Page
    _pageVersionInfo.addMenuItem(_itemCurrentVersion);
    _pageVersionInfo.addMenuItem(_itemCheckUpdates);
    _pageVersionInfo.addMenuItem(_itemBackVersion); 

    // 7. Exit Page
    _pageExit.addMenuItem(_itemSave);
    _pageExit.addMenuItem(_itemDiscard);
    _pageExit.addMenuItem(_itemBackFromExit); 

    _itemPassword.setAdjustedASCIIOrder();
    _itemSsid.setAdjustedASCIIOrder();

    _menu.setMenuPageCurrent(_pageMain);
}

// -----------------------------------------------------------
// LIFECYCLE
// -----------------------------------------------------------
void ModeConfig::enter() {
    _editConfig = GlobalConfig; 
    _mustExit = false;
    
    _menu.setMenuPageCurrent(_pageMain);
    _pageMain.setCurrentMenuItemIndex(0);
    
    _menu.setSplashDelay(0);
    _menu.init();
    _menu.reInit();
    _menu.drawMenu();
}

ModeAction ModeConfig::loop() {
    bool actionDetected = false;

    while(_editConfig.blePin < 100000){
        _editConfig.blePin += 100000;
        _menu.drawMenu(); 
    }
        
    if(_mustExit) return ModeAction::SWITCH_TO_STANDARD;

    if (_menu.readyForKey()) {
        int delta = _encoder.getDelta();
        if (delta != 0) {
            actionDetected = true;
            if(_buttons.IsKeyDown(ENCODER_BUTTON)){
                 _buttons.CancelCurrentChord();
                 if (delta > 0) _menu.registerKeyPress(GEM_KEY_LEFT);
                 else           _menu.registerKeyPress(GEM_KEY_RIGHT);
            } else {
                if (delta > 0) _menu.registerKeyPress(GEM_KEY_UP);
                else           _menu.registerKeyPress(GEM_KEY_DOWN);
            }
        }

        _buttons.tick();
        if (_buttons.hasEvent()) {
            KeyEvent evt = _buttons.popEvent();
            
            if (evt.IsSingle(ENCODER_BUTTON)) {
                _menu.registerKeyPress(GEM_KEY_OK);
                actionDetected = true;
            }
            if (evt.IsSingle(MODE_BUTTON)) {
                _menu.registerKeyPress(GEM_KEY_CANCEL); 
                actionDetected = true;
            }
        }
    }

    if (actionDetected) {
        _menu.drawMenu();
    }
    return ModeAction::NONE;
}

// ------------------------------------------------
// ERROR HELPER
// ------------------------------------------------
void ModeConfig::showError(const char* line1, const char* line2) {
    Serial.printf("[ERROR] %s %s\n", line1, line2);
    
    instance->_display.clearBuffer();
    instance->_display.setFont(u8g2_font_helvB08_tf);
    instance->_display.drawStr(0, 20, "Error:");
    instance->_display.drawStr(0, 35, line1);
    instance->_display.drawStr(0, 50, line2);
    instance->_display.sendBuffer();
    
    delay(3000); 
    
    // Zurück zur Version Info Seite
    instance->_menu.setMenuPageCurrent(instance->_pageVersionInfo);
    instance->_menu.drawMenu();
}

// ------------------------------------------------
// NAVIGATION & SYSTEM CALLBACKS
// ------------------------------------------------

void ModeConfig::callbackCheckExit() {
    if (instance->_editConfig == GlobalConfig) {
        instance->_mustExit = true; 
    } else {
        instance->_menu.setMenuPageCurrent(instance->_pageExit);
        instance->_menu.drawMenu();
    }
}

void ModeConfig::callbackSaveAndRestart() {
    instance->_display.clearBuffer();
    instance->_display.drawStr(10, 30, "Saving...");
    instance->_display.sendBuffer();
    ConfigManager::saveAndReboot(instance->_editConfig);
}

void ModeConfig::callbackDiscardAndRestart() {
    instance->_mustExit = true; 
}

void ModeConfig::callbackGoBackToMain() {
    instance->_menu.setMenuPageCurrent(instance->_pageMain);
    instance->_menu.drawMenu();
}

void ModeConfig::callbackGoBackToVersion() {
    instance->_menu.setMenuPageCurrent(instance->_pageVersionInfo);
    instance->_menu.drawMenu();
}

void ModeConfig::callbackGenerateNewPIN() {
    instance->_editConfig.blePin = (esp_random() % 900000) + 100000;
}

// ------------------------------------------------
// WI-FI TEST
// ------------------------------------------------

void ModeConfig::callbackTestWifi() {
    instance->_display.clearBuffer();
    instance->_display.drawStr(0, 20, "Testing Connection...");
    instance->_display.sendBuffer();

    WiFi.disconnect();
    delay(500);

    bool success = instance->connectToWiFi(true);

    instance->_display.clearBuffer();
    if (success) {
        instance->_display.drawStr(0, 30, "Connection");
        instance->_display.drawStr(0, 50, "Successful!");
    } else {
        instance->_display.drawStr(0, 30, "Connection");
        instance->_display.drawStr(0, 50, "Failed!");
    }
    instance->_display.sendBuffer();
    delay(2000);
    instance->_menu.drawMenu();
}

void ModeConfig::callbackTestMqtt() {
    instance->_display.clearBuffer();
    instance->_display.drawStr(0, 20, "Testing MQTT...");
    instance->_display.sendBuffer();

    if (!instance->connectToWiFi(true)) {
        instance->showError("WiFi Failed", "Check Settings");
        return;
    }

    const char* host = instance->_editConfig.mqttHost;
    int port = instance->_editConfig.mqttPort;

    // Basic host validation
    auto isValidHost = [](const char* h)->bool{
        if (!h) return false;
        size_t l = strlen(h);
        if (l == 0) return false;
        for (size_t i = 0; i < l; ++i) {
            char c = h[i];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') return false;
        }
        return true;
    };

    auto isValidTopic = [](const char* t)->bool{
        if (!t) return false;
        size_t l = strlen(t);
        if (l == 0) return false;
        for (size_t i = 0; i < l; ++i) {
            char c = t[i];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') return false;
        }
        return true;
    };

    if (!isValidHost(host)) {
        instance->showError("MQTT Host invalid", "No whitespace, e.g. test.mosquitto.org");
        return;
    }

    if (!isValidTopic(instance->_editConfig.mqttCommandTopic) || !isValidTopic(instance->_editConfig.mqttStateTopic)) {
        instance->showError("MQTT Topic invalid", "No whitespace allowed in topics");
        return;
    }

    WiFiClient client;
    bool success = false;

    if (client.connect(host, port)) {
        success = true;
        client.stop();
    }

    instance->_display.clearBuffer();
    if (success) {
        instance->_display.drawStr(0, 30, "Connection");
        instance->_display.drawStr(0, 50, "Successful!");
    } else {
        instance->_display.drawStr(0, 30, "Connection");
        instance->_display.drawStr(0, 50, "Failed!");
    }
    instance->_display.sendBuffer();
    delay(2000);
    instance->_menu.drawMenu();
}

// ------------------------------------------------
// OTA / UPDATE LOGIK
// ------------------------------------------------

void ModeConfig::clearDynamicItems() {
    for (auto& entry : _releaseList) {
        delete entry.menuPtr; 
    }
    _releaseList.clear();
}

bool ModeConfig::connectToWiFi(bool useEditConfig) {
    if (WiFi.status() == WL_CONNECTED) return true;

    WiFi.begin(instance->_editConfig.wifiSSID, instance->_editConfig.wifiPassword); 
    
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) { 
        delay(500);
        timeout++;
    }
    return WiFi.status() == WL_CONNECTED;
}

void ModeConfig::callbackCheckForUpdates() {
    if (!instance->connectToWiFi(true)) { 
        instance->showError("WiFi Failed", "Check Settings");
        return;
    }

    instance->_display.clearBuffer();
    instance->_display.drawStr(10, 30, "Fetching GitHub...");
    instance->_display.sendBuffer();

    instance->fetchReleasesFromGitHub();
}

void ModeConfig::fetchReleasesFromGitHub() {
    Serial.println("--- Starting GitHub Fetch ---");
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());

    WiFiClientSecure client;
    client.setInsecure(); 
    HTTPClient http;
    
    String url = "https://api.github.com/repos/Matthias-Hess/MaxMan/releases"; 
    
    http.setTimeout(10000); 

    if (!http.begin(client, url)) {
        showError("HTTP Begin", "Failed");
        return;
    }

    http.addHeader("User-Agent", "ESP32-MaxMan"); 
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        int len = http.getSize();
        if (len > 25000) {
                showError("JSON too big", String(len).c_str());
                http.end();
                return;
        }

        String payload = http.getString();
        
        StaticJsonDocument<200> filter;
        filter[0]["tag_name"] = true;
        filter[0]["assets"][0]["name"] = true;
        filter[0]["assets"][0]["browser_download_url"] = true;
        
        DynamicJsonDocument doc(20480); 
        DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

        if (!error) {
            // 1. Alles Alte löschen
            instance->clearDynamicItems(); 
            
            // 2. Seite neu bauen
            delete instance->_pageVersionsSelect; 
            instance->_pageVersionsSelect = new GEMPage("Select Version");

            JsonArray arr = doc.as<JsonArray>();
            int count = 0;

            for (JsonObject repo : arr) {
                const char* tagName = repo["tag_name"];
                if (!tagName) continue;

                String label = String(tagName);
                
                if (label == String(APP_VERSION)) {
                    label += " (curr)";
                }

                String binUrl = "";
                JsonArray assets = repo["assets"];
                for (JsonObject asset : assets) {
                    const char* name = asset["name"];
                    const char* dlUrl = asset["browser_download_url"];
                    if (name && String(name).endsWith(".bin")) {
                        binUrl = String(dlUrl);
                        break; 
                    }
                }

                if (binUrl != "") {
                    char* labelCStr = strdup(label.c_str());
                    GEMItem* newItem = new GEMItem(labelCStr, callbackInstallUpdate);
                    
                    ReleaseEntry entry;
                    entry.tagName = label;
                    entry.downloadUrl = binUrl;
                    entry.menuPtr = newItem;
                    
                    instance->_releaseList.push_back(entry);
                    instance->_pageVersionsSelect->addMenuItem(*newItem);
                    count++;
                }
            }

            if (count == 0) {
                showError("No .bin files", "found");
            } else {
                // --- KORREKTUR HIER ---
                // Den Back-Button auch dynamisch erstellen!
                GEMItem* itemBack = new GEMItem("Back", callbackGoBackToVersion);
                
                // Zur Seite hinzufügen
                instance->_pageVersionsSelect->addMenuItem(*itemBack);
                
                // WICHTIG: Zur Aufräum-Liste hinzufügen, damit er beim nächsten Mal gelöscht wird
                ReleaseEntry entryBack;
                entryBack.tagName = "BackBtn"; // Dummy Name
                entryBack.menuPtr = itemBack;
                instance->_releaseList.push_back(entryBack);
                // ----------------------

                instance->_menu.setMenuPageCurrent(*instance->_pageVersionsSelect);
                instance->_menu.drawMenu();
            }

        } else {
            showError("JSON Error:", error.c_str());
        }
    } else {
        showError("HTTP Error", String(httpCode).c_str());
    }
    http.end();
}

void ModeConfig::callbackInstallUpdate(GEMCallbackData data) {
    for (const auto& entry : instance->_releaseList) {
        if (entry.menuPtr == data.pMenuItem) {
            performOTA(entry.downloadUrl.c_str());
            return;
        }
    }
}

void ModeConfig::performOTA(const char* url) {
    instance->_display.clearBuffer();
    instance->_display.setFont(u8g2_font_helvB10_tf);
    instance->_display.drawStr(0, 20, "Updating...");
    instance->_display.drawStr(0, 40, "Do not power off!");
    instance->_display.sendBuffer();

    WiFiClientSecure client;
    client.setInsecure(); 
    httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

    t_httpUpdate_return ret = httpUpdate.update(client, url);

    instance->_display.clearBuffer();
    switch (ret) {
        case HTTP_UPDATE_FAILED:
            instance->_display.drawStr(0, 30, "Update Failed!");
            Serial.printf("Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
            break;
        case HTTP_UPDATE_NO_UPDATES:
            instance->_display.drawStr(0, 30, "No Update?");
            break;
        case HTTP_UPDATE_OK:
            instance->_display.drawStr(0, 30, "Success!"); 
            break;
    }
    instance->_display.sendBuffer();
    delay(3000);
    instance->_menu.drawMenu();
}