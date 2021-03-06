#include "main.h"

RTC_DATA_ATTR int recordCounter = 0;
RTC_DATA_ATTR bme280record records[MAX_RTC_RECORDS];

void setup() {
  WiFi.mode(WIFI_MODE_NULL);

  Serial.begin(115200);
  #ifndef DEBUG
  Serial.setDebugOutput(0);
  #endif
  
  while (!Serial);

  #ifndef PRECONFIGURED
    setupEEPROM();

    // uncomment when you want to programmatically clear config
    // clearConfig();

    setupButton();

    if (!isConfigSaved()) {
      if (!setupAP()) {
        goToSleep();
      }

      listenForConfig();
      cleanupAP();
    }

    // do not do anything if button is pressed
    if (digitalRead(BTN_PIN) == BTN_PRESSED_STATE) {
      ardprintf("Button pressed upon startup, skipping WiFi setup");
      return;
    }
  #endif

  if (!setupbme280()) {
    goToSleep();
  };

  ardprintf("Measurement %d starting", recordCounter);

  // make a sensor reading
  if (!makeMeasurement(&records[recordCounter])) {
    ardprintf("Failed to perform reading :(");
    goToSleep();
    return;
  }

  ardprintf("Measurement %d done", recordCounter);
  recordCounter++;

  if (recordCounter < MAX_RTC_RECORDS) {
    ardprintf("Going to sleep, next up is: %d", recordCounter);
    goToSleep();
    return;
  }
  recordCounter = 0;

  if (!setupWiFi()) {
    goToSleep();
  }

  char jsonPayload[900];
  getJsonPayload(jsonPayload, records);

  char accessToken[60] = CFG_ACCESS_TOKEN;
  #ifndef PRECONFIGURED
  readFromEEPROM(accessToken, "access_token");
  #endif

  #ifndef ESP32S2
  makeSecureNetworkRequest("https://iotfreezer.com/api/measurements/multi", accessToken, jsonPayload, NULL, "POST", NULL);
  #else
  const char* ca_cert = \
  "-----BEGIN CERTIFICATE-----\n" \
  "MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/\n" \
  "MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n" \
  "DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow\n" \
  "PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD\n" \
  "Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n" \
  "AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O\n" \
  "rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq\n" \
  "OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b\n" \
  "xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw\n" \
  "7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD\n" \
  "aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV\n" \
  "HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG\n" \
  "SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69\n" \
  "ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr\n" \
  "AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz\n" \
  "R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5\n" \
  "JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo\n" \
  "Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ\n" \
  "-----END CERTIFICATE-----\n";
  makeSecureNetworkRequest("https://iotfreezer.com/api/measurements/multi", accessToken, jsonPayload, NULL, "POST", ca_cert);
  #endif
}

void loop() {
  /* do nothing in loop except check button, 
    esp will be in deep sleep in between measurements which will make setup re-run */

  #ifndef PRECONFIGURED
  if (checkButtonPressed()) {
      // do not execute anything else when button is pressed
    return;
  }
  #endif

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  goToSleep();
}