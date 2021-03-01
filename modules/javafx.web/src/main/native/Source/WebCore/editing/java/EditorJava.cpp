/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
 */
#include "config.h"

#include "Editor.h"
#include "CachedImage.h"
#include "DataObjectJava.h"
#include "DocumentFragment.h"
#include "Frame.h"
#include "HTMLEmbedElement.h"
#include "HTMLImageElement.h"
#include "HTMLInputElement.h"
#include "HTMLObjectElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "Pasteboard.h"
#include "RenderImage.h"
#include "SVGElement.h"
#include "SVGImageElement.h"
#include "XLinkNames.h"
#include "markup.h"

namespace WebCore {

// FIXME-java: Implemetation task is tracked at JDK-8146460.
void Editor::pasteWithPasteboard(Pasteboard* pasteboard, OptionSet<PasteOption> options)
{
    auto range = selectedRange();
    if (!range)
        return;

    bool chosePlainText;
    auto fragment = pasteboard->documentFragment(*m_document.frame(), *range, options.contains(PasteOption::AllowPlainText), chosePlainText);

    if (fragment && options.contains(PasteOption::AsQuotation))
        quoteFragmentForPasting(*fragment);

    if (fragment && shouldInsertFragment(*fragment, *range, EditorInsertAction::Pasted))
        pasteAsFragment(fragment.releaseNonNull(), canSmartReplaceWithPasteboard(*pasteboard), chosePlainText, options.contains(PasteOption::IgnoreMailBlockquote) ? MailBlockquoteHandling::IgnoreBlockquote : MailBlockquoteHandling::RespectBlockquote);
}

RefPtr<DocumentFragment> Editor::webContentFromPasteboard(Pasteboard&, const SimpleRange&, bool /*allowPlainText*/, bool& /*chosePlainText*/)
{
    notImplemented();
    return RefPtr<DocumentFragment>();
}

void Editor::writeImageToPasteboard(Pasteboard& pasteboard, Element& element, const URL& url, const String& title)
{
    pasteboard.writeImage(element, url, title);
}

void Editor::writeSelectionToPasteboard(Pasteboard& pasteboard)
{
    pasteboard.writeSelection(*selectedRange(), canSmartCopyOrDelete(), *m_document.frame(), DefaultSelectedTextType);
}

} // namespace WebCore
