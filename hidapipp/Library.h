/** @file
    @brief Header defining RAII class for opening/closing HIDAPI itself.

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

#ifndef INCLUDED_Library_h_GUID_9D646D0A_6369_439F_0E02_9F4D5DDDDB52
#define INCLUDED_Library_h_GUID_9D646D0A_6369_439F_0E02_9F4D5DDDDB52

// Internal Includes
// - none

// Library/third-party includes
#include <hidapi.h>

// Standard includes
#include <stdexcept>

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
} // namespace hidapi

#endif // INCLUDED_Library_h_GUID_9D646D0A_6369_439F_0E02_9F4D5DDDDB52
