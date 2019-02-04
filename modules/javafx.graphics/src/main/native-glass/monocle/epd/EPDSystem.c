/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

// Implementation of native methods in EPDSystem.java

#include <sys/ioctl.h>  // For ioctl
#include <sys/types.h>  // For uint

#include "com_sun_glass_ui_monocle_EPDSystem.h"
#include "com_sun_glass_ui_monocle_EPDSystem_IntStructure.h"
#include "com_sun_glass_ui_monocle_EPDSystem_FbVarScreenInfo.h"
#include "com_sun_glass_ui_monocle_EPDSystem_MxcfbWaveformModes.h"
#include "com_sun_glass_ui_monocle_EPDSystem_MxcfbUpdateData.h"

#include "Monocle.h"
#include "mxcfb.h"

// EPDSystem

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_ioctl
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong fd, jint request, jint value) {
    return ioctl((int) fd, (int) request, (__u32 *) & value);
}

// EPDSystem.IntStructure

struct integer {
    int value;
};

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024IntStructure_sizeof
(JNIEnv *UNUSED(env), jobject UNUSED(object)) {
    return (jint) sizeof (struct integer);
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024IntStructure_getInteger
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct integer *) asPtr(p))->value;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024IntStructure_setInteger
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint value) {
    struct integer *ptr = (struct integer *) asPtr(p);
    ptr->value = (int) value;
}

// EPDSystem.MxcfbWaveformModes

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbWaveformModes_sizeof
(JNIEnv *UNUSED(env), jobject UNUSED(object)) {
    return (jint) sizeof (struct mxcfb_waveform_modes);
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbWaveformModes_getModeInit
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_waveform_modes *) asPtr(p))->mode_init;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbWaveformModes_getModeDu
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_waveform_modes *) asPtr(p))->mode_du;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbWaveformModes_getModeGc4
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_waveform_modes *) asPtr(p))->mode_gc4;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbWaveformModes_getModeGc8
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_waveform_modes *) asPtr(p))->mode_gc8;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbWaveformModes_getModeGc16
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_waveform_modes *) asPtr(p))->mode_gc16;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbWaveformModes_getModeGc32
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_waveform_modes *) asPtr(p))->mode_gc32;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbWaveformModes_setModes
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint init, jint du, jint gc4, jint gc8, jint gc16, jint gc32) {
    struct mxcfb_waveform_modes *ptr = (struct mxcfb_waveform_modes *) asPtr(p);
    ptr->mode_init = (int) init;
    ptr->mode_du = (int) du;
    ptr->mode_gc4 = (int) gc4;
    ptr->mode_gc8 = (int) gc8;
    ptr->mode_gc16 = (int) gc16;
    ptr->mode_gc32 = (int) gc32;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbWaveformModes_print
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    struct mxcfb_waveform_modes *ptr = (struct mxcfb_waveform_modes *) asPtr(p);
    printf("mxcfb_waveform_modes (%u bytes at %p)\n", sizeof *ptr, ptr);
    printf("    mode_init = %d\n", ptr->mode_init);
    printf("    mode_du = %d\n", ptr->mode_du);
    printf("    mode_gc4 = %d\n", ptr->mode_gc4);
    printf("    mode_gc8 = %d\n", ptr->mode_gc8);
    printf("    mode_gc16 = %d\n", ptr->mode_gc16);
    printf("    mode_gc32 = %d\n", ptr->mode_gc32);
}

// EPDSystem.MxcfbUpdateData

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_sizeof
(JNIEnv *UNUSED(env), jobject UNUSED(object)) {
    return (jint) sizeof (struct mxcfb_update_data);
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_getUpdateRegionTop
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_update_data *) asPtr(p))->update_region.top;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_getUpdateRegionLeft
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_update_data *) asPtr(p))->update_region.left;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_getUpdateRegionWidth
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_update_data *) asPtr(p))->update_region.width;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_getUpdateRegionHeight
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_update_data *) asPtr(p))->update_region.height;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_getWaveformMode
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_update_data *) asPtr(p))->waveform_mode;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_getUpdateMode
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_update_data *) asPtr(p))->update_mode;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_getUpdateMarker
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_update_data *) asPtr(p))->update_marker;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_getTemp
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_update_data *) asPtr(p))->temp;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_getFlags
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_update_data *) asPtr(p))->flags;
}

JNIEXPORT jlong JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_getAltBufferDataVirtAddr
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return asJLong(((struct mxcfb_update_data *) asPtr(p))->alt_buffer_data.virt_addr);
}

JNIEXPORT jlong JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_getAltBufferDataPhysAddr
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jlong) ((struct mxcfb_update_data *) asPtr(p))->alt_buffer_data.phys_addr;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_getAltBufferDataWidth
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_update_data *) asPtr(p))->alt_buffer_data.width;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_getAltBufferDataHeight
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_update_data *) asPtr(p))->alt_buffer_data.height;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_getAltBufferDataUpdateRegionTop
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_update_data *) asPtr(p))->alt_buffer_data.alt_update_region.top;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_getAltBufferDataUpdateRegionLeft
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_update_data *) asPtr(p))->alt_buffer_data.alt_update_region.left;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_getAltBufferDataUpdateRegionWidth
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_update_data *) asPtr(p))->alt_buffer_data.alt_update_region.width;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_getAltBufferDataUpdateRegionHeight
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct mxcfb_update_data *) asPtr(p))->alt_buffer_data.alt_update_region.height;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_setUpdateRegion
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint top, jint left, jint width, jint height) {
    struct mxcfb_update_data *ptr = (struct mxcfb_update_data *) asPtr(p);
    ptr->update_region.top = (__u32) top;
    ptr->update_region.left = (__u32) left;
    ptr->update_region.width = (__u32) width;
    ptr->update_region.height = (__u32) height;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_setWaveformMode
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint mode) {
    struct mxcfb_update_data *ptr = (struct mxcfb_update_data *) asPtr(p);
    ptr->waveform_mode = (__u32) mode;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_setUpdateMode
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint mode) {
    struct mxcfb_update_data *ptr = (struct mxcfb_update_data *) asPtr(p);
    ptr->update_mode = (__u32) mode;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_setUpdateMarker
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint marker) {
    struct mxcfb_update_data *ptr = (struct mxcfb_update_data *) asPtr(p);
    ptr->update_marker = (__u32) marker;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_setTemp
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint temp) {
    struct mxcfb_update_data *ptr = (struct mxcfb_update_data *) asPtr(p);
    ptr->temp = (int) temp;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_setFlags
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint flags) {
    struct mxcfb_update_data *ptr = (struct mxcfb_update_data *) asPtr(p);
    ptr->flags = (uint) flags;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_setAltBufferData
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jlong virtAddr, jlong physAddr, jint width, jint height,
        jint updateRegionTop, jint updateRegionLeft, jint updateRegionWidth, jint updateRegionHeight) {
    struct mxcfb_update_data *ptr = (struct mxcfb_update_data *) asPtr(p);
    ptr->alt_buffer_data.virt_addr = asPtr(virtAddr);
    ptr->alt_buffer_data.phys_addr = (__u32) physAddr;
    ptr->alt_buffer_data.width = (__u32) width;
    ptr->alt_buffer_data.height = (__u32) height;
    ptr->alt_buffer_data.alt_update_region.top = (__u32) updateRegionTop;
    ptr->alt_buffer_data.alt_update_region.left = (__u32) updateRegionLeft;
    ptr->alt_buffer_data.alt_update_region.width = (__u32) updateRegionWidth;
    ptr->alt_buffer_data.alt_update_region.height = (__u32) updateRegionHeight;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024MxcfbUpdateData_print
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    struct mxcfb_update_data *ptr = (struct mxcfb_update_data *) asPtr(p);

    printf("mxcfb_update_data (%u bytes at %p)\n", sizeof *ptr, ptr);
    printf("    update_region.top = %u\n", ptr->update_region.top);
    printf("    update_region.left = %u\n", ptr->update_region.left);
    printf("    update_region.width = %u\n", ptr->update_region.width);
    printf("    update_region.height = %u\n", ptr->update_region.height);

    printf("    waveform_mode = %u\n", ptr->waveform_mode);
    printf("    update_mode = %u\n", ptr->update_mode);
    printf("    update_marker = %u\n", ptr->update_marker);
    printf("    temp = %d\n", ptr->temp);
    printf("    flags = %u\n", ptr->flags);

    printf("    alt_buffer_data.virt_addr = %p\n", ptr->alt_buffer_data.virt_addr);
    printf("    alt_buffer_data.phys_addr = %u\n", ptr->alt_buffer_data.phys_addr);
    printf("    alt_buffer_data.width = %u\n", ptr->alt_buffer_data.width);
    printf("    alt_buffer_data.height = %u\n", ptr->alt_buffer_data.height);
    printf("    alt_buffer_data.alt_update_region.top = %u\n", ptr->alt_buffer_data.alt_update_region.top);
    printf("    alt_buffer_data.alt_update_region.left = %u\n", ptr->alt_buffer_data.alt_update_region.left);
    printf("    alt_buffer_data.alt_update_region.width = %u\n", ptr->alt_buffer_data.alt_update_region.width);
    printf("    alt_buffer_data.alt_update_region.height = %u\n", ptr->alt_buffer_data.alt_update_region.height);
}

// EPDSystem.FbVarScreenInfo

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getGrayscale
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->grayscale;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getRedOffset
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->red.offset;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getRedLength
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->red.length;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getRedMsbRight
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->red.msb_right;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getGreenOffset
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->green.offset;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getGreenLength
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->green.length;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getGreenMsbRight
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->green.msb_right;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getBlueOffset
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->blue.offset;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getBlueLength
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->blue.length;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getBlueMsbRight
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->blue.msb_right;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getTranspOffset
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->transp.offset;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getTranspLength
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->transp.length;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getTranspMsbRight
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->transp.msb_right;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getNonstd
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->nonstd;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getActivate
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->activate;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getHeight
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->height;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getWidth
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->width;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getAccelFlags
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->accel_flags;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getPixclock
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->pixclock;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getLeftMargin
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->left_margin;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getRightMargin
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->right_margin;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getUpperMargin
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->upper_margin;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getLowerMargin
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->lower_margin;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getHsyncLen
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->hsync_len;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getVsyncLen
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->vsync_len;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getSync
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->sync;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getVmode
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->vmode;
}

JNIEXPORT jint JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_getRotate
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p) {
    return (jint) ((struct fb_var_screeninfo *) asPtr(p))->rotate;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_setGrayscale
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint grayscale) {
    struct fb_var_screeninfo *ptr = (struct fb_var_screeninfo *) asPtr(p);
    ptr->grayscale = grayscale;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_setNonstd
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint nonstd) {
    struct fb_var_screeninfo *ptr = (struct fb_var_screeninfo *) asPtr(p);
    ptr->nonstd = nonstd;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_setHeight
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint height) {
    struct fb_var_screeninfo *ptr = (struct fb_var_screeninfo *) asPtr(p);
    ptr->height = height;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_setWidth
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint width) {
    struct fb_var_screeninfo *ptr = (struct fb_var_screeninfo *) asPtr(p);
    ptr->width = width;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_setAccelFlags
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint accelFlags) {
    struct fb_var_screeninfo *ptr = (struct fb_var_screeninfo *) asPtr(p);
    ptr->accel_flags = accelFlags;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_setPixclock
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint pixclock) {
    struct fb_var_screeninfo *ptr = (struct fb_var_screeninfo *) asPtr(p);
    ptr->pixclock = pixclock;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_setLeftMargin
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint leftMargin) {
    struct fb_var_screeninfo *ptr = (struct fb_var_screeninfo *) asPtr(p);
    ptr->left_margin = leftMargin;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_setRightMargin
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint rightMargin) {
    struct fb_var_screeninfo *ptr = (struct fb_var_screeninfo *) asPtr(p);
    ptr->right_margin = rightMargin;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_setUpperMargin
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint upperMargin) {
    struct fb_var_screeninfo *ptr = (struct fb_var_screeninfo *) asPtr(p);
    ptr->upper_margin = upperMargin;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_setLowerMargin
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint lowerMargin) {
    struct fb_var_screeninfo *ptr = (struct fb_var_screeninfo *) asPtr(p);
    ptr->lower_margin = lowerMargin;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_setHsyncLen
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint hsyncLen) {
    struct fb_var_screeninfo *ptr = (struct fb_var_screeninfo *) asPtr(p);
    ptr->hsync_len = hsyncLen;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_setVsyncLen
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint vsyncLen) {
    struct fb_var_screeninfo *ptr = (struct fb_var_screeninfo *) asPtr(p);
    ptr->vsync_len = vsyncLen;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_setSync
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint sync) {
    struct fb_var_screeninfo *ptr = (struct fb_var_screeninfo *) asPtr(p);
    ptr->sync = sync;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_setVmode
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint vmode) {
    struct fb_var_screeninfo *ptr = (struct fb_var_screeninfo *) asPtr(p);
    ptr->vmode = vmode;
}

JNIEXPORT void JNICALL Java_com_sun_glass_ui_monocle_EPDSystem_00024FbVarScreenInfo_setRotate
(JNIEnv *UNUSED(env), jobject UNUSED(object), jlong p, jint rotate) {
    struct fb_var_screeninfo *ptr = (struct fb_var_screeninfo *) asPtr(p);
    ptr->rotate = rotate;
}
