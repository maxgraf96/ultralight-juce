//
// Created by Max on 23/05/2023.
//
#pragma once
#include "AppCore/JSHelpers.h"
#ifndef ULTRALIGHTJUCE_JSINTEROP_H
#define ULTRALIGHTJUCE_JSINTEROP_H

#include <JavaScriptCore/JSRetainPtr.h>
#include <JavaScriptCore/JavaScript.h>
#include <JuceHeader.h>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <typeinfo>

#include "Ultralight/View.h"
#include "Ultralight/RefPtr.h"

/// \brief Base class for all JS interoperation. This class is used to invoke JS methods from C++ and vice versa.
/// You can extend this class to add your own JS interoperation. An example of how to subclass it is given in
/// JSInteropExample.h.
class JSInteropBase :
        public ultralight::LoadListener
        {
public:
    JSInteropBase(ultralight::View& inView, juce::AudioProcessorValueTreeState& params, juce::AudioProcessorValueTreeState::Listener& parentComponent)
    : view(inView), audioParams(params), parent(parentComponent)
    {
    }

    /// \brief Core function. This function is called by Ultralight once the DOM has been loaded. This implementation
    /// sets up automatic callbacks for JUCE AudioProcessorValueTreeState (APVTS) changes in JS.
    void OnWindowObjectReady(ultralight::View* caller,
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

        // Register APVTS parameter update callback
        registerCppFunctionInJS("OnParameterUpdate", OnParameterUpdate);

        // === JUCE APVTS PARAMS ===
        // Propagate all parameters that were loaded from disk to JS
        for(auto param : audioParams.processor.getParameters()) {
            // Check if parameter is a float parameter
            if(auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param)) {
                parent.parameterChanged(floatParam->getParameterID(), floatParam->get());
            }
            // TODO Add other parameter types
        }
    }

    /// \brief APVTS parameter propagation to JS. See how it is handled in Resources/script.js.
    static JSValueRef OnParameterUpdate(JSContextRef ctx, JSObjectRef function,
                                        JSObjectRef thisObject, size_t argumentCount,
                                        const JSValueRef arguments[], JSValueRef* exception) {
        // Get the class instance pointer from the JS object
        auto* instance = GetInstance(ctx);

        // Get the parameter ID
        auto parameterID = JSValueToStringCopy(ctx, arguments[0], nullptr);
        // Allocate a char* of length JSStringGetLength(parameterID)
        char* parameterIDStr = new char[JSStringGetLength(parameterID) + 1];
        JSStringGetUTF8CString(parameterID, parameterIDStr, JSStringGetLength(parameterID));

        // Get the parameter value
        auto newValue = JSValueToNumber(ctx, arguments[1], nullptr);

        // Update the parameter value in JUCE
        instance->audioParams.getParameter(parameterIDStr)->setValueNotifyingHost(static_cast<float>(newValue));

        JSStringRelease(parameterID);
        delete[] parameterIDStr;
        return JSValueMakeNull(ctx);
    }

    // ========================================================================================================
    // C++ -> JS
    // ========================================================================================================
    template<typename... T>
    void invokeMethod(const juce::String& methodName, const T&... value) {
        // Get the JSContext from the View
        ultralight::Ref<ultralight::JSContext> context = view.LockJSContext();
        JSContextRef ctx = context.get();

        // Create a JSStringRef from the methodName and evaluate it
        JSRetainPtr<JSStringRef> methodNameSTR = adopt(JSStringCreateWithUTF8CString(methodName.toRawUTF8()));
        JSValueRef func = JSEvaluateScript(ctx, methodNameSTR.get(), nullptr, nullptr, 0, nullptr);

        // Check if the function is valid
        if (JSValueIsObject(ctx, func)) {
            JSObjectRef funcObj = JSValueToObject(ctx, func, nullptr);
            if (funcObj && JSObjectIsFunction(ctx, funcObj)) {
                // Valid function, call it with the given arguments
                // Check how many arguments we have
                constexpr std::size_t argCount = std::tuple_size<std::tuple<const T&...>>::value;

                JSValueRef args[] = { CreateJSValue(ctx, value)... };
                size_t num_args = argCount;
                JSValueRef exception = nullptr;
                JSValueRef result = JSObjectCallAsFunction(ctx, funcObj, nullptr, num_args, args, &exception);
                if (exception) {
                    // Handle any exceptions thrown from function here.
                    DBG("JSInterop::invokeMethod: " + methodName + " threw an exception on the JS side. Double check that the data you are passing is correct and that the JS function is valid.");
                }
                if (result) {
                    // Handle result (if any) here.
                }
            }
        } else {
            DBG("JSInterop::invokeMethod: " + methodName + " is not a valid JS function or threw an exception.");
        }
    }

    // ========================================================================================================
    // JS -> C++
    // ========================================================================================================
    /// \brief Registers a C++ lambda or function object with the JS of the view. Functions registered with this method
    /// can be called from JS and will produce a callback in C++.
    /// \param functionName The name of the function as it will be called from JS
    /// \param callbackFunction The C++ function that will be called when the JS function is called. Accepts lambdas.
    template<typename... T>
    void registerCppCallbackInJS(const juce::String& functionName, std::function<void(T...)> callbackFunction) {
        // Get JS context
        ultralight::Ref<ultralight::JSContext> context = view.LockJSContext();
        JSContextRef ctx = context.get();
        // Create a JavaScript String containing the name of our callback.
        JSStringRef name = JSStringCreateWithUTF8CString(functionName.toRawUTF8());

        // Create a JavaScript object with a private data member to hold the callback function and argument
        struct JSFunctionWrapper {
            std::function<void(T...)> callback;
            std::tuple<T...> arguments;
        };

        // Define a JavaScript callback function that will call the C++ lambda or function object
        JSObjectCallAsFunctionCallback jsCallback = [](JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                                       size_t argumentCount, const JSValueRef arguments[],
                                                       JSValueRef* exception) -> JSValueRef {
            // Retrieve the stored callback function and argument from the private data of the JS object
            JSObjectRef functionWrapper = JSValueToObject(ctx, JSObjectGetProperty(ctx, function, JSStringCreateWithUTF8CString("callback"), nullptr), nullptr);
            JSFunctionWrapper* wrapper = reinterpret_cast<JSFunctionWrapper*>(JSObjectGetPrivate(functionWrapper));
            if (wrapper != nullptr) {
                // Convert the JavaScript arguments to the desired C++ types and call the callback function
                std::apply(wrapper->callback, GetConvertedArguments<T...>(ctx, arguments, argumentCount, wrapper->arguments));
            }
            return JSValueMakeUndefined(ctx);
        };

        // Create a JavaScript object with the private data holding the callback function and argument
        JSClassDefinition functionWrapperClassDef = kJSClassDefinitionEmpty;
        functionWrapperClassDef.className = "JSFunctionWrapper";
        JSClassRef functionWrapperClass = JSClassCreate(&functionWrapperClassDef);
        JSObjectRef functionWrapper = JSObjectMake(ctx, functionWrapperClass, nullptr);
        JSObjectSetPrivate(functionWrapper, new JSFunctionWrapper{ callbackFunction });

        // Create a garbage-collected JavaScript function that is bound to our native C callback 'jsCallback'.
        JSObjectRef func = JSObjectMakeFunctionWithCallback(ctx, name, jsCallback);
        // Set the callback function as a private member of the JavaScript function object
        JSObjectSetProperty(ctx, func, JSStringCreateWithUTF8CString("callback"), functionWrapper, 0, nullptr);

        // Get the global JavaScript object (aka 'window')
        JSObjectRef globalObj = JSContextGetGlobalObject(ctx);
        // Store our function in the page's global JavaScript object so that it is accessible from the page as '{callbackFunction}()'.
        JSObjectSetProperty(ctx, globalObj, name, func, 0, nullptr);
        // Release the JavaScript String we created earlier.
        JSStringRelease(name);
    }

    /// \brief Registers a C++ function with the JS of the view. Functions registered with this method can be called
    /// from JS and will produce a callback in C++.
    /// \param functionName The name of the function as it will be called from JS
    /// \param callbackFunction The C++ function that will be called when the JS function is called
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

    // ================================== HELPER FUNCTIONS ==================================
    // C++ -> JS
    // Helper function to convert different types to JSValueRef
    // TODO: Add code for handling more complex data yourself (e.g., using a string parser)
    template<typename T>
    JSValueRef CreateJSValue(JSContextRef ctx, const T& value) {
        // Default implementation for unsupported types
        DBG("JSInterop::CreateJSValue: Unsupported type.");
        return JSValueMakeUndefined(ctx);
    }

    // float
    template<>
    JSValueRef CreateJSValue(JSContextRef ctx, const float& value) {
        return JSValueMakeNumber(ctx, static_cast<double>(value));
    }
    // int
    template<>
    JSValueRef CreateJSValue(JSContextRef ctx, const int& value) {
        return JSValueMakeNumber(ctx, static_cast<double>(value));
    }
    // bool
    template<>
    JSValueRef CreateJSValue(JSContextRef ctx, const bool& value) {
        return JSValueMakeBoolean(ctx, value);
    }
    // String
    template<>
    JSValueRef CreateJSValue(JSContextRef ctx, const juce::String& value) {
        return JSValueMakeString(ctx, JSStringCreateWithUTF8CString(value.toRawUTF8()));
    }

    // Lists/Arrays (using std::vectors)
    template<typename T>
    JSValueRef CreateJSValue(JSContextRef ctx, const std::vector<T>& value) {
        auto* jsValues = new JSValueRef[value.size()];
        for(int i = 0; i < value.size(); i++) {
            jsValues[i] = CreateJSValue(ctx, value[i]);
        }
        JSValueRef jsArray = JSObjectMakeArray(ctx, value.size(), jsValues, nullptr);
        delete[] jsValues;
        return jsArray;
    }


    //==============================================================================


    // JS -> C++
    // Helper function to convert different types of JSValueRef to C++ types
    template<typename T>
    static T GetJSValue(JSContextRef ctx, JSValueRef value) {
        // Default implementation for unsupported types
        DBG("JSInterop::GetJSValue: Unsupported type.");
        return T();
    }

    // float
    template<>
    static float GetJSValue<float>(JSContextRef ctx, JSValueRef value) {
        return static_cast<float>(JSValueToNumber(ctx, value, nullptr));
    }
    // int
    template<>
    static int GetJSValue<int>(JSContextRef ctx, JSValueRef value) {
        return static_cast<int>(JSValueToNumber(ctx, value, nullptr));
    }
    // bool
    template<>
    static bool GetJSValue<bool>(JSContextRef ctx, JSValueRef value) {
        return JSValueToBoolean(ctx, value);
    }
    // String
    template<>
    static juce::String GetJSValue<juce::String>(JSContextRef ctx, JSValueRef value) {
        auto jsString = JSValueToStringCopy(ctx, value, nullptr);
        auto length = JSStringGetMaximumUTF8CStringSize(jsString);
        char* buffer = new char[length];
        JSStringGetUTF8CString(jsString, buffer, length);
        auto string = juce::String(buffer);
        JSStringRelease(jsString);
        delete[] buffer;
        return string;
    }

    // Lists/Arrays (using std::vectors)
    template<>
    static std::vector<juce::String> GetJSValue<std::vector<juce::String>>(JSContextRef ctx, JSValueRef value) {
        return GetJSValueList<juce::String>(ctx, value);
    }
    template<>
    static std::vector<double> GetJSValue<std::vector<double>>(JSContextRef ctx, JSValueRef value) {
        return GetJSValueList<double>(ctx, value);
    }
    template<>
    static std::vector<float> GetJSValue<std::vector<float>>(JSContextRef ctx, JSValueRef value) {
        return GetJSValueList<float>(ctx, value);
    }
    template<>
    static std::vector<int> GetJSValue<std::vector<int>>(JSContextRef ctx, JSValueRef value) {
        return GetJSValueList<int>(ctx, value);
    }
    // Generic list
    template<typename T>
    static std::vector<T> GetJSValueList(JSContextRef ctx, JSValueRef value) {
        std::vector<T> list;
        JSObjectRef jsArray = JSValueToObject(ctx, value, nullptr);
        if (jsArray != nullptr) {
            auto length = static_cast<size_t>(JSValueToNumber(ctx, JSObjectGetProperty(ctx, jsArray,
                                                                                         JSStringCreateWithUTF8CString(
                                                                                                 "length"), nullptr),
                                                                nullptr));
            for (size_t i = 0; i < length; ++i) {
                JSValueRef jsValue = JSObjectGetPropertyAtIndex(ctx, jsArray, static_cast<uint32_t>(i), nullptr);
                list.push_back(GetJSValue<T>(ctx, jsValue));
            }
        } else {
            DBG("JSInterop::GetJSValueList: Value is not an array.");
        }
        return list;
    }


    // ====================================================================
    // Helper functions for resolving multiple arguments of different types in JS->C++ callbacks
    // ====================================================================
    template<typename... Args>
    static std::tuple<Args...> GetConvertedArguments(JSContextRef ctx, const JSValueRef arguments[], size_t argumentCount, std::tuple<Args...>& tuple) {
        return GetConvertedArgumentsHelper<0, Args...>(ctx, arguments, argumentCount, tuple);
    }

    template<size_t Index, typename... Args>
    typename std::enable_if<Index == sizeof...(Args), std::tuple<Args...>>::type
    static GetConvertedArgumentsHelper(JSContextRef ctx, const JSValueRef[], size_t, std::tuple<Args...>& tuple) {
        return tuple;
    }

    template<const size_t Index, typename... Args>
    typename std::enable_if<Index < sizeof...(Args), std::tuple<Args...>>::type
    static GetConvertedArgumentsHelper(JSContextRef ctx, const JSValueRef arguments[], size_t argumentCount, std::tuple<Args...>& tuple) {
        auto value = GetJSValue<typename std::tuple_element<Index, std::tuple<Args...>>::type>(ctx, arguments[Index]);
        std::get<Index>(tuple) = value;

        return GetConvertedArgumentsHelper<Index + 1, Args...>(ctx, arguments, argumentCount, tuple);
    }

    /// \brief Gets the instance pointer from the JS object
    static JSInteropBase* GetInstance(JSContextRef ctx) {
        JSObjectRef globalObj = JSContextGetGlobalObject(ctx);
        JSStringRef name = JSStringCreateWithUTF8CString("JSInterop");
        JSValueRef jsObj = JSObjectGetProperty(ctx, globalObj, name, nullptr);
        auto jsInteropJSObjRef = JSValueToObject(ctx, jsObj, nullptr);
        JSStringRelease(name);
        auto* instance = static_cast<JSInteropBase*>(JSObjectGetPrivate(jsInteropJSObjRef));

        if(instance == nullptr) {
            DBG("JSInterop::GetInstance: Failed to get instance.");
            return nullptr;
        }
        return instance;
    }

    // ================================== FIELDS ==================================
    // The view that we want to interact with
    ultralight::View& view;
    // Reference to the JUCE AudioProcessorValueTreeState and its listener
    juce::AudioProcessorValueTreeState& audioParams;
    juce::AudioProcessorValueTreeState::Listener& parent;


};
#endif //ULTRALIGHTJUCE_JSINTEROP_H
