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

class JSInterop :
        public ultralight::LoadListener
        {
public:
    JSInterop(ultralight::View& inView, juce::AudioProcessorValueTreeState& params)
    : view(inView), audioParams(params)
    {}

    // ========================================================================================================
    // C++ -> JS
    // ========================================================================================================

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

    // ========================================================================================================
    // JS -> C++
    // ========================================================================================================
    /// \brief Registers a C++ function with the JS of the view. Functions registered with this method can be called
    /// from JS and will produce a callback in C++.
    /// \param functionName
    /// \param callbackFunction
    void registerCppFunctionInJS(const juce::String& functionName, JSObjectCallAsFunctionCallback callbackFunction) {
        // Get JS context
        ultralight::Ref<ultralight::JSContext> context = view.LockJSContext();
        JSContextRef ctx = context.get();
        // Create a JavaScript String containing the name of our callback.
        JSStringRef name = JSStringCreateWithUTF8CString(functionName.toRawUTF8());
        // Create a garbage-collected JavaScript function that is bound to our native C callback 'callbackFunction'.
        JSObjectRef func = JSObjectMakeFunctionWithCallback(ctx, name, callbackFunction);
        // Get the global JavaScript object (aka 'window')
        JSObjectRef globalObj = JSContextGetGlobalObject(ctx);
        // Store our function in the page's global JavaScript object so that it accessible from the page as '{callbackFunction}()'.
        JSObjectSetProperty(ctx, globalObj, name, func, 0, 0);
        // Release the JavaScript String we created earlier.
        JSStringRelease(name);
    }

    void OnDOMReady(ultralight::View* caller,
                            uint64_t frame_id,
                            bool is_main_frame,
                            const ultralight::String& url) override {
        // Get JS context
        ultralight::Ref<ultralight::JSContext> context = view.LockJSContext();
        JSContextRef ctx = context.get();
        // Register this class (JSInterop) as a global object in JavaScript
        JSObjectRef globalObj = JSContextGetGlobalObject(ctx);
        JSStringRef name = JSStringCreateWithUTF8CString("JSInterop");
        // We need to create our custom JS class definition here in order to use JSObjectSetPrivate
        // Default JS classes don't allow us to set a private instance pointer
        JSClassDefinition classDef = kJSClassDefinitionEmpty;
        classDef.callAsConstructor = nullptr; // Set to NULL for simplicity
        JSClassRef classRef = JSClassCreate(&classDef);
        JSObjectRef jsObj = JSObjectMake(ctx, classRef, nullptr);
        // Key point: Set the instance pointer as the private data of the JS object
        // This means that we can access the instance pointer and thereby member variables from the callback
        auto success = JSObjectSetPrivate(jsObj, this);
        // Add the class reference to the global JS object ("window")
        JSObjectSetProperty(ctx, globalObj, name, jsObj, 0, nullptr);
        JSClassRelease(classRef);

        // Register all global functions here
        // They need to be wrapped as shown in OnButtonClickWrapper below in order to access member variables
        // If you don't require access to member variables, you can register them directly (without wrapping)
        // In that case, the "JSValueRef OnButtonClick(){...}" below would be made static
        registerCppFunctionInJS("OnButtonClick", OnButtonClick);
        registerCppFunctionInJS("OnGainUpdate", OnGainUpdate);
    }

    /// \brief Gets the instance pointer from the JS object
    static JSInterop* GetInstance(JSContextRef ctx) {
        JSObjectRef globalObj = JSContextGetGlobalObject(ctx);
        JSStringRef name = JSStringCreateWithUTF8CString("JSInterop");
        JSValueRef jsObj = JSObjectGetProperty(ctx, globalObj, name, nullptr);
        auto jsInteropJSObjRef = JSValueToObject(ctx, jsObj, nullptr);
        JSStringRelease(name);
        auto* instance = static_cast<JSInterop*>(JSObjectGetPrivate(jsInteropJSObjRef));

        if(instance == nullptr) {
            DBG("JSInterop::GetInstance: Failed to get instance.");
            return nullptr;
        }
        return instance;
    }

    static JSValueRef OnButtonClick(JSContextRef ctx, JSObjectRef function,
                                    JSObjectRef thisObject, size_t argumentCount,
                                    const JSValueRef arguments[], JSValueRef* exception) {
        // Get the class instance pointer from the JS object
        auto* instance = GetInstance(ctx);
        // This is just some dummy JS call
        const char* str = "document.getElementById('result').innerText = 'Ultralight rocks!'";
        // Create our string of JavaScript
        JSStringRef script = JSStringCreateWithUTF8CString(str);
        // Execute it with JSEvaluateScript, ignoring other parameters for now
        JSEvaluateScript(ctx, script, 0, 0, 0, 0);
        // Release our string (we only Release what we Create)
        JSStringRelease(script);
        return JSValueMakeNull(ctx);
    }

    static JSValueRef OnGainUpdate(JSContextRef ctx, JSObjectRef function,
                             JSObjectRef thisObject, size_t argumentCount,
                             const JSValueRef arguments[], JSValueRef* exception) {
        // Get the class instance pointer from the JS object
        auto* instance = GetInstance(ctx);

        // Update the gain value in JUCE
        float newValue = JSValueToNumber(ctx, arguments[0], nullptr);

        // Use instance instead of "this"
        auto* gainParam = instance->audioParams.getParameter("gain");
        if(gainParam != nullptr)
            gainParam->setValueNotifyingHost(newValue);
        return JSValueMakeNull(ctx);
    }


private:
    // The view (in the future hopefully views) that we want to interact with
    ultralight::View& view;
    juce::AudioProcessorValueTreeState& audioParams;


};
#endif //ULTRALIGHTJUCE_JSINTEROP_H
