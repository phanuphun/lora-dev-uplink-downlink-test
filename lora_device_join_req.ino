#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#define CFG_as923 1  //  923 Hz
#define CFG_sx1276_radio 1

// ===== (1) OTAA KEYS =====
// AppEUI & DevEUI -> LSB-first  |  AppKey -> MSB-first
// AppEUI & DevEUI -> LSB-first   |   AppKey -> MSB-first
static const u1_t PROGMEM APPEUI[8] = { 0x01, 0x00, 0x57, 0x4C, 0x4D, 0x43, 0x41, 0x48 };  // LSB of 4841434D4C570001
static const u1_t PROGMEM DEVEUI[8] = { 0x45, 0x01, 0x00, 0x03, 0x24, 0x81, 0x54, 0x82 };  // LSB of 8254812403000145
static const u1_t PROGMEM APPKEY[16] = { 0x48, 0x41, 0x43, 0x2D, 0x4D, 0x4C, 0x57, 0x20,
                                         0x20, 0x03, 0x18, 0x00, 0x00, 0x02, 0x54, 0x81 };  // MSB of 4841432d4d4c57202003180000025481
void os_getArtEui(u1_t* buf) {
  memcpy_P(buf, APPEUI, 8);
}
void os_getDevEui(u1_t* buf) {
  memcpy_P(buf, DEVEUI, 8);
}
void os_getDevKey(u1_t* buf) {
  memcpy_P(buf, APPKEY, 16);
}

// ===== (2) PIN MAP (TTGO LoRa32 T3_V1.6) =====
const lmic_pinmap lmic_pins = {
  .nss = 18,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 14,
  .dio = { 26, 33, 32 }
};

static osjob_t sendjob;
const unsigned TX_INTERVAL = 10;  // ส่งทุก 60 วินาที

void do_send(osjob_t* j) {
  if (LMIC.opmode & OP_TXRXPEND) return;
  // ตัวอย่างเพย์โหลด 2 ไบต์ (จะเห็นเป็น base64 ฝั่ง ChirpStack)
  uint8_t payload[2] = { 0x01, 0x02 };
  LMIC_setTxData2(/*FPort*/ 2, payload, sizeof(payload), /*confirmed*/ 0);
}

void onEvent(ev_t ev) {
  switch (ev) {
    case EV_JOINING: Serial.println(F("EV_JOINING")); break;
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));
      LMIC_setLinkCheckMode(0);  // ลดทราฟฟิก
      do_send(&sendjob);         // ส่งทันทีหนึ่งครั้ง
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE"));
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;
    case EV_JOIN_FAILED: Serial.println(F("EV_JOIN_FAILED")); break;
    default: break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  SPI.begin(/*SCK*/ 5, /*MISO*/ 19, /*MOSI*/ 27, /*SS*/ 18);
  os_init();
  LMIC_reset();
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);  // ชดเชย clock ESP32 ~1%
  Serial.println(F("Start OTAA join..."));
  LMIC_startJoining();  // LMIC จะ retry เองจนกว่าจะสำเร็จ
}

void loop() {
  os_runloop_once();
}