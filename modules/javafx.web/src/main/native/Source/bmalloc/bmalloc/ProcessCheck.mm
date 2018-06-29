/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "ProcessCheck.h"

#if !BPLATFORM(WATCHOS)

#import <Foundation/Foundation.h>

namespace bmalloc {

bool gigacageEnabledForProcess()
{
    static NSString *appName = [[NSBundle mainBundle] bundleIdentifier];
    if (appName) {
        static bool isWebProcess = [appName isEqualToString:@"com.apple.WebKit.WebContent.Development"]
            || [appName isEqualToString:@"com.apple.WebKit.WebContent"]
            || [appName isEqualToString:@"com.apple.WebProcess"];
        return isWebProcess;
    }

    static NSString *processName = [[NSProcessInfo processInfo] processName];
    static bool isOptInBinary = [processName isEqualToString:@"jsc"]
        || [processName isEqualToString:@"wasm"]
        || [processName hasPrefix:@"test"];

    return isOptInBinary;
}

}

#endif
