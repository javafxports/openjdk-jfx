/*
 * Copyright (C) Research In Motion Limited 2010, 2012. All rights reserved.
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "SVGPathUtilities.h"

#include "FloatPoint.h"
#include "Path.h"
#include "PathTraversalState.h"
#include "SVGPathBlender.h"
#include "SVGPathBuilder.h"
#include "SVGPathByteStreamBuilder.h"
#include "SVGPathByteStreamSource.h"
#include "SVGPathConsumer.h"
#include "SVGPathElement.h"
#include "SVGPathParser.h"
#include "SVGPathSegListBuilder.h"
#include "SVGPathSegListSource.h"
#include "SVGPathStringBuilder.h"
#include "SVGPathStringSource.h"
#include "SVGPathTraversalStateBuilder.h"

namespace WebCore {

Path buildPathFromString(const String& d)
{
    if (d.isEmpty())
        return { };

    Path path;
    SVGPathBuilder builder(path);
    SVGPathStringSource source(d);
    SVGPathParser::parse(source, builder);
    return path;
}

String buildStringFromPath(const Path& path)
{
    StringBuilder builder;

    if (!path.isNull() && !path.isEmpty()) {
        path.apply([&builder] (const PathElement& element) {
            switch (element.type) {
            case PathElementMoveToPoint:
                builder.append('M');
                builder.appendECMAScriptNumber(element.points[0].x());
                builder.append(' ');
                builder.appendECMAScriptNumber(element.points[0].y());
                break;
            case PathElementAddLineToPoint:
                builder.append('L');
                builder.appendECMAScriptNumber(element.points[0].x());
                builder.append(' ');
                builder.appendECMAScriptNumber(element.points[0].y());
                break;
            case PathElementAddQuadCurveToPoint:
                builder.append('Q');
                builder.appendECMAScriptNumber(element.points[0].x());
                builder.append(' ');
                builder.appendECMAScriptNumber(element.points[0].y());
                builder.append(',');
                builder.appendECMAScriptNumber(element.points[1].x());
                builder.append(' ');
                builder.appendECMAScriptNumber(element.points[1].y());
                break;
            case PathElementAddCurveToPoint:
                builder.append('C');
                builder.appendECMAScriptNumber(element.points[0].x());
                builder.append(' ');
                builder.appendECMAScriptNumber(element.points[0].y());
                builder.append(',');
                builder.appendECMAScriptNumber(element.points[1].x());
                builder.append(' ');
                builder.appendECMAScriptNumber(element.points[1].y());
                builder.append(',');
                builder.appendECMAScriptNumber(element.points[2].x());
                builder.append(' ');
                builder.appendECMAScriptNumber(element.points[2].y());
                break;
            case PathElementCloseSubpath:
                builder.append('Z');
                break;
            }
        });
    }

    return builder.toString();
}

bool buildSVGPathByteStreamFromSVGPathSegListValues(const SVGPathSegListValues& list, SVGPathByteStream& result, PathParsingMode parsingMode)
{
    result.clear();
    if (list.isEmpty())
        return true;

    SVGPathSegListSource source(list);
    return SVGPathParser::parseToByteStream(source, result, parsingMode);
}

bool appendSVGPathByteStreamFromSVGPathSeg(RefPtr<SVGPathSeg>&& pathSeg, SVGPathByteStream& result, PathParsingMode parsingMode)
{
    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=15412 - Implement normalized path segment lists!
    ASSERT(parsingMode == UnalteredParsing);

    SVGPathSegListValues appendedItemList(PathSegUnalteredRole);
    appendedItemList.append(WTFMove(pathSeg));

    SVGPathByteStream appendedByteStream;
    SVGPathSegListSource source(appendedItemList);
    bool ok = SVGPathParser::parseToByteStream(source, result, parsingMode, false);

    if (ok)
        result.append(appendedByteStream);

    return ok;
}

Path buildPathFromByteStream(const SVGPathByteStream& stream)
{
    if (stream.isEmpty())
        return { };

    Path path;
    SVGPathBuilder builder(path);
    SVGPathByteStreamSource source(stream);
    SVGPathParser::parse(source, builder);
    return path;
}

bool buildSVGPathSegListValuesFromByteStream(const SVGPathByteStream& stream, SVGPathElement& element, SVGPathSegListValues& result, PathParsingMode parsingMode)
{
    if (stream.isEmpty())
        return true;

    SVGPathSegListBuilder builder(element, result, parsingMode == NormalizedParsing ? PathSegNormalizedRole : PathSegUnalteredRole);
    SVGPathByteStreamSource source(stream);
    return SVGPathParser::parse(source, builder, parsingMode);
}

bool buildStringFromByteStream(const SVGPathByteStream& stream, String& result, PathParsingMode parsingMode)
{
    if (stream.isEmpty())
        return true;

    SVGPathByteStreamSource source(stream);
    return SVGPathParser::parseToString(source, result, parsingMode);
}

bool buildStringFromSVGPathSegListValues(const SVGPathSegListValues& list, String& result, PathParsingMode parsingMode)
{
    result = String();
    if (list.isEmpty())
        return true;

    SVGPathSegListSource source(list);
    return SVGPathParser::parseToString(source, result, parsingMode);
}

bool buildSVGPathByteStreamFromString(const String& d, SVGPathByteStream& result, PathParsingMode parsingMode)
{
    result.clear();
    if (d.isEmpty())
        return true;

    SVGPathStringSource source(d);
    return SVGPathParser::parseToByteStream(source, result, parsingMode);
}

bool canBlendSVGPathByteStreams(const SVGPathByteStream& fromStream, const SVGPathByteStream& toStream)
{
    SVGPathByteStreamSource fromSource(fromStream);
    SVGPathByteStreamSource toSource(toStream);
    return SVGPathBlender::canBlendPaths(fromSource, toSource);
}

bool buildAnimatedSVGPathByteStream(const SVGPathByteStream& fromStream, const SVGPathByteStream& toStream, SVGPathByteStream& result, float progress)
{
    ASSERT(&toStream != &result);
    result.clear();
    if (toStream.isEmpty())
        return true;

    SVGPathByteStreamBuilder builder(result);

    SVGPathByteStreamSource fromSource(fromStream);
    SVGPathByteStreamSource toSource(toStream);
    return SVGPathBlender::blendAnimatedPath(fromSource, toSource, builder, progress);
}

bool addToSVGPathByteStream(SVGPathByteStream& streamToAppendTo, const SVGPathByteStream& byStream, unsigned repeatCount)
{
    // Why return when streamToAppendTo is empty? Don't we still need to append?
    if (streamToAppendTo.isEmpty() || byStream.isEmpty())
        return true;

    // Is it OK to make the SVGPathByteStreamBuilder from a stream, and then clear that stream?
    SVGPathByteStreamBuilder builder(streamToAppendTo);

    SVGPathByteStream fromStreamCopy = streamToAppendTo;
    streamToAppendTo.clear();

    SVGPathByteStreamSource fromSource(fromStreamCopy);
    SVGPathByteStreamSource bySource(byStream);
    return SVGPathBlender::addAnimatedPath(fromSource, bySource, builder, repeatCount);
}

bool getSVGPathSegAtLengthFromSVGPathByteStream(const SVGPathByteStream& stream, float length, unsigned& pathSeg)
{
    if (stream.isEmpty())
        return false;

    PathTraversalState traversalState(PathTraversalState::Action::SegmentAtLength);
    SVGPathTraversalStateBuilder builder(traversalState, length);

    SVGPathByteStreamSource source(stream);
    bool ok = SVGPathParser::parse(source, builder);
    pathSeg = builder.pathSegmentIndex();
    return ok;
}

bool getTotalLengthOfSVGPathByteStream(const SVGPathByteStream& stream, float& totalLength)
{
    if (stream.isEmpty())
        return false;

    PathTraversalState traversalState(PathTraversalState::Action::TotalLength);

    SVGPathTraversalStateBuilder builder(traversalState);

    SVGPathByteStreamSource source(stream);
    bool ok = SVGPathParser::parse(source, builder);
    totalLength = builder.totalLength();
    return ok;
}

bool getPointAtLengthOfSVGPathByteStream(const SVGPathByteStream& stream, float length, FloatPoint& point)
{
    if (stream.isEmpty())
        return false;

    PathTraversalState traversalState(PathTraversalState::Action::VectorAtLength);

    SVGPathTraversalStateBuilder builder(traversalState, length);

    SVGPathByteStreamSource source(stream);
    bool ok = SVGPathParser::parse(source, builder);
    point = builder.currentPoint();
    return ok;
}

}
