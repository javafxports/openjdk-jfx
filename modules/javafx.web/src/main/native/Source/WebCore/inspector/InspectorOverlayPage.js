const lightGridColor = "rgba(0,0,0,0.2)";
const darkGridColor = "rgba(0,0,0,0.5)";
const transparentColor = "rgba(0, 0, 0, 0)";
const gridBackgroundColor = "rgba(255, 255, 255, 0.6)";

// CSS Shapes highlight colors
const shapeHighlightColor = "rgba(96, 82, 127, 0.8)";
const shapeMarginHighlightColor = "rgba(96, 82, 127, 0.6)";

const paintRectFillColor = "rgba(255, 0, 0, 0.5)";

function drawPausedInDebuggerMessage(message)
{
    var pausedInDebugger = document.getElementById("paused-in-debugger");
    pausedInDebugger.textContent = message;
    pausedInDebugger.style.visibility = "visible";
    document.body.classList.add("dimmed");
}

function quadToPath(quad)
{
    context.beginPath();
    context.moveTo(quad[0].x, quad[0].y);
    context.lineTo(quad[1].x, quad[1].y);
    context.lineTo(quad[2].x, quad[2].y);
    context.lineTo(quad[3].x, quad[3].y);
    context.closePath();
    return context;
}

function drawOutlinedQuad(quad, fillColor, outlineColor)
{
    context.save();
    context.lineWidth = 2;
    quadToPath(quad).clip();
    context.fillStyle = fillColor;
    context.fill();
    if (outlineColor) {
        context.strokeStyle = outlineColor;
        context.stroke();
    }
    context.restore();
}

function pathCommand(context, commands, name, index, length) {
    context[name].apply(context, commands.slice(index + 1, index + length + 1));
    return index + length + 1;
}

function drawPath(context, commands, fillColor, outlineColor)
{
    context.save();
    context.beginPath();

    var commandsIndex = 0;
    var commandsLength = commands.length;
    while(commandsIndex < commandsLength) {
        switch(commands[commandsIndex]) {
        // 1 point
        case 'M':
            commandsIndex = pathCommand(context, commands, "moveTo", commandsIndex, 2);
            break;
        // 1 point
        case 'L':
            commandsIndex = pathCommand(context, commands, "lineTo", commandsIndex, 2);
            break;
        // 3 points
        case 'C':
            commandsIndex = pathCommand(context, commands, "bezierCurveTo", commandsIndex, 6);
            break;
        // 2 points
        case 'Q':
            commandsIndex = pathCommand(context, commands, "quadraticCurveTo", commandsIndex, 2);
            break;
        // 0 points
        case 'Z':
            commandsIndex = pathCommand(context, commands, "closePath", commandsIndex, 0);
            break;
        default:
            commandsIndex++;
        }
    }

    context.closePath();
    context.fillStyle = fillColor;
    context.fill();

    if (outlineColor) {
        context.lineWidth = 2;
        context.strokeStyle = outlineColor;
        context.stroke();
    }

    context.restore();
}

function drawOutlinedQuadWithClip(quad, clipQuad, fillColor)
{
    var canvas = document.getElementById("canvas");
    context.fillStyle = fillColor;
    context.save();
    context.lineWidth = 0;
    quadToPath(quad).fill();
    context.globalCompositeOperation = "destination-out";
    context.fillStyle = "red";
    quadToPath(clipQuad).fill();
    context.restore();
}

function quadEquals(quad1, quad2)
{
    return quad1[0].x === quad2[0].x && quad1[0].y === quad2[0].y &&
        quad1[1].x === quad2[1].x && quad1[1].y === quad2[1].y &&
        quad1[2].x === quad2[2].x && quad1[2].y === quad2[2].y &&
        quad1[3].x === quad2[3].x && quad1[3].y === quad2[3].y;
}

function drawGutter()
{
    var frameWidth = frameViewFullSize.width;
    var frameHeight = frameViewFullSize.height;

    if (!frameWidth || document.body.offsetWidth <= frameWidth)
        rightGutter.style.removeProperty("display");
    else {
        rightGutter.style.display = "block";
        rightGutter.style.left = frameWidth + "px";
    }

    if (!frameHeight || document.body.offsetHeight <= frameHeight)
        bottomGutter.style.removeProperty("display");
    else {
        bottomGutter.style.display = "block";
        bottomGutter.style.top = frameHeight + "px";
    }
}

var updatePaintRectsIntervalID;

function updatePaintRects(paintRectList)
{
    var context = paintRectsCanvas.getContext("2d");
    context.save();
    context.scale(window.devicePixelRatio, window.devicePixelRatio);

    context.clearRect(0, 0, paintRectsCanvas.width, paintRectsCanvas.height);

    context.fillStyle = paintRectFillColor;

    for (var rectObject of paintRectList)
        context.fillRect(rectObject.x, rectObject.y, rectObject.width, rectObject.height);

    context.restore();
}

function reset(resetData)
{
    var deviceScaleFactor = resetData.deviceScaleFactor;
    var viewportSize = resetData.viewportSize;
    window.frameViewFullSize = resetData.frameViewFullSize;

    window.canvas = document.getElementById("canvas");
    window.paintRectsCanvas = document.getElementById("paintrects-canvas");

    window.context = canvas.getContext("2d");
    window.rightGutter = document.getElementById("right-gutter");
    window.bottomGutter = document.getElementById("bottom-gutter");

    canvas.width = deviceScaleFactor * viewportSize.width;
    canvas.height = deviceScaleFactor * viewportSize.height;
    canvas.style.width = viewportSize.width + "px";
    canvas.style.height = viewportSize.height + "px";
    context.scale(deviceScaleFactor, deviceScaleFactor);

    // We avoid getting the context for the paint rects canvas until we need to paint, to avoid backing store allocation.
    paintRectsCanvas.width = deviceScaleFactor * viewportSize.width;
    paintRectsCanvas.height = deviceScaleFactor * viewportSize.height;
    paintRectsCanvas.style.width = viewportSize.width + "px";
    paintRectsCanvas.style.height = viewportSize.height + "px";

    document.getElementById("paused-in-debugger").style.visibility = "hidden";
    document.getElementById("element-title-container").innerHTML = "";
    document.body.classList.remove("dimmed");
}

function DOMBuilder(tagName, className)
{
    this.element = document.createElement(tagName);
    this.element.className = className;
}

DOMBuilder.prototype.appendTextNode = function(content)
{
    let node = document.createTextNode(content);
    this.element.appendChild(node);
    return node;
}

DOMBuilder.prototype.appendSpan = function(className, value)
{
    var span = document.createElement("span");
    span.className = className;
    span.textContent = value;
    this.element.appendChild(span);
    return span;
}

DOMBuilder.prototype.appendSpanIfNotNull = function(className, value, prefix)
{
    return value ? this.appendSpan(className, (prefix ? prefix : "") + value) : null;
}

DOMBuilder.prototype.appendProperty = function(className, propertyName, value)
{
    var builder = new DOMBuilder("div", className);
    builder.appendSpan("css-property", propertyName);
    builder.appendTextNode(" ");
    builder.appendSpan("value", value);
    this.element.appendChild(builder.element);
    return builder.element;
}

DOMBuilder.prototype.appendPropertyIfNotNull = function(className, propertyName, value)
{
    return value ? this.appendProperty(className, propertyName, value) : null;
}

function _truncateString(value, maxLength)
{
    return value && value.length > maxLength ? value.substring(0, 50) + "\u2026" : value;
}

function _createElementTitle(elementData)
{
    let builder = new DOMBuilder("div", "element-title");

    builder.appendSpanIfNotNull("tag-name", elementData.tagName);
    builder.appendSpanIfNotNull("node-id", CSS.escape(elementData.idValue), "#");

    let classes = elementData.classes;
    if (classes && classes.length)
        builder.appendSpan("class-name", _truncateString(classes.map((className) => "." + CSS.escape(className)).join(""), 50));

    builder.appendSpanIfNotNull("pseudo-type", elementData.pseudoElement, "::");

    builder.appendTextNode(" ");
    builder.appendSpan("node-width", elementData.size.width);
    // \xd7 is the code for the &times; HTML entity.
    builder.appendSpan("px", "px \xd7 ");
    builder.appendSpan("node-height", elementData.size.height);
    builder.appendSpan("px", "px");

    builder.appendPropertyIfNotNull("role-name", "Role", elementData.role);

    document.getElementById("element-title-container").appendChild(builder.element);

    return builder.element;
}

function _drawElementTitle(elementData, fragmentHighlight, scroll)
{
    if (!elementData || !fragmentHighlight.quads.length)
        return;

    var elementTitle = _createElementTitle(elementData);

    var marginQuad = fragmentHighlight.quads[0];

    var titleWidth = elementTitle.offsetWidth + 6;
    var titleHeight = elementTitle.offsetHeight + 4;

    var anchorTop = marginQuad[0].y;
    var anchorBottom = marginQuad[3].y;

    const arrowHeight = 7;
    var renderArrowUp = false;
    var renderArrowDown = false;

    var boxX = marginQuad[0].x;

    boxX = Math.max(2, boxX - scroll.x);
    anchorTop -= scroll.y;
    anchorBottom -= scroll.y;

    if (boxX + titleWidth > canvas.width)
        boxX = canvas.width - titleWidth - 2;

    var boxY;
    if (anchorTop > canvas.height) {
        boxY = canvas.height - titleHeight - arrowHeight;
        renderArrowDown = true;
    } else if (anchorBottom < 0) {
        boxY = arrowHeight;
        renderArrowUp = true;
    } else if (anchorBottom + titleHeight + arrowHeight < canvas.height) {
        boxY = anchorBottom + arrowHeight - 4;
        renderArrowUp = true;
    } else if (anchorTop - titleHeight - arrowHeight > 0) {
        boxY = anchorTop - titleHeight - arrowHeight + 3;
        renderArrowDown = true;
    } else
        boxY = arrowHeight;

    context.save();
    context.translate(0.5, 0.5);
    context.beginPath();
    context.moveTo(boxX, boxY);
    if (renderArrowUp) {
        context.lineTo(boxX + 2 * arrowHeight, boxY);
        context.lineTo(boxX + 3 * arrowHeight, boxY - arrowHeight);
        context.lineTo(boxX + 4 * arrowHeight, boxY);
    }
    context.lineTo(boxX + titleWidth, boxY);
    context.lineTo(boxX + titleWidth, boxY + titleHeight);
    if (renderArrowDown) {
        context.lineTo(boxX + 4 * arrowHeight, boxY + titleHeight);
        context.lineTo(boxX + 3 * arrowHeight, boxY + titleHeight + arrowHeight);
        context.lineTo(boxX + 2 * arrowHeight, boxY + titleHeight);
    }
    context.lineTo(boxX, boxY + titleHeight);
    context.closePath();
    context.fillStyle = "rgb(255, 255, 194)";
    context.fill();
    context.strokeStyle = "rgb(128, 128, 128)";
    context.stroke();

    context.restore();

    elementTitle.style.top = (boxY + 3) + "px";
    elementTitle.style.left = (boxX + 3) + "px";
}

function _drawShapeHighlight(shapeInfo)
{
    if (shapeInfo.marginShape)
        drawPath(context, shapeInfo.marginShape, shapeMarginHighlightColor);

    if (shapeInfo.shape)
        drawPath(context, shapeInfo.shape, shapeHighlightColor);

    if (!(shapeInfo.shape || shapeInfo.marginShape))
        drawOutlinedQuad(shapeInfo.bounds, shapeHighlightColor, shapeHighlightColor);
}

function _drawFragmentHighlight(highlight)
{
    if (!highlight.quads.length)
        return;

    context.save();

    var quads = highlight.quads.slice();
    var contentQuad = quads.pop();
    var paddingQuad = quads.pop();
    var borderQuad = quads.pop();
    var marginQuad = quads.pop();

    var hasContent = contentQuad && highlight.contentColor !== transparentColor || highlight.contentOutlineColor !== transparentColor;
    var hasPadding = paddingQuad && highlight.paddingColor !== transparentColor;
    var hasBorder = borderQuad && highlight.borderColor !== transparentColor;
    var hasMargin = marginQuad && highlight.marginColor !== transparentColor;

    var clipQuad;
    if (hasMargin && (!hasBorder || !quadEquals(marginQuad, borderQuad))) {
        drawOutlinedQuadWithClip(marginQuad, borderQuad, highlight.marginColor);
        clipQuad = borderQuad;
    }
    if (hasBorder && (!hasPadding || !quadEquals(borderQuad, paddingQuad))) {
        drawOutlinedQuadWithClip(borderQuad, paddingQuad, highlight.borderColor);
        clipQuad = paddingQuad;
    }
    if (hasPadding && (!hasContent || !quadEquals(paddingQuad, contentQuad))) {
        drawOutlinedQuadWithClip(paddingQuad, contentQuad, highlight.paddingColor);
        clipQuad = contentQuad;
    }
    if (hasContent)
        drawOutlinedQuad(contentQuad, highlight.contentColor, highlight.contentOutlineColor);

    var width = canvas.width;
    var height = canvas.height;
    var minX = Number.MAX_VALUE, minY = Number.MAX_VALUE, maxX = Number.MIN_VALUE; maxY = Number.MIN_VALUE;
    for (var i = 0; i < highlight.quads.length; ++i) {
        var quad = highlight.quads[i];
        for (var j = 0; j < quad.length; ++j) {
            minX = Math.min(minX, quad[j].x);
            maxX = Math.max(maxX, quad[j].x);
            minY = Math.min(minY, quad[j].y);
            maxY = Math.max(maxY, quad[j].y);
        }
    }

    context.restore();
}

function showPageIndication()
{
    document.body.classList.add("indicate");
}

function hidePageIndication()
{
    document.body.classList.remove("indicate");
}

function drawNodeHighlight(allHighlights)
{
    var elementTitleContainer = document.getElementById("element-title-container");
    while (elementTitleContainer.hasChildNodes())
        elementTitleContainer.removeChild(elementTitleContainer.lastChild);

    for (var highlight of allHighlights) {
        context.save();
        context.translate(-highlight.scrollOffset.x, -highlight.scrollOffset.y);

        for (var fragment of highlight.fragments)
            _drawFragmentHighlight(fragment);

        if (highlight.elementData && highlight.elementData.shapeOutsideData)
            _drawShapeHighlight(highlight.elementData.shapeOutsideData);

        context.restore();

        if (allHighlights.length === 1) {
            for (var fragment of highlight.fragments)
                _drawElementTitle(highlight.elementData, fragment, highlight.scrollOffset);
        }
    }
}

function drawQuadHighlight(highlight)
{
    context.save();
    drawOutlinedQuad(highlight.quads[0], highlight.contentColor, highlight.contentOutlineColor);
    context.restore();
}

function setPlatform(platform)
{
    document.body.classList.add("platform-" + platform);
}

function dispatch(message)
{
    var functionName = message.shift();
    window[functionName].apply(null, message);
}

function log(text)
{
    var logEntry = document.createElement("div");
    logEntry.textContent = text;
    document.getElementById("log").appendChild(logEntry);
}
