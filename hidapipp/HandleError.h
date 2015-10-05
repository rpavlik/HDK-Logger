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
// - none

// Library/third-party includes
#include <hidapi.h>

// Standard includes
#include <cassert>
#include <stdio.h>
#include <stdexcept>
#include <cstddef>

namespace hidapi {
namespace detail {
    inline void handle_error(hid_device &dev) {
        const wchar_t *errMsg = hid_error(&dev);
        if (nullptr == errMsg) {
            assert(0 && "in hidapi::detail::handle_error but no error? should "
                        "not happen.");
            return;
        }
        fprintf(stderr, "hidapi error: %ls\n", errMsg);
        throw std::runtime_error("hidapi error");
    }
} // namespace detail
} // namespace hidapi
#endif // INCLUDED_HandleError_h_GUID_9CF6A1DF_E762_4DB1_4845_533BC418C481
