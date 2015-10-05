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

#ifndef INCLUDED_Config_h_GUID_80F00BB1_88BD_4D85_1F44_4DFDFC8B20B7
#define INCLUDED_Config_h_GUID_80F00BB1_88BD_4D85_1F44_4DFDFC8B20B7

// Internal Includes
// - none

// Library/third-party includes
// - none

// Standard includes
// - none

#ifndef HIDAPIPP_HAVE_WSTRING
#ifndef __ANDROID__
/// Identifies that the std::wstring type exists, enabling non-throwing methods
/// of error handling, etc.
#define HIDAPIPP_HAVE_WSTRING
#endif
#endif

#ifndef HIDAPI_USE_FPRINTF
/// Prints some errors to stderr.
#define HIDAPI_USE_FPRINTF
#endif

// Obey "SKIP" config options overall.
#if defined(HIDAPIPP_HAVE_WSTRING) && defined(HIDAPIPP_SKIP_WSTRING)
#undef HIDAPIPP_HAVE_WSTRING
#endif

#if defined(HIDAPIPP_USE_FPRINTF) && defined(HIDAPIPP_SKIP_FPRINTF)
#undef HIDAPIPP_USE_FPRINTF
#endif

#endif // INCLUDED_Config_h_GUID_80F00BB1_88BD_4D85_1F44_4DFDFC8B20B7
