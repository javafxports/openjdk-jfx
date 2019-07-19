/*
 * Copyright (C) 2019 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WHLSLSynthesizeEnumerationFunctions.h"

#if ENABLE(WEBGPU)

#include "WHLSLAddressSpace.h"
#include "WHLSLProgram.h"
#include "WHLSLTypeReference.h"

namespace WebCore {

namespace WHLSL {

void synthesizeEnumerationFunctions(Program& program)
{
    bool isOperator = true;
    for (auto& enumerationDefinition : program.enumerationDefinitions()) {
        {
            AST::VariableDeclaration variableDeclaration1(Lexer::Token(enumerationDefinition->origin()), AST::Qualifiers(), { AST::TypeReference::wrap(Lexer::Token(enumerationDefinition->origin()), enumerationDefinition) }, String(), WTF::nullopt, WTF::nullopt);
            AST::VariableDeclaration variableDeclaration2(Lexer::Token(enumerationDefinition->origin()), AST::Qualifiers(), { AST::TypeReference::wrap(Lexer::Token(enumerationDefinition->origin()), enumerationDefinition) }, String(), WTF::nullopt, WTF::nullopt);
            AST::VariableDeclarations parameters;
            parameters.append(WTFMove(variableDeclaration1));
            parameters.append(WTFMove(variableDeclaration2));
            AST::NativeFunctionDeclaration nativeFunctionDeclaration(AST::FunctionDeclaration(Lexer::Token(enumerationDefinition->origin()), AST::AttributeBlock(), WTF::nullopt, AST::TypeReference::wrap(Lexer::Token(enumerationDefinition->origin()), program.intrinsics().boolType()), "operator=="_str, WTFMove(parameters), WTF::nullopt, isOperator));
            program.append(WTFMove(nativeFunctionDeclaration));
        }

        {
            AST::VariableDeclaration variableDeclaration(Lexer::Token(enumerationDefinition->origin()), AST::Qualifiers(), { AST::TypeReference::wrap(Lexer::Token(enumerationDefinition->origin()), enumerationDefinition) }, String(), WTF::nullopt, WTF::nullopt);
            AST::VariableDeclarations parameters;
            parameters.append(WTFMove(variableDeclaration));
            AST::NativeFunctionDeclaration nativeFunctionDeclaration(AST::FunctionDeclaration(Lexer::Token(enumerationDefinition->origin()), AST::AttributeBlock(), WTF::nullopt, enumerationDefinition->type().clone(), "operator.value"_str, WTFMove(parameters), WTF::nullopt, isOperator));
            program.append(WTFMove(nativeFunctionDeclaration));
        }

        {
            AST::VariableDeclaration variableDeclaration(Lexer::Token(enumerationDefinition->origin()), AST::Qualifiers(), { AST::TypeReference::wrap(Lexer::Token(enumerationDefinition->origin()), enumerationDefinition) }, String(), WTF::nullopt, WTF::nullopt);
            AST::VariableDeclarations parameters;
            parameters.append(WTFMove(variableDeclaration));
            AST::NativeFunctionDeclaration nativeFunctionDeclaration(AST::FunctionDeclaration(Lexer::Token(enumerationDefinition->origin()), AST::AttributeBlock(), WTF::nullopt, enumerationDefinition->type().clone(), "operator cast"_str, WTFMove(parameters), WTF::nullopt, isOperator));
            program.append(WTFMove(nativeFunctionDeclaration));
        }

        {
            AST::VariableDeclaration variableDeclaration(Lexer::Token(enumerationDefinition->origin()), AST::Qualifiers(), enumerationDefinition->type().clone(), String(), WTF::nullopt, WTF::nullopt);
            AST::VariableDeclarations parameters;
            parameters.append(WTFMove(variableDeclaration));
            AST::NativeFunctionDeclaration nativeFunctionDeclaration(AST::FunctionDeclaration(Lexer::Token(enumerationDefinition->origin()), AST::AttributeBlock(), WTF::nullopt, { AST::TypeReference::wrap(Lexer::Token(enumerationDefinition->origin()), enumerationDefinition) }, "operator cast"_str, WTFMove(parameters), WTF::nullopt, isOperator));
            program.append(WTFMove(nativeFunctionDeclaration));
        }
    }
}

} // namespace WHLSL

} // namespace WebCore

#endif // ENABLE(WEBGPU)
