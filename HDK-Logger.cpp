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
#include "hidapipp/hidapipp.h"

// Standard includes
#include <stdio.h>
#include <iostream>
#include <chrono>

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

    std::cout << "Opening " << hdk_path << std::endl;

    /// Open the device
    auto dev = hidapi::UniqueDevice{hdk_path};

    /// Enable blocking mode on this device
    hid_set_nonblocking(dev.get(), 0);

    using clock = std::chrono::system_clock;
    // Set end time for the loop shortly into the future.
    auto endTime = clock::now() + std::chrono::milliseconds(500);
    while (clock::now() < endTime) {
        /// Read some data using the non-throwing interface
        auto result = dev.read();
        /// Handle error
        if (hidapi::had_error(result)) {
            fprintf(stderr, "HIDAPI had an error reading from the HDK: %ls\n",
                    hidapi::get_error(result));
            return -1;
        }
        /// Get the actual data and do something with it.
        auto const &data = hidapi::get_data(result);
        std::cout << "Report size: " << data.size()
                  << " Version number: " << int(data[0])
                  << " Sequence number: " << int(data[1]) << "\n";
    }

    return 0;
}
