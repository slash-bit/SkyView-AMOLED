#include "Platform_ESP32.h"
#include "SoCHelper.h"
#include "EEPROMHelper.h"
#include "BluetoothHelper.h"
#include "NMEAHelper.h"
#include "GDL90Helper.h"

#include "SkyView.h"

#include "WiFiHelper.h"   // HOSTNAME

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

String BT_name = HOSTNAME;

Bluetooth_ctl_t ESP32_BT_ctl = {
  .mutex   = portMUX_INITIALIZER_UNLOCKED,
  .command = BT_CMD_NONE,
  .status  = BT_STATUS_NC
};

/* LE */
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* AppDevice;
static BLEClient* pClient;

static BLEUUID  serviceUUID(SERVICE_UUID);
static BLEUUID  charUUID(CHARACTERISTIC_UUID);

cbuf *BLE_FIFO_RX, *BLE_FIFO_TX;

static unsigned long BT_TimeMarker = 0;
static unsigned long BLE_Notify_TimeMarker = 0;

class AppClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    ESP32_BT_ctl.status = BT_STATUS_NC;
    Serial.println(F("BLE: disconnected from Server."));
  }
};

class AppAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      if (AppDevice) {
        AppDevice->~BLEAdvertisedDevice();
      }
      AppDevice = new BLEAdvertisedDevice(advertisedDevice);
      ESP32_BT_ctl.command = BT_CMD_CONNECT;
    }
  }
};

static void AppNotifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    if (length > 0) {
      BLE_FIFO_RX->write((char *) pData, (BLE_FIFO_RX->room() > length ?
                                          length : BLE_FIFO_RX->room()));
    }
}

void setup() {
  Serial.begin(115200);
  BLEDevice::init(BT_name);
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new AppAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30, false);
}

void loop() {
  if (ESP32_BT_ctl.command == BT_CMD_CONNECT && AppDevice != nullptr) {
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new AppClientCallback());
    pClient->connect(AppDevice);
    pRemoteCharacteristic = pClient->getService(serviceUUID)->getCharacteristic(charUUID);
    if (pRemoteCharacteristic->canNotify()) {
      pRemoteCharacteristic->registerForNotify(AppNotifyCallback);
    }
    ESP32_BT_ctl.status = BT_STATUS_CONNECTED;
    ESP32_BT_ctl.command = BT_CMD_NONE;
  }
}

static void ESP32_BT_BLE_Connection_Manager(void *parameter) {
  int command;
  int status;

  while (true) {

    portENTER_CRITICAL(&ESP32_BT_ctl.mutex);
    command = ESP32_BT_ctl.command;
    portEXIT_CRITICAL(&ESP32_BT_ctl.mutex);

    switch (command) {
      case BT_CMD_CONNECT:
        portENTER_CRITICAL(&ESP32_BT_ctl.mutex);
        status = ESP32_BT_ctl.status;
        portEXIT_CRITICAL(&ESP32_BT_ctl.mutex);

        if (status == BT_STATUS_CON) {
          portENTER_CRITICAL(&ESP32_BT_ctl.mutex);
          ESP32_BT_ctl.command = BT_CMD_NONE;
          portEXIT_CRITICAL(&ESP32_BT_ctl.mutex);
          break;
        }

        if (AppDevice != nullptr) {
          pClient = BLEDevice::createClient();
          pClient->setClientCallbacks(new AppClientCallback());
          if (pClient->connect(AppDevice)) {
            pRemoteCharacteristic = pClient->getService(serviceUUID)->getCharacteristic(charUUID);
            if (pRemoteCharacteristic->canNotify()) {
              pRemoteCharacteristic->registerForNotify(AppNotifyCallback);
            }
            portENTER_CRITICAL(&ESP32_BT_ctl.mutex);
            ESP32_BT_ctl.status = BT_STATUS_CON;
            ESP32_BT_ctl.command = BT_CMD_NONE;
            portEXIT_CRITICAL(&ESP32_BT_ctl.mutex);

            Serial.println(F("BLE: Connected to device."));
          } else {
            portENTER_CRITICAL(&ESP32_BT_ctl.mutex);
            ESP32_BT_ctl.status = BT_STATUS_NC;
            portEXIT_CRITICAL(&ESP32_BT_ctl.mutex);

            Serial.println(F("BLE: Unable to connect to device."));
          }
        } else {
          Serial.println(F("BLE: No device to connect."));
        }
        break;

      case BT_CMD_DISCONNECT:
        portENTER_CRITICAL(&ESP32_BT_ctl.mutex);
        status = ESP32_BT_ctl.status;
        portEXIT_CRITICAL(&ESP32_BT_ctl.mutex);

        if (status != BT_STATUS_CON) {
          portENTER_CRITICAL(&ESP32_BT_ctl.mutex);
          ESP32_BT_ctl.command = BT_CMD_NONE;
          portEXIT_CRITICAL(&ESP32_BT_ctl.mutex);
          break;
        }

        if (pClient && pClient->isConnected()) {
          pClient->disconnect();
          Serial.println(F("BLE: Disconnected from device."));

          portENTER_CRITICAL(&ESP32_BT_ctl.mutex);
          ESP32_BT_ctl.status = BT_STATUS_NC;
          ESP32_BT_ctl.command = BT_CMD_NONE;
          portEXIT_CRITICAL(&ESP32_BT_ctl.mutex);
        } else {
          Serial.println(F("BLE: No device to disconnect."));
        }
        break;

      case BT_CMD_SHUTDOWN:
        if (pClient && pClient->isConnected()) {
          pClient->disconnect();
        }
        vTaskDelete(NULL);
        break;

      default:
        break;
    }

    delay(1000);
  }
}

static bool ESP32_BLEConnectToServer() {
  if (!pClient->connect(AppDevice)) {
    Serial.println(F("BLE: Failed to connect to device."));
    return false;
  }

  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print(F("BLE: Failed to find our service UUID: "));
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print(F("BLE: Failed to find our characteristic UUID: "));
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(AppNotifyCallback);
  }

  ESP32_BT_ctl.status = BT_STATUS_CON;
  return true;
}

static void ESP32_Bluetooth_setup() {
  switch (settings->connection) {
    case CON_BLUETOOTH_SPP: {
      // Bluetooth Classic setup (if needed, otherwise can be removed for BLE focus)
      esp_bt_controller_mem_release(ESP_BT_MODE_BLE);

      BT_name += String(SoC->getChipId() & 0x00FFFFFFU, HEX);

      SerialBT.setPin(settings->key);
      SerialBT.begin(BT_name.c_str(), true);

      xTaskCreate(ESP32_BT_SPP_Connection_Manager, "BT SPP ConMgr Task", 1024, NULL, tskIDLE_PRIORITY, NULL);

      BT_TimeMarker = millis();
    }
    break;

    case CON_BLUETOOTH_LE: {
      BLE_FIFO_RX = new cbuf(BLE_FIFO_RX_SIZE);
      BLE_FIFO_TX = new cbuf(BLE_FIFO_TX_SIZE);

      esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

      BLEDevice::init("");

      pClient = BLEDevice::createClient();
      pClient->setClientCallbacks(new AppClientCallback());

      BLEScan* pBLEScan = BLEDevice::getScan();
      pBLEScan->setAdvertisedDeviceCallbacks(new AppAdvertisedDeviceCallbacks());
      pBLEScan->setInterval(1349);
      pBLEScan->setWindow(449);
      pBLEScan->setActiveScan(true);
      pBLEScan->start(3, false);

      BLE_Notify_TimeMarker = millis();
    }
    break;

    default:
      break;
  }
}

static void ESP32_Bluetooth_loop() {

  bool hasData = false;

  switch(settings->connection) {
    case CON_BLUETOOTH_SPP: {
      // Bluetooth Classic (SPP) logic - can be removed if focusing on BLE
      portENTER_CRITICAL(&ESP32_BT_ctl.mutex);
      int command = ESP32_BT_ctl.command;
      int status = ESP32_BT_ctl.status;
      portEXIT_CRITICAL(&ESP32_BT_ctl.mutex);

      if (status == BT_STATUS_NC && command == BT_CMD_NONE) {
        portENTER_CRITICAL(&ESP32_BT_ctl.mutex);
        ESP32_BT_ctl.command = BT_CMD_CONNECT;
        portEXIT_CRITICAL(&ESP32_BT_ctl.mutex);
      } else {
        switch (settings->protocol) {
          case PROTOCOL_GDL90:
            hasData = GDL90_isConnected();
            break;
          case PROTOCOL_NMEA:
          default:
            hasData = NMEA_isConnected();
            break;
        }

        if (hasData) {
          BT_TimeMarker = millis();
        } else if (millis() - BT_TimeMarker > BT_NODATA_TIMEOUT && command == BT_CMD_NONE) {
          portENTER_CRITICAL(&ESP32_BT_ctl.mutex);
          ESP32_BT_ctl.command = BT_CMD_DISCONNECT;
          portEXIT_CRITICAL(&ESP32_BT_ctl.mutex);

          BT_TimeMarker = millis();
        }
      }
    }
    break;

    case CON_BLUETOOTH_LE: {
      if (ESP32_BT_ctl.command == BT_CMD_CONNECT) {
        if (ESP32_BLEConnectToServer()) {
          Serial.println(F("BLE: connected to Server."));
        }
        ESP32_BT_ctl.command = BT_CMD_NONE;
      }

      switch (settings->protocol) {
        case PROTOCOL_GDL90:
          hasData = GDL90_isConnected();
          break;
        case PROTOCOL_NMEA:
        default:
          hasData = NMEA_isConnected();
          break;
      }

      if (hasData) {
        BT_TimeMarker = millis();
      } else if (millis() - BT_TimeMarker > BT_NODATA_TIMEOUT) {
        Serial.println(F("BLE: attempt to (re)connect..."));

        if (pClient && pClient->isConnected()) {
          pClient->disconnect();
        }

        BLEDevice::getScan()->start(3, false);

        BT_TimeMarker = millis();
      }

      // Notify changed value
      if (ESP32_BT_ctl.status == BT_STATUS_CON && (millis() - BLE_Notify_TimeMarker > 10)) {
        uint8_t chunk[BLE_MAX_WRITE_CHUNK_SIZE];
        size_t size = (BLE_FIFO_TX->available() < BLE_MAX_WRITE_CHUNK_SIZE ? BLE_FIFO_TX->available() : BLE_MAX_WRITE_CHUNK_SIZE);

        if (size > 0) {
          BLE_FIFO_TX->read((char *) chunk, size);

