/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2013-2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

// A class that stores static enablers for all experimental features. Note that
// the method names must line up with the JavaScript method they enable for code
// generation to work properly.

class RuntimeEnabledFeatures {
    WTF_MAKE_NONCOPYABLE(RuntimeEnabledFeatures);
public:
    void setDisplayContentsEnabled(bool isEnabled) { m_isDisplayContentsEnabled = isEnabled; }
    bool displayContentsEnabled() const { return m_isDisplayContentsEnabled; }

    void setLinkPreloadEnabled(bool isEnabled) { m_isLinkPreloadEnabled = isEnabled; }
    bool linkPreloadEnabled() const { return m_isLinkPreloadEnabled; }

    void setLinkPrefetchEnabled(bool isEnabled) { m_isLinkPrefetchEnabled = isEnabled; }
    bool linkPrefetchEnabled() const { return m_isLinkPrefetchEnabled; }

    void setMediaPreloadingEnabled(bool isEnabled) { m_isMediaPreloadingEnabled = isEnabled; }
    bool mediaPreloadingEnabled() const { return m_isMediaPreloadingEnabled; }

    void setResourceTimingEnabled(bool isEnabled) { m_isResourceTimingEnabled = isEnabled; }
    bool resourceTimingEnabled() const { return m_isResourceTimingEnabled; }

    void setUserTimingEnabled(bool isEnabled) { m_isUserTimingEnabled = isEnabled; }
    bool userTimingEnabled() const { return m_isUserTimingEnabled; }

    bool performanceTimelineEnabled() const { return resourceTimingEnabled() || userTimingEnabled(); }

    void setShadowDOMEnabled(bool isEnabled) { m_isShadowDOMEnabled = isEnabled; }
    bool shadowDOMEnabled() const { return m_isShadowDOMEnabled; }

    void setInputEventsEnabled(bool isEnabled) { m_inputEventsEnabled = isEnabled; }
    bool inputEventsEnabled() const { return m_inputEventsEnabled; }

    void setInteractiveFormValidationEnabled(bool isEnabled) { m_isInteractiveFormValidationEnabled = isEnabled; }
    bool interactiveFormValidationEnabled() const { return m_isInteractiveFormValidationEnabled; }

    void setCustomElementsEnabled(bool areEnabled) { m_areCustomElementsEnabled = areEnabled; }
    bool customElementsEnabled() const { return m_areCustomElementsEnabled; }

    void setMenuItemElementEnabled(bool isEnabled) { m_isMenuItemElementEnabled = isEnabled; }
    bool menuItemElementEnabled() const { return m_isMenuItemElementEnabled; }

    void setDirectoryUploadEnabled(bool isEnabled) { m_isDirectoryUploadEnabled = isEnabled; }
    bool directoryUploadEnabled() const { return m_isDirectoryUploadEnabled; }

    void setDataTransferItemsEnabled(bool areEnabled) { m_areDataTransferItemsEnabled = areEnabled; }
    bool dataTransferItemsEnabled() const { return m_areDataTransferItemsEnabled; }

    void setCustomPasteboardDataEnabled(bool isEnabled) { m_isCustomPasteboardDataEnabled = isEnabled; }
    bool customPasteboardDataEnabled() const { return m_isCustomPasteboardDataEnabled; }

#if ENABLE(ATTACHMENT_ELEMENT)
    void setAttachmentElementEnabled(bool areEnabled) { m_isAttachmentElementEnabled = areEnabled; }
    bool attachmentElementEnabled() const { return m_isAttachmentElementEnabled; }
#endif

    void setModernMediaControlsEnabled(bool areEnabled) { m_areModernMediaControlsEnabled = areEnabled; }
    bool modernMediaControlsEnabled() const { return m_areModernMediaControlsEnabled; }

    void setWebAuthenticationEnabled(bool isEnabled) { m_isWebAuthenticationEnabled = isEnabled; }
    bool webAuthenticationEnabled() const { return m_isWebAuthenticationEnabled; }

    void setIsSecureContextAttributeEnabled(bool isEnabled) { m_isSecureContextAttributeEnabled = isEnabled; }
    bool isSecureContextAttributeEnabled() const { return m_isSecureContextAttributeEnabled; }

#if ENABLE(INDEXED_DATABASE_IN_WORKERS)
    void setIndexedDBWorkersEnabled(bool isEnabled) { m_isIndexedDBWorkersEnabled = isEnabled; }
    bool indexedDBWorkersEnabled() const { return m_isIndexedDBWorkersEnabled; }
#endif

#if ENABLE(MEDIA_STREAM)
    bool mediaDevicesEnabled() const { return m_isMediaDevicesEnabled; }
    void setMediaDevicesEnabled(bool isEnabled) { m_isMediaDevicesEnabled = isEnabled; }
    bool mediaStreamEnabled() const { return m_isMediaStreamEnabled; }
    void setMediaStreamEnabled(bool isEnabled) { m_isMediaStreamEnabled = isEnabled; }
    bool screenCaptureEnabled() const { return m_isScreenCaptureEnabled; }
    void setScreenCaptureEnabled(bool isEnabled) { m_isScreenCaptureEnabled = isEnabled; }
#endif

#if ENABLE(WEB_RTC)
    bool peerConnectionEnabled() const { return m_isPeerConnectionEnabled; }
    void setPeerConnectionEnabled(bool isEnabled) { m_isPeerConnectionEnabled = isEnabled; }
    bool webRTCLegacyAPIEnabled() const { return m_webRTCLegacyAPIEnabled; }
    void setWebRTCLegacyAPIEnabled(bool isEnabled) { m_webRTCLegacyAPIEnabled = isEnabled; }
    bool mdnsICECandidatesEnabled() const { return m_mdnsICECandidatesEnabled; }
    void setMDNSICECandidatesEnabled(bool isEnabled) { m_mdnsICECandidatesEnabled = isEnabled; }
#endif

#if ENABLE(LEGACY_CSS_VENDOR_PREFIXES)
    void setLegacyCSSVendorPrefixesEnabled(bool isEnabled) { m_isLegacyCSSVendorPrefixesEnabled = isEnabled; }
    bool legacyCSSVendorPrefixesEnabled() const { return m_isLegacyCSSVendorPrefixesEnabled; }
#endif

#if ENABLE(INPUT_TYPE_DATE)
    bool inputTypeDateEnabled() const { return m_isInputTypeDateEnabled; }
    void setInputTypeDateEnabled(bool isEnabled) { m_isInputTypeDateEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_DATETIME_INCOMPLETE)
    bool inputTypeDateTimeEnabled() const { return m_isInputTypeDateTimeEnabled; }
    void setInputTypeDateTimeEnabled(bool isEnabled) { m_isInputTypeDateTimeEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_DATETIMELOCAL)
    bool inputTypeDateTimeLocalEnabled() const { return m_isInputTypeDateTimeLocalEnabled; }
    void setInputTypeDateTimeLocalEnabled(bool isEnabled) { m_isInputTypeDateTimeLocalEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_MONTH)
    bool inputTypeMonthEnabled() const { return m_isInputTypeMonthEnabled; }
    void setInputTypeMonthEnabled(bool isEnabled) { m_isInputTypeMonthEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_TIME)
    bool inputTypeTimeEnabled() const { return m_isInputTypeTimeEnabled; }
    void setInputTypeTimeEnabled(bool isEnabled) { m_isInputTypeTimeEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_WEEK)
    bool inputTypeWeekEnabled() const { return m_isInputTypeWeekEnabled; }
    void setInputTypeWeekEnabled(bool isEnabled) { m_isInputTypeWeekEnabled = isEnabled; }
#endif

#if ENABLE(GAMEPAD)
    void setGamepadsEnabled(bool areEnabled) { m_areGamepadsEnabled = areEnabled; }
    bool gamepadsEnabled() const { return m_areGamepadsEnabled; }
#endif

#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
    // FIXME: This is not used.
    void setAnimationTriggersEnabled(bool areEnabled) { m_areAnimationTriggersEnabled = areEnabled; }
    bool animationTriggersEnabled() const { return m_areAnimationTriggersEnabled; }
#endif

    void setWebAnimationsEnabled(bool areEnabled) { m_areWebAnimationsEnabled = areEnabled; }
    bool webAnimationsEnabled() const { return m_areWebAnimationsEnabled; }

    void setWebAnimationsCSSIntegrationEnabled(bool isEnabled) { m_isWebAnimationsCSSIntegrationEnabled = isEnabled; }
    bool webAnimationsCSSIntegrationEnabled() const { return m_areWebAnimationsEnabled && m_isWebAnimationsCSSIntegrationEnabled; }

#if ENABLE(WEBGL2)
    void setWebGL2Enabled(bool isEnabled) { m_isWebGL2Enabled = isEnabled; }
    bool webGL2Enabled() const { return m_isWebGL2Enabled; }
#endif

#if ENABLE(WEBGPU)
    void setWebGPUEnabled(bool isEnabled) { m_isWebGPUEnabled = isEnabled; }
    bool webGPUEnabled() const { return m_isWebGPUEnabled; }
#endif

    void setImageBitmapOffscreenCanvasEnabled(bool isEnabled) { m_isImageBitmapOffscreenCanvasEnabled = isEnabled; }
    bool imageBitmapOffscreenCanvasEnabled() const { return m_isImageBitmapOffscreenCanvasEnabled; }

    void setCacheAPIEnabled(bool isEnabled) { m_isCacheAPIEnabled = isEnabled; }
    bool cacheAPIEnabled() const { return m_isCacheAPIEnabled; }

    void setFetchAPIEnabled(bool isEnabled) { m_isFetchAPIEnabled = isEnabled; }
    bool fetchAPIEnabled() const { return m_isFetchAPIEnabled; }

    void setWebSocketEnabled(bool isEnabled) { m_isWebSocketEnabled = isEnabled; }
    bool webSocketEnabled() const { return m_isWebSocketEnabled; }

#if ENABLE(STREAMS_API)
    void setReadableByteStreamAPIEnabled(bool isEnabled) { m_isReadableByteStreamAPIEnabled = isEnabled; }
    bool readableByteStreamAPIEnabled() const { return m_isReadableByteStreamAPIEnabled; }
    void setWritableStreamAPIEnabled(bool isEnabled) { m_isWritableStreamAPIEnabled = isEnabled; }
    bool writableStreamAPIEnabled() const { return m_isWritableStreamAPIEnabled; }
#endif

#if ENABLE(DOWNLOAD_ATTRIBUTE)
    void setDownloadAttributeEnabled(bool isEnabled) { m_isDownloadAttributeEnabled = isEnabled; }
    bool downloadAttributeEnabled() const { return m_isDownloadAttributeEnabled; }
#endif

#if ENABLE(INTERSECTION_OBSERVER)
    void setIntersectionObserverEnabled(bool isEnabled) { m_intersectionObserverEnabled = isEnabled; }
    bool intersectionObserverEnabled() const { return m_intersectionObserverEnabled; }
#endif

#if ENABLE(ENCRYPTED_MEDIA)
    void setEncryptedMediaAPIEnabled(bool isEnabled) { m_encryptedMediaAPIEnabled = isEnabled; }
    bool encryptedMediaAPIEnabled() const { return m_encryptedMediaAPIEnabled; }
#endif

#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    void setLegacyEncryptedMediaAPIEnabled(bool isEnabled) { m_legacyEncryptedMediaAPIEnabled = isEnabled; }
    bool legacyEncryptedMediaAPIEnabled() const { return m_legacyEncryptedMediaAPIEnabled; }
#endif

#if ENABLE(SERVICE_WORKER)
    bool serviceWorkerEnabled() const { return m_serviceWorkerEnabled; }
    void setServiceWorkerEnabled(bool isEnabled) { m_serviceWorkerEnabled = isEnabled; }
#endif

    bool fetchAPIKeepAliveEnabled() const { return m_fetchAPIKeepAliveEnabled; }
    void setFetchAPIKeepAliveEnabled(bool isEnabled) { m_fetchAPIKeepAliveEnabled = isEnabled; }

    bool spectreGadgetsEnabled() const;

    void setInspectorAdditionsEnabled(bool isEnabled) { m_inspectorAdditionsEnabled = isEnabled; }
    bool inspectorAdditionsEnabled() const { return m_inspectorAdditionsEnabled; }

    void setWebVREnabled(bool isEnabled) { m_webVREnabled = isEnabled; }
    bool webVREnabled() const { return m_webVREnabled; }

    void setAccessibilityObjectModelEnabled(bool isEnabled) { m_accessibilityObjectModelEnabled = isEnabled; }
    bool accessibilityObjectModelEnabled() const { return m_accessibilityObjectModelEnabled; }

    void setAriaReflectionEnabled(bool isEnabled) { m_ariaReflectionEnabled = isEnabled; }
    bool ariaReflectionEnabled() const { return m_ariaReflectionEnabled; }

    void setMediaCapabilitiesEnabled(bool isEnabled) { m_mediaCapabilitiesEnabled = isEnabled; }
    bool mediaCapabilitiesEnabled() const { return m_mediaCapabilitiesEnabled; }

    void setResourceLoadStatisticsDebugMode(bool isEnabled) { m_resourceLoadStatisticsDebugMode = isEnabled; }
    bool resourceLoadStatisticsDebugMode() const { return m_resourceLoadStatisticsDebugMode; }

    void setRestrictedHTTPResponseAccess(bool isEnabled) { m_isRestrictedHTTPResponseAccess = isEnabled; }
    bool restrictedHTTPResponseAccess() const { return m_isRestrictedHTTPResponseAccess; }

    void setCrossOriginResourcePolicyEnabled(bool isEnabled) { m_crossOriginResourcePolicyEnabled = isEnabled; }
    bool crossOriginResourcePolicyEnabled() const { return m_crossOriginResourcePolicyEnabled; }

    void setWebGLCompressedTextureASTCSupportEnabled(bool isEnabled) { m_isWebGLCompressedTextureASTCSupportEnabled = isEnabled; }
    bool webGLCompressedTextureASTCSupportEnabled() const { return m_isWebGLCompressedTextureASTCSupportEnabled; }

    void setStorageAccessPromptsEnabled(bool isEnabled)  { m_promptForStorageAccessAPIEnabled = isEnabled; }
    bool storageAccessPromptsEnabled() const { return m_promptForStorageAccessAPIEnabled; }

    void setServerTimingEnabled(bool isEnabled) { m_isServerTimingEnabled = isEnabled; }
    bool serverTimingEnabled() const { return m_isServerTimingEnabled; }

    void setExperimentalPlugInSandboxProfilesEnabled(bool isEnabled) { m_experimentalPlugInSandboxProfilesEnabled = isEnabled; }
    bool experimentalPlugInSandboxProfilesEnabled() const { return m_experimentalPlugInSandboxProfilesEnabled; }

    void setDisabledAdaptationsMetaTagEnabled(bool isEnabled) { m_disabledAdaptationsMetaTagEnabled = isEnabled; }
    bool disabledAdaptationsMetaTagEnabled() const { return m_disabledAdaptationsMetaTagEnabled; }

#if USE(SYSTEM_PREVIEW)
    void setSystemPreviewEnabled(bool isEnabled) { m_systemPreviewEnabled = isEnabled; }
    bool systemPreviewEnabled() const { return m_systemPreviewEnabled; }
#endif

    void setAttrStyleEnabled(bool isEnabled) { m_attrStyleEnabled = isEnabled; }
    bool attrStyleEnabled() const { return m_attrStyleEnabled; }

    WEBCORE_EXPORT static RuntimeEnabledFeatures& sharedFeatures();

private:
    // Never instantiate.
    RuntimeEnabledFeatures();

    bool m_areModernMediaControlsEnabled { false };
    bool m_isLinkPreloadEnabled { true };
    bool m_isLinkPrefetchEnabled { false };
    bool m_isMediaPreloadingEnabled { false };
    bool m_isResourceTimingEnabled { false };
    bool m_isUserTimingEnabled { false };
    bool m_isInteractiveFormValidationEnabled { false };
    bool m_isWebAuthenticationEnabled { false };
    bool m_isSecureContextAttributeEnabled { false };
    bool m_isDisplayContentsEnabled { true };
    bool m_isShadowDOMEnabled { true };
    bool m_areCustomElementsEnabled { true };
    bool m_isMenuItemElementEnabled { false };
    bool m_isDirectoryUploadEnabled { false };
    bool m_areDataTransferItemsEnabled { false };
    bool m_isCustomPasteboardDataEnabled { false };
    bool m_inputEventsEnabled { true };

#if ENABLE(ATTACHMENT_ELEMENT)
    bool m_isAttachmentElementEnabled { false };
#endif

#if ENABLE(INDEXED_DATABASE_IN_WORKERS)
    bool m_isIndexedDBWorkersEnabled { true };
#endif

#if ENABLE(MEDIA_STREAM)
    bool m_isMediaDevicesEnabled { false };
    bool m_isMediaStreamEnabled { true };
    bool m_isScreenCaptureEnabled { false };
#endif

#if ENABLE(WEB_RTC)
    bool m_isPeerConnectionEnabled { true };
    bool m_webRTCLegacyAPIEnabled { false };
    bool m_mdnsICECandidatesEnabled { false };
#endif

#if ENABLE(LEGACY_CSS_VENDOR_PREFIXES)
    bool m_isLegacyCSSVendorPrefixesEnabled { false };
#endif

#if ENABLE(INPUT_TYPE_DATE)
    bool m_isInputTypeDateEnabled { true };
#endif

#if ENABLE(INPUT_TYPE_DATETIME_INCOMPLETE)
    bool m_isInputTypeDateTimeEnabled { false };
#endif

#if ENABLE(INPUT_TYPE_DATETIMELOCAL)
    bool m_isInputTypeDateTimeLocalEnabled { true };
#endif

#if ENABLE(INPUT_TYPE_MONTH)
    bool m_isInputTypeMonthEnabled { true };
#endif

#if ENABLE(INPUT_TYPE_TIME)
    bool m_isInputTypeTimeEnabled { true };
#endif

#if ENABLE(INPUT_TYPE_WEEK)
    bool m_isInputTypeWeekEnabled { true };
#endif

#if ENABLE(GAMEPAD)
    bool m_areGamepadsEnabled { false };
#endif

#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
    bool m_areAnimationTriggersEnabled { false };
#endif

#if ENABLE(STREAMS_API)
    bool m_isReadableByteStreamAPIEnabled { false };
    bool m_isWritableStreamAPIEnabled { false };
#endif

    bool m_areWebAnimationsEnabled { true };
    bool m_isWebAnimationsCSSIntegrationEnabled { false };

#if ENABLE(WEBGL2)
    bool m_isWebGL2Enabled { false };
#endif

#if ENABLE(WEBGPU)
    bool m_isWebGPUEnabled { false };
#endif

    bool m_isImageBitmapOffscreenCanvasEnabled { true };
    bool m_isCacheAPIEnabled { false };
    bool m_isFetchAPIEnabled { true };

    bool m_isWebSocketEnabled { true };

#if ENABLE(DOWNLOAD_ATTRIBUTE)
    bool m_isDownloadAttributeEnabled { false };
#endif

#if ENABLE(ENCRYPTED_MEDIA)
    bool m_encryptedMediaAPIEnabled { false };
#endif

#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    bool m_legacyEncryptedMediaAPIEnabled { false };
#endif

#if ENABLE(INTERSECTION_OBSERVER)
    bool m_intersectionObserverEnabled { false };
#endif

#if ENABLE(SERVICE_WORKER)
    bool m_serviceWorkerEnabled { false };
#endif

    bool m_fetchAPIKeepAliveEnabled { false };
    bool m_inspectorAdditionsEnabled { false };
    bool m_webVREnabled { false };
    bool m_accessibilityObjectModelEnabled { false };
    bool m_ariaReflectionEnabled { true };
    bool m_mediaCapabilitiesEnabled { false };
    bool m_resourceLoadStatisticsDebugMode { false };
    bool m_isRestrictedHTTPResponseAccess { true };
    bool m_crossOriginResourcePolicyEnabled { true };
    bool m_isWebGLCompressedTextureASTCSupportEnabled { false };
    bool m_promptForStorageAccessAPIEnabled { false };
    bool m_isServerTimingEnabled { false };
    bool m_experimentalPlugInSandboxProfilesEnabled { false };
    bool m_disabledAdaptationsMetaTagEnabled { false };

#if USE(SYSTEM_PREVIEW)
    bool m_systemPreviewEnabled { false };
#endif

    bool m_attrStyleEnabled { false };

    friend class WTF::NeverDestroyed<RuntimeEnabledFeatures>;
};

} // namespace WebCore
