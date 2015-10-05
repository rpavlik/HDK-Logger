/** @file
    @brief Header providing for safe and easy enumeration of HID devices using
   HIDAPI and typically, range-for.

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

#ifndef INCLUDED_Enumeration_h_GUID_8DCCF2B6_D5F5_498C_A135_CCD723390301
#define INCLUDED_Enumeration_h_GUID_8DCCF2B6_D5F5_498C_A135_CCD723390301

// Internal Includes
#include "HandleError.h"

// Library/third-party includes
#include <hidapi.h>

// Standard includes
#include <memory>

namespace hidapi {
namespace detail {
    /// Internal type of a device in an enumeration process, for easy range-for
    /// usage
    class EnumerationIterator {
      public:
        /// Default constructor, aka end iterator.
        EnumerationIterator() {}
        /// Cosntructor from an enumerator entry (device info pointer)
        explicit EnumerationIterator(hid_device_info *dev) : dev_(dev) {}

        /// Check validity
        explicit operator bool() const { return dev_; }

        /// Dereference to get a device info pointer.
        hid_device_info *operator*() const { return dev_; }

        /// Pre-increment.
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
} // namespace detail

/// HIDAPI enumeration object for safe enumeration. Ideal for usage in a
/// range-for loop. Note that once this object is destroyed, all data referred
/// to by the iterators goes away, so if you (for instance) want to keep a
/// device path around, make a copy of it.
///
/// Suggested usage is:
/// `for (auto cur_dev : hidapi::Enumeration()) { /* do stuff */ }`
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
} // namespace hidapi
#endif // INCLUDED_Enumeration_h_GUID_8DCCF2B6_D5F5_498C_A135_CCD723390301
