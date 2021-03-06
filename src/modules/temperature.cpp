#include "modules/temperature.hpp"

waybar::modules::Temperature::Temperature(const std::string& id, const Json::Value& config)
  : ALabel(config, "{temperatureC}°C", 10)
{
  if (config_["hwmon-path"].isString()) {
    file_path_ = config_["hwmon-path"].asString();
  } else {
    auto zone =
      config_["thermal-zone"].isInt() ? config_["thermal-zone"].asInt() : 0;
    file_path_ = fmt::format("/sys/class/thermal/thermal_zone{}/temp", zone);
  }
#ifdef FILESYSTEM_EXPERIMENTAL
  if (!std::experimental::filesystem::exists(file_path_)) {
#else
  if (!std::filesystem::exists(file_path_)) {
#endif
    throw std::runtime_error("Can't open " + file_path_);
  }
  label_.set_name("temperature");
  if (!id.empty()) {
    label_.get_style_context()->add_class(id);
  }
  thread_ = [this] {
    dp.emit();
    thread_.sleep_for(interval_);
  };
}

auto waybar::modules::Temperature::update() -> void
{
  auto [temperature_c, temperature_f] = getTemperature();
  auto critical = isCritical(temperature_c);
  auto format = format_;
  if (critical) {
    format =
      config_["format-critical"].isString() ? config_["format-critical"].asString() : format;
    label_.get_style_context()->add_class("critical");
  } else {
    label_.get_style_context()->remove_class("critical");
  }
  label_.set_markup(fmt::format(format,
    fmt::arg("temperatureC", temperature_c), fmt::arg("temperatureF", temperature_f)));
}

std::tuple<uint16_t, uint16_t> waybar::modules::Temperature::getTemperature()
{
  std::ifstream temp(file_path_);
  if (!temp.is_open()) {
    throw std::runtime_error("Can't open " + file_path_);
  }
  std::string line;
  if (temp.good()) {
    getline(temp, line);
  }
  temp.close();
  auto temperature_c = std::strtol(line.c_str(), nullptr, 10) / 1000.0;
  auto temperature_f = temperature_c * 1.8 + 32;
  std::tuple<uint16_t, uint16_t> temperatures(std::round(temperature_c), std::round(temperature_f));
  return temperatures;
}

bool waybar::modules::Temperature::isCritical(uint16_t temperature_c)
{
  return config_["critical-threshold"].isInt() && temperature_c >= config_["critical-threshold"].asInt();
}