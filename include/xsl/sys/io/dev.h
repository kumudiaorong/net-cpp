#pragma once
#ifndef XSL_SYS_IO_DEV
#  define XSL_SYS_IO_DEV
#  include "xsl/ai/dev.h"
#  include "xsl/coro/semaphore.h"
#  include "xsl/feature.h"
#  include "xsl/logctl.h"
#  include "xsl/sync.h"
#  include "xsl/sys/io/def.h"
#  include "xsl/wheel/type_traits.h"

#  include <unistd.h>

#  include <tuple>
#  include <type_traits>
#  include <utility>
XSL_SYS_IO_NB
class NativeDevice {
public:
  NativeDevice(int fd) noexcept : _fd(fd) {}
  NativeDevice(NativeDevice &&rhs) noexcept : _fd(std::exchange(rhs._fd, -1)) {}
  NativeDevice &operator=(NativeDevice &&rhs) noexcept {
    _fd = std::exchange(rhs._fd, -1);
    return *this;
  }
  int raw() const noexcept { return _fd; }

  ~NativeDevice() noexcept {
    if (_fd == -1) {
      return;
    }
    LOG5("close fd: {}", _fd);
    close(_fd);
  }

protected:
  int _fd;
};

namespace impl_dev {
  template <class... Flags>
  class AsyncDevice;

  // using AsyncReadDevice = AsyncDevice<feature::In, feature::placeholder>;
  // using AsyncWriteDevice = AsyncDevice<feature::placeholder, feature::Out>;
  // using AsyncReadWriteDevice = AsyncDevice<feature::In, feature::Out>;

  template <class... Flags>
  using AsyncDeviceCompose = feature::origanize_feature_flags_t<
      impl_dev::AsyncDevice<feature::Item<wheel::type_traits::is_same_pack, feature::In<void>,
                                          feature::Out<void>, feature::InOut<void>>,
                            feature::Dyn, feature::Own>,
      Flags...>;

  template <class... Flags>
  class Device;

  // using ReadDevice = Device<feature::In, feature::placeholder>;
  // using WriteDevice = Device<feature::placeholder, feature::Out>;
  // using ReadWriteDevice = Device<feature::In, feature::Out>;

  template <class... Flags>
  using DeviceCompose = feature::origanize_feature_flags_t<
      impl_dev::Device<feature::Item<wheel::type_traits::is_same_pack, feature::In<void>,
                                     feature::Out<void>, feature::InOut<void>>>,
      Flags...>;

  template <>
  class Device<feature::placeholder> {
  public:
    explicit Device(NativeDevice &&dev) noexcept
        : _dev(std::make_shared<NativeDevice>(std::move(dev))) {}
    Device(std::shared_ptr<NativeDevice> dev) noexcept : _dev(std::move(dev)) {}
    Device(Device &&rhs) noexcept : _dev(std::move(rhs._dev)) {}
    Device &operator=(Device &&rhs) noexcept {
      _dev = std::move(rhs._dev);
      return *this;
    }
    ~Device() noexcept { LOG6("Device dtor, use count: {}", _dev.use_count()); }
    int raw() const noexcept { return _dev->raw(); }

  protected:
    std::shared_ptr<NativeDevice> _dev;
  };

  template <class T>
  class Device<feature::In<T>> : public Device<feature::placeholder> {
  private:
    using Base = Device<feature::placeholder>;

  public:
    using Base::Base;
    /**
    @brief Create an async device for reading

    @tparam Flags, list is <feature::Dyn, feature::Own>
    @param poller
    @return AsyncDevice<feature::In, feature::placeholder, Flags...>
     */

    template <class... Flags>
    inline AsyncDeviceCompose<feature::In<T>, Flags...> async(
        std::shared_ptr<sync::Poller> &poller) && noexcept {
      auto sem = std::make_shared<coro::CountingSemaphore<1>>();
      poller->add(_dev->raw(), sync::IOM_EVENTS::IN | sync::IOM_EVENTS::ET,
                  sync::PollCallback<sync::IOM_EVENTS::IN>{sem});
      return {std::move(_dev), std::move(sem)};
    }
  };

  static_assert(std::is_same_v<Device<feature::In<int>>, DeviceCompose<feature::In<int>>>,
                "Device<feature::In, feature::placeholder> is not DeviceCompose<feature::In>");

  template <class T>
  class Device<feature::Out<T>> : public Device<feature::placeholder> {
  private:
    using Base = Device<feature::placeholder>;

  public:
    using Base::Base;
    // template <class... Flags>
    // AsyncDeviceCompose<feature::Out<T>, Flags...> async(
    //     std::shared_ptr<sync::Poller> &poller) && noexcept;
    template <class... Flags>
    inline AsyncDeviceCompose<feature::Out<T>, Flags...> async(
        std::shared_ptr<sync::Poller> &poller) && noexcept {
      auto sem = std::make_shared<coro::CountingSemaphore<1>>();
      poller->add(_dev->raw(), sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
                  sync::PollCallback<sync::IOM_EVENTS::OUT>{sem});
      return {std::move(_dev), std::move(sem)};
    }
  };

  static_assert(std::is_same_v<Device<feature::Out<int>>, DeviceCompose<feature::Out<int>>>,
                "Device<feature::placeholder, feature::Out> is not DeviceCompose<feature::Out>");

  template <class T>
  class Device<feature::InOut<T>> : public Device<feature::placeholder> {
  private:
    using Base = Device<feature::placeholder>;

  public:
    using Base::Base;
    std::tuple<DeviceCompose<feature::In<T>>, DeviceCompose<feature::Out<T>>> split() && noexcept {
      return {DeviceCompose<feature::In<T>>{std::move(_dev)},
              DeviceCompose<feature::Out<T>>{std::move(_dev)}};
    }
    // template <class... Flags>
    // AsyncDeviceCompose<feature::InOut<T>, Flags...> async(
    //     std::shared_ptr<sync::Poller> &poller) && noexcept;
    template <class... Flags>
    inline AsyncDeviceCompose<feature::InOut<T>, Flags...> async(
        std::shared_ptr<sync::Poller> &poller) && noexcept {
      auto read_sem = std::make_shared<coro::CountingSemaphore<1>>();
      auto write_sem = std::make_shared<coro::CountingSemaphore<1>>();
      poller->add(
          _dev->raw(), sync::IOM_EVENTS::IN | sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
          sync::PollCallback<sync::IOM_EVENTS::IN, sync::IOM_EVENTS::OUT>{read_sem, write_sem});
      return {std::move(_dev), std::move(read_sem), std::move(write_sem)};
    }
  };

  static_assert(
      std::is_same_v<Device<feature::InOut<int>>, DeviceCompose<feature::InOut<int>>>,
      "Device<feature::In, feature::Out> is not DeviceCompose<feature::In, feature::Out>");

  template <class T, class U, class V>
  class AsyncDevice<feature::In<T>, U, V>
      : public std::conditional_t<std::is_same_v<U, feature::Dyn>,
                                  ai::AsyncDevice<feature::In<T>, V>, feature::placeholder> {
  public:
    AsyncDevice(NativeDevice &&dev, std::shared_ptr<coro::CountingSemaphore<1>> sem) noexcept
        : _dev(std::make_shared<NativeDevice>(std::move(dev))), _sem(std::move(sem)) {}

    AsyncDevice(std::shared_ptr<NativeDevice> dev,
                std::shared_ptr<coro::CountingSemaphore<1>> sem) noexcept
        : _dev(std::move(dev)), _sem(std::move(sem)) {}

    AsyncDevice(AsyncDevice &&rhs) noexcept
        : _dev(std::move(rhs._dev)), _sem(std::move(rhs._sem)) {}

    AsyncDevice &operator=(AsyncDevice &&rhs) noexcept {
      _dev = std::move(rhs._dev);
      _sem = std::move(rhs._sem);
      return *this;
    }

    ~AsyncDevice() noexcept { LOG6("AsyncDevice dtor, use count: {}", _sem.use_count()); }

    int raw() const noexcept { return _dev->raw(); }

    coro::CountingSemaphore<1> &sem() noexcept { return *_sem; }

  protected:
    std::shared_ptr<NativeDevice> _dev;
    std::shared_ptr<coro::CountingSemaphore<1>> _sem;
  };

  static_assert(
      std::is_same_v<AsyncDevice<feature::In<int>, feature::placeholder, feature::placeholder>,
                     AsyncDeviceCompose<feature::In<int>>>,
      "AsyncDevice<feature::In, feature::placeholder> is not AsyncDeviceCompose<feature::In>");

  static_assert(std::is_same_v<AsyncDevice<feature::In<int>, feature::Dyn, feature::placeholder>,
                               AsyncDeviceCompose<feature::In<int>, feature::Dyn>>,
                "AsyncDevice<feature::In, feature::placeholder, feature::Dyn> is not "
                "AsyncDeviceCompose<feature::In, feature::Dyn>");

  static_assert(std::is_base_of_v<ai::AsyncDevice<feature::In<int>>,
                                  AsyncDeviceCompose<feature::In<int>, feature::Dyn>>,
                "AsyncDeviceCompose<feature::In, feature::Dyn> is not derived from "
                "ai::Device<feature::In>");

  // template <class... Flags>
  // inline AsyncDeviceCompose<feature::In, feature::placeholder, Flags...>
  // Device<feature::In, feature::placeholder>::async(
  //     std::shared_ptr<sync::Poller> &poller) && noexcept {
  //   auto sem = std::make_shared<coro::CountingSemaphore<1>>();
  //   poller->add(_dev->raw(), sync::IOM_EVENTS::IN | sync::IOM_EVENTS::ET,
  //               sync::PollCallback<sync::IOM_EVENTS::IN>{sem});
  //   return {std::move(_dev), std::move(sem)};
  // }

  template <class T, class U, class V>
  class AsyncDevice<feature::Out<T>, U, V>
      : public std::conditional_t<std::is_same_v<U, feature::Dyn>,
                                  ai::AsyncDevice<feature::Out<T>, V>, feature::placeholder> {
  public:
    AsyncDevice(NativeDevice &&dev, std::shared_ptr<coro::CountingSemaphore<1>> sem) noexcept
        : _dev(std::make_shared<NativeDevice>(std::move(dev))), _sem(std::move(sem)) {}

    AsyncDevice(std::shared_ptr<NativeDevice> dev,
                std::shared_ptr<coro::CountingSemaphore<1>> sem) noexcept
        : _dev(std::move(dev)), _sem(std::move(sem)) {}

    AsyncDevice(AsyncDevice &&rhs) noexcept
        : _dev(std::move(rhs._dev)), _sem(std::move(rhs._sem)) {}

    AsyncDevice &operator=(AsyncDevice &&rhs) noexcept {
      _dev = std::move(rhs._dev);
      _sem = std::move(rhs._sem);
      return *this;
    }

    ~AsyncDevice() noexcept { LOG6("AsyncDevice dtor, use count: {}", _sem.use_count()); }

    int raw() const noexcept { return _dev->raw(); }

    coro::CountingSemaphore<1> &sem() noexcept { return *_sem; }

  protected:
    std::shared_ptr<NativeDevice> _dev;
    std::shared_ptr<coro::CountingSemaphore<1>> _sem;
  };

  static_assert(
      std::is_same_v<AsyncDevice<feature::Out<int>, feature::placeholder, feature::placeholder>,
                     AsyncDeviceCompose<feature::Out<int>>>,
      "AsyncDevice<feature::placeholder, feature::Out> is not AsyncDeviceCompose<feature::Out>");

  static_assert(std::is_same_v<AsyncDevice<feature::Out<int>, feature::Dyn, feature::placeholder>,
                               AsyncDeviceCompose<feature::Out<int>, feature::Dyn>>,
                "AsyncDevice<feature::placeholder, feature::Out, feature::Dyn> is not "
                "AsyncDeviceCompose<feature::Out, feature::Dyn>");

  static_assert(std::is_base_of_v<ai::AsyncDevice<feature::Out<int>>,
                                  AsyncDeviceCompose<feature::Out<int>, feature::Dyn>>,
                "AsyncDeviceCompose<feature::Out, feature::Dyn> is not derived from "
                "ai::Device<feature::Out>");

  // template <class... Flags>
  // inline AsyncDeviceCompose<feature::placeholder, feature::Out, Flags...>
  // Device<feature::placeholder, feature::Out>::async(
  //     std::shared_ptr<sync::Poller> &poller) && noexcept {
  //   auto sem = std::make_shared<coro::CountingSemaphore<1>>();
  //   poller->add(_dev->raw(), sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
  //               sync::PollCallback<sync::IOM_EVENTS::OUT>{sem});
  //   return {std::move(_dev), std::move(sem)};
  // }

  template <class T, class U, class V>
  class AsyncDevice<feature::InOut<T>, U, V>
      : public std::conditional_t<std::is_same_v<U, feature::Dyn>,
                                  ai::AsyncDevice<feature::InOut<T>, V>, feature::placeholder> {
  public:
    AsyncDevice(std::shared_ptr<NativeDevice> dev,
                std::shared_ptr<coro::CountingSemaphore<1>> read_sem,
                std::shared_ptr<coro::CountingSemaphore<1>> write_sem) noexcept
        : _dev(std::move(dev)), _read_sem(std::move(read_sem)), _write_sem(std::move(write_sem)) {}

    AsyncDevice(AsyncDevice &&rhs) noexcept
        : _dev(std::move(rhs._dev)),
          _read_sem(std::move(rhs._read_sem)),
          _write_sem(std::move(rhs._write_sem)) {}

    AsyncDevice &operator=(AsyncDevice &&rhs) noexcept {
      _dev = std::move(rhs._dev);
      _read_sem = std::move(rhs._read_sem);
      _write_sem = std::move(rhs._write_sem);
      return *this;
    }

    ~AsyncDevice() noexcept {
      LOG6("AsyncDevice dtor, read use count: {}, write use count: {}", _read_sem.use_count(),
           _write_sem.use_count());
    }

    int raw() const noexcept { return _dev->raw(); }

    coro::CountingSemaphore<1> &read_sem() noexcept { return *_read_sem; }

    coro::CountingSemaphore<1> &write_sem() noexcept { return *_write_sem; }

    auto split() && noexcept {
      using ReadDevice = AsyncDevice<feature::In<T>, U, V>;
      using WriteDevice = AsyncDevice<feature::Out<T>, U, V>;
      return std::make_tuple(ReadDevice{_dev, std::move(_read_sem)},
                             WriteDevice{_dev, std::move(_write_sem)});
    }

  protected:
    std::shared_ptr<NativeDevice> _dev;
    std::shared_ptr<coro::CountingSemaphore<1>> _read_sem;
    std::shared_ptr<coro::CountingSemaphore<1>> _write_sem;
  };

  static_assert(
      std::is_same_v<AsyncDevice<feature::InOut<int>, feature::placeholder, feature::placeholder>,
                     AsyncDeviceCompose<feature::InOut<int>>>,
      "AsyncDevice<feature::In, feature::Out> is not AsyncDeviceCompose<feature::In, "
      "feature::Out>");

  static_assert(std::is_same_v<AsyncDevice<feature::InOut<int>, feature::Dyn, feature::placeholder>,
                               AsyncDeviceCompose<feature::InOut<int>, feature::Dyn>>,
                "AsyncDevice<feature::In, feature::Out, feature::Dyn> is not "
                "AsyncDeviceCompose<feature::In, feature::Out, feature::Dyn>");

  static_assert(std::is_base_of_v<ai::AsyncDevice<feature::InOut<int>>,
                                  AsyncDeviceCompose<feature::InOut<int>, feature::Dyn>>,
                "AsyncDeviceCompose<feature::In, feature::Out, feature::Dyn> is not derived from "
                "ai::Device<>");

  // template <class... Flags>
  // inline AsyncDeviceCompose<feature::In, feature::Out, Flags...>
  // Device<feature::In, feature::Out>::async(std::shared_ptr<sync::Poller> &poller) && noexcept {
  //   auto read_sem = std::make_shared<coro::CountingSemaphore<1>>();
  //   auto write_sem = std::make_shared<coro::CountingSemaphore<1>>();
  //   poller->add(
  //       _dev->raw(), sync::IOM_EVENTS::IN | sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
  //       sync::PollCallback<sync::IOM_EVENTS::IN, sync::IOM_EVENTS::OUT>{read_sem, write_sem});
  //   return {std::move(_dev), std::move(read_sem), std::move(write_sem)};
  // }
}  // namespace impl_dev

template <class... Flags>
using Device = impl_dev::DeviceCompose<Flags...>;

template <class... Flags>
using AsyncDevice = impl_dev::AsyncDeviceCompose<Flags...>;
XSL_SYS_IO_NE
#endif
