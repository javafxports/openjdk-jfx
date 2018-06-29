/*
 *  Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 *  Copyright (C) 2007-2018 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#pragma once

#include "ArrayAllocationProfile.h"
#include "ArrayBufferSharingMode.h"
#include "BigIntPrototype.h"
#include "BooleanPrototype.h"
#include "ExceptionHelpers.h"
#include "InternalFunction.h"
#include "JSArray.h"
#include "JSArrayBufferPrototype.h"
#include "JSCPoison.h"
#include "JSClassRef.h"
#include "JSGlobalLexicalEnvironment.h"
#include "JSSegmentedVariableObject.h"
#include "JSWeakObjectMapRefInternal.h"
#include "LazyProperty.h"
#include "LazyClassStructure.h"
#include "NumberPrototype.h"
#include "RuntimeFlags.h"
#include "SpecialPointer.h"
#include "StringPrototype.h"
#include "SymbolPrototype.h"
#include "VM.h"
#include "Watchpoint.h"
#include <JavaScriptCore/JSBase.h>
#include <array>
#include <wtf/HashSet.h>
#include <wtf/PoisonedUniquePtr.h>
#include <wtf/RetainPtr.h>

struct OpaqueJSClass;
struct OpaqueJSClassContextData;
OBJC_CLASS JSWrapperMap;

namespace Inspector {
class JSGlobalObjectInspectorController;
}

namespace JSC {
class ArrayConstructor;
class ArrayPrototype;
class AsyncIteratorPrototype;
class AsyncFunctionPrototype;
class AsyncGeneratorPrototype;
class AsyncGeneratorFunctionPrototype;
class BooleanPrototype;
class ConsoleClient;
class Debugger;
class ErrorConstructor;
class ErrorPrototype;
class EvalCodeBlock;
class EvalExecutable;
class FunctionConstructor;
class FunctionPrototype;
class GeneratorPrototype;
class GeneratorFunctionPrototype;
class GetterSetter;
class GlobalCodeBlock;
class IndirectEvalExecutable;
class InputCursor;
class JSArrayBuffer;
class JSArrayBufferConstructor;
class JSArrayBufferPrototype;
class JSGlobalObjectDebuggable;
class JSInternalPromise;
class JSModuleLoader;
class JSModuleRecord;
class JSPromise;
class JSPromiseConstructor;
class JSPromisePrototype;
class JSSharedArrayBuffer;
class JSSharedArrayBufferConstructor;
class JSSharedArrayBufferPrototype;
class JSTypedArrayViewConstructor;
class JSTypedArrayViewPrototype;
class DirectEvalExecutable;
class LLIntOffsetsExtractor;
class MapPrototype;
class Microtask;
class ModuleLoaderPrototype;
class ModuleProgramExecutable;
class NativeErrorConstructor;
class NullGetterFunction;
class NullSetterFunction;
class ObjectConstructor;
class ProgramCodeBlock;
class ProgramExecutable;
class RegExpConstructor;
class RegExpPrototype;
class SetPrototype;
class SourceCode;
class SourceOrigin;
class UnlinkedModuleProgramCodeBlock;
class VariableEnvironment;
struct ActivationStackNode;
struct HashTable;

template<typename Watchpoint> class ObjectPropertyChangeAdaptiveWatchpoint;

#define DEFINE_STANDARD_BUILTIN(macro, upperName, lowerName) macro(upperName, lowerName, lowerName, JS ## upperName, upperName, object)

#define FOR_EACH_SIMPLE_BUILTIN_TYPE_WITH_CONSTRUCTOR(macro) \
    macro(String, string, stringObject, StringObject, String, object) \
    macro(Symbol, symbol, symbolObject, SymbolObject, Symbol, object) \
    macro(Number, number, numberObject, NumberObject, Number, object) \
    macro(Boolean, boolean, booleanObject, BooleanObject, Boolean, object) \
    macro(Error, error, error, ErrorInstance, Error, object) \
    macro(Map, map, map, JSMap, Map, object) \
    macro(Set, set, set, JSSet, Set, object) \
    macro(JSPromise, promise, promise, JSPromise, Promise, object)

#define FOR_BIG_INT_BUILTIN_TYPE_WITH_CONSTRUCTOR(macro) \
    macro(BigInt, bigInt, bigIntObject, BigIntObject, BigInt, object)

#define FOR_EACH_BUILTIN_DERIVED_ITERATOR_TYPE(macro) \
    macro(StringIterator, stringIterator, stringIterator, JSStringIterator, StringIterator, iterator) \

#define FOR_EACH_SIMPLE_BUILTIN_TYPE(macro) \
    FOR_EACH_SIMPLE_BUILTIN_TYPE_WITH_CONSTRUCTOR(macro) \
    macro(JSInternalPromise, internalPromise, internalPromise, JSInternalPromise, InternalPromise, object) \

#define FOR_EACH_LAZY_BUILTIN_TYPE(macro) \
    macro(Date, date, date, DateInstance, Date, object) \
    DEFINE_STANDARD_BUILTIN(macro, WeakMap, weakMap) \
    DEFINE_STANDARD_BUILTIN(macro, WeakSet, weakSet) \

#if ENABLE(WEBASSEMBLY)
#define FOR_EACH_WEBASSEMBLY_CONSTRUCTOR_TYPE(macro) \
    macro(WebAssemblyCompileError, webAssemblyCompileError, WebAssemblyCompileError, WebAssemblyCompileError, CompileError, error) \
    macro(WebAssemblyInstance,     webAssemblyInstance,     WebAssemblyInstance,     WebAssemblyInstance,     Instance,     object) \
    macro(WebAssemblyLinkError,    webAssemblyLinkError,    WebAssemblyLinkError,    WebAssemblyLinkError,    LinkError,    error) \
    macro(WebAssemblyMemory,       webAssemblyMemory,       WebAssemblyMemory,       WebAssemblyMemory,       Memory,       object) \
    macro(WebAssemblyModule,       webAssemblyModule,       WebAssemblyModule,       WebAssemblyModule,       Module,       object) \
    macro(WebAssemblyRuntimeError, webAssemblyRuntimeError, WebAssemblyRuntimeError, WebAssemblyRuntimeError, RuntimeError, error) \
    macro(WebAssemblyTable,        webAssemblyTable,        WebAssemblyTable,        WebAssemblyTable,        Table,        object)
#else
#define FOR_EACH_WEBASSEMBLY_CONSTRUCTOR_TYPE(macro)
#endif // ENABLE(WEBASSEMBLY)

#define DECLARE_SIMPLE_BUILTIN_TYPE(capitalName, lowerName, properName, instanceType, jsName, prototypeBase) \
    class JS ## capitalName; \
    class capitalName ## Prototype; \
    class capitalName ## Constructor;

class IteratorPrototype;
FOR_EACH_SIMPLE_BUILTIN_TYPE(DECLARE_SIMPLE_BUILTIN_TYPE)
FOR_BIG_INT_BUILTIN_TYPE_WITH_CONSTRUCTOR(DECLARE_SIMPLE_BUILTIN_TYPE)
FOR_EACH_LAZY_BUILTIN_TYPE(DECLARE_SIMPLE_BUILTIN_TYPE)
FOR_EACH_BUILTIN_DERIVED_ITERATOR_TYPE(DECLARE_SIMPLE_BUILTIN_TYPE)
FOR_EACH_WEBASSEMBLY_CONSTRUCTOR_TYPE(DECLARE_SIMPLE_BUILTIN_TYPE)

#undef DECLARE_SIMPLE_BUILTIN_TYPE

enum class JSPromiseRejectionOperation : unsigned {
    Reject, // When a promise is rejected without any handlers.
    Handle, // When a handler is added to a rejected promise for the first time.
};

struct GlobalObjectMethodTable {
    typedef bool (*SupportsRichSourceInfoFunctionPtr)(const JSGlobalObject*);
    SupportsRichSourceInfoFunctionPtr supportsRichSourceInfo;

    typedef bool (*ShouldInterruptScriptFunctionPtr)(const JSGlobalObject*);
    ShouldInterruptScriptFunctionPtr shouldInterruptScript;

    typedef RuntimeFlags (*JavaScriptRuntimeFlagsFunctionPtr)(const JSGlobalObject*);
    JavaScriptRuntimeFlagsFunctionPtr javaScriptRuntimeFlags;

    typedef void (*QueueTaskToEventLoopFunctionPtr)(JSGlobalObject&, Ref<Microtask>&&);
    QueueTaskToEventLoopFunctionPtr queueTaskToEventLoop;

    typedef bool (*ShouldInterruptScriptBeforeTimeoutPtr)(const JSGlobalObject*);
    ShouldInterruptScriptBeforeTimeoutPtr shouldInterruptScriptBeforeTimeout;

    typedef JSInternalPromise* (*ModuleLoaderImportModulePtr)(JSGlobalObject*, ExecState*, JSModuleLoader*, JSString*, JSValue, const SourceOrigin&);
    ModuleLoaderImportModulePtr moduleLoaderImportModule;

    typedef Identifier (*ModuleLoaderResolvePtr)(JSGlobalObject*, ExecState*, JSModuleLoader*, JSValue, JSValue, JSValue);
    ModuleLoaderResolvePtr moduleLoaderResolve;

    typedef JSInternalPromise* (*ModuleLoaderFetchPtr)(JSGlobalObject*, ExecState*, JSModuleLoader*, JSValue, JSValue, JSValue);
    ModuleLoaderFetchPtr moduleLoaderFetch;

    typedef JSObject* (*ModuleLoaderCreateImportMetaPropertiesPtr)(JSGlobalObject*, ExecState*, JSModuleLoader*, JSValue, JSModuleRecord*, JSValue);
    ModuleLoaderCreateImportMetaPropertiesPtr moduleLoaderCreateImportMetaProperties;

    typedef JSValue (*ModuleLoaderEvaluatePtr)(JSGlobalObject*, ExecState*, JSModuleLoader*, JSValue, JSValue, JSValue);
    ModuleLoaderEvaluatePtr moduleLoaderEvaluate;

    typedef void (*PromiseRejectionTrackerPtr)(JSGlobalObject*, ExecState*, JSPromise*, JSPromiseRejectionOperation);
    PromiseRejectionTrackerPtr promiseRejectionTracker;

    typedef String (*DefaultLanguageFunctionPtr)();
    DefaultLanguageFunctionPtr defaultLanguage;
};

class JSGlobalObject : public JSSegmentedVariableObject {
private:
    typedef HashSet<RefPtr<OpaqueJSWeakObjectMap>> WeakMapSet;
    typedef HashMap<OpaqueJSClass*, std::unique_ptr<OpaqueJSClassContextData>> OpaqueJSClassDataMap;

    struct JSGlobalObjectRareData {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        JSGlobalObjectRareData()
            : profileGroup(0)
        {
        }

        WeakMapSet weakMaps;
        unsigned profileGroup;

        OpaqueJSClassDataMap opaqueJSClassData;
    };

// Our hashtable code-generator tries to access these properties, so we make them public.
// However, we'd like it better if they could be protected.
public:
    template<typename T> using Initializer = typename LazyProperty<JSGlobalObject, T>::Initializer;

    Register m_globalCallFrame[CallFrame::headerSizeInRegisters];

    WriteBarrier<JSObject> m_globalThis;

    WriteBarrier<JSGlobalLexicalEnvironment> m_globalLexicalEnvironment;
    WriteBarrier<JSScope> m_globalScopeExtension;
    WriteBarrier<JSObject> m_globalCallee;
    WriteBarrier<RegExpConstructor> m_regExpConstructor;
    WriteBarrier<ErrorConstructor> m_errorConstructor;
    WriteBarrier<Structure> m_nativeErrorPrototypeStructure;
    WriteBarrier<Structure> m_nativeErrorStructure;
    LazyProperty<JSGlobalObject, NativeErrorConstructor> m_evalErrorConstructor;
    WriteBarrier<NativeErrorConstructor> m_rangeErrorConstructor;
    LazyProperty<JSGlobalObject, NativeErrorConstructor> m_referenceErrorConstructor;
    LazyProperty<JSGlobalObject, NativeErrorConstructor> m_syntaxErrorConstructor;
    WriteBarrier<NativeErrorConstructor> m_typeErrorConstructor;
    LazyProperty<JSGlobalObject, NativeErrorConstructor> m_URIErrorConstructor;
    WriteBarrier<ObjectConstructor> m_objectConstructor;
    WriteBarrier<ArrayConstructor> m_arrayConstructor;
    WriteBarrier<JSPromiseConstructor> m_promiseConstructor;
    WriteBarrier<JSInternalPromiseConstructor> m_internalPromiseConstructor;

    WriteBarrier<NullGetterFunction> m_nullGetterFunction;
    WriteBarrier<NullSetterFunction> m_nullSetterFunction;

    WriteBarrier<JSFunction> m_parseIntFunction;
    WriteBarrier<JSFunction> m_parseFloatFunction;

    WriteBarrier<JSFunction> m_evalFunction;
    WriteBarrier<JSFunction> m_callFunction;
    WriteBarrier<JSFunction> m_applyFunction;
    WriteBarrier<JSFunction> m_throwTypeErrorFunction;
    LazyProperty<JSGlobalObject, JSFunction> m_arrayProtoToStringFunction;
    LazyProperty<JSGlobalObject, JSFunction> m_arrayProtoValuesFunction;
    LazyProperty<JSGlobalObject, JSFunction> m_initializePromiseFunction;
    LazyProperty<JSGlobalObject, JSFunction> m_iteratorProtocolFunction;
    LazyProperty<JSGlobalObject, JSFunction> m_promiseResolveFunction;
    WriteBarrier<JSFunction> m_objectProtoValueOfFunction;
    WriteBarrier<JSFunction> m_numberProtoToStringFunction;
    WriteBarrier<JSFunction> m_newPromiseCapabilityFunction;
    WriteBarrier<JSFunction> m_functionProtoHasInstanceSymbolFunction;
    LazyProperty<JSGlobalObject, GetterSetter> m_throwTypeErrorGetterSetter;
    WriteBarrier<JSObject> m_regExpProtoExec;
    WriteBarrier<JSObject> m_regExpProtoSymbolReplace;
    WriteBarrier<JSObject> m_regExpProtoGlobalGetter;
    WriteBarrier<JSObject> m_regExpProtoUnicodeGetter;
    WriteBarrier<GetterSetter> m_throwTypeErrorArgumentsCalleeAndCallerGetterSetter;

    WriteBarrier<JSModuleLoader> m_moduleLoader;

    WriteBarrier<ObjectPrototype> m_objectPrototype;
    WriteBarrier<FunctionPrototype> m_functionPrototype;
    WriteBarrier<ArrayPrototype> m_arrayPrototype;
    WriteBarrier<RegExpPrototype> m_regExpPrototype;
    WriteBarrier<IteratorPrototype> m_iteratorPrototype;
    WriteBarrier<AsyncIteratorPrototype> m_asyncIteratorPrototype;
    WriteBarrier<GeneratorFunctionPrototype> m_generatorFunctionPrototype;
    WriteBarrier<GeneratorPrototype> m_generatorPrototype;
    WriteBarrier<AsyncGeneratorPrototype> m_asyncGeneratorPrototype;
    WriteBarrier<ModuleLoaderPrototype> m_moduleLoaderPrototype;

    LazyProperty<JSGlobalObject, Structure> m_debuggerScopeStructure;
    LazyProperty<JSGlobalObject, Structure> m_withScopeStructure;
    WriteBarrier<Structure> m_strictEvalActivationStructure;
    WriteBarrier<Structure> m_lexicalEnvironmentStructure;
    LazyProperty<JSGlobalObject, Structure> m_moduleEnvironmentStructure;
    WriteBarrier<Structure> m_directArgumentsStructure;
    WriteBarrier<Structure> m_scopedArgumentsStructure;
    WriteBarrier<Structure> m_clonedArgumentsStructure;

    WriteBarrier<Structure> m_objectStructureForObjectConstructor;

    // Lists the actual structures used for having these particular indexing shapes.
    WriteBarrier<Structure> m_originalArrayStructureForIndexingShape[NumberOfIndexingShapes];
    // Lists the structures we should use during allocation for these particular indexing shapes.
    // These structures will differ from the originals list above when we are having a bad time.
    WriteBarrier<Structure> m_arrayStructureForIndexingShapeDuringAllocation[NumberOfIndexingShapes];

    LazyProperty<JSGlobalObject, Structure> m_callbackConstructorStructure;
    LazyProperty<JSGlobalObject, Structure> m_callbackFunctionStructure;
    LazyProperty<JSGlobalObject, Structure> m_callbackObjectStructure;
    WriteBarrier<Structure> m_propertyNameIteratorStructure;
#if JSC_OBJC_API_ENABLED
    LazyProperty<JSGlobalObject, Structure> m_objcCallbackFunctionStructure;
    LazyProperty<JSGlobalObject, Structure> m_objcWrapperObjectStructure;
#endif
    LazyProperty<JSGlobalObject, Structure> m_nullPrototypeObjectStructure;
    WriteBarrier<Structure> m_calleeStructure;
    WriteBarrier<Structure> m_strictFunctionStructure;
    WriteBarrier<Structure> m_arrowFunctionStructure;
    WriteBarrier<Structure> m_sloppyFunctionStructure;
    LazyProperty<JSGlobalObject, Structure> m_boundFunctionStructure;
    LazyProperty<JSGlobalObject, Structure> m_customGetterSetterFunctionStructure;
    WriteBarrier<Structure> m_getterSetterStructure;
    LazyProperty<JSGlobalObject, Structure> m_nativeStdFunctionStructure;
    PropertyOffset m_functionNameOffset;
    WriteBarrier<Structure> m_regExpStructure;
    WriteBarrier<AsyncFunctionPrototype> m_asyncFunctionPrototype;
    WriteBarrier<AsyncGeneratorFunctionPrototype> m_asyncGeneratorFunctionPrototype;
    WriteBarrier<Structure> m_asyncFunctionStructure;
    WriteBarrier<Structure> m_asyncGeneratorFunctionStructure;
    WriteBarrier<Structure> m_generatorFunctionStructure;
    WriteBarrier<Structure> m_dollarVMStructure;
    WriteBarrier<Structure> m_iteratorResultObjectStructure;
    WriteBarrier<Structure> m_regExpMatchesArrayStructure;
    WriteBarrier<Structure> m_regExpMatchesArrayWithGroupsStructure;
    WriteBarrier<Structure> m_moduleRecordStructure;
    WriteBarrier<Structure> m_moduleNamespaceObjectStructure;
    WriteBarrier<Structure> m_proxyObjectStructure;
    WriteBarrier<Structure> m_callableProxyObjectStructure;
    WriteBarrier<Structure> m_proxyRevokeStructure;
    WriteBarrier<Structure> m_moduleLoaderStructure;
    WriteBarrier<JSArrayBufferPrototype> m_arrayBufferPrototype;
    WriteBarrier<Structure> m_arrayBufferStructure;
#if ENABLE(SHARED_ARRAY_BUFFER)
    WriteBarrier<JSArrayBufferPrototype> m_sharedArrayBufferPrototype;
    WriteBarrier<Structure> m_sharedArrayBufferStructure;
#endif

#define DEFINE_STORAGE_FOR_SIMPLE_TYPE(capitalName, lowerName, properName, instanceType, jsName, prototypeBase) \
    WriteBarrier<capitalName ## Prototype> m_ ## lowerName ## Prototype; \
    WriteBarrier<Structure> m_ ## properName ## Structure;

    FOR_EACH_SIMPLE_BUILTIN_TYPE(DEFINE_STORAGE_FOR_SIMPLE_TYPE)
    FOR_BIG_INT_BUILTIN_TYPE_WITH_CONSTRUCTOR(DEFINE_STORAGE_FOR_SIMPLE_TYPE)
    FOR_EACH_BUILTIN_DERIVED_ITERATOR_TYPE(DEFINE_STORAGE_FOR_SIMPLE_TYPE)

#if ENABLE(WEBASSEMBLY)
    WriteBarrier<Structure> m_webAssemblyStructure;
    WriteBarrier<Structure> m_webAssemblyModuleRecordStructure;
    WriteBarrier<Structure> m_webAssemblyFunctionStructure;
    WriteBarrier<Structure> m_webAssemblyWrapperFunctionStructure;
    WriteBarrier<Structure> m_webAssemblyToJSCalleeStructure;
    FOR_EACH_WEBASSEMBLY_CONSTRUCTOR_TYPE(DEFINE_STORAGE_FOR_SIMPLE_TYPE)
#endif // ENABLE(WEBASSEMBLY)

#undef DEFINE_STORAGE_FOR_SIMPLE_TYPE

#define DEFINE_STORAGE_FOR_LAZY_TYPE(capitalName, lowerName, properName, instanceType, jsName, prototypeBase) \
    LazyClassStructure m_ ## properName ## Structure;
    FOR_EACH_LAZY_BUILTIN_TYPE(DEFINE_STORAGE_FOR_LAZY_TYPE)
#undef DEFINE_STORAGE_FOR_LAZY_TYPE

    WriteBarrier<GetterSetter> m_speciesGetterSetter;

    LazyProperty<JSGlobalObject, JSTypedArrayViewPrototype> m_typedArrayProto;
    LazyProperty<JSGlobalObject, JSTypedArrayViewConstructor> m_typedArraySuperConstructor;

#define DECLARE_TYPED_ARRAY_TYPE_STRUCTURE(name) LazyClassStructure m_typedArray ## name;
    FOR_EACH_TYPED_ARRAY_TYPE(DECLARE_TYPED_ARRAY_TYPE_STRUCTURE)
#undef DECLARE_TYPED_ARRAY_TYPE_STRUCTURE

    JSCell* m_specialPointers[Special::TableSize]; // Special pointers used by the LLInt and JIT.
    JSCell* m_linkTimeConstants[LinkTimeConstantCount];

    String m_name;

    Debugger* m_debugger;

    VM& m_vm;

    template<typename T> using PoisonedUniquePtr = WTF::PoisonedUniquePtr<JSGlobalObjectPoison, T>;

#if ENABLE(REMOTE_INSPECTOR)
    PoisonedUniquePtr<Inspector::JSGlobalObjectInspectorController> m_inspectorController;
    PoisonedUniquePtr<JSGlobalObjectDebuggable> m_inspectorDebuggable;
#endif

#if ENABLE(INTL)
    HashSet<String> m_intlCollatorAvailableLocales;
    HashSet<String> m_intlDateTimeFormatAvailableLocales;
    HashSet<String> m_intlNumberFormatAvailableLocales;
#endif // ENABLE(INTL)

    RefPtr<WatchpointSet> m_masqueradesAsUndefinedWatchpoint;
    RefPtr<WatchpointSet> m_havingABadTimeWatchpoint;
    RefPtr<WatchpointSet> m_varInjectionWatchpoint;

    std::unique_ptr<JSGlobalObjectRareData> m_rareData;

    WeakRandom m_weakRandom;

    InlineWatchpointSet& arrayIteratorProtocolWatchpoint() { return m_arrayIteratorProtocolWatchpoint; }
    InlineWatchpointSet& mapIteratorProtocolWatchpoint() { return m_mapIteratorProtocolWatchpoint; }
    InlineWatchpointSet& setIteratorProtocolWatchpoint() { return m_setIteratorProtocolWatchpoint; }
    InlineWatchpointSet& stringIteratorProtocolWatchpoint() { return m_stringIteratorProtocolWatchpoint; }
    InlineWatchpointSet& mapSetWatchpoint() { return m_mapSetWatchpoint; }
    InlineWatchpointSet& setAddWatchpoint() { return m_setAddWatchpoint; }
    InlineWatchpointSet& arraySpeciesWatchpoint() { return m_arraySpeciesWatchpoint; }
    InlineWatchpointSet& numberToStringWatchpoint() { return m_numberToStringWatchpoint; }
    // If this hasn't been invalidated, it means the array iterator protocol
    // is not observable to user code yet.
    InlineWatchpointSet m_arrayIteratorProtocolWatchpoint;
    InlineWatchpointSet m_mapIteratorProtocolWatchpoint;
    InlineWatchpointSet m_setIteratorProtocolWatchpoint;
    InlineWatchpointSet m_stringIteratorProtocolWatchpoint;
    InlineWatchpointSet m_mapSetWatchpoint;
    InlineWatchpointSet m_setAddWatchpoint;
    InlineWatchpointSet m_arraySpeciesWatchpoint;
    InlineWatchpointSet m_numberToStringWatchpoint;
    PoisonedUniquePtr<ObjectPropertyChangeAdaptiveWatchpoint<InlineWatchpointSet>> m_arrayPrototypeSymbolIteratorWatchpoint;
    PoisonedUniquePtr<ObjectPropertyChangeAdaptiveWatchpoint<InlineWatchpointSet>> m_arrayIteratorPrototypeNext;
    PoisonedUniquePtr<ObjectPropertyChangeAdaptiveWatchpoint<InlineWatchpointSet>> m_mapPrototypeSymbolIteratorWatchpoint;
    PoisonedUniquePtr<ObjectPropertyChangeAdaptiveWatchpoint<InlineWatchpointSet>> m_mapIteratorPrototypeNextWatchpoint;
    PoisonedUniquePtr<ObjectPropertyChangeAdaptiveWatchpoint<InlineWatchpointSet>> m_setPrototypeSymbolIteratorWatchpoint;
    PoisonedUniquePtr<ObjectPropertyChangeAdaptiveWatchpoint<InlineWatchpointSet>> m_setIteratorPrototypeNextWatchpoint;
    PoisonedUniquePtr<ObjectPropertyChangeAdaptiveWatchpoint<InlineWatchpointSet>> m_stringPrototypeSymbolIteratorWatchpoint;
    PoisonedUniquePtr<ObjectPropertyChangeAdaptiveWatchpoint<InlineWatchpointSet>> m_stringIteratorPrototypeNextWatchpoint;
    PoisonedUniquePtr<ObjectPropertyChangeAdaptiveWatchpoint<InlineWatchpointSet>> m_mapPrototypeSetWatchpoint;
    PoisonedUniquePtr<ObjectPropertyChangeAdaptiveWatchpoint<InlineWatchpointSet>> m_setPrototypeAddWatchpoint;
    PoisonedUniquePtr<ObjectPropertyChangeAdaptiveWatchpoint<InlineWatchpointSet>> m_numberPrototypeToStringWatchpoint;

    bool isArrayPrototypeIteratorProtocolFastAndNonObservable();
    bool isMapPrototypeIteratorProtocolFastAndNonObservable();
    bool isSetPrototypeIteratorProtocolFastAndNonObservable();
    bool isStringPrototypeIteratorProtocolFastAndNonObservable();
    bool isMapPrototypeSetFastAndNonObservable();
    bool isSetPrototypeAddFastAndNonObservable();

    bool m_evalEnabled { true };
    bool m_webAssemblyEnabled { true };
    String m_evalDisabledErrorMessage;
    String m_webAssemblyDisabledErrorMessage;
    RuntimeFlags m_runtimeFlags;
    ConsoleClient* m_consoleClient { nullptr };

    static JS_EXPORTDATA const GlobalObjectMethodTable s_globalObjectMethodTable;
    const GlobalObjectMethodTable* m_globalObjectMethodTable;

    void createRareDataIfNeeded()
    {
        if (m_rareData)
            return;
        m_rareData = std::make_unique<JSGlobalObjectRareData>();
    }

public:
    typedef JSSegmentedVariableObject Base;
    static const unsigned StructureFlags = Base::StructureFlags | HasStaticPropertyTable | OverridesGetOwnPropertySlot | OverridesGetPropertyNames | IsImmutablePrototypeExoticObject;

    JS_EXPORT_PRIVATE static JSGlobalObject* create(VM&, Structure*);

    DECLARE_EXPORT_INFO;

    bool hasDebugger() const;
    bool hasInteractiveDebugger() const;
    const RuntimeFlags& runtimeFlags() const { return m_runtimeFlags; }

protected:
    JS_EXPORT_PRIVATE explicit JSGlobalObject(VM&, Structure*, const GlobalObjectMethodTable* = nullptr, RefPtr<ThreadLocalCache> = nullptr);

    JS_EXPORT_PRIVATE void finishCreation(VM&);

    JS_EXPORT_PRIVATE void finishCreation(VM&, JSObject*);

    void addGlobalVar(const Identifier&);

public:
    JS_EXPORT_PRIVATE ~JSGlobalObject();
    JS_EXPORT_PRIVATE static void destroy(JSCell*);

    JS_EXPORT_PRIVATE static void visitChildren(JSCell*, SlotVisitor&);

    JS_EXPORT_PRIVATE static bool getOwnPropertySlot(JSObject*, ExecState*, PropertyName, PropertySlot&);
    JS_EXPORT_PRIVATE static bool put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);

    JS_EXPORT_PRIVATE static void defineGetter(JSObject*, ExecState*, PropertyName, JSObject* getterFunc, unsigned attributes);
    JS_EXPORT_PRIVATE static void defineSetter(JSObject*, ExecState*, PropertyName, JSObject* setterFunc, unsigned attributes);
    JS_EXPORT_PRIVATE static bool defineOwnProperty(JSObject*, ExecState*, PropertyName, const PropertyDescriptor&, bool shouldThrow);

    void addVar(ExecState* exec, const Identifier& propertyName)
    {
        if (!hasProperty(exec, propertyName))
            addGlobalVar(propertyName);
    }
    void addFunction(ExecState*, const Identifier&);

    JSScope* globalScope() { return m_globalLexicalEnvironment.get(); }
    JSGlobalLexicalEnvironment* globalLexicalEnvironment() { return m_globalLexicalEnvironment.get(); }

    JSScope* globalScopeExtension() { return m_globalScopeExtension.get(); }
    void setGlobalScopeExtension(JSScope*);
    void clearGlobalScopeExtension();

    // The following accessors return pristine values, even if a script
    // replaces the global object's associated property.

    GetterSetter* speciesGetterSetter() const { return m_speciesGetterSetter.get(); }

    RegExpConstructor* regExpConstructor() const { return m_regExpConstructor.get(); }

    ErrorConstructor* errorConstructor() const { return m_errorConstructor.get(); }
    ArrayConstructor* arrayConstructor() const { return m_arrayConstructor.get(); }
    ObjectConstructor* objectConstructor() const { return m_objectConstructor.get(); }
    JSPromiseConstructor* promiseConstructor() const { return m_promiseConstructor.get(); }
    JSInternalPromiseConstructor* internalPromiseConstructor() const { return m_internalPromiseConstructor.get(); }
    NativeErrorConstructor* evalErrorConstructor() const { return m_evalErrorConstructor.get(this); }
    NativeErrorConstructor* rangeErrorConstructor() const { return m_rangeErrorConstructor.get(); }
    NativeErrorConstructor* referenceErrorConstructor() const { return m_referenceErrorConstructor.get(this); }
    NativeErrorConstructor* syntaxErrorConstructor() const { return m_syntaxErrorConstructor.get(this); }
    NativeErrorConstructor* typeErrorConstructor() const { return m_typeErrorConstructor.get(); }
    NativeErrorConstructor* URIErrorConstructor() const { return m_URIErrorConstructor.get(this); }

    NullGetterFunction* nullGetterFunction() const { return m_nullGetterFunction.get(); }
    NullSetterFunction* nullSetterFunction() const { return m_nullSetterFunction.get(); }

    JSFunction* parseIntFunction() const { return m_parseIntFunction.get(); }
    JSFunction* parseFloatFunction() const { return m_parseFloatFunction.get(); }

    JSFunction* evalFunction() const { return m_evalFunction.get(); }
    JSFunction* callFunction() const { return m_callFunction.get(); }
    JSFunction* applyFunction() const { return m_applyFunction.get(); }
    JSFunction* throwTypeErrorFunction() const { return m_throwTypeErrorFunction.get(); }
    JSFunction* arrayProtoToStringFunction() const { return m_arrayProtoToStringFunction.get(this); }
    JSFunction* arrayProtoValuesFunction() const { return m_arrayProtoValuesFunction.get(this); }
    JSFunction* initializePromiseFunction() const { return m_initializePromiseFunction.get(this); }
    JSFunction* iteratorProtocolFunction() const { return m_iteratorProtocolFunction.get(this); }
    JSFunction* promiseResolveFunction() const { return m_promiseResolveFunction.get(this); }
    JSFunction* objectProtoValueOfFunction() const { return m_objectProtoValueOfFunction.get(); }
    JSFunction* numberProtoToStringFunction() const { return m_numberProtoToStringFunction.get(); }
    JSFunction* newPromiseCapabilityFunction() const { return m_newPromiseCapabilityFunction.get(); }
    JSFunction* functionProtoHasInstanceSymbolFunction() const { return m_functionProtoHasInstanceSymbolFunction.get(); }
    JSObject* regExpProtoExecFunction() const { return m_regExpProtoExec.get(); }
    JSObject* regExpProtoSymbolReplaceFunction() const { return m_regExpProtoSymbolReplace.get(); }
    JSObject* regExpProtoGlobalGetter() const { return m_regExpProtoGlobalGetter.get(); }
    JSObject* regExpProtoUnicodeGetter() const { return m_regExpProtoUnicodeGetter.get(); }
    GetterSetter* throwTypeErrorArgumentsCalleeAndCallerGetterSetter()
    {
        return m_throwTypeErrorArgumentsCalleeAndCallerGetterSetter.get();
    }

    JSModuleLoader* moduleLoader() const { return m_moduleLoader.get(); }

    ObjectPrototype* objectPrototype() const { return m_objectPrototype.get(); }
    FunctionPrototype* functionPrototype() const { return m_functionPrototype.get(); }
    ArrayPrototype* arrayPrototype() const { return m_arrayPrototype.get(); }
    BooleanPrototype* booleanPrototype() const { return m_booleanPrototype.get(); }
    StringPrototype* stringPrototype() const { return m_stringPrototype.get(); }
    SymbolPrototype* symbolPrototype() const { return m_symbolPrototype.get(); }
    NumberPrototype* numberPrototype() const { return m_numberPrototype.get(); }
    BigIntPrototype* bigIntPrototype() const { return m_bigIntPrototype.get(); }
    JSObject* datePrototype() const { return m_dateStructure.prototype(this); }
    RegExpPrototype* regExpPrototype() const { return m_regExpPrototype.get(); }
    ErrorPrototype* errorPrototype() const { return m_errorPrototype.get(); }
    IteratorPrototype* iteratorPrototype() const { return m_iteratorPrototype.get(); }
    AsyncIteratorPrototype* asyncIteratorPrototype() const { return m_asyncIteratorPrototype.get(); }
    GeneratorFunctionPrototype* generatorFunctionPrototype() const { return m_generatorFunctionPrototype.get(); }
    GeneratorPrototype* generatorPrototype() const { return m_generatorPrototype.get(); }
    AsyncFunctionPrototype* asyncFunctionPrototype() const { return m_asyncFunctionPrototype.get(); }
    MapPrototype* mapPrototype() const { return m_mapPrototype.get(); }
    // Workaround for the name conflict between JSCell::setPrototype.
    SetPrototype* jsSetPrototype() const { return m_setPrototype.get(); }
    AsyncGeneratorPrototype* asyncGeneratorPrototype() const { return m_asyncGeneratorPrototype.get(); }
    AsyncGeneratorFunctionPrototype* asyncGeneratorFunctionPrototype() const { return m_asyncGeneratorFunctionPrototype.get(); }

    Structure* debuggerScopeStructure() const { return m_debuggerScopeStructure.get(this); }
    Structure* withScopeStructure() const { return m_withScopeStructure.get(this); }
    Structure* strictEvalActivationStructure() const { return m_strictEvalActivationStructure.get(); }
    Structure* activationStructure() const { return m_lexicalEnvironmentStructure.get(); }
    Structure* moduleEnvironmentStructure() const { return m_moduleEnvironmentStructure.get(this); }
    Structure* directArgumentsStructure() const { return m_directArgumentsStructure.get(); }
    Structure* scopedArgumentsStructure() const { return m_scopedArgumentsStructure.get(); }
    Structure* clonedArgumentsStructure() const { return m_clonedArgumentsStructure.get(); }
    Structure* objectStructureForObjectConstructor() const { return m_objectStructureForObjectConstructor.get(); }
    Structure* originalArrayStructureForIndexingType(IndexingType indexingType) const
    {
        ASSERT(indexingType & IsArray);
        return m_originalArrayStructureForIndexingShape[(indexingType & IndexingShapeMask) >> IndexingShapeShift].get();
    }
    Structure* arrayStructureForIndexingTypeDuringAllocation(IndexingType indexingType) const
    {
        ASSERT(indexingType & IsArray);
        return m_arrayStructureForIndexingShapeDuringAllocation[(indexingType & IndexingShapeMask) >> IndexingShapeShift].get();
    }
    Structure* arrayStructureForIndexingTypeDuringAllocation(ExecState* exec, IndexingType indexingType, JSValue newTarget) const
    {
        return InternalFunction::createSubclassStructure(exec, newTarget, arrayStructureForIndexingTypeDuringAllocation(indexingType));
    }
    Structure* arrayStructureForProfileDuringAllocation(ExecState* exec, ArrayAllocationProfile* profile, JSValue newTarget) const
    {
        return arrayStructureForIndexingTypeDuringAllocation(exec, ArrayAllocationProfile::selectIndexingTypeFor(profile), newTarget);
    }

    bool isOriginalArrayStructure(Structure* structure)
    {
        return originalArrayStructureForIndexingType(structure->indexingType() | IsArray) == structure;
    }

    Structure* booleanObjectStructure() const { return m_booleanObjectStructure.get(); }
    Structure* callbackConstructorStructure() const { return m_callbackConstructorStructure.get(this); }
    Structure* callbackFunctionStructure() const { return m_callbackFunctionStructure.get(this); }
    Structure* callbackObjectStructure() const { return m_callbackObjectStructure.get(this); }
    Structure* propertyNameIteratorStructure() const { return m_propertyNameIteratorStructure.get(); }
#if JSC_OBJC_API_ENABLED
    Structure* objcCallbackFunctionStructure() const { return m_objcCallbackFunctionStructure.get(this); }
    Structure* objcWrapperObjectStructure() const { return m_objcWrapperObjectStructure.get(this); }
#endif
    Structure* dateStructure() const { return m_dateStructure.get(this); }
    Structure* nullPrototypeObjectStructure() const { return m_nullPrototypeObjectStructure.get(this); }
    Structure* errorStructure() const { return m_errorStructure.get(); }
    Structure* calleeStructure() const { return m_calleeStructure.get(); }
    Structure* strictFunctionStructure() const { return m_strictFunctionStructure.get(); }
    Structure* sloppyFunctionStructure() const { return m_sloppyFunctionStructure.get(); }
    Structure* arrowFunctionStructure() const { return m_arrowFunctionStructure.get(); }
    Structure* boundFunctionStructure() const { return m_boundFunctionStructure.get(this); }
    Structure* customGetterSetterFunctionStructure() const { return m_customGetterSetterFunctionStructure.get(this); }
    Structure* getterSetterStructure() const { return m_getterSetterStructure.get(); }
    Structure* nativeStdFunctionStructure() const { return m_nativeStdFunctionStructure.get(this); }
    PropertyOffset functionNameOffset() const { return m_functionNameOffset; }
    Structure* numberObjectStructure() const { return m_numberObjectStructure.get(); }
    Structure* mapStructure() const { return m_mapStructure.get(); }
    Structure* regExpStructure() const { return m_regExpStructure.get(); }
    Structure* generatorFunctionStructure() const { return m_generatorFunctionStructure.get(); }
    Structure* asyncFunctionStructure() const { return m_asyncFunctionStructure.get(); }
    Structure* asyncGeneratorFunctionStructure() const { return m_asyncGeneratorFunctionStructure.get(); }
    Structure* stringObjectStructure() const { return m_stringObjectStructure.get(); }
    Structure* symbolObjectStructure() const { return m_symbolObjectStructure.get(); }
    Structure* bigIntObjectStructure() const { return m_bigIntObjectStructure.get(); }
    Structure* iteratorResultObjectStructure() const { return m_iteratorResultObjectStructure.get(); }
    Structure* regExpMatchesArrayStructure() const { return m_regExpMatchesArrayStructure.get(); }
    Structure* regExpMatchesArrayWithGroupsStructure() const { return m_regExpMatchesArrayWithGroupsStructure.get(); }
    Structure* moduleRecordStructure() const { return m_moduleRecordStructure.get(); }
    Structure* moduleNamespaceObjectStructure() const { return m_moduleNamespaceObjectStructure.get(); }
    Structure* proxyObjectStructure() const { return m_proxyObjectStructure.get(); }
    Structure* callableProxyObjectStructure() const { return m_callableProxyObjectStructure.get(); }
    Structure* proxyRevokeStructure() const { return m_proxyRevokeStructure.get(); }
    Structure* moduleLoaderStructure() const { return m_moduleLoaderStructure.get(); }
    Structure* restParameterStructure() const { return arrayStructureForIndexingTypeDuringAllocation(ArrayWithContiguous); }
#if ENABLE(WEBASSEMBLY)
    Structure* webAssemblyModuleRecordStructure() const { return m_webAssemblyModuleRecordStructure.get(); }
    Structure* webAssemblyFunctionStructure() const { return m_webAssemblyFunctionStructure.get(); }
    Structure* webAssemblyWrapperFunctionStructure() const { return m_webAssemblyWrapperFunctionStructure.get(); }
    Structure* webAssemblyToJSCalleeStructure() const { return m_webAssemblyToJSCalleeStructure.get(); }
#endif // ENABLE(WEBASSEMBLY)

    JS_EXPORT_PRIVATE void setRemoteDebuggingEnabled(bool);
    JS_EXPORT_PRIVATE bool remoteDebuggingEnabled() const;

#if ENABLE(REMOTE_INSPECTOR)
    Inspector::JSGlobalObjectInspectorController& inspectorController() const { return *m_inspectorController.get(); }
    JSGlobalObjectDebuggable& inspectorDebuggable() { return *m_inspectorDebuggable.get(); }
#endif

#if ENABLE(INTL)
    const HashSet<String>& intlCollatorAvailableLocales();
    const HashSet<String>& intlDateTimeFormatAvailableLocales();
    const HashSet<String>& intlNumberFormatAvailableLocales();
#endif // ENABLE(INTL)

    void setConsoleClient(ConsoleClient* consoleClient) { m_consoleClient = consoleClient; }
    ConsoleClient* consoleClient() const { return m_consoleClient; }

    void setName(const String&);
    const String& name() const { return m_name; }

    JSArrayBufferPrototype* arrayBufferPrototype(ArrayBufferSharingMode sharingMode) const
    {
        switch (sharingMode) {
        case ArrayBufferSharingMode::Default:
            return m_arrayBufferPrototype.get();
#if ENABLE(SHARED_ARRAY_BUFFER)
        case ArrayBufferSharingMode::Shared:
            return m_sharedArrayBufferPrototype.get();
#else
        default:
            return m_arrayBufferPrototype.get();
#endif
        }
    }
    Structure* arrayBufferStructure(ArrayBufferSharingMode sharingMode) const
    {
        switch (sharingMode) {
        case ArrayBufferSharingMode::Default:
            return m_arrayBufferStructure.get();
#if ENABLE(SHARED_ARRAY_BUFFER)
        case ArrayBufferSharingMode::Shared:
            return m_sharedArrayBufferStructure.get();
#else
        default:
            return m_arrayBufferStructure.get();
#endif
        }
        RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
    }

#define DEFINE_ACCESSORS_FOR_SIMPLE_TYPE(capitalName, lowerName, properName, instanceType, jsName, prototypeBase) \
    Structure* properName ## Structure() { return m_ ## properName ## Structure.get(); }

    FOR_EACH_SIMPLE_BUILTIN_TYPE(DEFINE_ACCESSORS_FOR_SIMPLE_TYPE)
    FOR_BIG_INT_BUILTIN_TYPE_WITH_CONSTRUCTOR(DEFINE_ACCESSORS_FOR_SIMPLE_TYPE)
    FOR_EACH_WEBASSEMBLY_CONSTRUCTOR_TYPE(DEFINE_ACCESSORS_FOR_SIMPLE_TYPE)
    FOR_EACH_BUILTIN_DERIVED_ITERATOR_TYPE(DEFINE_ACCESSORS_FOR_SIMPLE_TYPE)

#undef DEFINE_ACCESSORS_FOR_SIMPLE_TYPE

#define DEFINE_ACCESSORS_FOR_LAZY_TYPE(capitalName, lowerName, properName, instanceType, jsName, prototypeBase) \
    Structure* properName ## Structure() { return m_ ## properName ## Structure.get(this); }

    FOR_EACH_LAZY_BUILTIN_TYPE(DEFINE_ACCESSORS_FOR_LAZY_TYPE)

#undef DEFINE_ACCESSORS_FOR_LAZY_TYPE

    LazyClassStructure& lazyTypedArrayStructure(TypedArrayType type)
    {
        switch (type) {
        case NotTypedArray:
            RELEASE_ASSERT_NOT_REACHED();
            return m_typedArrayInt8;
#define TYPED_ARRAY_TYPE_CASE(name) case Type ## name: return m_typedArray ## name;
            FOR_EACH_TYPED_ARRAY_TYPE(TYPED_ARRAY_TYPE_CASE)
#undef TYPED_ARRAY_TYPE_CASE
        }
        RELEASE_ASSERT_NOT_REACHED();
        return m_typedArrayInt8;
    }
    const LazyClassStructure& lazyTypedArrayStructure(TypedArrayType type) const
    {
        return const_cast<const LazyClassStructure&>(const_cast<JSGlobalObject*>(this)->lazyTypedArrayStructure(type));
    }

    Structure* typedArrayStructure(TypedArrayType type) const
    {
        return lazyTypedArrayStructure(type).get(this);
    }
    Structure* typedArrayStructureConcurrently(TypedArrayType type) const
    {
        return lazyTypedArrayStructure(type).getConcurrently();
    }
    bool isOriginalTypedArrayStructure(Structure* structure)
    {
        TypedArrayType type = structure->classInfo()->typedArrayStorageType;
        if (type == NotTypedArray)
            return false;
        return typedArrayStructureConcurrently(type) == structure;
    }

    JSObject* typedArrayConstructor(TypedArrayType type) const
    {
        return lazyTypedArrayStructure(type).constructor(this);
    }

    JSCell* actualPointerFor(Special::Pointer pointer)
    {
        ASSERT(pointer < Special::TableSize);
        return m_specialPointers[pointer];
    }
    JSCell* jsCellForLinkTimeConstant(LinkTimeConstant type)
    {
        unsigned index = static_cast<unsigned>(type);
        ASSERT(index < LinkTimeConstantCount);
        return m_linkTimeConstants[index];
    }

    WatchpointSet* masqueradesAsUndefinedWatchpoint() { return m_masqueradesAsUndefinedWatchpoint.get(); }
    WatchpointSet* havingABadTimeWatchpoint() { return m_havingABadTimeWatchpoint.get(); }
    WatchpointSet* varInjectionWatchpoint() { return m_varInjectionWatchpoint.get(); }

    bool isHavingABadTime() const
    {
        return m_havingABadTimeWatchpoint->hasBeenInvalidated();
    }

    void haveABadTime(VM&);

    bool objectPrototypeIsSane();
    bool arrayPrototypeChainIsSane();
    bool stringPrototypeChainIsSane();

    void setProfileGroup(unsigned value) { createRareDataIfNeeded(); m_rareData->profileGroup = value; }
    unsigned profileGroup() const
    {
        if (!m_rareData)
            return 0;
        return m_rareData->profileGroup;
    }

    Debugger* debugger() const { return m_debugger; }
    void setDebugger(Debugger* debugger) { m_debugger = debugger; }

    const GlobalObjectMethodTable* globalObjectMethodTable() const { return m_globalObjectMethodTable; }

    static bool supportsRichSourceInfo(const JSGlobalObject*) { return true; }

    JS_EXPORT_PRIVATE ExecState* globalExec();

    static bool shouldInterruptScript(const JSGlobalObject*) { return true; }
    static bool shouldInterruptScriptBeforeTimeout(const JSGlobalObject*) { return false; }
    static RuntimeFlags javaScriptRuntimeFlags(const JSGlobalObject*) { return RuntimeFlags(); }

    JS_EXPORT_PRIVATE void queueMicrotask(Ref<Microtask>&&);

    bool evalEnabled() const { return m_evalEnabled; }
    bool webAssemblyEnabled() const { return m_webAssemblyEnabled; }
    const String& evalDisabledErrorMessage() const { return m_evalDisabledErrorMessage; }
    const String& webAssemblyDisabledErrorMessage() const { return m_webAssemblyDisabledErrorMessage; }
    void setEvalEnabled(bool enabled, const String& errorMessage = String())
    {
        m_evalEnabled = enabled;
        m_evalDisabledErrorMessage = errorMessage;
    }
    void setWebAssemblyEnabled(bool enabled, const String& errorMessage = String())
    {
        m_webAssemblyEnabled = enabled;
        m_webAssemblyDisabledErrorMessage = errorMessage;
    }

    void resetPrototype(VM&, JSValue prototype);

    VM& vm() const { return m_vm; }
    JSObject* globalThis() const;
    WriteBarrier<JSObject>* addressOfGlobalThis() { return &m_globalThis; }

    static Structure* createStructure(VM& vm, JSValue prototype)
    {
        Structure* result = Structure::create(vm, 0, prototype, TypeInfo(GlobalObjectType, StructureFlags), info());
        result->setTransitionWatchpointIsLikelyToBeFired(true);
        return result;
    }

    void registerWeakMap(OpaqueJSWeakObjectMap* map)
    {
        createRareDataIfNeeded();
        m_rareData->weakMaps.add(map);
    }

    void unregisterWeakMap(OpaqueJSWeakObjectMap* map)
    {
        if (m_rareData)
            m_rareData->weakMaps.remove(map);
    }

    OpaqueJSClassDataMap& opaqueJSClassData()
    {
        createRareDataIfNeeded();
        return m_rareData->opaqueJSClassData;
    }

    static ptrdiff_t weakRandomOffset() { return OBJECT_OFFSETOF(JSGlobalObject, m_weakRandom); }
    double weakRandomNumber() { return m_weakRandom.get(); }
    unsigned weakRandomInteger() { return m_weakRandom.getUint32(); }
    WeakRandom& weakRandom() { return m_weakRandom; }

    bool needsSiteSpecificQuirks() const { return m_needsSiteSpecificQuirks; }

#if JSC_OBJC_API_ENABLED
    JSWrapperMap* wrapperMap() const { return m_wrapperMap.get(); }
    void setWrapperMap(JSWrapperMap* map) { m_wrapperMap = map; }
#endif

    ThreadLocalCache& threadLocalCache() const { return *m_threadLocalCache.get(); }

protected:
    struct GlobalPropertyInfo {
        GlobalPropertyInfo(const Identifier& i, JSValue v, unsigned a)
            : identifier(i)
            , value(v)
            , attributes(a)
        {
        }

        const Identifier identifier;
        JSValue value;
        unsigned attributes;
    };
    JS_EXPORT_PRIVATE void addStaticGlobals(GlobalPropertyInfo*, int count);

    void setNeedsSiteSpecificQuirks(bool needQuirks) { m_needsSiteSpecificQuirks = needQuirks; }

private:
    friend class LLIntOffsetsExtractor;

    void setGlobalThis(VM&, JSObject* globalThis);

    JS_EXPORT_PRIVATE void init(VM&);

    JS_EXPORT_PRIVATE static void clearRareData(JSCell*);

    bool m_needsSiteSpecificQuirks { false };
#if JSC_OBJC_API_ENABLED
    RetainPtr<JSWrapperMap> m_wrapperMap;
#endif

    RefPtr<ThreadLocalCache> m_threadLocalCache;
};

JSGlobalObject* asGlobalObject(JSValue);

inline JSGlobalObject* asGlobalObject(JSValue value)
{
    ASSERT(asObject(value)->isGlobalObject());
    return jsCast<JSGlobalObject*>(asObject(value));
}

inline JSArray* constructEmptyArray(ExecState* exec, ArrayAllocationProfile* profile, JSGlobalObject* globalObject, unsigned initialLength = 0, JSValue newTarget = JSValue())
{
    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);
    Structure* structure;
    if (initialLength >= MIN_ARRAY_STORAGE_CONSTRUCTION_LENGTH)
        structure = globalObject->arrayStructureForIndexingTypeDuringAllocation(exec, ArrayWithArrayStorage, newTarget);
    else
        structure = globalObject->arrayStructureForProfileDuringAllocation(exec, profile, newTarget);
    RETURN_IF_EXCEPTION(scope, nullptr);

    JSArray* result = JSArray::tryCreate(vm, structure, initialLength);
    if (UNLIKELY(!result)) {
        throwOutOfMemoryError(exec, scope);
        return nullptr;
    }
    return ArrayAllocationProfile::updateLastAllocationFor(profile, result);
}

inline JSArray* constructEmptyArray(ExecState* exec, ArrayAllocationProfile* profile, unsigned initialLength = 0, JSValue newTarget = JSValue())
{
    return constructEmptyArray(exec, profile, exec->lexicalGlobalObject(), initialLength, newTarget);
}

inline JSArray* constructArray(ExecState* exec, ArrayAllocationProfile* profile, JSGlobalObject* globalObject, const ArgList& values, JSValue newTarget = JSValue())
{
    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);
    Structure* structure = globalObject->arrayStructureForProfileDuringAllocation(exec, profile, newTarget);
    RETURN_IF_EXCEPTION(scope, nullptr);
    return ArrayAllocationProfile::updateLastAllocationFor(profile, constructArray(exec, structure, values));
}

inline JSArray* constructArray(ExecState* exec, ArrayAllocationProfile* profile, const ArgList& values, JSValue newTarget = JSValue())
{
    return constructArray(exec, profile, exec->lexicalGlobalObject(), values, newTarget);
}

inline JSArray* constructArray(ExecState* exec, ArrayAllocationProfile* profile, JSGlobalObject* globalObject, const JSValue* values, unsigned length, JSValue newTarget = JSValue())
{
    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);
    Structure* structure = globalObject->arrayStructureForProfileDuringAllocation(exec, profile, newTarget);
    RETURN_IF_EXCEPTION(scope, nullptr);
    return ArrayAllocationProfile::updateLastAllocationFor(profile, constructArray(exec, structure, values, length));
}

inline JSArray* constructArray(ExecState* exec, ArrayAllocationProfile* profile, const JSValue* values, unsigned length, JSValue newTarget = JSValue())
{
    return constructArray(exec, profile, exec->lexicalGlobalObject(), values, length, newTarget);
}

inline JSArray* constructArrayNegativeIndexed(ExecState* exec, ArrayAllocationProfile* profile, JSGlobalObject* globalObject, const JSValue* values, unsigned length, JSValue newTarget = JSValue())
{
    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);
    Structure* structure = globalObject->arrayStructureForProfileDuringAllocation(exec, profile, newTarget);
    RETURN_IF_EXCEPTION(scope, nullptr);
    return ArrayAllocationProfile::updateLastAllocationFor(profile, constructArrayNegativeIndexed(exec, structure, values, length));
}

inline JSArray* constructArrayNegativeIndexed(ExecState* exec, ArrayAllocationProfile* profile, const JSValue* values, unsigned length, JSValue newTarget = JSValue())
{
    return constructArrayNegativeIndexed(exec, profile, exec->lexicalGlobalObject(), values, length, newTarget);
}

inline JSObject* ExecState::globalThisValue() const
{
    return lexicalGlobalObject()->globalThis();
}

inline JSObject* JSScope::globalThis()
{
    return globalObject()->globalThis();
}

inline JSObject* JSGlobalObject::globalThis() const
{
    return m_globalThis.get();
}

} // namespace JSC
