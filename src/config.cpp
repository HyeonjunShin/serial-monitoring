#include "config.hpp"
#include <fstream>
#include <iostream>

Config load_config(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open config file: " + filename);
  }

  nlohmann::json data = nlohmann::json::parse(file);
  file.close();

  return data.get<Config>();
}
