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
    inline bool operator==(EnumerationIterator const &lhs,
                           EnumerationIterator const &rhs) {
        return *lhs == *rhs;
    }

    inline bool operator!=(EnumerationIterator const &lhs,
                           EnumerationIterator const &rhs) {
        return *lhs != *rhs;
    }
    class EnumerationDeleter {
      public:
        void operator()(struct hid_device_info *devs) {
            hid_free_enumeration(devs);
        }
    };
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

class Enumeration {
  public:
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
/// CRTP base class for device objects
template <typename Derived> class DeviceBase {
  public:
    explicit operator bool() const { return (nullptr != get_()); }
    hid_device *get() const { return get_(); }

    DataVector read(size_t maxLength) {
        auto ret = DataVector(maxLength);
        auto result = hid_read(get(), ret.data(), maxLength);
        handle_buffer_and_error(ret, result);
        return ret;
    }

    DataVector get_feature_report(unsigned char reportId, size_t maxLength) {
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
class UniqueDevice : public DeviceBase<UniqueDevice> {
  public:
    using base = DeviceBase<UniqueDevice>;
    UniqueDevice() : base() {}
    UniqueDevice(unsigned short vid, unsigned short pid,
                 const wchar_t *serial_number = nullptr)
        : base(), dev_(hid_open(vid, pid, serial_number)) {}
    UniqueDevice(const char *path) : base(), dev_(hid_open_path(path)) {}
    UniqueDevice(std::string const &path) : UniqueDevice(path.c_str()) {}

    hid_device *get() const { return dev_.get(); }

  private:
    detail::DeviceUniquePtr dev_;
};

/// A wrapper class for a HID device with ownership semantics equal to those of
/// std::shared_ptr
class SharedDevice : public DeviceBase<SharedDevice> {
  public:
    using base = DeviceBase<SharedDevice>;
    SharedDevice() {}
    SharedDevice(unsigned short vid, unsigned short pid,
                 const wchar_t *serial_number = nullptr)
        : base(),
          dev_(hid_open(vid, pid, serial_number), detail::DeviceDeleter()) {}
    SharedDevice(const char *path)
        : base(), dev_(hid_open_path(path), detail::DeviceDeleter()) {}
    SharedDevice(std::string const &path) : SharedDevice(path.c_str()) {}

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
