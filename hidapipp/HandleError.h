/** @file
    @brief Header defining internal utility function used to handle errors
   reportable through `hid_error()`

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

#ifndef INCLUDED_HandleError_h_GUID_9CF6A1DF_E762_4DB1_4845_533BC418C481
#define INCLUDED_HandleError_h_GUID_9CF6A1DF_E762_4DB1_4845_533BC418C481

// Internal Includes
#include "Config.h"

// Library/third-party includes
#include <hidapi.h>

// Standard includes
#include <cassert>
#include <cstddef>
#include <stdexcept>

#ifdef HIDAPIPP_USE_FPRINTF
#include <stdio.h>
#endif

namespace hidapi {
namespace detail {
    inline const wchar_t *handle_error(hid_device &dev) {
        const wchar_t *errMsg = hid_error(&dev);
        if (nullptr == errMsg) {
            assert(0 && "in hidapi::detail::handle_error but no error? should "
                        "not happen.");
            return errMsg;
        }
        return errMsg;
    }

    inline void handle_error_throwing(const wchar_t *errMsg) {
        if (errMsg) {
#ifdef HIDAPIPP_USE_FPRINTF
            fprintf(stderr, "hidapi error: %ls\n", errMsg);
#endif
            throw std::runtime_error("hidapi error");
        } else {
#ifdef HIDAPIPP_USE_FPRINTF
            fprintf(
                stderr,
                "hidapi error, but could not retrieve HIDAPI error message\n");
#endif
            throw std::runtime_error("hidapi error, but could not retrieve "
                                     "HIDAPI error message - should not "
                                     "happen");
        }
    }
    inline void handle_error_throwing(hid_device &dev) {
        handle_error_throwing(handle_error(dev));
    }

} // namespace detail
} // namespace hidapi
#endif // INCLUDED_HandleError_h_GUID_9CF6A1DF_E762_4DB1_4845_533BC418C481
