/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#include "BaseClickableWithKeyInputType.h"
#include "FileChooser.h"
#include "FileIconLoader.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class DragData;
class FileList;
class FileListCreator;
class Icon;

class FileInputType final : public BaseClickableWithKeyInputType, private FileChooserClient, private FileIconLoaderClient {
public:
    explicit FileInputType(HTMLInputElement&);
    virtual ~FileInputType();

    static Vector<FileChooserFileInfo> filesFromFormControlState(const FormControlState&);

private:
    const AtomicString& formControlType() const final;
    FormControlState saveFormControlState() const final;
    void restoreFormControlState(const FormControlState&) final;
    bool appendFormData(DOMFormData&, bool) const final;
    bool valueMissing(const String&) const final;
    String valueMissingText() const final;
    void handleDOMActivateEvent(Event&) final;
    RenderPtr<RenderElement> createInputRenderer(RenderStyle&&) final;
    bool canSetStringValue() const final;
    FileList* files() final;
    void setFiles(RefPtr<FileList>&&) final;
    enum class RequestIcon { Yes, No };
    void setFiles(RefPtr<FileList>&&, RequestIcon);
    String displayString() const final;
    bool canSetValue(const String&) final;
    bool getTypeSpecificValue(String&) final; // Checked first, before internal storage or the value attribute.
    void setValue(const String&, bool valueChanged, TextFieldEventBehavior) final;

#if ENABLE(DRAG_SUPPORT)
    bool receiveDroppedFiles(const DragData&) final;
#endif

    Icon* icon() const final;
    bool isFileUpload() const final;
    void createShadowSubtree() final;
    void disabledAttributeChanged() final;
    void multipleAttributeChanged() final;
    String defaultToolTip() const final;

    void filesChosen(const Vector<FileChooserFileInfo>&, const String& displayString = { }, Icon* = nullptr) final;

    // FileIconLoaderClient implementation.
    void iconLoaded(RefPtr<Icon>&&) final;

    void requestIcon(const Vector<String>&);

    void applyFileChooserSettings(const FileChooserSettings&);

    bool allowsDirectories() const;

    RefPtr<FileChooser> m_fileChooser;
    std::unique_ptr<FileIconLoader> m_fileIconLoader;

    Ref<FileList> m_fileList;
    RefPtr<FileListCreator> m_fileListCreator;
    RefPtr<Icon> m_icon;
    String m_displayString;
};

} // namespace WebCore
