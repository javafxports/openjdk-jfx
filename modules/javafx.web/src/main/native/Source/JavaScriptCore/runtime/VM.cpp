/*
 * Copyright (C) 2008-2018 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "VM.h"

#include "ArgList.h"
#include "ArrayBufferNeuteringWatchpoint.h"
#include "BuiltinExecutables.h"
#include "BytecodeIntrinsicRegistry.h"
#include "CodeBlock.h"
#include "CodeCache.h"
#include "CommonIdentifiers.h"
#include "CommonSlowPaths.h"
#include "CustomGetterSetter.h"
#include "DFGWorklist.h"
#include "DirectEvalExecutable.h"
#include "Disassembler.h"
#include "ErrorInstance.h"
#include "EvalCodeBlock.h"
#include "Exception.h"
#include "ExecutableToCodeBlockEdge.h"
#include "FTLThunks.h"
#include "FastMallocAlignedMemoryAllocator.h"
#include "FunctionCodeBlock.h"
#include "FunctionConstructor.h"
#include "FunctionExecutable.h"
#include "GCActivityCallback.h"
#include "GetterSetter.h"
#include "GigacageAlignedMemoryAllocator.h"
#include "HasOwnPropertyCache.h"
#include "Heap.h"
#include "HeapIterationScope.h"
#include "HeapProfiler.h"
#include "HostCallReturnValue.h"
#include "Identifier.h"
#include "IncrementalSweeper.h"
#include "IndirectEvalExecutable.h"
#include "InferredTypeTable.h"
#include "InferredValue.h"
#include "Interpreter.h"
#include "JITCode.h"
#include "JITWorklist.h"
#include "JSAPIValueWrapper.h"
#include "JSArray.h"
#include "JSAsyncFunction.h"
#include "JSBigInt.h"
#include "JSBoundFunction.h"
#include "JSCInlines.h"
#include "JSCustomGetterSetterFunction.h"
#include "JSDestructibleObjectHeapCellType.h"
#include "JSFixedArray.h"
#include "JSFunction.h"
#include "JSGlobalObjectFunctions.h"
#include "JSInternalPromiseDeferred.h"
#include "JSLock.h"
#include "JSMap.h"
#include "JSMapIterator.h"
#include "JSPromiseDeferred.h"
#include "JSPropertyNameEnumerator.h"
#include "JSSegmentedVariableObjectHeapCellType.h"
#include "JSScriptFetchParameters.h"
#include "JSScriptFetcher.h"
#include "JSSet.h"
#include "JSSetIterator.h"
#include "JSSourceCode.h"
#include "JSStringHeapCellType.h"
#include "JSTemplateObjectDescriptor.h"
#include "JSWeakMap.h"
#include "JSWeakSet.h"
#include "JSWebAssembly.h"
#include "JSWebAssemblyCodeBlockHeapCellType.h"
#include "JSWithScope.h"
#include "LLIntData.h"
#include "Lexer.h"
#include "Lookup.h"
#include "MinimumReservedZoneSize.h"
#include "ModuleProgramCodeBlock.h"
#include "ModuleProgramExecutable.h"
#include "NativeExecutable.h"
#include "NativeStdFunctionCell.h"
#include "Nodes.h"
#include "Parser.h"
#include "ProfilerDatabase.h"
#include "ProgramCodeBlock.h"
#include "ProgramExecutable.h"
#include "PromiseDeferredTimer.h"
#include "PropertyMapHashTable.h"
#include "RegExpCache.h"
#include "RegExpObject.h"
#include "RegisterAtOffsetList.h"
#include "RuntimeType.h"
#include "SamplingProfiler.h"
#include "ShadowChicken.h"
#include "SimpleTypedArrayController.h"
#include "SourceProviderCache.h"
#include "StackVisitor.h"
#include "StrictEvalActivation.h"
#include "StrongInlines.h"
#include "StructureInlines.h"
#include "TestRunnerUtils.h"
#include "ThreadLocalCacheInlines.h"
#include "ThunkGenerators.h"
#include "TypeProfiler.h"
#include "TypeProfilerLog.h"
#include "UnlinkedCodeBlock.h"
#include "VMEntryScope.h"
#include "VMInspector.h"
#include "WasmWorklist.h"
#include "Watchdog.h"
#include "WeakGCMapInlines.h"
#include "WebAssemblyFunction.h"
#include "WebAssemblyWrapperFunction.h"
#include <wtf/CurrentTime.h>
#include <wtf/ProcessID.h>
#include <wtf/ReadWriteLock.h>
#include <wtf/SimpleStats.h>
#include <wtf/StringPrintStream.h>
#include <wtf/Threading.h>
#include <wtf/text/AtomicStringTable.h>
#include <wtf/text/SymbolRegistry.h>

#if !ENABLE(JIT)
#include "CLoopStack.h"
#include "CLoopStackInlines.h"
#endif

#if ENABLE(DFG_JIT)
#include "ConservativeRoots.h"
#endif

#if ENABLE(REGEXP_TRACING)
#include "RegExp.h"
#endif

using namespace WTF;

namespace JSC {

// Note: Platform.h will enforce that ENABLE(ASSEMBLER) is true if either
// ENABLE(JIT) or ENABLE(YARR_JIT) or both are enabled. The code below
// just checks for ENABLE(JIT) or ENABLE(YARR_JIT) with this premise in mind.

#if ENABLE(ASSEMBLER)
static bool enableAssembler(ExecutableAllocator& executableAllocator)
{
    if (!Options::useJIT() && !Options::useRegExpJIT())
        return false;

    if (!executableAllocator.isValid()) {
        if (Options::crashIfCantAllocateJITMemory())
            CRASH();
        return false;
    }

    char* canUseJITString = getenv("JavaScriptCoreUseJIT");
    return !canUseJITString || atoi(canUseJITString);
}
#endif // ENABLE(!ASSEMBLER)

bool VM::canUseAssembler()
{
#if ENABLE(ASSEMBLER)
    static std::once_flag onceKey;
    static bool enabled = false;
    std::call_once(onceKey, [] {
        enabled = enableAssembler(ExecutableAllocator::singleton());
    });
    return enabled;
#else
    return false; // interpreter only
#endif
}

bool VM::canUseJIT()
{
#if ENABLE(JIT)
    static std::once_flag onceKey;
    static bool enabled = false;
    std::call_once(onceKey, [] {
        enabled = VM::canUseAssembler() && Options::useJIT();
    });
    return enabled;
#else
    return false; // interpreter only
#endif
}

bool VM::canUseRegExpJIT()
{
#if ENABLE(YARR_JIT)
    static std::once_flag onceKey;
    static bool enabled = false;
    std::call_once(onceKey, [] {
        enabled = VM::canUseAssembler() && Options::useRegExpJIT();
    });
    return enabled;
#else
    return false; // interpreter only
#endif
}

VM::VM(VMType vmType, HeapType heapType)
    : m_apiLock(adoptRef(new JSLock(this)))
#if USE(CF)
    , m_runLoop(CFRunLoopGetCurrent())
#endif // USE(CF)
    , heap(this, heapType)
    , fastMallocAllocator(std::make_unique<FastMallocAlignedMemoryAllocator>())
    , primitiveGigacageAllocator(std::make_unique<GigacageAlignedMemoryAllocator>(Gigacage::Primitive))
    , jsValueGigacageAllocator(std::make_unique<GigacageAlignedMemoryAllocator>(Gigacage::JSValue))
    , auxiliaryJSValueStrictHeapCellType(std::make_unique<HeapCellType>(CellAttributes(DoesNotNeedDestruction, HeapCell::Auxiliary, SecurityKind::JSValueStrict)))
    , auxiliaryDangerousBitsHeapCellType(std::make_unique<HeapCellType>(CellAttributes(DoesNotNeedDestruction, HeapCell::Auxiliary, SecurityKind::DangerousBits)))
    , cellJSValueOOBHeapCellType(std::make_unique<HeapCellType>(CellAttributes(DoesNotNeedDestruction, HeapCell::JSCell, SecurityKind::JSValueOOB)))
    , cellDangerousBitsHeapCellType(std::make_unique<HeapCellType>(CellAttributes(DoesNotNeedDestruction, HeapCell::JSCell, SecurityKind::JSValueOOB)))
    , destructibleCellHeapCellType(std::make_unique<HeapCellType>(CellAttributes(NeedsDestruction, HeapCell::JSCell, SecurityKind::DangerousBits)))
    , stringHeapCellType(std::make_unique<JSStringHeapCellType>())
    , destructibleObjectHeapCellType(std::make_unique<JSDestructibleObjectHeapCellType>())
    , segmentedVariableObjectHeapCellType(std::make_unique<JSSegmentedVariableObjectHeapCellType>())
#if ENABLE(WEBASSEMBLY)
    , webAssemblyCodeBlockHeapCellType(std::make_unique<JSWebAssemblyCodeBlockHeapCellType>())
#endif
    , primitiveGigacageAuxiliarySpace("Primitive Gigacage Auxiliary", heap, auxiliaryDangerousBitsHeapCellType.get(), primitiveGigacageAllocator.get())
    , jsValueGigacageAuxiliarySpace("JSValue Gigacage Auxiliary", heap, auxiliaryJSValueStrictHeapCellType.get(), jsValueGigacageAllocator.get())
    , cellJSValueOOBSpace("JSCell JSValueOOB", heap, cellJSValueOOBHeapCellType.get(), fastMallocAllocator.get())
    , cellDangerousBitsSpace("JSCell DangerousBits", heap, cellDangerousBitsHeapCellType.get(), fastMallocAllocator.get())
    , jsValueGigacageCellSpace("JSValue Gigacage JSCell", heap, cellJSValueOOBHeapCellType.get(), jsValueGigacageAllocator.get())
    , destructibleCellSpace("Destructible JSCell", heap, destructibleCellHeapCellType.get(), fastMallocAllocator.get())
    , stringSpace("JSString", heap, stringHeapCellType.get(), fastMallocAllocator.get())
    , destructibleObjectSpace("JSDestructibleObject", heap, destructibleObjectHeapCellType.get(), fastMallocAllocator.get())
    , eagerlySweptDestructibleObjectSpace("Eagerly Swept JSDestructibleObject", heap, destructibleObjectHeapCellType.get(), fastMallocAllocator.get())
    , segmentedVariableObjectSpace("JSSegmentedVariableObjectSpace", heap, segmentedVariableObjectHeapCellType.get(), fastMallocAllocator.get())
#if ENABLE(WEBASSEMBLY)
    , webAssemblyCodeBlockSpace("JSWebAssemblyCodeBlockSpace", heap, webAssemblyCodeBlockHeapCellType.get(), fastMallocAllocator.get())
#endif
    , asyncFunctionSpace ISO_SUBSPACE_INIT(heap, cellJSValueOOBHeapCellType.get(), JSAsyncFunction)
    , asyncGeneratorFunctionSpace ISO_SUBSPACE_INIT(heap, cellJSValueOOBHeapCellType.get(), JSAsyncGeneratorFunction)
    , boundFunctionSpace ISO_SUBSPACE_INIT(heap, cellJSValueOOBHeapCellType.get(), JSBoundFunction)
    , customGetterSetterFunctionSpace ISO_SUBSPACE_INIT(heap, cellJSValueOOBHeapCellType.get(), JSCustomGetterSetterFunction)
    , directEvalExecutableSpace ISO_SUBSPACE_INIT(heap, destructibleCellHeapCellType.get(), DirectEvalExecutable)
    , executableToCodeBlockEdgeSpace ISO_SUBSPACE_INIT(heap, cellDangerousBitsHeapCellType.get(), ExecutableToCodeBlockEdge)
    , functionExecutableSpace ISO_SUBSPACE_INIT(heap, destructibleCellHeapCellType.get(), FunctionExecutable)
    , functionSpace ISO_SUBSPACE_INIT(heap, cellJSValueOOBHeapCellType.get(), JSFunction)
    , generatorFunctionSpace ISO_SUBSPACE_INIT(heap, cellJSValueOOBHeapCellType.get(), JSGeneratorFunction)
    , indirectEvalExecutableSpace ISO_SUBSPACE_INIT(heap, destructibleCellHeapCellType.get(), IndirectEvalExecutable)
    , inferredTypeSpace ISO_SUBSPACE_INIT(heap, destructibleCellHeapCellType.get(), InferredType)
    , inferredValueSpace ISO_SUBSPACE_INIT(heap, destructibleCellHeapCellType.get(), InferredValue)
    , moduleProgramExecutableSpace ISO_SUBSPACE_INIT(heap, destructibleCellHeapCellType.get(), ModuleProgramExecutable)
    , nativeExecutableSpace ISO_SUBSPACE_INIT(heap, destructibleCellHeapCellType.get(), NativeExecutable)
    , nativeStdFunctionSpace ISO_SUBSPACE_INIT(heap, cellJSValueOOBHeapCellType.get(), JSNativeStdFunction)
    , programExecutableSpace ISO_SUBSPACE_INIT(heap, destructibleCellHeapCellType.get(), ProgramExecutable)
    , propertyTableSpace ISO_SUBSPACE_INIT(heap, destructibleCellHeapCellType.get(), PropertyTable)
    , structureRareDataSpace ISO_SUBSPACE_INIT(heap, destructibleCellHeapCellType.get(), StructureRareData)
    , structureSpace ISO_SUBSPACE_INIT(heap, destructibleCellHeapCellType.get(), Structure)
    , weakSetSpace ISO_SUBSPACE_INIT(heap, destructibleObjectHeapCellType.get(), JSWeakSet)
    , weakMapSpace ISO_SUBSPACE_INIT(heap, destructibleObjectHeapCellType.get(), JSWeakMap)
#if ENABLE(WEBASSEMBLY)
    , webAssemblyFunctionSpace ISO_SUBSPACE_INIT(heap, cellJSValueOOBHeapCellType.get(), WebAssemblyFunction)
    , webAssemblyWrapperFunctionSpace ISO_SUBSPACE_INIT(heap, cellJSValueOOBHeapCellType.get(), WebAssemblyWrapperFunction)
#endif
    , executableToCodeBlockEdgesWithConstraints(executableToCodeBlockEdgeSpace)
    , executableToCodeBlockEdgesWithFinalizers(executableToCodeBlockEdgeSpace)
    , inferredTypesWithFinalizers(inferredTypeSpace)
    , inferredValuesWithFinalizers(inferredValueSpace)
    , evalCodeBlockSpace ISO_SUBSPACE_INIT(heap, destructibleCellHeapCellType.get(), EvalCodeBlock)
    , functionCodeBlockSpace ISO_SUBSPACE_INIT(heap, destructibleCellHeapCellType.get(), FunctionCodeBlock)
    , moduleProgramCodeBlockSpace ISO_SUBSPACE_INIT(heap, destructibleCellHeapCellType.get(), ModuleProgramCodeBlock)
    , programCodeBlockSpace ISO_SUBSPACE_INIT(heap, destructibleCellHeapCellType.get(), ProgramCodeBlock)
    , vmType(vmType)
    , clientData(0)
    , topEntryFrame(nullptr)
    , topCallFrame(CallFrame::noCaller())
    , promiseDeferredTimer(std::make_unique<PromiseDeferredTimer>(*this))
    , m_atomicStringTable(vmType == Default ? Thread::current().atomicStringTable() : new AtomicStringTable)
    , propertyNames(nullptr)
    , emptyList(new ArgList)
    , machineCodeBytesPerBytecodeWordForBaselineJIT(std::make_unique<SimpleStats>())
    , customGetterSetterFunctionMap(*this)
    , stringCache(*this)
    , symbolImplToSymbolMap(*this)
    , structureCache(*this)
    , interpreter(0)
    , entryScope(0)
    , m_regExpCache(new RegExpCache(this))
#if ENABLE(REGEXP_TRACING)
    , m_rtTraceList(new RTTraceList())
#endif
#if ENABLE(GC_VALIDATION)
    , m_initializingObjectClass(0)
#endif
    , m_stackPointerAtVMEntry(0)
    , m_codeCache(std::make_unique<CodeCache>())
    , m_builtinExecutables(std::make_unique<BuiltinExecutables>(*this))
    , m_typeProfilerEnabledCount(0)
    , m_primitiveGigacageEnabled(IsWatched)
    , m_controlFlowProfilerEnabledCount(0)
    , m_shadowChicken(std::make_unique<ShadowChicken>())
{
    interpreter = new Interpreter(*this);
    StackBounds stack = Thread::current().stack();
    updateSoftReservedZoneSize(Options::softReservedZoneSize());
    setLastStackTop(stack.origin());

    defaultThreadLocalCache = ThreadLocalCache::create(heap);
    defaultThreadLocalCache->install(*this);

    // Need to be careful to keep everything consistent here
    JSLockHolder lock(this);
    AtomicStringTable* existingEntryAtomicStringTable = Thread::current().setCurrentAtomicStringTable(m_atomicStringTable);
    propertyNames = new CommonIdentifiers(this);
    structureStructure.set(*this, Structure::createStructure(*this));
    structureRareDataStructure.set(*this, StructureRareData::createStructure(*this, 0, jsNull()));
    terminatedExecutionErrorStructure.set(*this, TerminatedExecutionError::createStructure(*this, 0, jsNull()));
    stringStructure.set(*this, JSString::createStructure(*this, 0, jsNull()));
    propertyNameEnumeratorStructure.set(*this, JSPropertyNameEnumerator::createStructure(*this, 0, jsNull()));
    customGetterSetterStructure.set(*this, CustomGetterSetter::createStructure(*this, 0, jsNull()));
    domAttributeGetterSetterStructure.set(*this, DOMAttributeGetterSetter::createStructure(*this, 0, jsNull()));
    scopedArgumentsTableStructure.set(*this, ScopedArgumentsTable::createStructure(*this, 0, jsNull()));
    apiWrapperStructure.set(*this, JSAPIValueWrapper::createStructure(*this, 0, jsNull()));
    nativeExecutableStructure.set(*this, NativeExecutable::createStructure(*this, 0, jsNull()));
    evalExecutableStructure.set(*this, EvalExecutable::createStructure(*this, 0, jsNull()));
    programExecutableStructure.set(*this, ProgramExecutable::createStructure(*this, 0, jsNull()));
    functionExecutableStructure.set(*this, FunctionExecutable::createStructure(*this, 0, jsNull()));
#if ENABLE(WEBASSEMBLY)
    webAssemblyCodeBlockStructure.set(*this, JSWebAssemblyCodeBlock::createStructure(*this, 0, jsNull()));
#endif
    moduleProgramExecutableStructure.set(*this, ModuleProgramExecutable::createStructure(*this, 0, jsNull()));
    regExpStructure.set(*this, RegExp::createStructure(*this, 0, jsNull()));
    symbolStructure.set(*this, Symbol::createStructure(*this, 0, jsNull()));
    symbolTableStructure.set(*this, SymbolTable::createStructure(*this, 0, jsNull()));
    fixedArrayStructure.set(*this, JSFixedArray::createStructure(*this, 0, jsNull()));
    sourceCodeStructure.set(*this, JSSourceCode::createStructure(*this, 0, jsNull()));
    scriptFetcherStructure.set(*this, JSScriptFetcher::createStructure(*this, 0, jsNull()));
    scriptFetchParametersStructure.set(*this, JSScriptFetchParameters::createStructure(*this, 0, jsNull()));
    structureChainStructure.set(*this, StructureChain::createStructure(*this, 0, jsNull()));
    sparseArrayValueMapStructure.set(*this, SparseArrayValueMap::createStructure(*this, 0, jsNull()));
    templateObjectDescriptorStructure.set(*this, JSTemplateObjectDescriptor::createStructure(*this, 0, jsNull()));
    arrayBufferNeuteringWatchpointStructure.set(*this, ArrayBufferNeuteringWatchpoint::createStructure(*this));
    unlinkedFunctionExecutableStructure.set(*this, UnlinkedFunctionExecutable::createStructure(*this, 0, jsNull()));
    unlinkedProgramCodeBlockStructure.set(*this, UnlinkedProgramCodeBlock::createStructure(*this, 0, jsNull()));
    unlinkedEvalCodeBlockStructure.set(*this, UnlinkedEvalCodeBlock::createStructure(*this, 0, jsNull()));
    unlinkedFunctionCodeBlockStructure.set(*this, UnlinkedFunctionCodeBlock::createStructure(*this, 0, jsNull()));
    unlinkedModuleProgramCodeBlockStructure.set(*this, UnlinkedModuleProgramCodeBlock::createStructure(*this, 0, jsNull()));
    propertyTableStructure.set(*this, PropertyTable::createStructure(*this, 0, jsNull()));
    inferredTypeStructure.set(*this, InferredType::createStructure(*this, 0, jsNull()));
    inferredTypeTableStructure.set(*this, InferredTypeTable::createStructure(*this, 0, jsNull()));
    inferredValueStructure.set(*this, InferredValue::createStructure(*this, 0, jsNull()));
    functionRareDataStructure.set(*this, FunctionRareData::createStructure(*this, 0, jsNull()));
    exceptionStructure.set(*this, Exception::createStructure(*this, 0, jsNull()));
    promiseDeferredStructure.set(*this, JSPromiseDeferred::createStructure(*this, 0, jsNull()));
    internalPromiseDeferredStructure.set(*this, JSInternalPromiseDeferred::createStructure(*this, 0, jsNull()));
    programCodeBlockStructure.set(*this, ProgramCodeBlock::createStructure(*this, 0, jsNull()));
    moduleProgramCodeBlockStructure.set(*this, ModuleProgramCodeBlock::createStructure(*this, 0, jsNull()));
    evalCodeBlockStructure.set(*this, EvalCodeBlock::createStructure(*this, 0, jsNull()));
    functionCodeBlockStructure.set(*this, FunctionCodeBlock::createStructure(*this, 0, jsNull()));
    hashMapBucketSetStructure.set(*this, HashMapBucket<HashMapBucketDataKey>::createStructure(*this, 0, jsNull()));
    hashMapBucketMapStructure.set(*this, HashMapBucket<HashMapBucketDataKeyValue>::createStructure(*this, 0, jsNull()));
    setIteratorStructure.set(*this, JSSetIterator::createStructure(*this, 0, jsNull()));
    mapIteratorStructure.set(*this, JSMapIterator::createStructure(*this, 0, jsNull()));
    bigIntStructure.set(*this, JSBigInt::createStructure(*this, 0, jsNull()));
    executableToCodeBlockEdgeStructure.set(*this, ExecutableToCodeBlockEdge::createStructure(*this, nullptr, jsNull()));

    sentinelSetBucket.set(*this, JSSet::BucketType::createSentinel(*this));
    sentinelMapBucket.set(*this, JSMap::BucketType::createSentinel(*this));

    nativeStdFunctionCellStructure.set(*this, NativeStdFunctionCell::createStructure(*this, 0, jsNull()));
    smallStrings.initializeCommonStrings(*this);

    Thread::current().setCurrentAtomicStringTable(existingEntryAtomicStringTable);

#if ENABLE(JIT)
    jitStubs = std::make_unique<JITThunks>();
#endif

#if ENABLE(FTL_JIT)
    ftlThunks = std::make_unique<FTL::Thunks>();
#endif // ENABLE(FTL_JIT)

#if ENABLE(JIT)
    initializeHostCallReturnValue(); // This is needed to convince the linker not to drop host call return support.
#endif

    Gigacage::addPrimitiveDisableCallback(primitiveGigacageDisabledCallback, this);

    heap.notifyIsSafeToCollect();

    LLInt::Data::performAssertions(*this);

    if (UNLIKELY(Options::useProfiler())) {
        m_perBytecodeProfiler = std::make_unique<Profiler::Database>(*this);

        StringPrintStream pathOut;
        const char* profilerPath = getenv("JSC_PROFILER_PATH");
        if (profilerPath)
            pathOut.print(profilerPath, "/");
        pathOut.print("JSCProfile-", getCurrentProcessID(), "-", m_perBytecodeProfiler->databaseID(), ".json");
        m_perBytecodeProfiler->registerToSaveAtExit(pathOut.toCString().data());
    }

    callFrameForCatch = nullptr;

    // Initialize this last, as a free way of asserting that VM initialization itself
    // won't use this.
    m_typedArrayController = adoptRef(new SimpleTypedArrayController());

    m_bytecodeIntrinsicRegistry = std::make_unique<BytecodeIntrinsicRegistry>(*this);

    if (Options::useTypeProfiler())
        enableTypeProfiler();
    if (Options::useControlFlowProfiler())
        enableControlFlowProfiler();
#if ENABLE(SAMPLING_PROFILER)
    if (Options::useSamplingProfiler()) {
        setShouldBuildPCToCodeOriginMapping();
        Ref<Stopwatch> stopwatch = Stopwatch::create();
        stopwatch->start();
        m_samplingProfiler = adoptRef(new SamplingProfiler(*this, WTFMove(stopwatch)));
        if (Options::samplingProfilerPath())
            m_samplingProfiler->registerForReportAtExit();
        m_samplingProfiler->start();
    }
#endif // ENABLE(SAMPLING_PROFILER)

    if (Options::alwaysGeneratePCToCodeOriginMap())
        setShouldBuildPCToCodeOriginMapping();

    if (Options::watchdog()) {
        Watchdog& watchdog = ensureWatchdog();
        watchdog.setTimeLimit(Seconds::fromMilliseconds(Options::watchdog()));
    }

#if ENABLE(JIT)
    // Make sure that any stubs that the JIT is going to use are initialized in non-compilation threads.
    if (canUseJIT()) {
        getCTIInternalFunctionTrampolineFor(CodeForCall);
        getCTIInternalFunctionTrampolineFor(CodeForConstruct);
    }
#endif

    VMInspector::instance().add(this);
}

static StaticReadWriteLock s_destructionLock;

void waitForVMDestruction()
{
    auto locker = holdLock(s_destructionLock.write());
}

VM::~VM()
{
    auto destructionLocker = holdLock(s_destructionLock.read());

    Gigacage::removePrimitiveDisableCallback(primitiveGigacageDisabledCallback, this);
    promiseDeferredTimer->stopRunningTasks();
#if ENABLE(WEBASSEMBLY)
    if (Wasm::existingWorklistOrNull())
        Wasm::ensureWorklist().stopAllPlansForContext(wasmContext);
#endif
    if (UNLIKELY(m_watchdog))
        m_watchdog->willDestroyVM(this);
    m_traps.willDestroyVM();
    VMInspector::instance().remove(this);

    // Never GC, ever again.
    heap.incrementDeferralDepth();

#if ENABLE(SAMPLING_PROFILER)
    if (m_samplingProfiler) {
        m_samplingProfiler->reportDataToOptionFile();
        m_samplingProfiler->shutdown();
    }
#endif // ENABLE(SAMPLING_PROFILER)

#if ENABLE(JIT)
    JITWorklist::instance()->completeAllForVM(*this);
#endif // ENABLE(JIT)

#if ENABLE(DFG_JIT)
    // Make sure concurrent compilations are done, but don't install them, since there is
    // no point to doing so.
    for (unsigned i = DFG::numberOfWorklists(); i--;) {
        if (DFG::Worklist* worklist = DFG::existingWorklistForIndexOrNull(i)) {
            worklist->removeNonCompilingPlansForVM(*this);
            worklist->waitUntilAllPlansForVMAreReady(*this);
            worklist->removeAllReadyPlansForVM(*this);
        }
    }
#endif // ENABLE(DFG_JIT)

    waitForAsynchronousDisassembly();

    // Clear this first to ensure that nobody tries to remove themselves from it.
    m_perBytecodeProfiler = nullptr;

    ASSERT(currentThreadIsHoldingAPILock());
    m_apiLock->willDestroyVM(this);
    heap.lastChanceToFinalize();

#if !USE(FAST_TLS_FOR_TLC)
    ThreadLocalCache::destructor(threadLocalCacheData);
#endif

    delete interpreter;
#ifndef NDEBUG
    interpreter = reinterpret_cast<Interpreter*>(0xbbadbeef);
#endif

    delete emptyList;

    delete propertyNames;
    if (vmType != Default)
        delete m_atomicStringTable;

    delete clientData;
    delete m_regExpCache;

#if ENABLE(YARR_JIT_ALL_PARENS_EXPRESSIONS)
    if (m_regExpPatternContexBuffer)
        delete[] m_regExpPatternContexBuffer;
#endif

#if ENABLE(REGEXP_TRACING)
    delete m_rtTraceList;
#endif

#if ENABLE(DFG_JIT)
    for (unsigned i = 0; i < m_scratchBuffers.size(); ++i)
        fastFree(m_scratchBuffers[i]);
#endif
}

void VM::primitiveGigacageDisabledCallback(void* argument)
{
    static_cast<VM*>(argument)->primitiveGigacageDisabled();
}

void VM::primitiveGigacageDisabled()
{
    if (m_apiLock->currentThreadIsHoldingLock()) {
        m_primitiveGigacageEnabled.fireAll(*this, "Primitive gigacage disabled");
        return;
    }

    // This is totally racy, and that's OK. The point is, it's up to the user to ensure that they pass the
    // uncaged buffer in a nicely synchronized manner.
    m_needToFirePrimitiveGigacageEnabled = true;
}

void VM::setLastStackTop(void* lastStackTop)
{
    m_lastStackTop = lastStackTop;
}

Ref<VM> VM::createContextGroup(HeapType heapType)
{
    return adoptRef(*new VM(APIContextGroup, heapType));
}

Ref<VM> VM::create(HeapType heapType)
{
    return adoptRef(*new VM(Default, heapType));
}

Ref<VM> VM::createLeaked(HeapType heapType)
{
    return create(heapType);
}

bool VM::sharedInstanceExists()
{
    return sharedInstanceInternal();
}

VM& VM::sharedInstance()
{
    GlobalJSLock globalLock;
    VM*& instance = sharedInstanceInternal();
    if (!instance)
        instance = adoptRef(new VM(APIShared, SmallHeap)).leakRef();
    return *instance;
}

VM*& VM::sharedInstanceInternal()
{
    static VM* sharedInstance;
    return sharedInstance;
}

Watchdog& VM::ensureWatchdog()
{
    if (!m_watchdog)
        m_watchdog = adoptRef(new Watchdog(this));
    return *m_watchdog;
}

HeapProfiler& VM::ensureHeapProfiler()
{
    if (!m_heapProfiler)
        m_heapProfiler = std::make_unique<HeapProfiler>(*this);
    return *m_heapProfiler;
}

#if ENABLE(SAMPLING_PROFILER)
SamplingProfiler& VM::ensureSamplingProfiler(RefPtr<Stopwatch>&& stopwatch)
{
    if (!m_samplingProfiler)
        m_samplingProfiler = adoptRef(new SamplingProfiler(*this, WTFMove(stopwatch)));
    return *m_samplingProfiler;
}
#endif // ENABLE(SAMPLING_PROFILER)

#if ENABLE(JIT)
static ThunkGenerator thunkGeneratorForIntrinsic(Intrinsic intrinsic)
{
    switch (intrinsic) {
    case CharCodeAtIntrinsic:
        return charCodeAtThunkGenerator;
    case CharAtIntrinsic:
        return charAtThunkGenerator;
    case Clz32Intrinsic:
        return clz32ThunkGenerator;
    case FromCharCodeIntrinsic:
        return fromCharCodeThunkGenerator;
    case SqrtIntrinsic:
        return sqrtThunkGenerator;
    case AbsIntrinsic:
        return absThunkGenerator;
    case FloorIntrinsic:
        return floorThunkGenerator;
    case CeilIntrinsic:
        return ceilThunkGenerator;
    case TruncIntrinsic:
        return truncThunkGenerator;
    case RoundIntrinsic:
        return roundThunkGenerator;
    case ExpIntrinsic:
        return expThunkGenerator;
    case LogIntrinsic:
        return logThunkGenerator;
    case IMulIntrinsic:
        return imulThunkGenerator;
    case RandomIntrinsic:
        return randomThunkGenerator;
    case BoundThisNoArgsFunctionCallIntrinsic:
        return boundThisNoArgsFunctionCallGenerator;
    default:
        return nullptr;
    }
}

#endif // ENABLE(JIT)

NativeExecutable* VM::getHostFunction(NativeFunction function, NativeFunction constructor, const String& name)
{
    return getHostFunction(function, NoIntrinsic, constructor, nullptr, name);
}

NativeExecutable* VM::getHostFunction(NativeFunction function, Intrinsic intrinsic, NativeFunction constructor, const DOMJIT::Signature* signature, const String& name)
{
#if ENABLE(JIT)
    if (canUseJIT()) {
        return jitStubs->hostFunctionStub(
            this, function, constructor,
            intrinsic != NoIntrinsic ? thunkGeneratorForIntrinsic(intrinsic) : 0,
            intrinsic, signature, name);
    }
#else // ENABLE(JIT)
    UNUSED_PARAM(intrinsic);
#endif // ENABLE(JIT)
    return NativeExecutable::create(*this,
        adoptRef(*new NativeJITCode(MacroAssemblerCodeRef::createLLIntCodeRef(llint_native_call_trampoline), JITCode::HostCallThunk)), function,
        adoptRef(*new NativeJITCode(MacroAssemblerCodeRef::createLLIntCodeRef(llint_native_construct_trampoline), JITCode::HostCallThunk)), constructor,
        NoIntrinsic, signature, name);
}

MacroAssemblerCodePtr VM::getCTIInternalFunctionTrampolineFor(CodeSpecializationKind kind)
{
#if ENABLE(JIT)
    if (canUseJIT()) {
        if (kind == CodeForCall)
            return jitStubs->ctiInternalFunctionCall(this);
        return jitStubs->ctiInternalFunctionConstruct(this);
    }
#endif
    if (kind == CodeForCall)
        return MacroAssemblerCodePtr::createLLIntCodePtr(llint_internal_function_call_trampoline);
    return MacroAssemblerCodePtr::createLLIntCodePtr(llint_internal_function_construct_trampoline);
}

VM::ClientData::~ClientData()
{
}

void VM::resetDateCache()
{
    localTimeOffsetCache.reset();
    cachedDateString = String();
    cachedDateStringValue = std::numeric_limits<double>::quiet_NaN();
    dateInstanceCache.reset();
}

void VM::whenIdle(std::function<void()> callback)
{
    if (!entryScope) {
        callback();
        return;
    }

    entryScope->addDidPopListener(callback);
}

void VM::deleteAllLinkedCode(DeleteAllCodeEffort effort)
{
    whenIdle([=] () {
        heap.deleteAllCodeBlocks(effort);
    });
}

void VM::deleteAllCode(DeleteAllCodeEffort effort)
{
    whenIdle([=] () {
        m_codeCache->clear();
        m_regExpCache->deleteAllCode();
        heap.deleteAllCodeBlocks(effort);
        heap.deleteAllUnlinkedCodeBlocks(effort);
        heap.reportAbandonedObjectGraph();
    });
}

SourceProviderCache* VM::addSourceProviderCache(SourceProvider* sourceProvider)
{
    auto addResult = sourceProviderCacheMap.add(sourceProvider, nullptr);
    if (addResult.isNewEntry)
        addResult.iterator->value = adoptRef(new SourceProviderCache);
    return addResult.iterator->value.get();
}

void VM::clearSourceProviderCaches()
{
    sourceProviderCacheMap.clear();
}

void VM::throwException(ExecState* exec, Exception* exception)
{
    if (Options::breakOnThrow()) {
        CodeBlock* codeBlock = exec->codeBlock();
        dataLog("Throwing exception in call frame ", RawPointer(exec), " for code block ", codeBlock, "\n");
        CRASH();
    }

    ASSERT(exec == topCallFrame || exec == exec->lexicalGlobalObject()->globalExec() || exec == exec->vmEntryGlobalObject()->globalExec());

    interpreter->notifyDebuggerOfExceptionToBeThrown(*this, exec, exception);

    setException(exception);

#if ENABLE(EXCEPTION_SCOPE_VERIFICATION)
    m_nativeStackTraceOfLastThrow = StackTrace::captureStackTrace(Options::unexpectedExceptionStackTraceLimit());
    m_throwingThread = &Thread::current();
#endif
}

JSValue VM::throwException(ExecState* exec, JSValue thrownValue)
{
    VM& vm = *this;
    Exception* exception = jsDynamicCast<Exception*>(vm, thrownValue);
    if (!exception)
        exception = Exception::create(*this, thrownValue);

    throwException(exec, exception);
    return JSValue(exception);
}

JSObject* VM::throwException(ExecState* exec, JSObject* error)
{
    return asObject(throwException(exec, JSValue(error)));
}

void VM::setStackPointerAtVMEntry(void* sp)
{
    m_stackPointerAtVMEntry = sp;
    updateStackLimits();
}

size_t VM::updateSoftReservedZoneSize(size_t softReservedZoneSize)
{
    size_t oldSoftReservedZoneSize = m_currentSoftReservedZoneSize;
    m_currentSoftReservedZoneSize = softReservedZoneSize;
#if !ENABLE(JIT)
    interpreter->cloopStack().setSoftReservedZoneSize(softReservedZoneSize);
#endif

    updateStackLimits();

    return oldSoftReservedZoneSize;
}

#if OS(WINDOWS)
// On Windows the reserved stack space consists of committed memory, a guard page, and uncommitted memory,
// where the guard page is a barrier between committed and uncommitted memory.
// When data from the guard page is read or written, the guard page is moved, and memory is committed.
// This is how the system grows the stack.
// When using the C stack on Windows we need to precommit the needed stack space.
// Otherwise we might crash later if we access uncommitted stack memory.
// This can happen if we allocate stack space larger than the page guard size (4K).
// The system does not get the chance to move the guard page, and commit more memory,
// and we crash if uncommitted memory is accessed.
// The MSVC compiler fixes this by inserting a call to the _chkstk() function,
// when needed, see http://support.microsoft.com/kb/100775.
// By touching every page up to the stack limit with a dummy operation,
// we force the system to move the guard page, and commit memory.

static void preCommitStackMemory(void* stackLimit)
{
    const int pageSize = 4096;
    for (volatile char* p = reinterpret_cast<char*>(&stackLimit); p > stackLimit; p -= pageSize) {
        char ch = *p;
        *p = ch;
    }
}
#endif

inline void VM::updateStackLimits()
{
#if OS(WINDOWS)
    void* lastSoftStackLimit = m_softStackLimit;
#endif

    const StackBounds& stack = Thread::current().stack();
    size_t reservedZoneSize = Options::reservedZoneSize();
    // We should have already ensured that Options::reservedZoneSize() >= minimumReserveZoneSize at
    // options initialization time, and the option value should not have been changed thereafter.
    // We don't have the ability to assert here that it hasn't changed, but we can at least assert
    // that the value is sane.
    RELEASE_ASSERT(reservedZoneSize >= minimumReservedZoneSize);

    if (m_stackPointerAtVMEntry) {
        ASSERT(stack.isGrowingDownward());
        char* startOfStack = reinterpret_cast<char*>(m_stackPointerAtVMEntry);
        m_softStackLimit = stack.recursionLimit(startOfStack, Options::maxPerThreadStackUsage(), m_currentSoftReservedZoneSize);
        m_stackLimit = stack.recursionLimit(startOfStack, Options::maxPerThreadStackUsage(), reservedZoneSize);
    } else {
        m_softStackLimit = stack.recursionLimit(m_currentSoftReservedZoneSize);
        m_stackLimit = stack.recursionLimit(reservedZoneSize);
    }

#if OS(WINDOWS)
    // We only need to precommit stack memory dictated by the VM::m_softStackLimit limit.
    // This is because VM::m_softStackLimit applies to stack usage by LLINT asm or JIT
    // generated code which can allocate stack space that the C++ compiler does not know
    // about. As such, we have to precommit that stack memory manually.
    //
    // In contrast, we do not need to worry about VM::m_stackLimit because that limit is
    // used exclusively by C++ code, and the C++ compiler will automatically commit the
    // needed stack pages.
    if (lastSoftStackLimit != m_softStackLimit)
        preCommitStackMemory(m_softStackLimit);
#endif
}

#if ENABLE(DFG_JIT)
void VM::gatherConservativeRoots(ConservativeRoots& conservativeRoots)
{
    auto lock = holdLock(m_scratchBufferLock);
    for (auto* scratchBuffer : m_scratchBuffers) {
        if (scratchBuffer->activeLength()) {
            void* bufferStart = scratchBuffer->dataBuffer();
            conservativeRoots.add(bufferStart, static_cast<void*>(static_cast<char*>(bufferStart) + scratchBuffer->activeLength()));
        }
    }
}
#endif

void logSanitizeStack(VM* vm)
{
    if (Options::verboseSanitizeStack() && vm->topCallFrame) {
        int dummy;
        auto& stackBounds = Thread::current().stack();
        dataLog(
            "Sanitizing stack for VM = ", RawPointer(vm), " with top call frame at ", RawPointer(vm->topCallFrame),
            ", current stack pointer at ", RawPointer(&dummy), ", in ",
            pointerDump(vm->topCallFrame->codeBlock()), ", last code origin = ",
            vm->topCallFrame->codeOrigin(), ", last stack top = ", RawPointer(vm->lastStackTop()), ", in stack range [", RawPointer(stackBounds.origin()), ", ", RawPointer(stackBounds.end()), "]\n");
    }
}

#if ENABLE(YARR_JIT_ALL_PARENS_EXPRESSIONS)
char* VM::acquireRegExpPatternContexBuffer()
{
    ASSERT(!m_regExpPatternContextLock.isLocked());

    m_regExpPatternContextLock.lock();
    if (!m_regExpPatternContexBuffer)
        m_regExpPatternContexBuffer = new char[VM::patternContextBufferSize];
    return m_regExpPatternContexBuffer;
}

void VM::releaseRegExpPatternContexBuffer()
{
    ASSERT(m_regExpPatternContextLock.isLocked());

    m_regExpPatternContextLock.unlock();
}
#endif

#if ENABLE(REGEXP_TRACING)
void VM::addRegExpToTrace(RegExp* regExp)
{
    gcProtect(regExp);
    m_rtTraceList->add(regExp);
}

void VM::dumpRegExpTrace()
{
    // The first RegExp object is ignored.  It is create by the RegExpPrototype ctor and not used.
    RTTraceList::iterator iter = ++m_rtTraceList->begin();

    if (iter != m_rtTraceList->end()) {
        dataLogF("\nRegExp Tracing\n");
        dataLogF("Regular Expression                              8 Bit          16 Bit        match()    Matches    Average\n");
        dataLogF(" <Match only / Match>                         JIT Addr      JIT Address       calls      found   String len\n");
        dataLogF("----------------------------------------+----------------+----------------+----------+----------+-----------\n");

        unsigned reCount = 0;

        for (; iter != m_rtTraceList->end(); ++iter, ++reCount) {
            (*iter)->printTraceData();
            gcUnprotect(*iter);
        }

        dataLogF("%d Regular Expressions\n", reCount);
    }

    m_rtTraceList->clear();
}
#else
void VM::dumpRegExpTrace()
{
}
#endif

WatchpointSet* VM::ensureWatchpointSetForImpureProperty(const Identifier& propertyName)
{
    auto result = m_impurePropertyWatchpointSets.add(propertyName.string(), nullptr);
    if (result.isNewEntry)
        result.iterator->value = adoptRef(new WatchpointSet(IsWatched));
    return result.iterator->value.get();
}

void VM::registerWatchpointForImpureProperty(const Identifier& propertyName, Watchpoint* watchpoint)
{
    ensureWatchpointSetForImpureProperty(propertyName)->add(watchpoint);
}

void VM::addImpureProperty(const String& propertyName)
{
    if (RefPtr<WatchpointSet> watchpointSet = m_impurePropertyWatchpointSets.take(propertyName))
        watchpointSet->fireAll(*this, "Impure property added");
}

static bool enableProfilerWithRespectToCount(unsigned& counter, std::function<void()> doEnableWork)
{
    bool needsToRecompile = false;
    if (!counter) {
        doEnableWork();
        needsToRecompile = true;
    }
    counter++;

    return needsToRecompile;
}

static bool disableProfilerWithRespectToCount(unsigned& counter, std::function<void()> doDisableWork)
{
    RELEASE_ASSERT(counter > 0);
    bool needsToRecompile = false;
    counter--;
    if (!counter) {
        doDisableWork();
        needsToRecompile = true;
    }

    return needsToRecompile;
}

bool VM::enableTypeProfiler()
{
    auto enableTypeProfiler = [this] () {
        this->m_typeProfiler = std::make_unique<TypeProfiler>();
        this->m_typeProfilerLog = std::make_unique<TypeProfilerLog>();
    };

    return enableProfilerWithRespectToCount(m_typeProfilerEnabledCount, enableTypeProfiler);
}

bool VM::disableTypeProfiler()
{
    auto disableTypeProfiler = [this] () {
        this->m_typeProfiler.reset(nullptr);
        this->m_typeProfilerLog.reset(nullptr);
    };

    return disableProfilerWithRespectToCount(m_typeProfilerEnabledCount, disableTypeProfiler);
}

bool VM::enableControlFlowProfiler()
{
    auto enableControlFlowProfiler = [this] () {
        this->m_controlFlowProfiler = std::make_unique<ControlFlowProfiler>();
    };

    return enableProfilerWithRespectToCount(m_controlFlowProfilerEnabledCount, enableControlFlowProfiler);
}

bool VM::disableControlFlowProfiler()
{
    auto disableControlFlowProfiler = [this] () {
        this->m_controlFlowProfiler.reset(nullptr);
    };

    return disableProfilerWithRespectToCount(m_controlFlowProfilerEnabledCount, disableControlFlowProfiler);
}

void VM::dumpTypeProfilerData()
{
    if (!typeProfiler())
        return;

    typeProfilerLog()->processLogEntries(ASCIILiteral("VM Dump Types"));
    typeProfiler()->dumpTypeProfilerData(*this);
}

void VM::queueMicrotask(JSGlobalObject& globalObject, Ref<Microtask>&& task)
{
    m_microtaskQueue.append(std::make_unique<QueuedTask>(*this, &globalObject, WTFMove(task)));
}

void VM::drainMicrotasks()
{
    while (!m_microtaskQueue.isEmpty())
        m_microtaskQueue.takeFirst()->run();
}

void QueuedTask::run()
{
    m_microtask->run(m_globalObject->globalExec());
}

void sanitizeStackForVM(VM* vm)
{
    logSanitizeStack(vm);
    if (vm->topCallFrame) {
        auto& stackBounds = Thread::current().stack();
        ASSERT(vm->currentThreadIsHoldingAPILock());
        ASSERT_UNUSED(stackBounds, stackBounds.contains(vm->lastStackTop()));
    }
#if !ENABLE(JIT)
    vm->interpreter->cloopStack().sanitizeStack();
#else
    sanitizeStackForVMImpl(vm);
#endif
}

size_t VM::committedStackByteCount()
{
#if ENABLE(JIT)
    // When using the C stack, we don't know how many stack pages are actually
    // committed. So, we use the current stack usage as an estimate.
    ASSERT(Thread::current().stack().isGrowingDownward());
    int8_t* current = reinterpret_cast<int8_t*>(&current);
    int8_t* high = reinterpret_cast<int8_t*>(Thread::current().stack().origin());
    return high - current;
#else
    return CLoopStack::committedByteCount();
#endif
}

#if !ENABLE(JIT)
bool VM::ensureStackCapacityForCLoop(Register* newTopOfStack)
{
    return interpreter->cloopStack().ensureCapacityFor(newTopOfStack);
}

bool VM::isSafeToRecurseSoftCLoop() const
{
    return interpreter->cloopStack().isSafeToRecurse();
}
#endif // !ENABLE(JIT)

#if ENABLE(EXCEPTION_SCOPE_VERIFICATION)
void VM::verifyExceptionCheckNeedIsSatisfied(unsigned recursionDepth, ExceptionEventLocation& location)
{
    if (!Options::validateExceptionChecks())
        return;

    if (UNLIKELY(m_needExceptionCheck)) {
        auto throwDepth = m_simulatedThrowPointRecursionDepth;
        auto& throwLocation = m_simulatedThrowPointLocation;

        dataLog(
            "ERROR: Unchecked JS exception:\n"
            "    This scope can throw a JS exception: ", throwLocation, "\n"
            "        (ExceptionScope::m_recursionDepth was ", throwDepth, ")\n"
            "    But the exception was unchecked as of this scope: ", location, "\n"
            "        (ExceptionScope::m_recursionDepth was ", recursionDepth, ")\n"
            "\n");

        StringPrintStream out;
        std::unique_ptr<StackTrace> currentTrace = StackTrace::captureStackTrace(Options::unexpectedExceptionStackTraceLimit());

        if (Options::dumpSimulatedThrows()) {
            out.println("The simulated exception was thrown at:");
            m_nativeStackTraceOfLastSimulatedThrow->dump(out, "    ");
            out.println();
        }
        out.println("Unchecked exception detected at:");
        currentTrace->dump(out, "    ");
        out.println();

        dataLog(out.toCString());
        RELEASE_ASSERT(!m_needExceptionCheck);
    }
}
#endif

#if USE(CF)
void VM::registerRunLoopTimer(JSRunLoopTimer* timer)
{
    ASSERT(runLoop());
    ASSERT(!m_runLoopTimers.contains(timer));
    m_runLoopTimers.add(timer);
    timer->setRunLoop(runLoop());
}

void VM::unregisterRunLoopTimer(JSRunLoopTimer* timer)
{
    ASSERT(m_runLoopTimers.contains(timer));
    m_runLoopTimers.remove(timer);
    timer->setRunLoop(nullptr);
}

void VM::setRunLoop(CFRunLoopRef runLoop)
{
    ASSERT(runLoop);
    m_runLoop = runLoop;
    for (auto timer : m_runLoopTimers)
        timer->setRunLoop(runLoop);
}
#endif // USE(CF)

ScratchBuffer* VM::scratchBufferForSize(size_t size)
{
    if (!size)
        return nullptr;

    auto locker = holdLock(m_scratchBufferLock);

    if (size > m_sizeOfLastScratchBuffer) {
        // Protect against a N^2 memory usage pathology by ensuring
        // that at worst, we get a geometric series, meaning that the
        // total memory usage is somewhere around
        // max(scratch buffer size) * 4.
        m_sizeOfLastScratchBuffer = size * 2;

        ScratchBuffer* newBuffer = ScratchBuffer::create(m_sizeOfLastScratchBuffer);
        RELEASE_ASSERT(newBuffer);
        m_scratchBuffers.append(newBuffer);
    }

    ScratchBuffer* result = m_scratchBuffers.last();
    return result;
}

} // namespace JSC
