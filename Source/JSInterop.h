//
// Created by Max on 23/05/2023.
//
#pragma once
#include "Ultralight/View.h"
#ifndef ULTRALIGHTJUCE_JSINTEROP_H
#define ULTRALIGHTJUCE_JSINTEROP_H

#include <JuceHeader.h>
#include "Ultralight/RefPtr.h"
#include "JavaScriptCore/JavaScriptCore.h"
#include <JavaScriptCore/JSRetainPtr.h>

class JSInterop {
public:
    JSInterop(ultralight::View& inView) : view(inView) {}

    template<typename T>
    void InvokeMethod(const juce::String& methodName, const T& value) {
        ultralight::Ref<ultralight::JSContext> context = view.LockJSContext();
        JSContextRef ctx = context.get();

        JSRetainPtr<JSStringRef> methodNameSTR = adopt(JSStringCreateWithUTF8CString(methodName.toRawUTF8()));
        JSValueRef func = JSEvaluateScript(ctx, methodNameSTR.get(), nullptr, nullptr, 0, nullptr);

        if (JSValueIsObject(ctx, func)) {
            JSObjectRef funcObj = JSValueToObject(ctx, func, 0);
            if (funcObj && JSObjectIsFunction(ctx, funcObj)) {
                JSValueRef argVal = CreateJSValue(ctx, value);
                JSValueRef args[] = { argVal };
                size_t num_args = 1;
                JSValueRef exception = nullptr;
                JSValueRef result = JSObjectCallAsFunction(ctx, funcObj, 0, num_args, args, &exception);
                if (exception) {
                    // Handle any exceptions thrown from function here.
                }
                if (result) {
                    // Handle result (if any) here.
                }
            }
        } else {
            DBG("JSInterop::InvokeMethod: " + methodName + " is not a valid JS function or threw and exception.");
        }
    }

    // Helper function to convert different types to JSValueRef
    template<typename T>
    JSValueRef CreateJSValue(JSContextRef ctx, const T& value) {
        // Default implementation for unsupported types
        DBG("JSInterop::CreateJSValue: Unsupported type.");
        return JSValueMakeUndefined(ctx);
    }

    // Specialization for float
    template<>
    JSValueRef CreateJSValue(JSContextRef ctx, const float& value) {
        return JSValueMakeNumber(ctx, static_cast<double>(value));
    }

    // Specialization for int
    template<>
    JSValueRef CreateJSValue(JSContextRef ctx, const int& value) {
        return JSValueMakeNumber(ctx, static_cast<double>(value));
    }

    // Specialization for bool
    template<>
    JSValueRef CreateJSValue(JSContextRef ctx, const bool& value) {
        return JSValueMakeBoolean(ctx, value);
    }

private:
    // The view (in the future hopefully views) that we want to interact with
    ultralight::View& view;


};
#endif //ULTRALIGHTJUCE_JSINTEROP_H
