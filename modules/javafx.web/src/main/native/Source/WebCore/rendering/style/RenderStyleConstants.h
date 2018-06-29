/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Graham Dennis (graham.dennis@gmail.com)
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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
 *
 */

#pragma once

#include <initializer_list>

namespace WTF {
class TextStream;
}

namespace WebCore {

static const size_t PrintColorAdjustBits = 1;
enum PrintColorAdjust {
    PrintColorAdjustEconomy,
    PrintColorAdjustExact
};

// The difference between two styles.  The following values are used:
// - StyleDifferenceEqual - The two styles are identical
// - StyleDifferenceRecompositeLayer - The layer needs its position and transform updated, but no repaint
// - StyleDifferenceRepaint - The object just needs to be repainted.
// - StyleDifferenceRepaintIfTextOrBorderOrOutline - The object needs to be repainted if it contains text or a border or outline.
// - StyleDifferenceRepaintLayer - The layer and its descendant layers needs to be repainted.
// - StyleDifferenceLayoutPositionedMovementOnly - Only the position of this positioned object has been updated
// - StyleDifferenceSimplifiedLayout - Only overflow needs to be recomputed
// - StyleDifferenceSimplifiedLayoutAndPositionedMovement - Both positioned movement and simplified layout updates are required.
// - StyleDifferenceLayout - A full layout is required.
enum StyleDifference {
    StyleDifferenceEqual,
    StyleDifferenceRecompositeLayer,
    StyleDifferenceRepaint,
    StyleDifferenceRepaintIfTextOrBorderOrOutline,
    StyleDifferenceRepaintLayer,
    StyleDifferenceLayoutPositionedMovementOnly,
    StyleDifferenceSimplifiedLayout,
    StyleDifferenceSimplifiedLayoutAndPositionedMovement,
    StyleDifferenceLayout,
    StyleDifferenceNewStyle
};

// When some style properties change, different amounts of work have to be done depending on
// context (e.g. whether the property is changing on an element which has a compositing layer).
// A simple StyleDifference does not provide enough information so we return a bit mask of
// StyleDifferenceContextSensitiveProperties from RenderStyle::diff() too.
enum StyleDifferenceContextSensitiveProperty {
    ContextSensitivePropertyNone        = 0,
    ContextSensitivePropertyTransform   = 1 << 0,
    ContextSensitivePropertyOpacity     = 1 << 1,
    ContextSensitivePropertyFilter      = 1 << 2,
    ContextSensitivePropertyClipRect    = 1 << 3,
    ContextSensitivePropertyClipPath    = 1 << 4,
    ContextSensitivePropertyWillChange  = 1 << 5,
};

// Static pseudo styles. Dynamic ones are produced on the fly.
enum PseudoId : unsigned char {
    // The order must be NOP ID, public IDs, and then internal IDs.
    NOPSEUDO, FIRST_LINE, FIRST_LETTER, MARKER, BEFORE, AFTER, SELECTION, SCROLLBAR,
    // Internal IDs follow:
    SCROLLBAR_THUMB, SCROLLBAR_BUTTON, SCROLLBAR_TRACK, SCROLLBAR_TRACK_PIECE, SCROLLBAR_CORNER, RESIZER,
    AFTER_LAST_INTERNAL_PSEUDOID,
    FIRST_PUBLIC_PSEUDOID = FIRST_LINE,
    FIRST_INTERNAL_PSEUDOID = SCROLLBAR_THUMB,
    PUBLIC_PSEUDOID_MASK = ((1 << FIRST_INTERNAL_PSEUDOID) - 1) & ~((1 << FIRST_PUBLIC_PSEUDOID) - 1)
};

class PseudoIdSet {
public:
    PseudoIdSet()
        : m_data(0)
    {
    }

    PseudoIdSet(std::initializer_list<PseudoId> initializerList)
        : m_data(0)
    {
        for (PseudoId pseudoId : initializerList)
            add(pseudoId);
    }

    static PseudoIdSet fromMask(unsigned rawPseudoIdSet)
    {
        return PseudoIdSet(rawPseudoIdSet);
    }

    bool has(PseudoId pseudoId) const
    {
        ASSERT((sizeof(m_data) * 8) > pseudoId);
        return m_data & (1U << pseudoId);
    }

    void add(PseudoId pseudoId)
    {
        ASSERT((sizeof(m_data) * 8) > pseudoId);
        m_data |= (1U << pseudoId);
    }

    void merge(PseudoIdSet source)
    {
        m_data |= source.m_data;
    }

    PseudoIdSet operator &(const PseudoIdSet& pseudoIdSet) const
    {
        return PseudoIdSet(m_data & pseudoIdSet.m_data);
    }

    PseudoIdSet operator |(const PseudoIdSet& pseudoIdSet) const
    {
        return PseudoIdSet(m_data | pseudoIdSet.m_data);
    }

    explicit operator bool() const
    {
        return m_data;
    }

    unsigned data() const { return m_data; }

    static ptrdiff_t dataMemoryOffset() { return OBJECT_OFFSETOF(PseudoIdSet, m_data); }

private:
    explicit PseudoIdSet(unsigned rawPseudoIdSet)
        : m_data(rawPseudoIdSet)
    {
    }

    unsigned m_data;
};

enum ColumnFill { ColumnFillBalance, ColumnFillAuto };

enum ColumnSpan { ColumnSpanNone = 0, ColumnSpanAll };

enum EBorderCollapse { BSEPARATE = 0, BCOLLAPSE = 1 };

// These have been defined in the order of their precedence for border-collapsing. Do
// not change this order! This order also must match the order in CSSValueKeywords.in.
enum EBorderStyle { BNONE, BHIDDEN, INSET, GROOVE, OUTSET, RIDGE, DOTTED, DASHED, SOLID, DOUBLE };

enum EBorderPrecedence { BOFF, BTABLE, BCOLGROUP, BCOL, BROWGROUP, BROW, BCELL };

enum OutlineIsAuto { AUTO_OFF = 0, AUTO_ON };

enum EPosition {
    StaticPosition = 0,
    RelativePosition = 1,
    AbsolutePosition = 2,
    StickyPosition = 3,
    // This value is required to pack our bits efficiently in RenderObject.
    FixedPosition = 6
};

enum EFloat {
    NoFloat, LeftFloat, RightFloat
};

enum EMarginCollapse { MCOLLAPSE, MSEPARATE, MDISCARD };

// Box decoration attributes. Not inherited.

enum EBoxDecorationBreak { DSLICE, DCLONE };

// Box attributes. Not inherited.

enum EBoxSizing { CONTENT_BOX, BORDER_BOX };

// Random visual rendering model attributes. Not inherited.

enum EOverflow {
    OVISIBLE, OHIDDEN, OSCROLL, OAUTO, OOVERLAY, OPAGEDX, OPAGEDY
};

enum EVerticalAlign {
    BASELINE, MIDDLE, SUB, SUPER, TEXT_TOP,
    TEXT_BOTTOM, TOP, BOTTOM, BASELINE_MIDDLE, LENGTH
};

enum EClear {
    CNONE = 0, CLEFT = 1, CRIGHT = 2, CBOTH = 3
};

enum ETableLayout {
    TAUTO, TFIXED
};

enum TextCombine {
    TextCombineNone, TextCombineHorizontal
};

enum EFillAttachment {
    ScrollBackgroundAttachment, LocalBackgroundAttachment, FixedBackgroundAttachment
};

enum EFillBox {
    BorderFillBox, PaddingFillBox, ContentFillBox, TextFillBox
};

enum EFillRepeat {
    RepeatFill, NoRepeatFill, RoundFill, SpaceFill
};

enum EFillLayerType {
    BackgroundFillLayer, MaskFillLayer
};

// CSS3 Background Values
enum EFillSizeType { Contain, Cover, SizeLength, SizeNone };

// CSS3 <position>
enum class Edge { Top, Right, Bottom, Left };

// CSS3 Mask Source Types
enum EMaskSourceType { MaskAlpha, MaskLuminance };

// CSS3 Marquee Properties

enum class MarqueeBehavior {
    None, Scroll, Slide, Alternate
};

enum class MarqueeDirection {
    Auto,
    Left,
    Right,
    Up,
    Down,
    Forward,
    Backward
};

// Deprecated Flexible Box Properties

enum EBoxPack { Start, Center, End, Justify };
enum EBoxAlignment { BSTRETCH, BSTART, BCENTER, BEND, BBASELINE };
enum EBoxOrient { HORIZONTAL, VERTICAL };
enum EBoxLines { SINGLE, MULTIPLE };
enum EBoxDirection { BNORMAL, BREVERSE };

// CSS3 Flexbox Properties

enum EAlignContent { AlignContentFlexStart, AlignContentFlexEnd, AlignContentCenter, AlignContentSpaceBetween, AlignContentSpaceAround, AlignContentStretch };
enum EFlexDirection { FlowRow, FlowRowReverse, FlowColumn, FlowColumnReverse };
enum EFlexWrap { FlexNoWrap, FlexWrap, FlexWrapReverse };
enum ItemPosition { ItemPositionLegacy, ItemPositionAuto, ItemPositionNormal, ItemPositionStretch, ItemPositionBaseline, ItemPositionLastBaseline, ItemPositionCenter, ItemPositionStart, ItemPositionEnd, ItemPositionSelfStart, ItemPositionSelfEnd, ItemPositionFlexStart, ItemPositionFlexEnd, ItemPositionLeft, ItemPositionRight };
enum OverflowAlignment { OverflowAlignmentDefault, OverflowAlignmentUnsafe, OverflowAlignmentSafe };
enum ItemPositionType { NonLegacyPosition, LegacyPosition };
enum ContentPosition { ContentPositionNormal, ContentPositionBaseline, ContentPositionLastBaseline, ContentPositionCenter, ContentPositionStart, ContentPositionEnd, ContentPositionFlexStart, ContentPositionFlexEnd, ContentPositionLeft, ContentPositionRight };
enum ContentDistributionType { ContentDistributionDefault, ContentDistributionSpaceBetween, ContentDistributionSpaceAround, ContentDistributionSpaceEvenly, ContentDistributionStretch };

enum ETextSecurity {
    TSNONE, TSDISC, TSCIRCLE, TSSQUARE
};

// CSS3 User Modify Properties

enum EUserModify {
    READ_ONLY, READ_WRITE, READ_WRITE_PLAINTEXT_ONLY
};

// CSS3 User Drag Values

enum EUserDrag {
    DRAG_AUTO, DRAG_NONE, DRAG_ELEMENT
};

// CSS3 User Select Values

enum EUserSelect {
    SELECT_NONE, SELECT_TEXT, SELECT_ALL
};

// CSS3 Image Values
enum ObjectFit {
    ObjectFitFill, ObjectFitContain, ObjectFitCover, ObjectFitNone, ObjectFitScaleDown
};

enum AspectRatioType {
    AspectRatioAuto, AspectRatioFromIntrinsic, AspectRatioFromDimensions, AspectRatioSpecified
};

enum EWordBreak {
    NormalWordBreak, BreakAllWordBreak, KeepAllWordBreak, BreakWordBreak
};

enum EOverflowWrap {
    NormalOverflowWrap, BreakOverflowWrap
};

enum ENBSPMode {
    NBNORMAL, SPACE
};

enum LineBreak {
    LineBreakAuto, LineBreakLoose, LineBreakNormal, LineBreakStrict, LineBreakAfterWhiteSpace
};

enum EResize {
    RESIZE_NONE, RESIZE_BOTH, RESIZE_HORIZONTAL, RESIZE_VERTICAL
};

// The order of this enum must match the order of the list style types in CSSValueKeywords.in.
enum EListStyleType {
    Disc,
    Circle,
    Square,
    DecimalListStyle,
    DecimalLeadingZero,
    ArabicIndic,
    BinaryListStyle,
    Bengali,
    Cambodian,
    Khmer,
    Devanagari,
    Gujarati,
    Gurmukhi,
    Kannada,
    LowerHexadecimal,
    Lao,
    Malayalam,
    Mongolian,
    Myanmar,
    Octal,
    Oriya,
    Persian,
    Urdu,
    Telugu,
    Tibetan,
    Thai,
    UpperHexadecimal,
    LowerRoman,
    UpperRoman,
    LowerGreek,
    LowerAlpha,
    LowerLatin,
    UpperAlpha,
    UpperLatin,
    Afar,
    EthiopicHalehameAaEt,
    EthiopicHalehameAaEr,
    Amharic,
    EthiopicHalehameAmEt,
    AmharicAbegede,
    EthiopicAbegedeAmEt,
    CjkEarthlyBranch,
    CjkHeavenlyStem,
    Ethiopic,
    EthiopicHalehameGez,
    EthiopicAbegede,
    EthiopicAbegedeGez,
    HangulConsonant,
    Hangul,
    LowerNorwegian,
    Oromo,
    EthiopicHalehameOmEt,
    Sidama,
    EthiopicHalehameSidEt,
    Somali,
    EthiopicHalehameSoEt,
    Tigre,
    EthiopicHalehameTig,
    TigrinyaEr,
    EthiopicHalehameTiEr,
    TigrinyaErAbegede,
    EthiopicAbegedeTiEr,
    TigrinyaEt,
    EthiopicHalehameTiEt,
    TigrinyaEtAbegede,
    EthiopicAbegedeTiEt,
    UpperGreek,
    UpperNorwegian,
    Asterisks,
    Footnotes,
    Hebrew,
    Armenian,
    LowerArmenian,
    UpperArmenian,
    Georgian,
    CJKIdeographic,
    Hiragana,
    Katakana,
    HiraganaIroha,
    KatakanaIroha,
    NoneListStyle
};

enum QuoteType {
    OPEN_QUOTE, CLOSE_QUOTE, NO_OPEN_QUOTE, NO_CLOSE_QUOTE
};

enum EBorderFit { BorderFitBorder, BorderFitLines };

enum EAnimationFillMode { AnimationFillModeNone, AnimationFillModeForwards, AnimationFillModeBackwards, AnimationFillModeBoth };

enum EAnimPlayState {
    AnimPlayStatePlaying = 0x0,
    AnimPlayStatePaused = 0x1
};

enum EWhiteSpace {
    NORMAL, PRE, PRE_WRAP, PRE_LINE, NOWRAP, KHTML_NOWRAP
};

// The order of this enum must match the order of the text align values in CSSValueKeywords.in.
enum ETextAlign {
    LEFT, RIGHT, CENTER, JUSTIFY, WEBKIT_LEFT, WEBKIT_RIGHT, WEBKIT_CENTER, TASTART, TAEND,
};

enum ETextTransform {
    CAPITALIZE, UPPERCASE, LOWERCASE, TTNONE
};

#if ENABLE(LETTERPRESS)
static const size_t TextDecorationBits = 5;
#else
static const size_t TextDecorationBits = 4;
#endif
enum TextDecoration {
    TextDecorationNone = 0x0,
    TextDecorationUnderline = 0x1,
    TextDecorationOverline = 0x2,
    TextDecorationLineThrough = 0x4,
    TextDecorationBlink = 0x8,
#if ENABLE(LETTERPRESS)
    TextDecorationLetterpress = 0x10,
#endif
};
inline TextDecoration operator| (TextDecoration a, TextDecoration b) { return TextDecoration(int(a) | int(b)); }
inline TextDecoration& operator|= (TextDecoration& a, TextDecoration b) { return a = a | b; }

enum TextDecorationStyle {
    TextDecorationStyleSolid,
    TextDecorationStyleDouble,
    TextDecorationStyleDotted,
    TextDecorationStyleDashed,
    TextDecorationStyleWavy
};

#if ENABLE(CSS3_TEXT)
enum TextAlignLast {
    TextAlignLastAuto, TextAlignLastStart, TextAlignLastEnd, TextAlignLastLeft, TextAlignLastRight, TextAlignLastCenter, TextAlignLastJustify
};

enum TextJustify {
    TextJustifyAuto, TextJustifyNone, TextJustifyInterWord, TextJustifyDistribute
};
#endif // CSS3_TEXT

enum TextDecorationSkipItems {
    TextDecorationSkipNone = 0,
    TextDecorationSkipInk = 1 << 0,
    TextDecorationSkipObjects = 1 << 1,
    TextDecorationSkipAuto = 1 << 2
};
typedef unsigned TextDecorationSkip;

enum TextUnderlinePosition {
    // FIXME: Implement support for 'under left' and 'under right' values.
    TextUnderlinePositionAuto = 0x1, TextUnderlinePositionAlphabetic = 0x2, TextUnderlinePositionUnder = 0x4
};

enum TextZoom {
    TextZoomNormal, TextZoomReset
};

enum BreakBetween {
    AutoBreakBetween, AvoidBreakBetween, AvoidColumnBreakBetween, AvoidPageBreakBetween, ColumnBreakBetween, PageBreakBetween, LeftPageBreakBetween, RightPageBreakBetween, RectoPageBreakBetween, VersoPageBreakBetween
};
bool alwaysPageBreak(BreakBetween);

enum BreakInside {
    AutoBreakInside, AvoidBreakInside, AvoidColumnBreakInside, AvoidPageBreakInside
};

enum HangingPunctuation {
    NoHangingPunctuation = 0,
    FirstHangingPunctuation = 1 << 0,
    LastHangingPunctuation = 1 << 1,
    AllowEndHangingPunctuation = 1 << 2,
    ForceEndHangingPunctuation = 1 << 3
};
inline HangingPunctuation operator| (HangingPunctuation a, HangingPunctuation b) { return HangingPunctuation(int(a) | int(b)); }
inline HangingPunctuation& operator|= (HangingPunctuation& a, HangingPunctuation b) { return a = a | b; }

enum EEmptyCell {
    SHOW, HIDE
};

enum ECaptionSide {
    CAPTOP, CAPBOTTOM, CAPLEFT, CAPRIGHT
};

enum EListStylePosition { OUTSIDE, INSIDE };

enum EVisibility { VISIBLE, HIDDEN, COLLAPSE };

enum ECursor {
    // The following must match the order in CSSValueKeywords.in.
    CursorAuto,
    CursorDefault,
    // CursorNone
    CursorContextMenu,
    CursorHelp,
    CursorPointer,
    CursorProgress,
    CursorWait,
    CursorCell,
    CursorCrosshair,
    CursorText,
    CursorVerticalText,
    CursorAlias,
    // CursorCopy
    CursorMove,
    CursorNoDrop,
    CursorNotAllowed,
    CursorGrab,
    CursorGrabbing,
    CursorEResize,
    CursorNResize,
    CursorNeResize,
    CursorNwResize,
    CursorSResize,
    CursorSeResize,
    CursorSwResize,
    CursorWResize,
    CursorEwResize,
    CursorNsResize,
    CursorNeswResize,
    CursorNwseResize,
    CursorColResize,
    CursorRowResize,
    CursorAllScroll,
    CursorZoomIn,
    CursorZoomOut,

    // The following are handled as exceptions so don't need to match.
    CursorCopy,
    CursorNone
};

#if ENABLE(CURSOR_VISIBILITY)
enum CursorVisibility {
    CursorVisibilityAuto,
    CursorVisibilityAutoHide,
};
#endif

// The order of this enum must match the order of the display values in CSSValueKeywords.in.
enum EDisplay {
    INLINE, BLOCK, LIST_ITEM, COMPACT, INLINE_BLOCK,
    TABLE, INLINE_TABLE, TABLE_ROW_GROUP,
    TABLE_HEADER_GROUP, TABLE_FOOTER_GROUP, TABLE_ROW,
    TABLE_COLUMN_GROUP, TABLE_COLUMN, TABLE_CELL,
    TABLE_CAPTION, BOX, INLINE_BOX,
    FLEX, WEBKIT_FLEX, INLINE_FLEX, WEBKIT_INLINE_FLEX,
    CONTENTS, GRID, INLINE_GRID, NONE
};

enum EInsideLink {
    NotInsideLink, InsideUnvisitedLink, InsideVisitedLink
};

enum EPointerEvents {
    PE_NONE, PE_AUTO, PE_STROKE, PE_FILL, PE_PAINTED, PE_VISIBLE,
    PE_VISIBLE_STROKE, PE_VISIBLE_FILL, PE_VISIBLE_PAINTED, PE_ALL
};

enum ETransformStyle3D {
    TransformStyle3DFlat, TransformStyle3DPreserve3D
};

enum EBackfaceVisibility {
    BackfaceVisibilityVisible, BackfaceVisibilityHidden
};

enum class TransformBox {
    BorderBox,
    FillBox,
    ViewBox
};

enum ELineClampType { LineClampLineCount, LineClampPercentage };

enum Hyphens { HyphensNone, HyphensManual, HyphensAuto };

enum ESpeakAs {
    SpeakNormal = 0,
    SpeakSpellOut = 1 << 0,
    SpeakDigits = 1 << 1,
    SpeakLiteralPunctuation = 1 << 2,
    SpeakNoPunctuation = 1 << 3
};
inline ESpeakAs operator| (ESpeakAs a, ESpeakAs b) { return ESpeakAs(int(a) | int(b)); }
inline ESpeakAs& operator|= (ESpeakAs& a, ESpeakAs b) { return a = a | b; }

enum TextEmphasisFill { TextEmphasisFillFilled, TextEmphasisFillOpen };

enum TextEmphasisMark { TextEmphasisMarkNone, TextEmphasisMarkAuto, TextEmphasisMarkDot, TextEmphasisMarkCircle, TextEmphasisMarkDoubleCircle, TextEmphasisMarkTriangle, TextEmphasisMarkSesame, TextEmphasisMarkCustom };

enum TextEmphasisPositions {
    TextEmphasisPositionOver = 1 << 0,
    TextEmphasisPositionUnder = 1 << 1,
    TextEmphasisPositionLeft = 1 << 2,
    TextEmphasisPositionRight = 1 << 3
};
typedef unsigned TextEmphasisPosition;

enum class TextOrientation { Mixed, Upright, Sideways };

enum TextOverflow { TextOverflowClip = 0, TextOverflowEllipsis };

enum EImageRendering {
    ImageRenderingAuto = 0,
    ImageRenderingOptimizeSpeed,
    ImageRenderingOptimizeQuality,
    ImageRenderingCrispEdges,
    ImageRenderingPixelated
};

enum ImageResolutionSource { ImageResolutionSpecified = 0, ImageResolutionFromImage };

enum ImageResolutionSnap { ImageResolutionNoSnap = 0, ImageResolutionSnapPixels };

enum Order { LogicalOrder = 0, VisualOrder };

enum ColumnAxis { HorizontalColumnAxis, VerticalColumnAxis, AutoColumnAxis };

enum ColumnProgression { NormalColumnProgression, ReverseColumnProgression };

enum LineSnap { LineSnapNone, LineSnapBaseline, LineSnapContain };

enum LineAlign { LineAlignNone, LineAlignEdges };

enum RubyPosition { RubyPositionBefore, RubyPositionAfter, RubyPositionInterCharacter };

static const size_t GridAutoFlowBits = 4;
enum InternalGridAutoFlowAlgorithm {
    InternalAutoFlowAlgorithmSparse = 0x1,
    InternalAutoFlowAlgorithmDense = 0x2,
};

enum InternalGridAutoFlowDirection {
    InternalAutoFlowDirectionRow = 0x4,
    InternalAutoFlowDirectionColumn = 0x8
};

enum GridAutoFlow {
    AutoFlowRow = InternalAutoFlowAlgorithmSparse | InternalAutoFlowDirectionRow,
    AutoFlowColumn = InternalAutoFlowAlgorithmSparse | InternalAutoFlowDirectionColumn,
    AutoFlowRowDense = InternalAutoFlowAlgorithmDense | InternalAutoFlowDirectionRow,
    AutoFlowColumnDense = InternalAutoFlowAlgorithmDense | InternalAutoFlowDirectionColumn
};

enum AutoRepeatType {
    NoAutoRepeat,
    AutoFill,
    AutoFit
};

// Reasonable maximum to prevent insane font sizes from causing crashes on some platforms (such as Windows).
static const float maximumAllowedFontSize = 1000000.0f;

#if ENABLE(CSS3_TEXT)
enum TextIndentLine { TextIndentFirstLine, TextIndentEachLine };
enum TextIndentType { TextIndentNormal, TextIndentHanging };
#endif

enum Isolation { IsolationAuto, IsolationIsolate };

// Fill, Stroke, ViewBox are just used for SVG.
enum CSSBoxType { BoxMissing = 0, MarginBox, BorderBox, PaddingBox, ContentBox, Fill, Stroke, ViewBox };

#if ENABLE(TOUCH_EVENTS)
enum class TouchAction {
    Auto,
    Manipulation
};
#endif

#if ENABLE(CSS_SCROLL_SNAP)
enum class ScrollSnapStrictness {
    None,
    Proximity,
    Mandatory
};

enum class ScrollSnapAxis {
    XAxis,
    YAxis,
    Block,
    Inline,
    Both
};

enum class ScrollSnapAxisAlignType {
    None,
    Start,
    Center,
    End
};
#endif

#if ENABLE(CSS_TRAILING_WORD)
enum class TrailingWord {
    Auto,
    PartiallyBalanced
};
#endif

#if ENABLE(APPLE_PAY)
enum class ApplePayButtonStyle {
    White,
    WhiteOutline,
    Black,
};

enum class ApplePayButtonType {
    Plain,
    Buy,
    SetUp,
    Donate,
};
#endif

WTF::TextStream& operator<<(WTF::TextStream&, EFillSizeType);
WTF::TextStream& operator<<(WTF::TextStream&, EFillAttachment);
WTF::TextStream& operator<<(WTF::TextStream&, EFillBox);
WTF::TextStream& operator<<(WTF::TextStream&, EFillRepeat);
WTF::TextStream& operator<<(WTF::TextStream&, EMaskSourceType);
WTF::TextStream& operator<<(WTF::TextStream&, Edge);

// These are all minimized combinations of paint-order.
enum class PaintOrder {
    Normal,
    Fill,
    FillMarkers,
    Stroke,
    StrokeMarkers,
    Markers,
    MarkersStroke
};

enum class PaintType {
    Fill,
    Stroke,
    Markers
};

enum class FontLoadingBehavior {
    Auto, Block, Swap, Fallback, Optional
};

extern const float defaultMiterLimit;

} // namespace WebCore
