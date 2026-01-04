#ifndef MODE_CONFIG_H
#define MODE_CONFIG_H

#include <U8g2lib.h>
#include <GEM_u8g2.h>
#include "AppMode.h"
#include "Encoder.h"
#include "ChordInput.h"
#include "MaxFanConstants.h"
#include "MaxFanConfig.h"
#include <vector> 
#include <esp_heap_caps.h> // Für Heap Checks

struct ReleaseEntry {
    String tagName;
    String downloadUrl;
    GEMItem* menuPtr; 
};

class ModeConfig : public AppMode {
public:
    ModeConfig(U8G2* display, Encoder* encoder, ChordInput* input);

    void enter() override;
    ModeAction loop() override;

private:
    static ModeConfig* instance;

    bool _mustExit;
    GEM_u8g2 _menu;
    ConfigData _editConfig; 

    // --- SEITEN (PAGES) ---
    GEMPage _pageMain;          
    GEMPage _pageRemote;        
    GEMPage _pageWifi;          
    GEMPage _pageBle;           
    GEMPage _pageMqtt;         
    GEMPage _pageDisplay;       
    GEMPage _pageVersionInfo;   
    GEMPage _pageExit;          

    // WICHTIG: Das hier ist jetzt ein Pointer (*), damit wir die Seite neu bauen können!
    GEMPage* _pageVersionsSelect; 

    // --- NAVIGATION ITEMS ---
    GEMItem _itemNavRemote;
    GEMItem _itemNavWifi;
    GEMItem _itemNavMqtt;
    GEMItem _itemNavBle;
    GEMItem _itemNavDisplay;
    GEMItem _itemNavVersion;
    GEMItem _itemNavExit;       

    // --- BACK BUTTONS ---
    GEMItem _itemBackRemote;
    GEMItem _itemBackWifi;
    GEMItem _itemBackMqtt;
    GEMItem _itemBackBle;
    GEMItem _itemBackDisplay;
    GEMItem _itemBackVersion;
    GEMItem _itemBackFromExit;  

    // --- PAGE ITEMS ---
    GEMItem _itemConnection;    
    GEMItem _itemSsid;
    GEMItem _itemPassword;
    GEMItem _itemMqttHost;
    GEMItem _itemMqttPort;
    GEMItem _itemMqttClientId;
    GEMItem _itemMqttUser;
    GEMItem _itemMqttPassword;
    GEMItem _itemMqttCommandTopic;
    GEMItem _itemMqttStateTopic;
    GEMItem _itemTestWifi;      
    GEMItem _itemTestMqtt;
    GEMItem _itemBlePin;
    GEMItem _itemGenerateNewPIN;
    GEMItem _itemDisplayTimeoutSeconds;
    GEMItem _itemCurrentVersion; 
    GEMItem _itemCheckUpdates;   
    GEMItem _itemSave;
    GEMItem _itemDiscard;

    // --- INTERNE LOGIK ---
    char versionLabel[32]; 
    std::vector<ReleaseEntry> _releaseList; 

    void clearDynamicItems();
    bool connectToWiFi(bool useEditConfig); 
    void fetchReleasesFromGitHub();
    void showError(const char* line1, const char* line2 = ""); // Error Helper
    static void performOTA(const char* url);

    // --- CALLBACKS ---
    static void callbackCheckExit(); 
    static void callbackSaveAndRestart();
    static void callbackDiscardAndRestart();
    static void callbackGoBackToMain();     
    static void callbackGoBackToVersion();  
    static void callbackGenerateNewPIN();
    static void callbackTestWifi(); 
    static void callbackTestMqtt();
    
    static void callbackCheckForUpdates();
    static void callbackInstallUpdate(GEMCallbackData data);
};

#endif