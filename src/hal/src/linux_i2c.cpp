#include "../include/hal/linux_i2c.h"

#if defined(__linux__)

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/i2c-dev.h>

#endif

namespace rtos {
namespace hal {

LinuxI2cBus::LinuxI2cBus() = default;

LinuxI2cBus::~LinuxI2cBus() {
  close();
}

bool LinuxI2cBus::open(const std::string& device) {
#if defined(__linux__)
  fd_ = ::open(device.c_str(), O_RDWR);
  return fd_ >= 0;
#else
  (void)device;
  return false;
#endif
}

void LinuxI2cBus::close() {
#if defined(__linux__)
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
#endif
}

bool LinuxI2cBus::write(uint8_t address,
                        const std::vector<uint8_t>& bytes) {
#if defined(__linux__)
  if (fd_ < 0 || bytes.empty()) {
    return false;
  }
  if (::ioctl(fd_, I2C_SLAVE, address) < 0) {
    return false;
  }
  return ::write(fd_, bytes.data(), bytes.size()) ==
         static_cast<ssize_t>(bytes.size());
#else
  (void)address;
  (void)bytes;
  return false;
#endif
}

bool LinuxI2cBus::read(uint8_t address, size_t length,
                       std::vector<uint8_t>* out) {
#if defined(__linux__)
  if (fd_ < 0 || !out || length == 0) {
    return false;
  }
  if (::ioctl(fd_, I2C_SLAVE, address) < 0) {
    return false;
  }
  out->assign(length, 0);
  return ::read(fd_, out->data(), length) ==
         static_cast<ssize_t>(length);
#else
  (void)address;
  (void)length;
  (void)out;
  return false;
#endif
}

}  // namespace hal
}  // namespace rtos
