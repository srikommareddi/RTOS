#include "../include/hal/linux_spi.h"

#if defined(__linux__)

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/spi/spidev.h>

#endif

namespace rtos {
namespace hal {

LinuxSpiBus::LinuxSpiBus() = default;

LinuxSpiBus::~LinuxSpiBus() {
  close();
}

bool LinuxSpiBus::open(const std::string& device) {
#if defined(__linux__)
  fd_ = ::open(device.c_str(), O_RDWR);
  if (fd_ < 0) {
    return false;
  }
  ::ioctl(fd_, SPI_IOC_WR_MODE, &mode_);
  ::ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz_);
  return true;
#else
  (void)device;
  return false;
#endif
}

void LinuxSpiBus::close() {
#if defined(__linux__)
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
#endif
}

bool LinuxSpiBus::transfer(const std::vector<uint8_t>& tx,
                           std::vector<uint8_t>* rx) {
#if defined(__linux__)
  if (fd_ < 0 || !rx) {
    return false;
  }
  rx->assign(tx.size(), 0);
  spi_ioc_transfer tr{};
  tr.tx_buf = reinterpret_cast<unsigned long>(tx.data());
  tr.rx_buf = reinterpret_cast<unsigned long>(rx->data());
  tr.len = tx.size();
  tr.speed_hz = speed_hz_;
  tr.bits_per_word = 8;
  return ::ioctl(fd_, SPI_IOC_MESSAGE(1), &tr) >= 0;
#else
  (void)tx;
  (void)rx;
  return false;
#endif
}

void LinuxSpiBus::set_mode(uint8_t mode) {
  mode_ = mode;
#if defined(__linux__)
  if (fd_ >= 0) {
    ::ioctl(fd_, SPI_IOC_WR_MODE, &mode_);
  }
#endif
}

void LinuxSpiBus::set_speed_hz(uint32_t speed_hz) {
  speed_hz_ = speed_hz;
#if defined(__linux__)
  if (fd_ >= 0) {
    ::ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz_);
  }
#endif
}

}  // namespace hal
}  // namespace rtos
