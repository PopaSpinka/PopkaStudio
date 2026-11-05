// Preferences.h — the board keeps settings in flash (NVS). In the browser they live in memory for as long as the page
// is open: same calls, same keys.
#pragma once
#include <stdint.h>
#include <string.h>
#include <map>
#include <string>
#include <vector>

class Preferences {
 public:
  bool begin(const char *name, bool read_only = false, const char * = nullptr) {
    ns_ = name ? name : "", ro_ = read_only;
    return true;
  }
  void end() {}
  bool clear() {
    for (auto it = store().begin(); it != store().end();) it = it->first.rfind(ns_ + "/", 0) == 0 ? store().erase(it) : ++it;
    return true;
  }
  bool remove(const char *key) { return store().erase(k(key)) > 0; }
  bool isKey(const char *key) { return store().count(k(key)) > 0; }

  size_t putBytes(const char *key, const void *v, size_t n) {
    if (ro_) return 0;
    auto &d = store()[k(key)];
    d.assign((const uint8_t *)v, (const uint8_t *)v + n);
    return n;
  }
  size_t getBytesLength(const char *key) {
    auto it = store().find(k(key));
    return it == store().end() ? 0 : it->second.size();
  }
  size_t getBytes(const char *key, void *buf, size_t max) {
    auto it = store().find(k(key));
    if (it == store().end()) return 0;
    const size_t n = it->second.size() < max ? it->second.size() : max;
    memcpy(buf, it->second.data(), n);
    return n;
  }

  size_t putUChar(const char *key, uint8_t v) { return put(key, v); }
  uint8_t getUChar(const char *key, uint8_t d = 0) { return get(key, d); }
  size_t putChar(const char *key, int8_t v) { return put(key, v); }
  int8_t getChar(const char *key, int8_t d = 0) { return get(key, d); }
  size_t putBool(const char *key, bool v) { return put<uint8_t>(key, v); }
  bool getBool(const char *key, bool d = false) { return get<uint8_t>(key, d) != 0; }
  size_t putShort(const char *key, int16_t v) { return put(key, v); }
  int16_t getShort(const char *key, int16_t d = 0) { return get(key, d); }
  size_t putUShort(const char *key, uint16_t v) { return put(key, v); }
  uint16_t getUShort(const char *key, uint16_t d = 0) { return get(key, d); }
  size_t putInt(const char *key, int32_t v) { return put(key, v); }
  int32_t getInt(const char *key, int32_t d = 0) { return get(key, d); }
  size_t putUInt(const char *key, uint32_t v) { return put(key, v); }
  uint32_t getUInt(const char *key, uint32_t d = 0) { return get(key, d); }
  size_t putFloat(const char *key, float v) { return put(key, v); }
  float getFloat(const char *key, float d = 0) { return get(key, d); }

 private:
  std::string ns_;
  bool ro_ = false;
  static std::map<std::string, std::vector<uint8_t>> &store() {
    static std::map<std::string, std::vector<uint8_t>> s;
    return s;
  }
  std::string k(const char *key) const { return ns_ + "/" + (key ? key : ""); }
  template <class T> size_t put(const char *key, T v) { return putBytes(key, &v, sizeof(v)); }
  template <class T> T get(const char *key, T d) {
    T v;
    return getBytes(key, &v, sizeof(v)) == sizeof(v) ? v : d;
  }
};
