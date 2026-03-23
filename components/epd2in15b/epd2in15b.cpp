#include "epd2in15b.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

namespace esphome {
namespace epd2in15b {

static const char *const TAG = "epd2in15b";

void EPD2in15B::send_command_(uint8_t cmd) {
  this->dc_pin_->digital_write(false);
  this->enable();
  this->write_byte(cmd);
  this->disable();
}

void EPD2in15B::send_data_(uint8_t data) {
  this->dc_pin_->digital_write(true);
  this->enable();
  this->write_byte(data);
  this->disable();
}

void EPD2in15B::set_window_(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end) {
  this->send_command_(0x44);
  this->send_data_((x_start >> 3) & 0x1F);
  this->send_data_((x_end >> 3) & 0x1F);
  this->send_command_(0x45);
  this->send_data_(y_start & 0xFF);
  this->send_data_((y_start >> 8) & 0x01);
  this->send_data_(y_end & 0xFF);
  this->send_data_((y_end >> 8) & 0x01);
}

void EPD2in15B::set_cursor_(uint16_t x, uint16_t y) {
  this->send_command_(0x4E);
  this->send_data_(x & 0x1F);
  this->send_command_(0x4F);
  this->send_data_(y & 0xFF);
  this->send_data_((y >> 8) & 0x01);
}

bool EPD2in15B::is_busy_() {
  if (this->busy_pin_ == nullptr) return false;
  return this->busy_pin_->digital_read();
}

void EPD2in15B::transition_(EPDState next, uint32_t delay_ms) {
  this->state_ = next;
  this->state_deadline_ms_ = millis() + delay_ms;
}

void EPD2in15B::do_reset_() {
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->digital_write(true);
    delay(10);
    this->reset_pin_->digital_write(false);
    delay(2);
    this->reset_pin_->digital_write(true);
  }
}

void EPD2in15B::setup() {
  this->init_internal_(EPD_BLACK_BUFFER_SIZE + EPD_RED_BUFFER_SIZE);
  if (this->buffer_ == nullptr) {
    ESP_LOGE(TAG, "Failed to allocate display buffer!");
    this->mark_failed();
    return;
  }
  this->black_buffer_ = this->buffer_;
  this->red_buffer_   = this->buffer_ + EPD_BLACK_BUFFER_SIZE;
  memset(this->black_buffer_, 0xFF, EPD_BLACK_BUFFER_SIZE);
  memset(this->red_buffer_,   0x00, EPD_RED_BUFFER_SIZE);

  this->dc_pin_->setup();
  this->dc_pin_->digital_write(false);

  if (this->pwr_pin_ != nullptr) {
    this->pwr_pin_->setup();
    this->pwr_pin_->digital_write(true);
  }
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(true);
  }
  if (this->busy_pin_ != nullptr) {
    this->busy_pin_->setup();
  }

  this->spi_setup();
  this->do_reset_();
  this->transition_(EPDState::INIT_WAIT_BUSY, 200);
}

void EPD2in15B::loop() {
  if (millis() < this->state_deadline_ms_) return;

  switch (this->state_) {

    case EPDState::INIT_WAIT_BUSY:
      if (this->is_busy_()) return;
      this->send_command_(0x12);  // SWRESET
      this->transition_(EPDState::INIT_SWRESET_WAIT, 10);
      break;

    case EPDState::INIT_SWRESET_WAIT:
      if (this->is_busy_()) return;
      this->send_command_(0x11);
      this->send_data_(0x03);
      this->set_window_(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1);
      this->send_command_(0x3C);
      this->send_data_(0x05);
      this->send_command_(0x18);
      this->send_data_(0x80);
      this->send_command_(0x21);
      this->send_data_(0x00);
      this->send_data_(0x80);
      this->set_cursor_(0, 0);
      if (this->update_pending_) {
        this->update_pending_ = false;
        this->transition_(EPDState::UPDATE_SEND_DATA, 0);
      } else {
        this->transition_(EPDState::IDLE, 0);
      }
      break;

    case EPDState::UPDATE_SEND_DATA:
      this->set_cursor_(0, 0);
      this->send_command_(0x24);
      this->dc_pin_->digital_write(true);
      this->enable();
      for (uint32_t i = 0; i < EPD_BLACK_BUFFER_SIZE; i++)
        this->write_byte(this->black_buffer_[i]);
      this->disable();
      this->send_command_(0x26);
      this->dc_pin_->digital_write(true);
      this->enable();
      for (uint32_t i = 0; i < EPD_RED_BUFFER_SIZE; i++)
        this->write_byte(this->red_buffer_[i]);
      this->disable();
      this->transition_(EPDState::UPDATE_ACTIVATE, 0);
      break;

    case EPDState::UPDATE_ACTIVATE:
      this->send_command_(0x20);
      this->transition_(EPDState::UPDATE_WAIT_BUSY, 50);
      break;

    case EPDState::UPDATE_WAIT_BUSY:
      if (this->is_busy_()) return;
      this->transition_(EPDState::IDLE, 0);
      break;

    case EPDState::IDLE:
    default:
      break;
  }
}

void EPD2in15B::update() {
  memset(this->black_buffer_, 0xFF, EPD_BLACK_BUFFER_SIZE);
  memset(this->red_buffer_,   0x00, EPD_RED_BUFFER_SIZE);
  this->do_update_();

  if (this->state_ != EPDState::IDLE) {
    this->update_pending_ = true;
    return;
  }
  this->transition_(EPDState::UPDATE_SEND_DATA, 0);
}

void EPD2in15B::dump_config() {
  LOG_DISPLAY("", "Waveshare 2.15\" B ePaper", this);
  ESP_LOGCONFIG(TAG, "  Width: %d, Height: %d", EPD_WIDTH, EPD_HEIGHT);
  LOG_PIN("  DC Pin: ", this->dc_pin_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  LOG_PIN("  Busy Pin: ", this->busy_pin_);
  LOG_PIN("  PWR Pin: ", this->pwr_pin_);
}

void EPD2in15B::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (this->black_buffer_ == nullptr || this->red_buffer_ == nullptr) return;
  if (x < 0 || x >= EPD_WIDTH || y < 0 || y >= EPD_HEIGHT) return;

  uint32_t byte_idx = (x / 8) + y * (EPD_WIDTH / 8);
  uint8_t  bit_mask = 0x80 >> (x % 8);

  // if (color.r > 200 && color.g < 100 && color.b < 100) {
  //   this->black_buffer_[byte_idx] |= bit_mask;
  //   this->red_buffer_[byte_idx]   |= bit_mask;
  // } else if (color.r < 50 && color.g < 50 && color.b < 50) {
  //   this->black_buffer_[byte_idx] &= ~bit_mask;
  //   this->red_buffer_[byte_idx]   &= ~bit_mask;
  // } else {
  //   this->black_buffer_[byte_idx] |= bit_mask;
  //   this->red_buffer_[byte_idx]   &= ~bit_mask;
  // }
  if (color.r > 200 && color.g < 100 && color.b < 100) {
    this->black_buffer_[byte_idx] |= bit_mask;
    this->red_buffer_[byte_idx]   &= ~bit_mask;
  } else if (color.r < 50 && color.g < 50 && color.b < 50) {
    this->black_buffer_[byte_idx] |= bit_mask;
    this->red_buffer_[byte_idx]   &= ~bit_mask;
  } else {
    this->black_buffer_[byte_idx] &= ~bit_mask;
    this->red_buffer_[byte_idx]   &= ~bit_mask;
  }
}

}  // namespace epd2in15b
}  // namespace esphome
