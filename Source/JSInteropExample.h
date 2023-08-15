//
// Created by Max on 04/07/2023.
//

#ifndef ULTRALIGHTJUCE_JSINTEROPEXAMPLE_H
#define ULTRALIGHTJUCE_JSINTEROPEXAMPLE_H

#include <JuceHeader.h>

#include "Ultralight/View.h"
#include "JSInteropBase.h"

// This class is an example of how to extend the JSInteropBase class.
// You can either create extensions of the JSInteropBase class for each of your components, or add all your functions
// to the JSInteropBase class. Both have their advantages and disadvantages. Extending the JSInteropBase class
// will produce more code, but also keep your components more modular. Adding all your functions to the JSInteropBase
// class will lead to one big file, which might make it harder to navigate.
// You can have multiple instances of the JSInteropXXX classes for several Views, but afaik not for the same View.
class JSInteropExample : public JSInteropBase {
public:
    JSInteropExample(ultralight::View& inView, juce::AudioProcessorValueTreeState& params, juce::AudioProcessorValueTreeState::Listener& parentComponent)
    : JSInteropBase(inView, params, parentComponent) {

    }

    // Need to override this method to register callbacks and function that will by used in JS
    void OnWindowObjectReady(ultralight::View* caller, uint64_t frame_id, bool is_main_frame, const ultralight::String& url)
    override {
        // ========================================================================================================
        // ESSENTIAL: call OnWindowObjectReady() on the base class (JSInteropBase) first! This will set up the APVTS callbacks
        // ========================================================================================================
        JSInteropBase::OnWindowObjectReady(caller, frame_id, is_main_frame, url);

        // ========================================================================================================
        // JS callbacks in C++. These need to be defined in the OnWindowObjectReady() method.
        // If you try to define them after the DOM is ready, they might not successfully be registered in JS.
        // ========================================================================================================
        // Then, register all your custom functions that are NOT APVTS updates here.
        // E.g., a button click with two arguments:
        // 1) Define callback function
        std::function<void(int, juce::String, std::vector<int>)> buttonClickCallback = [](int argument, juce::String argument2, std::vector<int> argument3) {
            DBG("Button clicked! Received argument: " << argument << " and argument2: " << argument2);
            DBG("Argument3 is a vector of size: " << argument3.size());
            DBG("Items in argument3: ");
            for (int element : argument3) {
                DBG(element);
            }
            // Do something with the arguments
            // ...
        };
        // 2) Register callback function in JS - now call OnMyButtonClick(int, String) from anywhere in your view's JS
        registerCppCallbackInJS("OnMyButtonClick", buttonClickCallback);
    }

    // Need to override this method to use JS functions in C++
    void OnDOMReady(ultralight::View* caller,
        uint64_t frame_id,
        bool is_main_frame,
        const ultralight::String& url) override {
        // call OnDOMReady() on the base class (JSInteropBase) first to propagate the default parameter values loaded from disk
        JSInteropBase::OnDOMReady(caller, frame_id, is_main_frame, url);
        // ========================================================================================================
        // Passing data from C++ to JS. These can be called from anywhere in your C++ code, after the DOM is ready.
        // ========================================================================================================
        // Primitive types
        invokeMethod("myJSFunction", 2, 3);
        // Strings
        invokeMethod("myJSFunction", juce::String("Hello"), juce::String("World"));
        // Arrays
        std::vector<int> myVector = { 1, 2, 3, 4, 5 };
        invokeMethod("myJSFunction", myVector);
        // Mix
        invokeMethod("myJSFunction", 2, juce::String("Hello"), myVector);
    }

};
#endif //ULTRALIGHTJUCE_JSINTEROPEXAMPLE_H
