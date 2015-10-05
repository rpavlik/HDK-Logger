/** @file
    @brief Header

    @date 2015

    @author
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

// Copyright 2015 Sensics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef INCLUDED_Device_h_GUID_A5AB4F26_B2C8_4F8F_5375_91F421C72017
#define INCLUDED_Device_h_GUID_A5AB4F26_B2C8_4F8F_5375_91F421C72017

// Internal Includes
#include "HandleError.h"

// Library/third-party includes
#include <hidapi.h>

// Standard includes
#include <stdexcept>
#include <vector>
#include <string>
#include <memory>
#include <cstddef> // for std::size_t

namespace hidapi {
namespace detail {
    /// Deleter functor for a HIDAPI device
    class DeviceDeleter {
      public:
        void operator()(hid_device *dev) {
            if (dev) {
                hid_close(dev);
            }
        }
    };

    using DeviceUniquePtr = std::unique_ptr<hid_device, detail::DeviceDeleter>;
    using DeviceSharedPtr = std::shared_ptr<hid_device>;
} // namespace detail

using DataByte = unsigned char;
using DataVector = std::vector<DataByte>;

static const std::size_t DEFAULT_MAX_LENGTH = 512;

/// CRTP base class for device objects: provides common functionality for
/// C++-wrapped HIDAPI devices.
template <typename Derived> class DeviceBase {
  public:
    /// Checks for validity of the object.
    explicit operator bool() const { return (nullptr != get_()); }

    /// Accessor for the raw HIDAPI opaque object, for functions that aren't
    /// wrapped.
    hid_device *get() const { return get_(); }

    /// Reads a HID report, if available.
    ///
    /// If the device has more than one report type, the first byte will be the
    /// report type.
    DataVector read(std::size_t maxLength = DEFAULT_MAX_LENGTH) {
        auto ret = DataVector(maxLength);
        auto result = hid_read(get(), ret.data(), maxLength);
        handle_buffer_and_error(ret, result);
        return ret;
    }

    /// Gets a HID feature report.
    ///
    /// The supplied report ID will be the first byte of the returned data
    /// vector.
    DataVector get_feature_report(unsigned char reportId,
                                  std::size_t maxLength = DEFAULT_MAX_LENGTH) {
        auto ret = DataVector(maxLength + 1);
        ret[0] = reportId;
        auto result = hid_getFeatureReport(get(), ret.data(), maxLength);
        handle_buffer_and_error(ret, result);
        return ret;
    }
    using derived_type = Derived;

  private:
    void handle_buffer_and_error(DataVector &ret, int callResult) {
        if (callResult < 0) {
            detail::handle_error(*get());
        }
        ret.resize(callResult);
    }
    hid_device *get_() const {
        return static_cast<derived_type const *>(this)->get();
    }
};

/// A wrapper class for a HID device with ownership semantics equal to those of
/// std::unique_ptr
///
/// Most functionality provided by hidapi::DeviceBase
class UniqueDevice : public DeviceBase<UniqueDevice> {
  public:
    using base = DeviceBase<UniqueDevice>;
    /// Default, empty constructor. Not very useful on its own, mostly for move
    /// assignment.
    UniqueDevice() : base() {}

    /// Constructor opening the first device with the given VID and PID and
    /// optionally matching the serial number. Wraps `hid_open()`
    UniqueDevice(unsigned short vid, unsigned short pid,
                 const wchar_t *serial_number = nullptr)
        : base(), dev_(hid_open(vid, pid, serial_number)) {}

    /// Constructor from platform-specific device path, often found through
    /// enumeration.  Wraps `hid_open_path()`
    UniqueDevice(const char *path) : base(), dev_(hid_open_path(path)) {}

    /// @overload
    UniqueDevice(std::string const &path) : UniqueDevice(path.c_str()) {}

    /// Accessor for the raw HIDAPI opaque object, for functions that aren't
    /// wrapped.
    hid_device *get() const { return dev_.get(); }

    /// Move constructor
    UniqueDevice(UniqueDevice &&other) : dev_(std::move(other.dev_)) {}

    /// Move assignment
    UniqueDevice &operator=(UniqueDevice &&other) {
        if (&other == this) {
            return *this;
        }
        dev_ = std::move(other.dev_);
        return *this;
    }

    /// Not copy constructible
    UniqueDevice(UniqueDevice const &) = delete;
    /// Not copy assignable
    UniqueDevice &operator=(UniqueDevice const &) = delete;

  private:
    detail::DeviceUniquePtr dev_;
};

/// A wrapper class for a HID device with ownership semantics equal to those of
/// std::shared_ptr
class SharedDevice : public DeviceBase<SharedDevice> {
  public:
    using base = DeviceBase<SharedDevice>;
    /// Default, empty constructor. Not very useful aside from assignment.
    SharedDevice() : base() {}
    /// Constructor opening the first device with the given VID and PID and
    /// optionally matching the serial number. Wraps `hid_open()`
    SharedDevice(unsigned short vid, unsigned short pid,
                 const wchar_t *serial_number = nullptr)
        : base(),
          dev_(hid_open(vid, pid, serial_number), detail::DeviceDeleter()) {}
    /// Constructor from platform-specific device path, often found through
    /// enumeration.  Wraps `hid_open_path()`
    SharedDevice(const char *path)
        : base(), dev_(hid_open_path(path), detail::DeviceDeleter()) {}
    /// @overload
    SharedDevice(std::string const &path) : SharedDevice(path.c_str()) {}

    /// Accessor for the raw HIDAPI opaque object, for functions that aren't
    /// wrapped.
    hid_device *get() const { return dev_.get(); }

  private:
    detail::DeviceSharedPtr dev_;
};
} // namespace hidapi

#endif // INCLUDED_Device_h_GUID_A5AB4F26_B2C8_4F8F_5375_91F421C72017
