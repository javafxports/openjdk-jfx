/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
 */
#ifndef JavaEnv_h
#define JavaEnv_h

#include <jni.h>

extern JavaVM* jvm;

JNIEnv* JNICALL DumpRenderTree_GetJavaEnv();

jclass getDumpRenderTreeClass();
jmethodID getWaitUntillDoneMethodId();
jmethodID getNotifyDoneMID();
jmethodID getOverridePreferenceMID();
jmethodID getGetBackForwardItemCountMID();
jmethodID getClearBackForwardListMID();
jmethodID getResolveURLMID();
jmethodID getLoadURLMID();
jmethodID getGoBackForward();


bool CheckAndClearException(JNIEnv* env);

#define jlong_to_ptr(a) ((void*)(uintptr_t)(a))
#define ptr_to_jlong(a) ((jlong)(uintptr_t)(a))

#define bool_to_jbool(a) ((a) ? JNI_TRUE : JNI_FALSE)
#define jbool_to_bool(a) (((a) == JNI_TRUE) ? true : false)

#endif // JavaEnv_h
