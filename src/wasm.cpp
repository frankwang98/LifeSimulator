#include "json.h"

// The returned buffer remains valid until the next call. JS copies it immediately via ccall.
extern "C" const char* simulate_world(const char* configuration) {
  static std::string result;
  try {
    if (!configuration) {
      throw std::invalid_argument("configuration is null");
    }
    result = life::simulationJson(life::parseConfig(configuration));
  } catch (const std::exception& error) {
    result = life::errorJson(error.what());
  }
  return result.c_str();
}
