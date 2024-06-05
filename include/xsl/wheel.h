#pragma once

#ifndef _XSL_WHEEL_H_
#  define _XSL_WHEEL_H_
#  include "xsl/def.h"
#  include "xsl/wheel/giant.h"
#  include "xsl/wheel/mutex.h"
#  include "xsl/wheel/result.h"
#  include "xsl/wheel/str.h"
#  include "xsl/wheel/type_traits.h"
#  include "xsl/wheel/vec.h"
XSL_NAMESPACE_BEGIN
using wheel::bool_from_bytes;
using wheel::bool_to_bytes;
using wheel::FixedString;
using wheel::FixedVec;
using wheel::from_string;
using wheel::i32_from_bytes;
using wheel::i32_to_bytes;
using wheel::LockGuard;
using wheel::Mutex;
using wheel::Result;
using wheel::ShareContainer;
using wheel::SharedLockGuard;
using wheel::SharedMutex;
using wheel::to_string;
using wheel::ToString;
using wheel::giant::all;
using wheel::giant::array;
using wheel::giant::atomic_flag;
using wheel::giant::atomic_int32_t;
using wheel::giant::atomic_uint32_t;
using wheel::giant::bind;
using wheel::giant::byte;
using wheel::giant::default_random_engine;
using wheel::giant::derived_from;
using wheel::giant::format;
using wheel::giant::forward;
using wheel::giant::forward_list;
using wheel::giant::function;
using wheel::giant::get_if;
using wheel::giant::holds_alternative;
using wheel::giant::list;
using wheel::giant::make_optional;
using wheel::giant::make_pair;
using wheel::giant::make_shared;
using wheel::giant::make_unique;
using wheel::giant::max;
using wheel::giant::min;
using wheel::giant::monostate;
using wheel::giant::move;
using wheel::giant::move_constructible;
using wheel::giant::nullopt;
using wheel::giant::optional;
using wheel::giant::pair;
using wheel::giant::queue;
using wheel::giant::random_device;
using wheel::giant::same_as;
using wheel::giant::shared_ptr;
using wheel::giant::size_t;
using wheel::giant::ssize_t;
using wheel::giant::stack;
using wheel::giant::string;
using wheel::giant::string_view;
using wheel::giant::tuple;
using wheel::giant::uniform_int_distribution;
using wheel::giant::unique_ptr;
using wheel::giant::unordered_map;
using wheel::giant::unordered_set;
using wheel::giant::variant;
using wheel::giant::vector;
using wheel::giant::visit;
namespace chrono = std::chrono;
namespace ranges = std::ranges;
namespace filesystem = std::filesystem;
XSL_NAMESPACE_END
#endif  // _XSL_UTILS_WHEEL_H_