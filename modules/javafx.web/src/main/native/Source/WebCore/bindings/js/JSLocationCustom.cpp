/*
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2006 Jon Shier (jshier@iastate.edu)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2010 Apple Inc. All rights reseved.
 *  Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "config.h"
#include "JSLocation.h"

#include "JSDOMBinding.h"
#include "JSDOMBindingSecurity.h"
#include "JSDOMExceptionHandling.h"
#include "RuntimeApplicationChecks.h"
#include <JavaScriptCore/JSFunction.h>
#include <JavaScriptCore/Lookup.h>

namespace WebCore {
using namespace JSC;

static bool getOwnPropertySlotCommon(JSLocation& thisObject, ExecState& state, PropertyName propertyName, PropertySlot& slot)
{
    VM& vm = state.vm();
    auto scope = DECLARE_THROW_SCOPE(vm);

    Frame* frame = thisObject.wrapped().frame();
    if (!frame) {
        slot.setUndefined();
        return true;
    }

    // When accessing Location cross-domain, functions are always the native built-in ones.
    // See JSDOMWindow::getOwnPropertySlotDelegate for additional details.

    // Our custom code is only needed to implement the Window cross-domain scheme, so if access is
    // allowed, return false so the normal lookup will take place.
    String message;
    if (BindingSecurity::shouldAllowAccessToFrame(state, *frame, message))
        return false;

    // https://html.spec.whatwg.org/#crossorigingetownpropertyhelper-(-o,-p-)
    if (propertyName == vm.propertyNames->toStringTagSymbol || propertyName == vm.propertyNames->hasInstanceSymbol || propertyName == vm.propertyNames->isConcatSpreadableSymbol) {
        slot.setValue(&thisObject, PropertyAttribute::ReadOnly | PropertyAttribute::DontEnum, jsUndefined());
        return true;
    }

    // We only allow access to Location.replace() cross origin.
    if (propertyName == vm.propertyNames->replace) {
        slot.setCustom(&thisObject, static_cast<unsigned>(PropertyAttribute::ReadOnly | PropertyAttribute::DontEnum), nonCachingStaticFunctionGetter<jsLocationInstanceFunctionReplace, 1>);
        return true;
    }

    // Getting location.href cross origin needs to throw. However, getOwnPropertyDescriptor() needs to return
    // a descriptor that has a setter but no getter.
    if (slot.internalMethodType() == PropertySlot::InternalMethodType::GetOwnProperty && propertyName == vm.propertyNames->href) {
        auto* entry = JSLocation::info()->staticPropHashTable->entry(propertyName);
        CustomGetterSetter* customGetterSetter = CustomGetterSetter::create(vm, nullptr, entry->propertyPutter());
        slot.setCustomGetterSetter(&thisObject, static_cast<unsigned>(JSC::PropertyAttribute::CustomAccessor | PropertyAttribute::DontEnum), customGetterSetter);
        return true;
    }

    throwSecurityError(state, scope, message);
    slot.setUndefined();
    return true;
}

bool JSLocation::getOwnPropertySlot(JSObject* object, ExecState* state, PropertyName propertyName, PropertySlot& slot)
{
    auto* thisObject = jsCast<JSLocation*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    if (getOwnPropertySlotCommon(*thisObject, *state, propertyName, slot))
        return true;
    return JSObject::getOwnPropertySlot(object, state, propertyName, slot);
}

bool JSLocation::getOwnPropertySlotByIndex(JSObject* object, ExecState* state, unsigned index, PropertySlot& slot)
{
    auto* thisObject = jsCast<JSLocation*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    if (getOwnPropertySlotCommon(*thisObject, *state, Identifier::from(state, index), slot))
        return true;
    return JSObject::getOwnPropertySlotByIndex(object, state, index, slot);
}

static bool putCommon(JSLocation& thisObject, ExecState& state, PropertyName propertyName)
{
    Frame* frame = thisObject.wrapped().frame();
    if (!frame)
        return true;

    VM& vm = state.vm();
    // Silently block access to toString and valueOf.
    if (propertyName == vm.propertyNames->toString || propertyName == vm.propertyNames->valueOf)
        return true;

    // Always allow assigning to the whole location.
    // However, alllowing assigning of pieces might inadvertently disclose parts of the original location.
    // So fall through to the access check for those.
    if (propertyName == vm.propertyNames->href)
        return false;

    // Block access and throw if there is a security error.
    if (!BindingSecurity::shouldAllowAccessToFrame(&state, frame, ThrowSecurityError))
        return true;

    return false;
}

bool JSLocation::put(JSCell* cell, ExecState* state, PropertyName propertyName, JSValue value, PutPropertySlot& putPropertySlot)
{
    auto* thisObject = jsCast<JSLocation*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    if (putCommon(*thisObject, *state, propertyName))
        return false;

    return JSObject::put(thisObject, state, propertyName, value, putPropertySlot);
}

bool JSLocation::putByIndex(JSCell* cell, ExecState* state, unsigned index, JSValue value, bool shouldThrow)
{
    auto* thisObject = jsCast<JSLocation*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    if (putCommon(*thisObject, *state, Identifier::from(state, index)))
        return false;

    return JSObject::putByIndex(cell, state, index, value, shouldThrow);
}

bool JSLocation::deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName)
{
    JSLocation* thisObject = jsCast<JSLocation*>(cell);
    // Only allow deleting by frames in the same origin.
    if (!BindingSecurity::shouldAllowAccessToFrame(exec, thisObject->wrapped().frame(), ThrowSecurityError))
        return false;
    return Base::deleteProperty(thisObject, exec, propertyName);
}

bool JSLocation::deletePropertyByIndex(JSCell* cell, ExecState* exec, unsigned propertyName)
{
    JSLocation* thisObject = jsCast<JSLocation*>(cell);
    // Only allow deleting by frames in the same origin.
    if (!BindingSecurity::shouldAllowAccessToFrame(exec, thisObject->wrapped().frame(), ThrowSecurityError))
        return false;
    return Base::deletePropertyByIndex(thisObject, exec, propertyName);
}

// https://html.spec.whatwg.org/#crossoriginproperties-(-o-)
static void addCrossOriginLocationPropertyNames(ExecState& state, PropertyNameArray& propertyNames)
{
    VM& vm = state.vm();
    static const Identifier* const properties[] = { &vm.propertyNames->href, &vm.propertyNames->replace };
    for (auto* property : properties)
        propertyNames.add(*property);
}

// https://html.spec.whatwg.org/#crossoriginownpropertykeys-(-o-)
static void addCrossOriginLocationOwnPropertyNames(ExecState& state, PropertyNameArray& propertyNames)
{
    VM& vm = state.vm();
    addCrossOriginLocationPropertyNames(state, propertyNames);

    propertyNames.add(vm.propertyNames->toStringTagSymbol);
    propertyNames.add(vm.propertyNames->hasInstanceSymbol);
    propertyNames.add(vm.propertyNames->isConcatSpreadableSymbol);
}

void JSLocation::getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSLocation* thisObject = jsCast<JSLocation*>(object);
    if (!BindingSecurity::shouldAllowAccessToFrame(exec, thisObject->wrapped().frame(), DoNotReportSecurityError)) {
        if (mode.includeDontEnumProperties())
            addCrossOriginLocationOwnPropertyNames(*exec, propertyNames);
        return;
    }
    Base::getOwnPropertyNames(thisObject, exec, propertyNames, mode);
}

bool JSLocation::defineOwnProperty(JSObject* object, ExecState* exec, PropertyName propertyName, const PropertyDescriptor& descriptor, bool throwException)
{
    JSLocation* thisObject = jsCast<JSLocation*>(object);
    if (!BindingSecurity::shouldAllowAccessToFrame(exec, thisObject->wrapped().frame(), ThrowSecurityError))
        return false;

    VM& vm = exec->vm();
    if (descriptor.isAccessorDescriptor() && (propertyName == vm.propertyNames->toString || propertyName == vm.propertyNames->valueOf))
        return false;
    return Base::defineOwnProperty(object, exec, propertyName, descriptor, throwException);
}

JSValue JSLocation::getPrototype(JSObject* object, ExecState* exec)
{
    JSLocation* thisObject = jsCast<JSLocation*>(object);
    if (!BindingSecurity::shouldAllowAccessToFrame(exec, thisObject->wrapped().frame(), DoNotReportSecurityError))
        return jsNull();

    return Base::getPrototype(object, exec);
}

bool JSLocation::preventExtensions(JSObject*, ExecState* exec)
{
    auto scope = DECLARE_THROW_SCOPE(exec->vm());

    throwTypeError(exec, scope, "Cannot prevent extensions on this object"_s);
    return false;
}

String JSLocation::toStringName(const JSObject* object, ExecState* exec)
{
    auto* thisObject = jsCast<const JSLocation*>(object);
    if (!BindingSecurity::shouldAllowAccessToFrame(exec, thisObject->wrapped().frame(), DoNotReportSecurityError))
        return "Object"_s;
    return "Location"_s;
}

bool JSLocationPrototype::put(JSCell* cell, ExecState* state, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    VM& vm = state->vm();
    auto* thisObject = jsCast<JSLocationPrototype*>(cell);
    if (propertyName == vm.propertyNames->toString || propertyName == vm.propertyNames->valueOf)
        return false;
    return Base::put(thisObject, state, propertyName, value, slot);
}

bool JSLocationPrototype::defineOwnProperty(JSObject* object, ExecState* exec, PropertyName propertyName, const PropertyDescriptor& descriptor, bool throwException)
{
    VM& vm = exec->vm();
    if (descriptor.isAccessorDescriptor() && (propertyName == vm.propertyNames->toString || propertyName == vm.propertyNames->valueOf))
        return false;
    return Base::defineOwnProperty(object, exec, propertyName, descriptor, throwException);
}

} // namespace WebCore
