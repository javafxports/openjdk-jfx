/*
 * Copyright (C) 2017 Apple Inc.  All rights reserved.
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

#include "config.h"
#include "InspectorCanvas.h"

#include "AffineTransform.h"
#include "CachedImage.h"
#include "CanvasGradient.h"
#include "CanvasPattern.h"
#include "CanvasRenderingContext.h"
#include "CanvasRenderingContext2D.h"
#include "Document.h"
#include "FloatPoint.h"
#include "Gradient.h"
#include "HTMLCanvasElement.h"
#include "HTMLImageElement.h"
#include "HTMLVideoElement.h"
#include "Image.h"
#include "ImageBitmap.h"
#include "ImageBitmapRenderingContext.h"
#include "ImageBuffer.h"
#include "ImageData.h"
#include "InspectorDOMAgent.h"
#include "InstrumentingAgents.h"
#include "JSCanvasDirection.h"
#include "JSCanvasFillRule.h"
#include "JSCanvasLineCap.h"
#include "JSCanvasLineJoin.h"
#include "JSCanvasTextAlign.h"
#include "JSCanvasTextBaseline.h"
#include "JSImageSmoothingQuality.h"
#include "JSMainThreadExecState.h"
#include "Path2D.h"
#include "Pattern.h"
#include "RecordingSwizzleTypes.h"
#include "SVGPathUtilities.h"
#include "StringAdaptors.h"
#if ENABLE(WEBGL)
#include "WebGLRenderingContext.h"
#endif
#if ENABLE(WEBGL2)
#include "WebGL2RenderingContext.h"
#endif
#if ENABLE(WEBGPU)
#include "WebGPURenderingContext.h"
#endif
#include <JavaScriptCore/IdentifiersFactory.h>
#include <JavaScriptCore/ScriptCallStack.h>
#include <JavaScriptCore/ScriptCallStackFactory.h>
#include <wtf/CurrentTime.h>


namespace WebCore {

using namespace Inspector;

Ref<InspectorCanvas> InspectorCanvas::create(CanvasRenderingContext& context)
{
    return adoptRef(*new InspectorCanvas(context));
}

InspectorCanvas::InspectorCanvas(CanvasRenderingContext& context)
    : m_identifier("canvas:" + IdentifiersFactory::createIdentifier())
    , m_context(context)
{
}

HTMLCanvasElement* InspectorCanvas::canvasElement()
{
    auto* canvasBase = &m_context.canvasBase();
    if (is<HTMLCanvasElement>(canvasBase))
        return downcast<HTMLCanvasElement>(canvasBase);
    return nullptr;
}

void InspectorCanvas::resetRecordingData()
{
    m_initialState = nullptr;
    m_frames = nullptr;
    m_currentActions = nullptr;
    m_actionNeedingSnapshot = nullptr;
    m_serializedDuplicateData = nullptr;
    m_indexedDuplicateData.clear();
    m_recordingName = { };
    m_bufferLimit = 100 * 1024 * 1024;
    m_bufferUsed = 0;
    m_singleFrame = true;

    m_context.setCallTracingActive(false);
}

bool InspectorCanvas::hasRecordingData() const
{
    return m_bufferUsed > 0;
}

bool InspectorCanvas::currentFrameHasData() const
{
    return !!m_frames;
}

static bool shouldSnapshotWebGLAction(const String& name)
{
    return name == "clear"
        || name == "drawArrays"
        || name == "drawElements";
}

void InspectorCanvas::recordAction(const String& name, Vector<RecordCanvasActionVariant>&& parameters)
{
    if (!m_initialState) {
        m_initialState = buildInitialState();
        m_bufferUsed += m_initialState->memoryCost();
    }

    if (!m_frames)
        m_frames = JSON::ArrayOf<Inspector::Protocol::Recording::Frame>::create();

    if (!m_currentActions) {
        m_currentActions = JSON::ArrayOf<JSON::Value>::create();

        auto frame = Inspector::Protocol::Recording::Frame::create()
            .setActions(m_currentActions)
            .release();

        m_frames->addItem(WTFMove(frame));

        m_currentFrameStartTime = monotonicallyIncreasingTimeMS();
    }

    appendActionSnapshotIfNeeded();

    auto action = buildAction(name, WTFMove(parameters));
    m_bufferUsed += action->memoryCost();
    m_currentActions->addItem(action);

#if ENABLE(WEBGL)
    if (is<WebGLRenderingContext>(m_context) && shouldSnapshotWebGLAction(name))
        m_actionNeedingSnapshot = action;
#endif
}

RefPtr<Inspector::Protocol::Recording::InitialState>&& InspectorCanvas::releaseInitialState()
{
    return WTFMove(m_initialState);
}

RefPtr<JSON::ArrayOf<Inspector::Protocol::Recording::Frame>>&& InspectorCanvas::releaseFrames()
{
    appendActionSnapshotIfNeeded();

    return WTFMove(m_frames);
}

RefPtr<JSON::ArrayOf<JSON::Value>>&& InspectorCanvas::releaseData()
{
    m_indexedDuplicateData.clear();
    return WTFMove(m_serializedDuplicateData);
}

void InspectorCanvas::finalizeFrame()
{
    if (m_frames && m_frames->length() && !std::isnan(m_currentFrameStartTime)) {
        auto currentFrame = static_cast<Inspector::Protocol::Recording::Frame*>(m_frames->get(m_frames->length() - 1).get());
        currentFrame->setDuration(monotonicallyIncreasingTimeMS() - m_currentFrameStartTime);

        m_currentFrameStartTime = NAN;
    }

    m_currentActions = nullptr;
}

void InspectorCanvas::markCurrentFrameIncomplete()
{
    if (!m_currentActions || !m_frames || !m_frames->length())
        return;

    static_cast<Inspector::Protocol::Recording::Frame*>(m_frames->get(m_frames->length() - 1).get())->setIncomplete(true);
}

void InspectorCanvas::setBufferLimit(long memoryLimit)
{
    m_bufferLimit = std::min<long>(memoryLimit, std::numeric_limits<int>::max());
}

bool InspectorCanvas::hasBufferSpace() const
{
    return m_bufferUsed < m_bufferLimit;
}

Ref<Inspector::Protocol::Canvas::Canvas> InspectorCanvas::buildObjectForCanvas(InstrumentingAgents& instrumentingAgents, bool captureBacktrace)
{
    Inspector::Protocol::Canvas::ContextType contextType;
    if (is<CanvasRenderingContext2D>(m_context))
        contextType = Inspector::Protocol::Canvas::ContextType::Canvas2D;
    else if (is<ImageBitmapRenderingContext>(m_context))
        contextType = Inspector::Protocol::Canvas::ContextType::BitmapRenderer;
#if ENABLE(WEBGL)
    else if (is<WebGLRenderingContext>(m_context))
        contextType = Inspector::Protocol::Canvas::ContextType::WebGL;
#endif
#if ENABLE(WEBGL2)
    else if (is<WebGL2RenderingContext>(m_context))
        contextType = Inspector::Protocol::Canvas::ContextType::WebGL2;
#endif
#if ENABLE(WEBGPU)
    else if (is<WebGPURenderingContext>(m_context))
        contextType = Inspector::Protocol::Canvas::ContextType::WebGPU;
#endif
    else {
        ASSERT_NOT_REACHED();
        contextType = Inspector::Protocol::Canvas::ContextType::Canvas2D;
    }

    auto canvas = Inspector::Protocol::Canvas::Canvas::create()
        .setCanvasId(m_identifier)
        .setContextType(contextType)
        .release();

    if (auto* node = canvasElement()) {
        String cssCanvasName = node->document().nameForCSSCanvasElement(*node);
        if (!cssCanvasName.isEmpty())
            canvas->setCssCanvasName(cssCanvasName);
        else {
            InspectorDOMAgent* domAgent = instrumentingAgents.inspectorDOMAgent();
            int nodeId = domAgent->boundNodeId(node);
            if (!nodeId) {
                if (int documentNodeId = domAgent->boundNodeId(&node->document())) {
                    ErrorString ignored;
                    nodeId = domAgent->pushNodeToFrontend(ignored, documentNodeId, node);
                }
            }

            if (nodeId)
                canvas->setNodeId(nodeId);
        }
    }

    if (is<ImageBitmapRenderingContext>(m_context)) {
        auto contextAttributes = Inspector::Protocol::Canvas::ContextAttributes::create()
            .release();
        contextAttributes->setAlpha(downcast<ImageBitmapRenderingContext>(m_context).hasAlpha());
        canvas->setContextAttributes(WTFMove(contextAttributes));
    }
#if ENABLE(WEBGL)
    else if (is<WebGLRenderingContextBase>(m_context)) {
        if (std::optional<WebGLContextAttributes> attributes = downcast<WebGLRenderingContextBase>(m_context).getContextAttributes()) {
            auto contextAttributes = Inspector::Protocol::Canvas::ContextAttributes::create()
                .release();
            contextAttributes->setAlpha(attributes->alpha);
            contextAttributes->setDepth(attributes->depth);
            contextAttributes->setStencil(attributes->stencil);
            contextAttributes->setAntialias(attributes->antialias);
            contextAttributes->setPremultipliedAlpha(attributes->premultipliedAlpha);
            contextAttributes->setPreserveDrawingBuffer(attributes->preserveDrawingBuffer);
            contextAttributes->setFailIfMajorPerformanceCaveat(attributes->failIfMajorPerformanceCaveat);
            canvas->setContextAttributes(WTFMove(contextAttributes));
        }
    }
#endif

    // FIXME: <https://webkit.org/b/180833> Web Inspector: support OffscreenCanvas for Canvas related operations

    if (auto* node = canvasElement()) {
        if (size_t memoryCost = node->memoryCost())
            canvas->setMemoryCost(memoryCost);
    }

    if (captureBacktrace) {
        auto stackTrace = Inspector::createScriptCallStack(JSMainThreadExecState::currentState(), Inspector::ScriptCallStack::maxCallStackSizeToCapture);
        canvas->setBacktrace(stackTrace->buildInspectorArray());
    }

    return canvas;
}

void InspectorCanvas::appendActionSnapshotIfNeeded()
{
    if (!m_actionNeedingSnapshot)
        return;

    m_actionNeedingSnapshot->addItem(indexForData(getCanvasContentAsDataURL()));
    m_actionNeedingSnapshot = nullptr;
}

String InspectorCanvas::getCanvasContentAsDataURL()
{
    // FIXME: <https://webkit.org/b/180833> Web Inspector: support OffscreenCanvas for Canvas related operations

    auto* node = canvasElement();
    if (!node)
        return String();

#if ENABLE(WEBGL)
    if (is<WebGLRenderingContextBase>(m_context))
        downcast<WebGLRenderingContextBase>(m_context).setPreventBufferClearForInspector(true);
#endif

    ExceptionOr<UncachedString> result = node->toDataURL(ASCIILiteral("image/png"));

#if ENABLE(WEBGL)
    if (is<WebGLRenderingContextBase>(m_context))
        downcast<WebGLRenderingContextBase>(m_context).setPreventBufferClearForInspector(false);
#endif

    if (result.hasException())
        return String();

    return result.releaseReturnValue().string;
}

int InspectorCanvas::indexForData(DuplicateDataVariant data)
{
    size_t index = m_indexedDuplicateData.find(data);
    if (index != notFound) {
        ASSERT(index < std::numeric_limits<int>::max());
        return static_cast<int>(index);
    }

    if (!m_serializedDuplicateData)
        m_serializedDuplicateData = JSON::ArrayOf<JSON::Value>::create();

    RefPtr<JSON::Value> item;
    WTF::switchOn(data,
        [&] (const HTMLImageElement* imageElement) {
            String dataURL = ASCIILiteral("data:,");

            if (CachedImage* cachedImage = imageElement->cachedImage()) {
                Image* image = cachedImage->image();
                if (image && image != &Image::nullImage()) {
                    std::unique_ptr<ImageBuffer> imageBuffer = ImageBuffer::create(image->size(), RenderingMode::Unaccelerated);
                    imageBuffer->context().drawImage(*image, FloatPoint(0, 0));
                    dataURL = imageBuffer->toDataURL("image/png");
                }
            }

            index = indexForData(dataURL);
        },
#if ENABLE(VIDEO)
        [&] (HTMLVideoElement* videoElement) {
            String dataURL = ASCIILiteral("data:,");

            unsigned videoWidth = videoElement->videoWidth();
            unsigned videoHeight = videoElement->videoHeight();
            std::unique_ptr<ImageBuffer> imageBuffer = ImageBuffer::create(FloatSize(videoWidth, videoHeight), RenderingMode::Unaccelerated);
            if (imageBuffer) {
                videoElement->paintCurrentFrameInContext(imageBuffer->context(), FloatRect(0, 0, videoWidth, videoHeight));
                dataURL = imageBuffer->toDataURL("image/png");
            }

            index = indexForData(dataURL);
        },
#endif
        [&] (HTMLCanvasElement* canvasElement) {
            String dataURL = ASCIILiteral("data:,");

            ExceptionOr<UncachedString> result = canvasElement->toDataURL(ASCIILiteral("image/png"));
            if (!result.hasException())
                dataURL = result.releaseReturnValue().string;

            index = indexForData(dataURL);
        },
        [&] (const CanvasGradient* canvasGradient) { item = buildArrayForCanvasGradient(*canvasGradient); },
        [&] (const CanvasPattern* canvasPattern) { item = buildArrayForCanvasPattern(*canvasPattern); },
        [&] (const ImageData* imageData) { item = buildArrayForImageData(*imageData); },
        [&] (ImageBitmap* imageBitmap) {
            index = indexForData(imageBitmap->buffer()->toDataURL("image/png"));
        },
        [&] (const ScriptCallFrame& scriptCallFrame) {
            auto array = JSON::ArrayOf<double>::create();
            array->addItem(indexForData(scriptCallFrame.functionName()));
            array->addItem(indexForData(scriptCallFrame.sourceURL()));
            array->addItem(static_cast<int>(scriptCallFrame.lineNumber()));
            array->addItem(static_cast<int>(scriptCallFrame.columnNumber()));
            item = WTFMove(array);
        },
        [&] (const String& value) { item = JSON::Value::create(value); }
    );

    if (item) {
        m_bufferUsed += item->memoryCost();
        m_serializedDuplicateData->addItem(WTFMove(item));

        m_indexedDuplicateData.append(data);
        index = m_indexedDuplicateData.size() - 1;
    }

    ASSERT(index < std::numeric_limits<int>::max());
    return static_cast<int>(index);
}

static RefPtr<JSON::ArrayOf<double>> buildArrayForAffineTransform(const AffineTransform& affineTransform)
{
    RefPtr<JSON::ArrayOf<double>> array = JSON::ArrayOf<double>::create();
    array->addItem(affineTransform.a());
    array->addItem(affineTransform.b());
    array->addItem(affineTransform.c());
    array->addItem(affineTransform.d());
    array->addItem(affineTransform.e());
    array->addItem(affineTransform.f());
    return array;
}

template <typename T>
static RefPtr<JSON::ArrayOf<JSON::Value>> buildArrayForVector(const Vector<T>& vector)
{
    RefPtr<JSON::ArrayOf<JSON::Value>> array = JSON::ArrayOf<JSON::Value>::create();
    for (auto& item : vector)
        array->addItem(item);
    return array;
}

RefPtr<Inspector::Protocol::Recording::InitialState> InspectorCanvas::buildInitialState()
{
    RefPtr<Inspector::Protocol::Recording::InitialState> initialState = Inspector::Protocol::Recording::InitialState::create()
        .release();

    auto attributes = JSON::Object::create();
    attributes->setInteger(ASCIILiteral("width"), m_context.canvasBase().width());
    attributes->setInteger(ASCIILiteral("height"), m_context.canvasBase().height());

    auto parameters = JSON::ArrayOf<JSON::Value>::create();

    if (is<CanvasRenderingContext2D>(m_context)) {
        const CanvasRenderingContext2D& context2d = downcast<CanvasRenderingContext2D>(m_context);
        const CanvasRenderingContext2D::State& state = context2d.state();

        attributes->setArray(ASCIILiteral("setTransform"), buildArrayForAffineTransform(state.transform));
        attributes->setDouble(ASCIILiteral("globalAlpha"), context2d.globalAlpha());
        attributes->setInteger(ASCIILiteral("globalCompositeOperation"), indexForData(context2d.globalCompositeOperation()));
        attributes->setDouble(ASCIILiteral("lineWidth"), context2d.lineWidth());
        attributes->setInteger(ASCIILiteral("lineCap"), indexForData(convertEnumerationToString(context2d.lineCap())));
        attributes->setInteger(ASCIILiteral("lineJoin"), indexForData(convertEnumerationToString(context2d.lineJoin())));
        attributes->setDouble(ASCIILiteral("miterLimit"), context2d.miterLimit());
        attributes->setDouble(ASCIILiteral("shadowOffsetX"), context2d.shadowOffsetX());
        attributes->setDouble(ASCIILiteral("shadowOffsetY"), context2d.shadowOffsetY());
        attributes->setDouble(ASCIILiteral("shadowBlur"), context2d.shadowBlur());
        attributes->setInteger(ASCIILiteral("shadowColor"), indexForData(context2d.shadowColor()));

        // The parameter to `setLineDash` is itself an array, so we need to wrap the parameters
        // list in an array to allow spreading.
        auto setLineDash = JSON::ArrayOf<JSON::Value>::create();
        setLineDash->addItem(buildArrayForVector(state.lineDash));
        attributes->setArray(ASCIILiteral("setLineDash"), WTFMove(setLineDash));

        attributes->setDouble(ASCIILiteral("lineDashOffset"), context2d.lineDashOffset());
        attributes->setInteger(ASCIILiteral("font"), indexForData(context2d.font()));
        attributes->setInteger(ASCIILiteral("textAlign"), indexForData(convertEnumerationToString(context2d.textAlign())));
        attributes->setInteger(ASCIILiteral("textBaseline"), indexForData(convertEnumerationToString(context2d.textBaseline())));
        attributes->setInteger(ASCIILiteral("direction"), indexForData(convertEnumerationToString(context2d.direction())));

        int strokeStyleIndex;
        if (auto canvasGradient = state.strokeStyle.canvasGradient())
            strokeStyleIndex = indexForData(canvasGradient.get());
        else if (auto canvasPattern = state.strokeStyle.canvasPattern())
            strokeStyleIndex = indexForData(canvasPattern.get());
        else
            strokeStyleIndex = indexForData(state.strokeStyle.color());
        attributes->setInteger(ASCIILiteral("strokeStyle"), strokeStyleIndex);

        int fillStyleIndex;
        if (auto canvasGradient = state.fillStyle.canvasGradient())
            fillStyleIndex = indexForData(canvasGradient.get());
        else if (auto canvasPattern = state.fillStyle.canvasPattern())
            fillStyleIndex = indexForData(canvasPattern.get());
        else
            fillStyleIndex = indexForData(state.fillStyle.color());
        attributes->setInteger(ASCIILiteral("fillStyle"), fillStyleIndex);

        attributes->setBoolean(ASCIILiteral("imageSmoothingEnabled"), context2d.imageSmoothingEnabled());
        attributes->setInteger(ASCIILiteral("imageSmoothingQuality"), indexForData(convertEnumerationToString(context2d.imageSmoothingQuality())));

        auto setPath = JSON::ArrayOf<JSON::Value>::create();
        setPath->addItem(indexForData(buildStringFromPath(context2d.getPath()->path())));
        attributes->setArray(ASCIILiteral("setPath"), WTFMove(setPath));
    }
#if ENABLE(WEBGL)
    else if (is<WebGLRenderingContextBase>(m_context)) {
        WebGLRenderingContextBase& contextWebGLBase = downcast<WebGLRenderingContextBase>(m_context);
        if (std::optional<WebGLContextAttributes> attributes = contextWebGLBase.getContextAttributes()) {
            RefPtr<JSON::Object> contextAttributes = JSON::Object::create();
            contextAttributes->setBoolean(ASCIILiteral("alpha"), attributes->alpha);
            contextAttributes->setBoolean(ASCIILiteral("depth"), attributes->depth);
            contextAttributes->setBoolean(ASCIILiteral("stencil"), attributes->stencil);
            contextAttributes->setBoolean(ASCIILiteral("antialias"), attributes->antialias);
            contextAttributes->setBoolean(ASCIILiteral("premultipliedAlpha"), attributes->premultipliedAlpha);
            contextAttributes->setBoolean(ASCIILiteral("preserveDrawingBuffer"), attributes->preserveDrawingBuffer);
            contextAttributes->setBoolean(ASCIILiteral("failIfMajorPerformanceCaveat"), attributes->failIfMajorPerformanceCaveat);
            parameters->addItem(WTFMove(contextAttributes));
        }
    }
#endif

    initialState->setAttributes(WTFMove(attributes));

    if (parameters->length())
        initialState->setParameters(WTFMove(parameters));

    initialState->setContent(getCanvasContentAsDataURL());

    return initialState;
}

RefPtr<JSON::ArrayOf<JSON::Value>> InspectorCanvas::buildAction(const String& name, Vector<RecordCanvasActionVariant>&& parameters)
{
    RefPtr<JSON::ArrayOf<JSON::Value>> action = JSON::ArrayOf<JSON::Value>::create();
    action->addItem(indexForData(name));

    RefPtr<JSON::ArrayOf<JSON::Value>> parametersData = JSON::ArrayOf<JSON::Value>::create();
    RefPtr<JSON::ArrayOf<int>> swizzleTypes = JSON::ArrayOf<int>::create();

    auto addParameter = [&parametersData, &swizzleTypes] (auto value, RecordingSwizzleTypes swizzleType) {
        parametersData->addItem(value);
        swizzleTypes->addItem(static_cast<int>(swizzleType));
    };

    for (RecordCanvasActionVariant& item : parameters) {
        WTF::switchOn(item,
            [&] (CanvasDirection value) { addParameter(indexForData(convertEnumerationToString(value)), RecordingSwizzleTypes::String); },
            [&] (CanvasFillRule value) { addParameter(indexForData(convertEnumerationToString(value)), RecordingSwizzleTypes::String); },
            [&] (CanvasLineCap value) { addParameter(indexForData(convertEnumerationToString(value)), RecordingSwizzleTypes::String); },
            [&] (CanvasLineJoin value) { addParameter(indexForData(convertEnumerationToString(value)), RecordingSwizzleTypes::String); },
            [&] (CanvasTextAlign value) { addParameter(indexForData(convertEnumerationToString(value)), RecordingSwizzleTypes::String); },
            [&] (CanvasTextBaseline value) { addParameter(indexForData(convertEnumerationToString(value)), RecordingSwizzleTypes::String); },
            [&] (const DOMMatrix2DInit& value) {
                RefPtr<JSON::ArrayOf<double>> array = JSON::ArrayOf<double>::create();
                array->addItem(value.a.value_or(1));
                array->addItem(value.b.value_or(0));
                array->addItem(value.c.value_or(0));
                array->addItem(value.d.value_or(1));
                array->addItem(value.e.value_or(0));
                array->addItem(value.f.value_or(0));
                addParameter(WTFMove(array), RecordingSwizzleTypes::DOMMatrix);
            },
            [&] (const Element*) {
                // Elements are not serializable, so add a string as a placeholder since the actual
                // element cannot be reconstructed in the frontend.
                addParameter(indexForData("Element"), RecordingSwizzleTypes::None);
            },
            [&] (HTMLImageElement* value) { addParameter(indexForData(value), RecordingSwizzleTypes::Image); },
            [&] (ImageBitmap* value) { addParameter(indexForData(value), RecordingSwizzleTypes::ImageBitmap); },
            [&] (ImageData* value) { addParameter(indexForData(value), RecordingSwizzleTypes::ImageData); },
            [&] (ImageSmoothingQuality value) { addParameter(indexForData(convertEnumerationToString(value)), RecordingSwizzleTypes::String); },
            [&] (const Path2D* value) { addParameter(indexForData(buildStringFromPath(value->path())), RecordingSwizzleTypes::Path2D); },
#if ENABLE(WEBGL)
            // FIXME: <https://webkit.org/b/176009> Web Inspector: send data for WebGL objects during a recording instead of a placeholder string
            [&] (const WebGLBuffer*) { addParameter(0, RecordingSwizzleTypes::WebGLBuffer); },
            [&] (const WebGLFramebuffer*) { addParameter(0, RecordingSwizzleTypes::WebGLFramebuffer); },
            [&] (const WebGLProgram*) { addParameter(0, RecordingSwizzleTypes::WebGLProgram); },
            [&] (const WebGLRenderbuffer*) { addParameter(0, RecordingSwizzleTypes::WebGLRenderbuffer); },
            [&] (const WebGLShader*) { addParameter(0, RecordingSwizzleTypes::WebGLShader); },
            [&] (const WebGLTexture*) { addParameter(0, RecordingSwizzleTypes::WebGLTexture); },
            [&] (const WebGLUniformLocation*) { addParameter(0, RecordingSwizzleTypes::WebGLUniformLocation); },
#endif
            [&] (const RefPtr<ArrayBuffer>&) { addParameter(0, RecordingSwizzleTypes::TypedArray); },
            [&] (const RefPtr<ArrayBufferView>&) { addParameter(0, RecordingSwizzleTypes::TypedArray); },
            [&] (const RefPtr<CanvasGradient>& value) { addParameter(indexForData(value.get()), RecordingSwizzleTypes::CanvasGradient); },
            [&] (const RefPtr<CanvasPattern>& value) { addParameter(indexForData(value.get()), RecordingSwizzleTypes::CanvasPattern); },
            [&] (const RefPtr<Float32Array>&) { addParameter(0, RecordingSwizzleTypes::TypedArray); },
            [&] (RefPtr<HTMLCanvasElement>& value) { addParameter(indexForData(value.get()), RecordingSwizzleTypes::Image); },
            [&] (const RefPtr<HTMLImageElement>& value) { addParameter(indexForData(value.get()), RecordingSwizzleTypes::Image); },
#if ENABLE(VIDEO)
            [&] (RefPtr<HTMLVideoElement>& value) { addParameter(indexForData(value.get()), RecordingSwizzleTypes::Image); },
#endif
            [&] (const RefPtr<ImageBitmap>& value) { addParameter(indexForData(value.get()), RecordingSwizzleTypes::ImageBitmap); },
            [&] (const RefPtr<ImageData>& value) { addParameter(indexForData(value.get()), RecordingSwizzleTypes::ImageData); },
            [&] (const RefPtr<Int32Array>&) { addParameter(0, RecordingSwizzleTypes::TypedArray); },
            [&] (const Vector<float>& value) { addParameter(buildArrayForVector(value), RecordingSwizzleTypes::Array); },
            [&] (const Vector<int>& value) { addParameter(buildArrayForVector(value), RecordingSwizzleTypes::Array); },
            [&] (const String& value) { addParameter(indexForData(value), RecordingSwizzleTypes::String); },
            [&] (double value) { addParameter(value, RecordingSwizzleTypes::Number); },
            [&] (float value) { addParameter(value, RecordingSwizzleTypes::Number); },
            [&] (int64_t value) { addParameter(static_cast<double>(value), RecordingSwizzleTypes::Number); },
            [&] (uint32_t value) { addParameter(static_cast<double>(value), RecordingSwizzleTypes::Number); },
            [&] (int32_t value) { addParameter(value, RecordingSwizzleTypes::Number); },
            [&] (uint8_t value) { addParameter(static_cast<int>(value), RecordingSwizzleTypes::Number); },
            [&] (bool value) { addParameter(value, RecordingSwizzleTypes::Boolean); }
        );
    }

    action->addItem(WTFMove(parametersData));
    action->addItem(WTFMove(swizzleTypes));

    RefPtr<JSON::ArrayOf<double>> trace = JSON::ArrayOf<double>::create();
    auto stackTrace = Inspector::createScriptCallStack(JSMainThreadExecState::currentState(), Inspector::ScriptCallStack::maxCallStackSizeToCapture);
    for (size_t i = 0; i < stackTrace->size(); ++i)
        trace->addItem(indexForData(stackTrace->at(i)));
    action->addItem(WTFMove(trace));

    return action;
}

RefPtr<JSON::ArrayOf<JSON::Value>> InspectorCanvas::buildArrayForCanvasGradient(const CanvasGradient& canvasGradient)
{
    const auto& gradient = canvasGradient.gradient();

    String type = gradient.type() == Gradient::Type::Radial ? ASCIILiteral("radial-gradient") : ASCIILiteral("linear-gradient");

    RefPtr<JSON::ArrayOf<float>> parameters = JSON::ArrayOf<float>::create();
    WTF::switchOn(gradient.data(),
        [&parameters] (const Gradient::LinearData& data) {
            parameters->addItem(data.point0.x());
            parameters->addItem(data.point0.y());
            parameters->addItem(data.point1.x());
            parameters->addItem(data.point1.y());
        },
        [&parameters] (const Gradient::RadialData& data) {
            parameters->addItem(data.point0.x());
            parameters->addItem(data.point0.y());
            parameters->addItem(data.startRadius);
            parameters->addItem(data.point1.x());
            parameters->addItem(data.point1.y());
            parameters->addItem(data.endRadius);
        }
    );

    RefPtr<JSON::ArrayOf<JSON::Value>> stops = JSON::ArrayOf<JSON::Value>::create();
    for (auto& colorStop : gradient.stops()) {
        RefPtr<JSON::ArrayOf<JSON::Value>> stop = JSON::ArrayOf<JSON::Value>::create();
        stop->addItem(colorStop.offset);
        stop->addItem(indexForData(colorStop.color.cssText()));
        stops->addItem(WTFMove(stop));
    }

    RefPtr<JSON::ArrayOf<JSON::Value>> array = JSON::ArrayOf<JSON::Value>::create();
    array->addItem(indexForData(type));
    array->addItem(WTFMove(parameters));
    array->addItem(WTFMove(stops));
    return array;
}

RefPtr<JSON::ArrayOf<JSON::Value>> InspectorCanvas::buildArrayForCanvasPattern(const CanvasPattern& canvasPattern)
{
    Image& tileImage = canvasPattern.pattern().tileImage();
    std::unique_ptr<ImageBuffer> imageBuffer = ImageBuffer::create(tileImage.size(), RenderingMode::Unaccelerated);
    imageBuffer->context().drawImage(tileImage, FloatPoint(0, 0));

    String repeat;
    bool repeatX = canvasPattern.pattern().repeatX();
    bool repeatY = canvasPattern.pattern().repeatY();
    if (repeatX && repeatY)
        repeat = ASCIILiteral("repeat");
    else if (repeatX && !repeatY)
        repeat = ASCIILiteral("repeat-x");
    else if (!repeatX && repeatY)
        repeat = ASCIILiteral("repeat-y");
    else
        repeat = ASCIILiteral("no-repeat");

    RefPtr<JSON::ArrayOf<JSON::Value>> array = JSON::ArrayOf<JSON::Value>::create();
    array->addItem(indexForData(imageBuffer->toDataURL("image/png")));
    array->addItem(indexForData(repeat));
    return array;
}

RefPtr<JSON::ArrayOf<JSON::Value>> InspectorCanvas::buildArrayForImageData(const ImageData& imageData)
{
    RefPtr<JSON::ArrayOf<int>> data = JSON::ArrayOf<int>::create();
    for (size_t i = 0; i < imageData.data()->length(); ++i)
        data->addItem(imageData.data()->item(i));

    RefPtr<JSON::ArrayOf<JSON::Value>> array = JSON::ArrayOf<JSON::Value>::create();
    array->addItem(WTFMove(data));
    array->addItem(imageData.width());
    array->addItem(imageData.height());
    return array;
}

} // namespace WebCore

