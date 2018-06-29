/*
 * Copyright (C) 2003-2017 Apple Inc.  All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 * Copyright (C) 2011 University of Szeged. All rights reserved.
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

// The vprintf_stderr_common function triggers this error in the Mac build.
// Feel free to remove this pragma if this file builds on Mac.
// According to http://gcc.gnu.org/onlinedocs/gcc-4.2.1/gcc/Diagnostic-Pragmas.html#Diagnostic-Pragmas
// we need to place this directive before any data or functions are defined.
#pragma GCC diagnostic ignored "-Wmissing-format-attribute"

#include "config.h"
#include "Assertions.h"

#include "Compiler.h"
#include <mutex>
#include <stdio.h>
#include <string.h>
#include <wtf/Lock.h>
#include <wtf/Locker.h>
#include <wtf/LoggingAccumulator.h>
#include <wtf/PrintStream.h>
#include <wtf/RetainPtr.h>
#include <wtf/StackTrace.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

#if HAVE(SIGNAL_H)
#include <signal.h>
#endif

#if USE(CF)
#include <CoreFoundation/CFString.h>
#if PLATFORM(COCOA)
#define USE_APPLE_SYSTEM_LOG 1
#include <asl.h>
#endif
#endif // USE(CF)

#if COMPILER(MSVC)
#include <crtdbg.h>
#endif

#if OS(WINDOWS)
#include <windows.h>
#if PLATFORM(JAVA) && defined(_DEBUG)
#pragma comment(lib, "user32.lib")
#endif
#endif

#if OS(DARWIN)
#include <sys/sysctl.h>
#include <unistd.h>
#endif

extern "C" {

static void logToStderr(const char* buffer)
{
#if USE(APPLE_SYSTEM_LOG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    asl_log(0, 0, ASL_LEVEL_NOTICE, "%s", buffer);
#pragma clang diagnostic pop
#endif
    fputs(buffer, stderr);
}

WTF_ATTRIBUTE_PRINTF(1, 0)
static void vprintf_stderr_common(const char* format, va_list args)
{
#if USE(CF) && !OS(WINDOWS)
    if (strstr(format, "%@")) {
        auto cfFormat = adoptCF(CFStringCreateWithCString(nullptr, format, kCFStringEncodingUTF8));

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
        auto str = adoptCF(CFStringCreateWithFormatAndArguments(nullptr, nullptr, cfFormat.get(), args));
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif
        CFIndex length = CFStringGetMaximumSizeForEncoding(CFStringGetLength(str.get()), kCFStringEncodingUTF8);
        constexpr unsigned InitialBufferSize { 256 };
        Vector<char, InitialBufferSize> buffer(length + 1);

        CFStringGetCString(str.get(), buffer.data(), length, kCFStringEncodingUTF8);

        logToStderr(buffer.data());
        return;
    }

#if USE(APPLE_SYSTEM_LOG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    va_list copyOfArgs;
    va_copy(copyOfArgs, args);
    asl_vlog(0, 0, ASL_LEVEL_NOTICE, format, copyOfArgs);
    va_end(copyOfArgs);
#pragma clang diagnostic pop
#endif

    // Fall through to write to stderr in the same manner as other platforms.

#elif HAVE(ISDEBUGGERPRESENT)
    if (IsDebuggerPresent()) {
        size_t size = 1024;
        Vector<char> buffer(size);
        do {
            buffer.grow(size);
            if (vsnprintf(buffer.data(), size, format, args) != -1) {
                OutputDebugStringA(buffer.data());
                break;
            }
            size *= 2;
        } while (size > 1024);
    }
#endif
    vfprintf(stderr, format, args);
}

#if COMPILER(GCC_OR_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif

static void vprintf_stderr_with_prefix(const char* prefix, const char* format, va_list args)
{
    size_t prefixLength = strlen(prefix);
    size_t formatLength = strlen(format);
    auto formatWithPrefix = std::make_unique<char[]>(prefixLength + formatLength + 1);
    memcpy(formatWithPrefix.get(), prefix, prefixLength);
    memcpy(formatWithPrefix.get() + prefixLength, format, formatLength);
    formatWithPrefix[prefixLength + formatLength] = 0;

    vprintf_stderr_common(formatWithPrefix.get(), args);
}

static void vprintf_stderr_with_trailing_newline(const char* format, va_list args)
{
    size_t formatLength = strlen(format);
    if (formatLength && format[formatLength - 1] == '\n') {
        vprintf_stderr_common(format, args);
        return;
    }

    auto formatWithNewline = std::make_unique<char[]>(formatLength + 2);
    memcpy(formatWithNewline.get(), format, formatLength);
    formatWithNewline[formatLength] = '\n';
    formatWithNewline[formatLength + 1] = 0;

    vprintf_stderr_common(formatWithNewline.get(), args);
}

#if COMPILER(GCC_OR_CLANG)
#pragma GCC diagnostic pop
#endif

WTF_ATTRIBUTE_PRINTF(1, 2)
static void printf_stderr_common(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf_stderr_common(format, args);
    va_end(args);
}

static void printCallSite(const char* file, int line, const char* function)
{
#if OS(WINDOWS) && defined(_DEBUG)
    _CrtDbgReport(_CRT_WARN, file, line, NULL, "%s\n", function);
#else
    // By using this format, which matches the format used by MSVC for compiler errors, developers
    // using Visual Studio can double-click the file/line number in the Output Window to have the
    // editor navigate to that line of code. It seems fine for other developers, too.
    printf_stderr_common("%s(%d) : %s\n", file, line, function);
#endif
}

void WTFReportAssertionFailure(const char* file, int line, const char* function, const char* assertion)
{
    if (assertion)
        printf_stderr_common("ASSERTION FAILED: %s\n", assertion);
    else
        printf_stderr_common("SHOULD NEVER BE REACHED\n");
    printCallSite(file, line, function);
}

void WTFReportAssertionFailureWithMessage(const char* file, int line, const char* function, const char* assertion, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf_stderr_with_prefix("ASSERTION FAILED: ", format, args);
    va_end(args);
    printf_stderr_common("\n%s\n", assertion);
    printCallSite(file, line, function);
}

void WTFReportArgumentAssertionFailure(const char* file, int line, const char* function, const char* argName, const char* assertion)
{
    printf_stderr_common("ARGUMENT BAD: %s, %s\n", argName, assertion);
    printCallSite(file, line, function);
}

class CrashLogPrintStream : public PrintStream {
public:
    WTF_ATTRIBUTE_PRINTF(2, 0)
    void vprintf(const char* format, va_list argList) override
    {
        vprintf_stderr_common(format, argList);
    }
};

void WTFReportBacktrace()
{
    static const int framesToShow = 31;
    static const int framesToSkip = 2;
    void* samples[framesToShow + framesToSkip];
    int frames = framesToShow + framesToSkip;

    WTFGetBacktrace(samples, &frames);
    WTFPrintBacktrace(samples + framesToSkip, frames - framesToSkip);
}

void WTFPrintBacktrace(void** stack, int size)
{
    CrashLogPrintStream out;
    StackTrace stackTrace(stack, size);
    out.print(stackTrace);
}

static WTFCrashHookFunction globalHook = 0;

void WTFSetCrashHook(WTFCrashHookFunction function)
{
    globalHook = function;
}

#if !defined(NDEBUG) || !OS(DARWIN)
void WTFCrash()
{
    if (globalHook)
        globalHook();

    WTFReportBacktrace();
#if OS(WINDOWS) && PLATFORM(JAVA) && defined(_DEBUG)
    ::MessageBoxW(NULL, L"Assert", L"Webnode", MB_OK | MB_TASKMODAL);
#else
#if ASAN_ENABLED
    __builtin_trap();
#else
    *(int *)(uintptr_t)0xbbadbeef = 0;
    // More reliable, but doesn't say BBADBEEF.
#if COMPILER(GCC_OR_CLANG)
    __builtin_trap();
#else
    ((void(*)())0)();
#endif // COMPILER(GCC_OR_CLANG)
#endif // ASAN_ENABLED
#endif
}
#else
// We need to keep WTFCrash() around (even on non-debug OS(DARWIN) builds) as a workaround
// for presently shipping (circa early 2016) SafariForWebKitDevelopment binaries which still
// expects to link to it.
void WTFCrash()
{
    CRASH();
}
#endif // !defined(NDEBUG) || !OS(DARWIN)

void WTFCrashWithSecurityImplication()
{
    CRASH();
}

#if HAVE(SIGNAL_H)
static NO_RETURN void dumpBacktraceSignalHandler(int sig)
{
    WTFReportBacktrace();
    exit(128 + sig);
}

static void installSignalHandlersForFatalErrors(void (*handler)(int))
{
    signal(SIGILL, handler); //    4: illegal instruction (not reset when caught).
    signal(SIGTRAP, handler); //   5: trace trap (not reset when caught).
    signal(SIGFPE, handler); //    8: floating point exception.
    signal(SIGBUS, handler); //   10: bus error.
    signal(SIGSEGV, handler); //  11: segmentation violation.
    signal(SIGSYS, handler); //   12: bad argument to system call.
    signal(SIGPIPE, handler); //  13: write on a pipe with no reader.
    signal(SIGXCPU, handler); //  24: exceeded CPU time limit.
    signal(SIGXFSZ, handler); //  25: exceeded file size limit.
}

static void resetSignalHandlersForFatalErrors()
{
    installSignalHandlersForFatalErrors(SIG_DFL);
}
#endif

void WTFInstallReportBacktraceOnCrashHook()
{
#if HAVE(SIGNAL_H)
    // Needed otherwise we are going to dump the stack trace twice
    // in case we hit an assertion.
    WTFSetCrashHook(&resetSignalHandlersForFatalErrors);
    installSignalHandlersForFatalErrors(&dumpBacktraceSignalHandler);
#endif
}

bool WTFIsDebuggerAttached()
{
#if OS(DARWIN)
    struct kinfo_proc info;
    int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };
    size_t size = sizeof(info);
    if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), &info, &size, nullptr, 0) == -1)
        return false;
    return info.kp_proc.p_flag & P_TRACED;
#else
    return false;
#endif
}

void WTFReportFatalError(const char* file, int line, const char* function, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf_stderr_with_prefix("FATAL ERROR: ", format, args);
    va_end(args);
    printf_stderr_common("\n");
    printCallSite(file, line, function);
}

void WTFReportError(const char* file, int line, const char* function, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf_stderr_with_prefix("ERROR: ", format, args);
    va_end(args);
    printf_stderr_common("\n");
    printCallSite(file, line, function);
}

class WTFLoggingAccumulator {
public:
    void accumulate(const String&);
    void resetAccumulatedLogs();
    String getAndResetAccumulatedLogs();

private:
    Lock accumulatorLock;
    StringBuilder loggingAccumulator;
};

void WTFLoggingAccumulator::accumulate(const String& log)
{
    Locker<Lock> locker(accumulatorLock);
    loggingAccumulator.append(log);
}

void WTFLoggingAccumulator::resetAccumulatedLogs()
{
    Locker<Lock> locker(accumulatorLock);
    loggingAccumulator.clear();
}

String WTFLoggingAccumulator::getAndResetAccumulatedLogs()
{
    Locker<Lock> locker(accumulatorLock);
    String result = loggingAccumulator.toString();
    loggingAccumulator.clear();
    return result;
}

static WTFLoggingAccumulator& loggingAccumulator()
{
    static WTFLoggingAccumulator* accumulator;
    static std::once_flag initializeAccumulatorOnce;
    std::call_once(initializeAccumulatorOnce, [] {
        accumulator = new WTFLoggingAccumulator;
    });

    return *accumulator;
}

void WTFSetLogChannelLevel(WTFLogChannel* channel, WTFLogLevel level)
{
    channel->level = level;
}

bool WTFWillLogWithLevel(WTFLogChannel* channel, WTFLogLevel level)
{
    return channel->level >= level && channel->state != WTFLogChannelOff;
}

void WTFLogWithLevel(WTFLogChannel* channel, WTFLogLevel level, const char* format, ...)
{
    if (level != WTFLogLevelAlways && level > channel->level)
        return;

    if (channel->level != WTFLogLevelAlways && channel->state == WTFLogChannelOff)
        return;

    va_list args;
    va_start(args, format);

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
    WTFLog(channel, format, args);
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

    va_end(args);
}

void WTFLog(WTFLogChannel* channel, const char* format, ...)
{
    if (channel->state == WTFLogChannelOff)
        return;

    if (channel->state == WTFLogChannelOn) {
        va_list args;
        va_start(args, format);
        vprintf_stderr_with_trailing_newline(format, args);
        va_end(args);
        return;
    }

    ASSERT(channel->state == WTFLogChannelOnWithAccumulation);

    va_list args;
    va_start(args, format);

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
    String loggingString = String::format(format, args);
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

    va_end(args);

    if (!loggingString.endsWith('\n'))
        loggingString.append('\n');

    loggingAccumulator().accumulate(loggingString);

    logToStderr(loggingString.utf8().data());
}

void WTFLogVerbose(const char* file, int line, const char* function, WTFLogChannel* channel, const char* format, ...)
{
    if (channel->state != WTFLogChannelOn)
        return;

    va_list args;
    va_start(args, format);

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
    WTFLog(channel, format, args);
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

    va_end(args);

    printCallSite(file, line, function);
}

void WTFLogAlwaysV(const char* format, va_list args)
{
    vprintf_stderr_with_trailing_newline(format, args);
}

void WTFLogAlways(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    WTFLogAlwaysV(format, args);
    va_end(args);
}

void WTFLogAlwaysAndCrash(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    WTFLogAlwaysV(format, args);
    va_end(args);
    CRASH();
}

WTFLogChannel* WTFLogChannelByName(WTFLogChannel* channels[], size_t count, const char* name)
{
    for (size_t i = 0; i < count; ++i) {
        WTFLogChannel* channel = channels[i];
        if (equalIgnoringASCIICase(name, channel->name))
            return channel;
    }

    return 0;
}

static void setStateOfAllChannels(WTFLogChannel* channels[], size_t channelCount, WTFLogChannelState state)
{
    for (size_t i = 0; i < channelCount; ++i)
        channels[i]->state = state;
}

void WTFInitializeLogChannelStatesFromString(WTFLogChannel* channels[], size_t count, const char* logLevel)
{
#if !RELEASE_LOG_DISABLED
    for (size_t i = 0; i < count; ++i) {
        WTFLogChannel* channel = channels[i];
        channel->osLogChannel = os_log_create(channel->subsystem, channel->name);
    }
#endif

    String logLevelString = logLevel;
    Vector<String> components;
    logLevelString.split(',', components);

    for (size_t i = 0; i < components.size(); ++i) {
        Vector<String> componentInfo;
        components[i].split('=', componentInfo);
        String component = componentInfo[0].stripWhiteSpace();

        WTFLogChannelState logChannelState = WTFLogChannelOn;
        if (component.startsWith('-')) {
            logChannelState = WTFLogChannelOff;
            component = component.substring(1);
        }

        if (equalLettersIgnoringASCIICase(component, "all")) {
            setStateOfAllChannels(channels, count, logChannelState);
            continue;
        }

        WTFLogLevel logChannelLevel = WTFLogLevelError;
        if (componentInfo.size() > 1) {
            String level = componentInfo[1].stripWhiteSpace();
            if (equalLettersIgnoringASCIICase(level, "error"))
                logChannelLevel = WTFLogLevelError;
            else if (equalLettersIgnoringASCIICase(level, "warning"))
                logChannelLevel = WTFLogLevelWarning;
            else if (equalLettersIgnoringASCIICase(level, "info"))
                logChannelLevel = WTFLogLevelInfo;
            else if (equalLettersIgnoringASCIICase(level, "debug"))
                logChannelLevel = WTFLogLevelDebug;
            else
                WTFLogAlways("Unknown logging level: %s", level.utf8().data());
        }

        if (WTFLogChannel* channel = WTFLogChannelByName(channels, count, component.utf8().data())) {
            channel->state = logChannelState;
            channel->level = logChannelLevel;
        } else
            WTFLogAlways("Unknown logging channel: %s", component.utf8().data());
    }
}

} // extern "C"

#if OS(DARWIN) && (CPU(X86_64) || CPU(ARM64))
#if CPU(X86_64)
#define STUFF_REGISTER_FOR_CRASH(reg, info) __asm__ volatile ("movq %0, %%" reg : : "r" (static_cast<uint64_t>(info)) : reg)

// This ordering was chosen to be consistent with JSC's JIT asserts. We probably shouldn't change this ordering
// since it would make tooling crash reports much harder. If, for whatever reason, we decide to change the ordering
// here we should update the abortWithuint64_t functions.
#define STUFF_FOR_CRASH_REGISTER1 "r11"
#define STUFF_FOR_CRASH_REGISTER2 "r10"
#define STUFF_FOR_CRASH_REGISTER3 "r9"
#define STUFF_FOR_CRASH_REGISTER4 "r8"
#define STUFF_FOR_CRASH_REGISTER5 "r15"

#elif CPU(ARM64) // CPU(X86_64)
#define STUFF_REGISTER_FOR_CRASH(reg, info) __asm__ volatile ("mov " reg ", %0" : : "r" (static_cast<uint64_t>(info)) : reg)

// See comment above on the ordering.
#define STUFF_FOR_CRASH_REGISTER1 "x16"
#define STUFF_FOR_CRASH_REGISTER2 "x17"
#define STUFF_FOR_CRASH_REGISTER3 "x18"
#define STUFF_FOR_CRASH_REGISTER4 "x19"
#define STUFF_FOR_CRASH_REGISTER5 "x20"

#endif // CPU(ARM64)

void WTFCrashWithInfo(int, const char*, const char*, int, uint64_t reason, uint64_t misc1, uint64_t misc2, uint64_t misc3, uint64_t misc4)
{
    STUFF_REGISTER_FOR_CRASH(STUFF_FOR_CRASH_REGISTER1, reason);
    STUFF_REGISTER_FOR_CRASH(STUFF_FOR_CRASH_REGISTER2, misc1);
    STUFF_REGISTER_FOR_CRASH(STUFF_FOR_CRASH_REGISTER3, misc2);
    STUFF_REGISTER_FOR_CRASH(STUFF_FOR_CRASH_REGISTER4, misc3);
    STUFF_REGISTER_FOR_CRASH(STUFF_FOR_CRASH_REGISTER5, misc4);
    CRASH();
}

void WTFCrashWithInfo(int, const char*, const char*, int, uint64_t reason, uint64_t misc1, uint64_t misc2, uint64_t misc3)
{
    STUFF_REGISTER_FOR_CRASH(STUFF_FOR_CRASH_REGISTER1, reason);
    STUFF_REGISTER_FOR_CRASH(STUFF_FOR_CRASH_REGISTER2, misc1);
    STUFF_REGISTER_FOR_CRASH(STUFF_FOR_CRASH_REGISTER3, misc2);
    STUFF_REGISTER_FOR_CRASH(STUFF_FOR_CRASH_REGISTER4, misc3);
    CRASH();
}

void WTFCrashWithInfo(int, const char*, const char*, int, uint64_t reason, uint64_t misc1, uint64_t misc2)
{
    STUFF_REGISTER_FOR_CRASH(STUFF_FOR_CRASH_REGISTER1, reason);
    STUFF_REGISTER_FOR_CRASH(STUFF_FOR_CRASH_REGISTER2, misc1);
    STUFF_REGISTER_FOR_CRASH(STUFF_FOR_CRASH_REGISTER3, misc2);
    CRASH();
}

void WTFCrashWithInfo(int, const char*, const char*, int, uint64_t reason, uint64_t misc1)
{
    STUFF_REGISTER_FOR_CRASH(STUFF_FOR_CRASH_REGISTER1, reason);
    STUFF_REGISTER_FOR_CRASH(STUFF_FOR_CRASH_REGISTER2, misc1);
    CRASH();
}

void WTFCrashWithInfo(int, const char*, const char*, int, uint64_t reason)
{
    STUFF_REGISTER_FOR_CRASH(STUFF_FOR_CRASH_REGISTER1, reason);
    CRASH();
}

void WTFCrashWithInfo(int, const char*, const char*, int)
{
    CRASH();
}

#else

void WTFCrashWithInfo(int, const char*, const char*, int, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) { CRASH(); }
void WTFCrashWithInfo(int, const char*, const char*, int, uint64_t, uint64_t, uint64_t, uint64_t) { CRASH(); }
void WTFCrashWithInfo(int, const char*, const char*, int, uint64_t, uint64_t, uint64_t) { CRASH(); }
void WTFCrashWithInfo(int, const char*, const char*, int, uint64_t, uint64_t) { CRASH(); }
void WTFCrashWithInfo(int, const char*, const char*, int, uint64_t) { CRASH(); }
void WTFCrashWithInfo(int, const char*, const char*, int) { CRASH(); }

#endif // OS(DARWIN) && (CPU(X64_64) || CPU(ARM64))

namespace WTF {

void resetAccumulatedLogs()
{
    loggingAccumulator().resetAccumulatedLogs();
}

String getAndResetAccumulatedLogs()
{
    return loggingAccumulator().getAndResetAccumulatedLogs();
}

} // namespace WTF
