if (ENABLE_VIDEO OR ENABLE_WEB_AUDIO)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/graphics/gstreamer"
        "${WEBCORE_DIR}/platform/graphics/gstreamer/mse"
        "${WEBCORE_DIR}/platform/graphics/gstreamer/eme"
    )

    list(APPEND WebCore_SOURCES
        platform/graphics/gstreamer/AudioTrackPrivateGStreamer.cpp
        platform/graphics/gstreamer/GRefPtrGStreamer.cpp
        platform/graphics/gstreamer/GStreamerUtilities.cpp
        platform/graphics/gstreamer/GstAllocatorFastMalloc.cpp
        platform/graphics/gstreamer/InbandTextTrackPrivateGStreamer.cpp
        platform/graphics/gstreamer/MediaPlayerPrivateGStreamer.cpp
        platform/graphics/gstreamer/MediaPlayerPrivateGStreamerBase.cpp
        platform/graphics/gstreamer/MediaSampleGStreamer.cpp
        platform/graphics/gstreamer/TextCombinerGStreamer.cpp
        platform/graphics/gstreamer/TextSinkGStreamer.cpp
        platform/graphics/gstreamer/TrackPrivateBaseGStreamer.cpp
        platform/graphics/gstreamer/VideoSinkGStreamer.cpp
        platform/graphics/gstreamer/VideoTrackPrivateGStreamer.cpp
        platform/graphics/gstreamer/WebKitWebSourceGStreamer.cpp

        platform/graphics/gstreamer/eme/GStreamerEMEUtilities.cpp
        platform/graphics/gstreamer/eme/WebKitClearKeyDecryptorGStreamer.cpp
        platform/graphics/gstreamer/eme/WebKitCommonEncryptionDecryptorGStreamer.cpp

        platform/graphics/gstreamer/mse/AppendPipeline.cpp
        platform/graphics/gstreamer/mse/GStreamerMediaDescription.cpp
        platform/graphics/gstreamer/mse/MediaPlayerPrivateGStreamerMSE.cpp
        platform/graphics/gstreamer/mse/MediaSourceClientGStreamerMSE.cpp
        platform/graphics/gstreamer/mse/MediaSourceGStreamer.cpp
        platform/graphics/gstreamer/mse/PlaybackPipeline.cpp
        platform/graphics/gstreamer/mse/SourceBufferPrivateGStreamer.cpp
        platform/graphics/gstreamer/mse/WebKitMediaSourceGStreamer.cpp
    )

    list(APPEND WebCore_SYSTEM_INCLUDE_DIRECTORIES
        ${GSTREAMER_INCLUDE_DIRS}
        ${GSTREAMER_BASE_INCLUDE_DIRS}
        ${GSTREAMER_APP_INCLUDE_DIRS}
        ${GSTREAMER_PBUTILS_INCLUDE_DIRS}
    )

    list(APPEND WebCore_LIBRARIES
        ${GSTREAMER_APP_LIBRARIES}
        ${GSTREAMER_BASE_LIBRARIES}
        ${GSTREAMER_LIBRARIES}
        ${GSTREAMER_PBUTILS_LIBRARIES}
        ${GSTREAMER_AUDIO_LIBRARIES}
    )

    # Avoiding a GLib deprecation warning due to GStreamer API using deprecated classes.
    set_source_files_properties(platform/audio/gstreamer/WebKitWebAudioSourceGStreamer.cpp PROPERTIES COMPILE_DEFINITIONS "GLIB_DISABLE_DEPRECATION_WARNINGS=1")
endif ()

if (ENABLE_VIDEO)
    list(APPEND WebCore_SYSTEM_INCLUDE_DIRECTORIES
        ${GSTREAMER_TAG_INCLUDE_DIRS}
        ${GSTREAMER_VIDEO_INCLUDE_DIRS}
    )

    list(APPEND WebCore_LIBRARIES
        ${GSTREAMER_TAG_LIBRARIES}
        ${GSTREAMER_VIDEO_LIBRARIES}
    )

    if (USE_GSTREAMER_MPEGTS)
        list(APPEND WebCore_SYSTEM_INCLUDE_DIRECTORIES
            ${GSTREAMER_MPEGTS_INCLUDE_DIRS}
        )
        list(APPEND WebCore_LIBRARIES
            ${GSTREAMER_MPEGTS_LIBRARIES}
        )
    endif ()

    if (USE_GSTREAMER_GL)
        list(APPEND WebCore_SYSTEM_INCLUDE_DIRECTORIES
            ${GSTREAMER_GL_INCLUDE_DIRS}
        )
        list(APPEND WebCore_LIBRARIES
            ${GSTREAMER_GL_LIBRARIES}
        )
        list(APPEND WebCore_SOURCES
            platform/graphics/gstreamer/VideoTextureCopierGStreamer.cpp
        )
    endif ()
endif ()

if (ENABLE_WEB_AUDIO)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/audio/gstreamer"
    )

    list(APPEND WebCore_SOURCES
        platform/audio/gstreamer/AudioDestinationGStreamer.cpp
        platform/audio/gstreamer/AudioFileReaderGStreamer.cpp
        platform/audio/gstreamer/AudioSourceProviderGStreamer.cpp
        platform/audio/gstreamer/FFTFrameGStreamer.cpp
        platform/audio/gstreamer/WebKitWebAudioSourceGStreamer.cpp
    )

    list(APPEND WebCore_SYSTEM_INCLUDE_DIRECTORIES
        ${GSTREAMER_AUDIO_INCLUDE_DIRS}
        ${GSTREAMER_FFT_INCLUDE_DIRS}
    )

    list(APPEND WebCore_LIBRARIES
        ${GSTREAMER_FFT_LIBRARIES}
    )
endif ()

if (ENABLE_ENCRYPTED_MEDIA)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/encryptedmedia/clearkey"
        ${LIBGCRYPT_INCLUDE_DIRS}
    )
    list(APPEND WebCore_SOURCES
        platform/encryptedmedia/clearkey/CDMClearKey.cpp

        platform/graphics/gstreamer/eme/CDMFactoryGStreamer.cpp
    )
    list(APPEND WebCore_LIBRARIES
        ${LIBGCRYPT_LIBRARIES} -lgpg-error
    )
endif ()

if (USE_CAIRO)
    list(APPEND WebCore_SOURCES
        platform/graphics/gstreamer/ImageGStreamerCairo.cpp
    )
endif ()
