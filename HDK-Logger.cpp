/** @file
    @brief Implementation

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

// Internal Includes
// - none

// Library/third-party includes
#include <hidapi.h>

// Standard includes
#include <stdio.h>
#include <iostream>
#include <memory>

#include <chrono>
#include <vector>

namespace hidapi {
/// RAII initialization/shutdown of the HIDAPI library.
class Library {
  public:
    Library() {
        if (hid_init()) {
            throw std::runtime_error("Could not initialize HIDAPI!");
        }
    }
    ~Library() { hid_exit(); }
    Library(Library const &) = delete;
    Library &operator=(Library const &) = delete;
};

namespace detail {
    /// Internal type of a device in an enumeration process, for easy range-for
    /// usage
    class EnumerationIterator {
      public:
        EnumerationIterator() {}
        EnumerationIterator(hid_device_info *dev) : dev_(dev) {}
        explicit operator bool() const { return dev_; }
        hid_device_info *operator*() const { return dev_; }
        EnumerationIterator &operator++() {
            dev_ = dev_->next;
            return *this;
        }

      private:
        struct hid_device_info *dev_ = nullptr;
    };

    /// Equality of enumeration iterator
    inline bool operator==(EnumerationIterator const &lhs,
                           EnumerationIterator const &rhs) {
        return *lhs == *rhs;
    }

    /// Inequality of enumeration iterator
    inline bool operator!=(EnumerationIterator const &lhs,
                           EnumerationIterator const &rhs) {
        return *lhs != *rhs;
    }

    /// Deleter functor for a HIDAPI enumeration
    class EnumerationDeleter {
      public:
        void operator()(struct hid_device_info *devs) {
            hid_free_enumeration(devs);
        }
    };

    /// Deleter functor for a HIDAPI device
    class DeviceDeleter {
      public:
        void operator()(hid_device *dev) {
            if (dev) {
                hid_close(dev);
            }
        }
    };

    inline void handle_error(hid_device &dev) {
        const wchar_t *errMsg = hid_error(&dev);
        if (nullptr == errMsg) {
            std::cerr << "in hidapi::detail::handle_error but no error?"
                      << std::endl;
            return;
        }
        fprintf(stderr, "hidapi error: %ls\n", errMsg);
        throw std::runtime_error("hidapi error");
    }

    using DeviceUniquePtr = std::unique_ptr<hid_device, detail::DeviceDeleter>;
    using DeviceSharedPtr = std::shared_ptr<hid_device>;
} // namespace detail

/// HIDAPI enumeration object for safe enumeration. Ideal for usage in a
/// range-for loop. Note that once this object is destroyed, all data referred
/// to by the iterators goes away, so if you (for instance) want to keep a
/// device path around, make a copy of it.
class Enumeration {
  public:
    /// Constructor - default arguments do not filter on VID/PID, but optionally
    /// can.
    Enumeration(unsigned short vid = 0x0000, unsigned short pid = 0x0000)
        : devs_(hid_enumerate(vid, pid)) {}

    detail::EnumerationIterator begin() const {
        return detail::EnumerationIterator(devs_.get());
    }
    detail::EnumerationIterator end() const {
        return detail::EnumerationIterator();
    }

  private:
    std::unique_ptr<struct hid_device_info, detail::EnumerationDeleter> devs_;
};

using DataByte = unsigned char;
using DataVector = std::vector<DataByte>;

static const size_t DEFAULT_MAX_LENGTH = 512;
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
    DataVector read(size_t maxLength = DEFAULT_MAX_LENGTH) {
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
                                  size_t maxLength = DEFAULT_MAX_LENGTH) {
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
    /// Default, empty constructor. Not very useful.
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

  private:
    detail::DeviceUniquePtr dev_;
};

/// A wrapper class for a HID device with ownership semantics equal to those of
/// std::shared_ptr
class SharedDevice : public DeviceBase<SharedDevice> {
  public:
    using base = DeviceBase<SharedDevice>;
    /// Default, empty constructor. Not very useful.
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

int main() {
    hidapi::Library lib;
    auto hdk_path = std::string{};
    for (auto cur_dev : hidapi::Enumeration()) {
        printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  "
               "serial_number: %ls",
               cur_dev->vendor_id, cur_dev->product_id, cur_dev->path,
               cur_dev->serial_number);
        printf("\n");
        printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
        printf("  Product:      %ls\n", cur_dev->product_string);
        printf("  Release:      %hx\n", cur_dev->release_number);
        printf("  Interface:    %d\n", cur_dev->interface_number);
        printf("\n");
        if (cur_dev->vendor_id == 0x1532 && cur_dev->product_id == 0x0b00) {
            printf("  *** This is an HDK tracker! ***\n");
            hdk_path.assign(cur_dev->path);
        }
    }
    if (hdk_path.empty()) {
        std::cerr
            << "Could not find an (unused) HDK tracker! Press enter to exit."
            << std::endl;
        std::cin.ignore();
        return -1;
    }

    auto dev = hidapi::UniqueDevice{hdk_path};

    // Enable blocking mode.
    hid_set_nonblocking(dev.get(), 0);

    return 0;
}
