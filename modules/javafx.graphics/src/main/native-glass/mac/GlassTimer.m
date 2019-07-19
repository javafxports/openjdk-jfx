/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#import "common.h"
#import "com_sun_glass_ui_mac_MacTimer.h"

#import "com_sun_glass_ui_Timer.h"

#import "ProcessInfo.h"
#import "GlassMacros.h"
#import "GlassHelper.h"
#import "GlassTimer.h"

#include <sys/time.h>

//#define VERBOSE
#ifndef VERBOSE
    #define LOG(MSG, ...)
#else
    #define LOG(MSG, ...) GLASS_LOG(MSG, ## __VA_ARGS__);
#endif

#define HIGH_PRIORITY_TIMER         1

static void *_GlassTimerTask(void *data)
{
    pthread_setname_np("Glass Timer Thread");

    GlassTimer *timer = (GlassTimer*)data;
    if (timer != NULL)
    {
        jint error = (*MAIN_JVM)->AttachCurrentThreadAsDaemon(MAIN_JVM, (void **)&timer->_env, NULL);
        if (error == 0)
        {
            timer->_running = YES;
            while (timer->_running == YES)
            {
                int64_t now = getTimeMicroseconds();
                if (timer->_paused == NO)
                {
                    if (timer->_runnable != NULL)
                    {
                        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
                        {
                            (*timer->_env)->CallVoidMethod(timer->_env, timer->_runnable, jRunnableRun);
                        }
                        [pool drain];
                        GLASS_CHECK_EXCEPTION(timer->_env);
                    }
                }
                int64_t duration = (getTimeMicroseconds() - now);
                int64_t sleep = (timer->_period - duration);
                if ((sleep < 0) || (sleep > timer->_period))
                {
#if 1
                    sched_yield();
#else
                    pthread_yield_np();
#endif
                }
                else
                {
                    usleep((useconds_t)sleep);
                }
            }

            if (timer->_runnable != NULL)
            {
                (*timer->_env)->DeleteGlobalRef(timer->_env, timer->_runnable);
            }
            timer->_runnable = NULL;

            NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
            {
                [timer release];
            }
            [pool drain];

            (*MAIN_JVM)->DetachCurrentThread(MAIN_JVM);
        }
        else
        {
            NSLog(@"ERROR: Glass could not attach Timer _thread to VM, result:%d\n", (int)error);
        }
    }

    return NULL;
}

JavaVMAttachArgs attachArgs = {JNI_VERSION_1_2, "PulseTimer-CVDisplayLink thread", NULL};

CVReturn CVOutputCallback(CVDisplayLinkRef displayLink,
                          const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime,
                          CVOptionFlags flagsIn, CVOptionFlags *flagsOut,
                          void *displayLinkContext) {

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    if (displayLinkContext != NULL)
    {
        GlassTimer *timer = (GlassTimer*)displayLinkContext;

        // CVDisplayLinkStart() starts a new CVDisplayLink thread. This thread
        // should be attached to JVM. Cocoa changes the thread when a new display
        // resolution is selected so we need to make sure that we are able to
        // call into Java. Attaching the same thread multiple times does nothing.
        // As the thread is attached as a daemon, it will not stop Java from exiting.

        jint ret = (*MAIN_JVM)->AttachCurrentThreadAsDaemon(MAIN_JVM, (void **)&timer->_env, (void*)&attachArgs);
        if (ret == 0)
        {
            if (timer->_runnable != NULL)
            {
                (*timer->_env)->CallVoidMethod(timer->_env, timer->_runnable, jRunnableRun);
            }

            if (timer->_paused == YES)
            {
                // _1pause() calls CVDisplayLinkStop() on current CVDisplayLink thread, which
                // in turn stops the thread. Hence the thread should be detached.

                ret = (*MAIN_JVM)->DetachCurrentThread(MAIN_JVM);
                if (ret != JNI_OK)
                {
                    NSLog(@"ERROR: CVOutputCallback(): Glass could not detach CVDisplayLink _thread to VM, result:%d\n", (int)ret);
                }
            }
        } else {
            NSLog(@"ERROR: Glass could not attach CVDisplayLink _thread to VM, result:%d\n", (int)ret);
        }
    }

    [pool release];
    return kCVReturnSuccess;
}


@implementation GlassTimer

- (id)initWithRunnable:(jobject)runnable withEnv:(JNIEnv*)env
{
    self = [super init];
    if (self != nil)
    {
        self->_paused = NO;
        CVReturn err = CVDisplayLinkSetOutputCallback(GlassDisplayLink, &CVOutputCallback, self);
        if (err != kCVReturnSuccess)
        {
            NSLog(@"CVDisplayLinkSetOutputCallback error: %d", err);
        }
        else
        {
            self->_runnable = (*env)->NewGlobalRef(env, runnable);
        }
    }
    return self;
}

- (id)initWithRunnable:(jobject)runnable withEnv:(JNIEnv*)env withPeriod:(jint)period
{
    self = [super init];
    if (self != nil)
    {
        self->_thread = NULL;
        self->_running = NO;
        self->_paused = NO;
        self->_runnable = NULL;
        self->_period = (1000 * period); // ms --> us

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

#if HIGH_PRIORITY_TIMER
        struct sched_param param;
        memset(&param, 0x00, sizeof(param));
        param.sched_priority = sched_get_priority_max(SCHED_FIFO)-1; // notice: 2nd highest priority (gznote: do we really want that?)
        pthread_attr_setschedparam(&attr, &param);
#endif

        int err = pthread_create(&self->_thread, &attr, _GlassTimerTask, self);
        if (err == 0)
        {
            // detach thread, so its resources can be reclaimed as soon as it's finished
            pthread_detach(self->_thread);

            self->_runnable = (*env)->NewGlobalRef(env, runnable);
        }
        else
        {
            NSLog(@"GlassTimer error: pthread_create returned %d", err);

            self = nil;
        }
    }
    return self;
}

@end

/*
 * Class:     com_sun_glass_ui_mac_MacTimer
 * Method:    _start
 * Signature: (Ljava/lang/Runnable;I)J
 */
JNIEXPORT jlong JNICALL
Java_com_sun_glass_ui_mac_MacTimer__1start__Ljava_lang_Runnable_2I(JNIEnv *env, jobject jThis,
                                                                   jobject jRunnable, jint jPeriod)
{
    LOG("Java_com_sun_glass_ui_mac_MacTimer__1start__Ljava_lang_Runnable_2I");

    jlong jTimerPtr = 0L;

    GLASS_ASSERT_MAIN_JAVA_THREAD(env);
    GLASS_POOL_ENTER;
    {
        jTimerPtr = ptr_to_jlong([[GlassTimer alloc] initWithRunnable:jRunnable withEnv:env withPeriod:jPeriod]);
    }
    GLASS_POOL_EXIT;
    GLASS_CHECK_EXCEPTION(env);

    return jTimerPtr;
}

/*
 * Class:     com_sun_glass_ui_mac_MacTimer
 * Method:    _start
 * Signature: (Ljava/lang/Runnable;)J
 */
JNIEXPORT jlong JNICALL Java_com_sun_glass_ui_mac_MacTimer__1start__Ljava_lang_Runnable_2
(JNIEnv *env, jobject jThis, jobject jRunnable)
{
    LOG("Java_com_sun_glass_ui_mac_MacTimer__1start__Ljava_lang_Runnable_2");

    jlong jTimerPtr = 0L;

    GLASS_ASSERT_MAIN_JAVA_THREAD(env);
    GLASS_POOL_ENTER;
    {
        jTimerPtr = ptr_to_jlong([[GlassTimer alloc] initWithRunnable:jRunnable withEnv:env]);
    }
    GLASS_POOL_EXIT;
    GLASS_CHECK_EXCEPTION(env);

    return jTimerPtr;
}

/*
 * Class:     com_sun_glass_ui_mac_MacTimer
 * Method:    _stop
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_sun_glass_ui_mac_MacTimer__1stop
(JNIEnv * env, jobject jRunnable, jlong jTimerPtr)
{
    LOG("Java_com_sun_glass_ui_mac_MacTimer__1stop");

    GLASS_ASSERT_MAIN_JAVA_THREAD(env);
    GLASS_POOL_ENTER;
    {
        GlassTimer *timer = (GlassTimer*)jlong_to_ptr(jTimerPtr);
        if (timer->_period == 0)
        {
            // stop the display link
            if (GlassDisplayLink != NULL && CVDisplayLinkIsRunning(GlassDisplayLink)) {
                CVDisplayLinkStop(GlassDisplayLink);
                CVDisplayLinkRelease(GlassDisplayLink);
            }
        }
        else
        {
            timer->_running = NO;
        }
    }
    GLASS_POOL_EXIT;
    GLASS_CHECK_EXCEPTION(env);
}

/*
 * Class:     com_sun_glass_ui_mac_MacTimer
 * Method:    _pause
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_sun_glass_ui_mac_MacTimer__1pause
(JNIEnv *env, jclass cls, jlong jTimerPtr)
{
    LOG("Java_com_sun_glass_ui_mac_MacTimer__1pause");

    GLASS_ASSERT_MAIN_JAVA_THREAD(env);
    GLASS_POOL_ENTER;
    {
        GlassTimer *timer = (GlassTimer*)jlong_to_ptr(jTimerPtr);
        if (timer->_period == 0)
        {
            // Stop the CVDisplayLink thread. There is no API to pause the CVDisplayLink
            // thread. CVDisplayLinkStop() stops the CVDisplayLink thread.
            if (GlassDisplayLink != NULL && CVDisplayLinkIsRunning(GlassDisplayLink))
            {
                CVReturn ret = CVDisplayLinkStop(GlassDisplayLink);
                if (ret == kCVReturnSuccess)
                {
                    timer->_paused = YES;
                }
                else
                {
                    NSLog(@"ERROR: MacTimer__1pause(): CVDisplayLinkStop() error: %d\n", (int)ret);
                }
            }
            else
            {
                NSLog(@"ERROR: MacTimer__1pause(): GlassDisplayLink is NULL or GlassDisplayLink is not running.");
            }
        }
        else
        {
            timer->_paused = YES;
        }
    }
    GLASS_POOL_EXIT;
    GLASS_CHECK_EXCEPTION(env);
}


/*
 * Class:     com_sun_glass_ui_mac_MacTimer
 * Method:    _resume
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_sun_glass_ui_mac_MacTimer__1resume
(JNIEnv *env, jclass cls, jlong jTimerPtr)
{
    LOG("Java_com_sun_glass_ui_mac_MacTimer__1resume");

    GLASS_ASSERT_MAIN_JAVA_THREAD(env);
    GLASS_POOL_ENTER;
    {
        GlassTimer *timer = (GlassTimer*)jlong_to_ptr(jTimerPtr);
        if (timer->_period == 0)
        {
            // Start the CVDisplayLink thread. There is no API to resume the CVDisplayLink
            // thread. CVDisplayLinkStart() starts a new CVDisplayLink thread.
            if (GlassDisplayLink != NULL && !CVDisplayLinkIsRunning(GlassDisplayLink))
            {
                CVReturn ret = CVDisplayLinkStart(GlassDisplayLink);
                if (ret == kCVReturnSuccess)
                {
                    timer->_paused = NO;
                }
                else
                {
                    NSLog(@"ERROR: MacTimer__1resume(): CVDisplayLinkStart error: %d\n", (int)ret);
                }
            }
        }
        else
        {
            timer->_paused = NO;
        }
    }
    GLASS_POOL_EXIT;
    GLASS_CHECK_EXCEPTION(env);
}

/*
 * Class:     com_sun_glass_ui_mac_MacTimer
 * Method:    _getMinPeriod
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_sun_glass_ui_mac_MacTimer__1getMinPeriod
(JNIEnv * env, jclass cls)
{
    LOG("Java_com_sun_glass_ui_mac_MacTimer__1getMinPeriod");

    int period = 0;

    GLASS_ASSERT_MAIN_JAVA_THREAD(env);
    GLASS_POOL_ENTER;
    {
        period = 0; // in ms
    }
    GLASS_POOL_EXIT;
    GLASS_CHECK_EXCEPTION(env);

    return period;
}

/*
 * Class:     com_sun_glass_ui_mac_MacTimer
 * Method:    _getMaxPeriod
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_sun_glass_ui_mac_MacTimer__1getMaxPeriod
(JNIEnv * env, jclass cls)
{
    LOG("Java_com_sun_glass_ui_mac_MacTimer__1getMaxPeriod");

    int period = 0;

    GLASS_ASSERT_MAIN_JAVA_THREAD(env);
    GLASS_POOL_ENTER;
    {
        period = 1000000; // in ms (100 sec)
    }
    GLASS_POOL_EXIT;
    GLASS_CHECK_EXCEPTION(env);

    return period;
}

/*
 * Class:     com_sun_glass_ui_mac_MacTimer
 * Method:    _initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_sun_glass_ui_mac_MacTimer__1initIDs
(JNIEnv *env, jclass jClass)
{

    initJavaIDsList(env);

    if (GlassDisplayLink == NULL)
    {
        CVReturn err = CVDisplayLinkCreateWithActiveCGDisplays(&GlassDisplayLink);
        if (err != kCVReturnSuccess)
        {
            NSLog(@"CVDisplayLinkCreateWithActiveCGDisplays error: %d", err);
        }
        err = CVDisplayLinkSetCurrentCGDisplay(GlassDisplayLink, kCGDirectMainDisplay);
        if (err != kCVReturnSuccess)
        {
            NSLog(@"CVDisplayLinkSetCurrentCGDisplay error: %d", err);
        }
        /*
         * set a null callback and start the link to prep for GlassTimer initialization
         */
        err = CVDisplayLinkSetOutputCallback(GlassDisplayLink, &CVOutputCallback, NULL);
        if (err != kCVReturnSuccess)
        {
            NSLog(@"CVDisplayLinkSetOutputCallback error: %d", err);
        }
        err = CVDisplayLinkStart(GlassDisplayLink);
        if (err != kCVReturnSuccess)
        {
            NSLog(@"CVDisplayLinkStart error: %d", err);
        }
    }
}

