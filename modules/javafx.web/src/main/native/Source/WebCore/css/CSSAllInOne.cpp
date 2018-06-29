/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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

// This all-in-one cpp file cuts down on template bloat to allow us to build our Windows release build.

#include "BasicShapeFunctions.cpp"
#include "CSSAspectRatioValue.cpp"
#include "CSSBasicShapes.cpp"
#include "CSSBorderImage.cpp"
#include "CSSBorderImageSliceValue.cpp"
#include "CSSCalculationValue.cpp"
#include "CSSCanvasValue.cpp"
#include "CSSComputedStyleDeclaration.cpp"
#include "CSSContentDistributionValue.cpp"
#include "CSSCrossfadeValue.cpp"
#include "CSSCursorImageValue.cpp"
#include "CSSDefaultStyleSheets.cpp"
#include "CSSFilterImageValue.cpp"
#include "CSSFontFace.cpp"
#include "CSSFontFaceRule.cpp"
#include "CSSFontFaceSet.cpp"
#include "CSSFontFaceSource.cpp"
#include "CSSFontFaceSrcValue.cpp"
#include "CSSFontFeatureValue.cpp"
#include "CSSFontSelector.cpp"
#include "CSSFontStyleValue.cpp"
#include "CSSFontValue.cpp"
#include "CSSFontVariationValue.cpp"
#include "CSSFunctionValue.cpp"
#include "CSSGradientValue.cpp"
#include "CSSGridLineNamesValue.cpp"
#include "CSSGridTemplateAreasValue.cpp"
#include "CSSGroupingRule.cpp"
#include "CSSImageGeneratorValue.cpp"
#include "CSSImageSetValue.cpp"
#include "CSSImageValue.cpp"
#include "CSSImportRule.cpp"
#include "CSSInheritedValue.cpp"
#include "CSSInitialValue.cpp"
#include "CSSKeyframeRule.cpp"
#include "CSSKeyframesRule.cpp"
#include "CSSLineBoxContainValue.cpp"
#include "CSSMarkup.cpp"
#include "CSSMediaRule.cpp"
#include "CSSNamedImageValue.cpp"
#include "CSSPageRule.cpp"
#include "CSSParser.cpp"
#include "CSSParserSelector.cpp"
#include "CSSProperty.cpp"
#include "CSSPropertySourceData.cpp"
#include "CSSReflectValue.cpp"
#include "CSSRevertValue.cpp"
#include "CSSRule.cpp"
#include "CSSRuleList.cpp"
#include "CSSSegmentedFontFace.cpp"
#include "CSSSelector.cpp"
#include "CSSSelectorList.cpp"
#include "CSSShadowValue.cpp"
#include "CSSStyleDeclaration.cpp"
#include "CSSStyleRule.cpp"
#include "CSSStyleSheet.cpp"
#include "CSSSupportsRule.cpp"
#include "CSSTimingFunctionValue.cpp"
#include "CSSToLengthConversionData.cpp"
#include "CSSToStyleMap.cpp"
#include "CSSUnicodeRangeValue.cpp"
#include "CSSUnsetValue.cpp"
#include "CSSValue.cpp"
#include "CSSValueList.cpp"
#include "CSSValuePool.cpp"
#include "DOMCSSNamespace.cpp"
#include "DocumentRuleSets.cpp"
#include "ElementRuleCollector.cpp"
#include "FontFace.cpp"
#include "FontVariantBuilder.cpp"
#include "InspectorCSSOMWrappers.cpp"
#include "LengthFunctions.cpp"
#include "MediaList.cpp"
#include "MediaQuery.cpp"
#include "MediaQueryEvaluator.cpp"
#include "MediaQueryExpression.cpp"
#include "MediaQueryList.cpp"
#include "MediaQueryMatcher.cpp"
#include "PageRuleCollector.cpp"
#include "PropertySetCSSStyleDeclaration.cpp"
#include "RGBColor.cpp"
#include "RuleFeature.cpp"
#include "RuleSet.cpp"
#include "SVGCSSComputedStyleDeclaration.cpp"
#include "SelectorChecker.cpp"
#include "SelectorFilter.cpp"
#include "StyleInvalidator.cpp"
#include "StyleMedia.cpp"
#include "StyleProperties.cpp"
#include "StylePropertyShorthand.cpp"
#include "StyleResolver.cpp"
#include "StyleRule.cpp"
#include "StyleRuleImport.cpp"
#include "StyleSheet.cpp"
#include "StyleSheetContents.cpp"
#include "StyleSheetList.cpp"
#include "TransformFunctions.cpp"
#include "ViewportStyleResolver.cpp"
#include "WebKitCSSMatrix.cpp"
#include "WebKitCSSViewportRule.cpp"

