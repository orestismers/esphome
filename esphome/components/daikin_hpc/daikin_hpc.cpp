#include "daikin_hpc.h"

#include "esphome/core/log.h"

namespace esphome {
namespace daikin_hpc {

static const char *const TAG = "daikin_hpc";

static constexpr uint8_t MODBUS_CMD_READ_REGISTER = 3;
static constexpr uint8_t MODBUS_CMD_WRITE_REGISTER = 6;

uint16_t DaikinHpcClimate::dataToUint16(const std::vector<uint8_t> &data) {
  if (data.size() != 2) {
    ESP_LOGW(TAG, "Tried to convert invalid data to unt16");
    return 0;
  }

  return (static_cast<uint16_t>(data[0]) << 8) | data[1];
}

inline float DaikinHpcClimate::dataToTemperature(const std::vector<uint8_t> &data) { return dataToUint16(data) * 0.1; }

void DaikinHpcClimate::setup() {
  waterTemperature_->set_icon("mdi:thermometer");
  waterTemperature_->set_unit_of_measurement("°C");
  waterTemperature_->set_accuracy_decimals(1);
  waterTemperature_->set_name("Water Temperature");
  waterTemperature_->set_entity_category(EntityCategory::ENTITY_CATEGORY_DIAGNOSTIC);

  airTemperature_->set_icon("mdi:thermometer");
  airTemperature_->set_unit_of_measurement("°C");
  airTemperature_->set_accuracy_decimals(1);
  airTemperature_->set_name("Air Temperature");
  airTemperature_->set_entity_category(EntityCategory::ENTITY_CATEGORY_NONE);
  airTemperature_->set_internal(false);

  motorSpeed_->set_icon("mdi:fan");
  motorSpeed_->set_unit_of_measurement("rpm");
  motorSpeed_->set_accuracy_decimals(0);
  motorSpeed_->set_name("Fan Speed");
  motorSpeed_->set_entity_category(EntityCategory::ENTITY_CATEGORY_DIAGNOSTIC);
}

void DaikinHpcClimate::on_modbus_data(const std::vector<uint8_t> &data) {
  switch (modbusSendQueue.front()) {
    case Register::WaterTemperature:
      waterTemperature_->publish_state(dataToTemperature(data));
      break;

    case Register::AirTemperature:
      airTemperature_->publish_state(dataToTemperature(data));
      break;

    case Register::MotorSpeed:
    case Register::Config:
    case Register::AbsoluteSetPoint:
    default:
      break;
  }

  modbusSendQueue.pop();
  readNextRegister();
}

void DaikinHpcClimate::update() {
  while (!modbusSendQueue.empty()) {
    modbusSendQueue.pop();
  }

  modbusSendQueue.push(Register::WaterTemperature);
  modbusSendQueue.push(Register::AirTemperature);
  modbusSendQueue.push(Register::MotorSpeed);

  readNextRegister();
}

void DaikinHpcClimate::readNextRegister() {
  if (modbusSendQueue.empty()) {
    return;
  }

  this->send(MODBUS_CMD_READ_REGISTER, static_cast<uint16_t>(modbusSendQueue.front()), 1);
}

void DaikinHpcClimate::dump_config() {
  ESP_LOGCONFIG(TAG, "DaikinHpcClimate:");
  ESP_LOGCONFIG(TAG, "  Address: 0x%02X", this->address_);
}

}  // namespace daikin_hpc
}  // namespace esphome
