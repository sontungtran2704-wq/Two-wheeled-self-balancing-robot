#include "Net.h"
#include <ESPmDNS.h>
#include <Preferences.h>
#if defined(ARDUINO_ARCH_ESP32)
#include <esp_wifi.h>
#endif
#include "Log.h"
#include "Commands.h"

Net net;

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_system.h>
// Sự kiện WiFi chạy ở task khác: chỉ ghi cờ, việc in ra làm trong Net::poll()
static volatile int16_t evDiscReason = -1;   // lý do STA mất kết nối (WIFI_REASON_*)
static volatile uint8_t evApJoin = 0, evApLeave = 0;
static void onWifiEvent(arduino_event_id_t ev, arduino_event_info_t info) {
  switch (ev) {
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:   evDiscReason = (int16_t)info.wifi_sta_disconnected.reason; break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:    evApJoin = 1;  break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: evApLeave = 1; break;
    default: break;
  }
}
int bbResetReason() { return (int)esp_reset_reason(); }
#include <lwip/sockets.h>
// Gửi KHÔNG chặn. Không dùng WiFiClient::availableForWrite(): core 3.x không cài đặt hàm này
// (dùng Print::availableForWrite luôn trả 0) nên trước đây xe không bao giờ gửi được byte nào.
static bool sendNoBlock(WiFiClient& c, const uint8_t* d, size_t n) {
  int fd = c.fd();
  if (fd < 0) return false;
  size_t off = 0;
  while (off < n) {
    int w = send(fd, d + off, n - off, MSG_DONTWAIT);
    if (w <= 0) break;                     // EAGAIN: bộ đệm đầy -> bỏ phần còn lại
    off += (size_t)w;
  }
  return off == n;
}
#else
static bool sendNoBlock(WiFiClient& c, const uint8_t* d, size_t n) { return c.write(d, n) == n; }
int bbResetReason() { return 0; }
#endif

Net::Net() : server(TCP_PORT), fromNvs(false), lastAccept(0), lastStaCheck(0), staFail(-1), wrFail(0) {
  strcpy(mode, "OFF");
  strncpy(ssid, WIFI_SSID_DEF, sizeof ssid - 1);  ssid[sizeof ssid - 1] = '\0';
  strncpy(pass, WIFI_PASS_DEF, sizeof pass - 1);  pass[sizeof pass - 1] = '\0';
}

void Net::loadCreds() {
  char ss[33] = "", pw[65] = "";
  Preferences p;
  p.begin("bb", true);
  size_t a = p.getString("ssid", ss, sizeof ss);
  size_t b = p.getString("pass", pw, sizeof pw);
  p.end();
  if (a > 0 && ss[0]) {
    strcpy(ssid, ss);
    strcpy(pass, b > 0 ? pw : "");
    fromNvs = true;
  }
}

void Net::saveCreds() {
  Preferences p;
  p.begin("bb", false);
  p.putString("ssid", ssid); p.putString("pass", pass);
  p.end();
  fromNvs = true;
}

bool Net::setCreds(const char* newSsid, size_t ls, const char* newPass) {
  if (ls == 0 || ls > 32 || strlen(newPass) > 64 || (strlen(newPass) > 0 && strlen(newPass) < 8))
    return false;
  memcpy(ssid, newSsid, ls); ssid[ls] = '\0';
  strcpy(pass, newPass);
  saveCreds();
  return true;
}

void Net::print() const {
#if WIFI_ENABLE
  bool ap = (strcmp(mode, "AP") == 0);
  IPAddress ip = ap ? WiFi.softAPIP() : WiFi.localIP();
  outf("WIFI mode=%s ip=%s host=%s.local port=%d rssi=%d ssid=%s\n", mode, ip.toString().c_str(),
       HOSTNAME, TCP_PORT, (strcmp(mode, "STA") == 0) ? (int)WiFi.RSSI() : 0,
       ap ? AP_SSID : ssid);
  if (ap && staFail >= 0) outf("WIFI_STA_FAIL status=%d (1=khong thay SSID/5GHz, 4=sai mat khau, 6=bi tu choi) ssid_da_luu=%s\n", staFail, ssid);
#else
  outln("WIFI disabled");
#endif
}

void Net::begin() {
#if WIFI_ENABLE
  WiFi.persistent(false);
#if defined(ARDUINO_ARCH_ESP32)
  WiFi.onEvent(onWifiEvent);
#endif
  WiFi.setHostname(HOSTNAME);
  bool sta = false;
  if (ssid[0]) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && (uint32_t)(millis() - t0) < WIFI_CONNECT_MS) delay(100);
    sta = (WiFi.status() == WL_CONNECTED);
    staFail = sta ? -1 : (int)WiFi.status();
  }
  if (sta) {
    strcpy(mode, "STA");
    WiFi.setAutoReconnect(true);
  } else {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS, 1, 0, 4);
    strcpy(mode, "AP");
  }
  WiFi.setSleep(false);                                   // giảm trễ gói tin
#if defined(ARDUINO_ARCH_ESP32)
  WiFi.setTxPower(WIFI_TX_POWER);                         // xem Config.h
  esp_wifi_set_ps(WIFI_PS_NONE);                          // tắt modem-sleep để ping ổn định <2ms
#endif
  if (MDNS.begin(HOSTNAME)) MDNS.addService("balancebot", "tcp", TCP_PORT);
  server.begin();
  server.setNoDelay(true);
#endif
  print();
}

// nhận client TCP mới (thay client cũ, kể cả khi client cũ đã chết nửa chừng) và đọc lệnh từ WiFi
void Net::poll() {
#if WIFI_ENABLE
  uint32_t now = millis();
#if defined(ARDUINO_ARCH_ESP32)
  if (evDiscReason >= 0) { int r = evDiscReason; evDiscReason = -1; outf("EVT:WIFI_STA_DISC,reason=%d\n", r); }
  if (evApJoin)  { evApJoin = 0;  outln("EVT:AP_JOIN"); }
  if (evApLeave) { evApLeave = 0; outln("EVT:AP_LEAVE"); }
  if (strcmp(mode, "STA") == 0) {                          // mất WiFi nhà: chủ động nối lại, không chờ stack tự làm
    if (WiFi.status() == WL_CONNECTED) lastStaCheck = now;
    else if ((uint32_t)(now - lastStaCheck) >= STA_RECONNECT_MS) { lastStaCheck = now; WiFi.reconnect(); }
  }
#endif
  if ((uint32_t)(now - lastAccept) >= 100) {
    lastAccept = now;
    WiFiClient c = server.available();
    if (c) {
      if (client) client.stop();
      client = c;
      client.setNoDelay(true);
      rx.reset();
      wrFail = 0;
      outln("EVT:WIFI_CLIENT");
    }
  }
  if (client && !client.connected()) {
    client.stop();                                        // dọn client đã đứt kết nối
  }
  while (client && client.connected() && client.available()) rx.feed((char)client.read(), handleCommand);
#endif
}

void Net::writeClient(const uint8_t* data, size_t n) {
#if WIFI_ENABLE
  if (client && client.connected()) {
    bool ok = sendNoBlock(client, data, n);
    // Bộ đệm gửi đầy: bỏ dòng này thay vì chặn vòng điều khiển.
    // Kẹt liên tục ~10 s thì coi client đã chết, bỏ để client mới (hoặc lần nối lại) vào được.
    if (ok) wrFail = 0;
    else if (++wrFail >= TCP_DROP_AFTER_FAILS) { client.stop(); wrFail = 0; }
  }
#endif
}
