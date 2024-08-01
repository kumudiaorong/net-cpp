#pragma once
#ifndef XSL_AI_DEV
#  define XSL_AI_DEV
#  include "xsl/ai/def.h"
#  include "xsl/coro/task.h"
#  include "xsl/feature.h"

#  include <cstddef>
#  include <optional>
#  include <span>
#  include <tuple>

XSL_AI_NB
namespace impl_dev {
  template <class... Flags>
  class AsyncDevice;

  template <class T>
  class AsyncDevice<feature::In<T>, feature::Own>;

  template <class T>
  class AsyncDevice<feature::In<T>, feature::placeholder> {
  public:
    virtual ~AsyncDevice() = default;
    virtual coro::Task<std::tuple<std::size_t, std::optional<std::errc>>> read(std::span<T> buf)
        = 0;
  };

  template <class T>
  class AsyncDevice<feature::Out<T>, feature::placeholder> {
  public:
    virtual ~AsyncDevice() = default;
    virtual coro::Task<std::tuple<std::size_t, std::optional<std::errc>>> write(
        std::span<const T> buf)
        = 0;
  };

  template <class T>
  class AsyncDevice<feature::InOut<T>, feature::placeholder>
      : public AsyncDevice<feature::In<T>, feature::placeholder>,
        public AsyncDevice<feature::Out<T>, feature::placeholder> {};
}  // namespace impl_dev

template <class... Flags>
using AsyncDevice = feature::origanize_feature_flags_t<
    impl_dev::AsyncDevice<feature::Item<wheel::type_traits::is_same_pack, feature::In<void>,
                                        feature::Out<void>, feature::InOut<void>>,
                          feature::Own>,
    Flags...>;

XSL_AI_NE
#endif