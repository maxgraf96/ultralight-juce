//
// Created by Max on 22/05/2023.
//

#ifndef ULTRALIGHTJUCE_GUIMAINCOMPONENT_H
#define ULTRALIGHTJUCE_GUIMAINCOMPONENT_H

#include <__utility/pair.h>
#include <memory>
#include <string>

#include <JuceHeader.h>
#include <juce_opengl/juce_opengl.h>
#include <Ultralight/Ultralight.h>
#include <AppCore/Platform.h>
#include <readerwriterqueue.h>
#include <JavaScriptCore/JSRetainPtr.h>

#include "FileWatcher.hpp"
#include "JSInterop.h"
#include "JavaScriptCore/JavaScriptCore.h"
#include "Ultralight/RefPtr.h"
#include "PluginProcessor.h"
#include "InspectorModalWindow.h"

using namespace ultralight;

static int WIDTH = 1024;
static int HEIGHT = 700;
static int INSPECTOR_Y_OFFSET = 500;

class GUIMainComponent :
        public juce::Component,
        public juce::AudioProcessorValueTreeState::Listener,
        public juce::Timer,
        public juce::KeyListener,
        public ultralight::LoadListener {
public:
    GUIMainComponent(juce::AudioProcessorValueTreeState &params) : audioParams(params) {
        // ================================== JUCE ==================================
        // Set component size
        setSize(WIDTH, HEIGHT);

        auto scale = juce::Desktop::getInstance().getDisplays().displays[0].scale;
        auto dpi = juce::Desktop::getInstance().getDisplays().displays[0].dpi;
        JUCE_SCALE = scale;
        DBG("Current DPI: " << dpi << " Scale: " << scale);

        // ================================== ULTRALIGHT ==================================
        // Create an HTML view that is WIDTH x HEIGHT
        view = AudioPluginAudioProcessor::RENDERER->CreateView(WIDTH * JUCE_SCALE, HEIGHT * JUCE_SCALE, true, nullptr);
        inspectorView = view->inspector();
        inspectorView->Resize(WIDTH * JUCE_SCALE, 500);

        // Set up JS interop for view
        jsInterop = std::make_unique<JSInterop>(*view, audioParams);

        view->set_load_listener(this);
        view->set_load_listener(jsInterop.get());

        // Load html file from URL - relative to resources folder set in PluginProcessor.cpp createPluginFilter() method
        view->LoadURL("file:///index.html");

        // Notify the View it has input focus (updates appearance)
        view->Focus();
        inspectorView->Focus();

        // ================================== MISCELLANEOUS ==================================
        // Add file watcher to watch for changes to index.html and reload the view
        fileWatcher = std::make_unique<FileWatcher>("/Users/max/CLionProjects/ultralight-juce/Resources");
        fileWatcher->AddCallback("index.html", [this](const std::string &filename) {
            DBG("File changed: " << filename);
            fileWatcherQueue.enqueue(filename);
        });
        fileWatcher->AddCallback("script.js", [this](const std::string &filename) {
            DBG("File changed: " << filename);
            fileWatcherQueue.enqueue(filename);
        });
        fileWatcher->Start();

        // Listen to APVTS changes
        audioParams.addParameterListener("gain", this);

        // Listen to keyboard presses
        addKeyListener(this);
        setWantsKeyboardFocus(true);

        // Start timer to periodically redraw GUI
        startTimerHz(60);
    }

    virtual void OnDOMReady(View *caller,
                            uint64_t frame_id,
                            bool is_main_frame,
                            const String &url) override {
        // Acquire the JS execution context for the current page.
        // This call will lock the execution context for the current
        // thread as long as the Ref<> is alive.
        Ref<JSContext> context = caller->LockJSContext();

        // Get the underlying JSContextRef for use with the
        // JavaScriptCore C API.
        JSContextRef ctx = context.get();

        // Get the ShowMessage function by evaluating a script. We could have
        // also used JSContextGetGlobalObject() and JSObjectGetProperty() to
        // retrieve this from the global window object as well.

        // Create our string of JavaScript, automatically managed by JSRetainPtr
        JSRetainPtr<JSStringRef> str = adopt(
                JSStringCreateWithUTF8CString("ShowMessage"));

        // Evaluate the string "ShowMessage"
        JSValueRef func = JSEvaluateScript(ctx, str.get(), 0, 0, 0, 0);

        // Check if 'func' is actually an Object and not null
        if (JSValueIsObject(ctx, func)) {
            // Cast 'func' to an Object, will return null if typecast failed.
            JSObjectRef funcObj = JSValueToObject(ctx, func, 0);
            // Check if 'funcObj' is a Function and not null
            if (funcObj && JSObjectIsFunction(ctx, funcObj)) {
                // Create a JS string from null-terminated UTF8 C-string, store it
                // in a smart pointer to release it when it goes out of scope.
                JSRetainPtr<JSStringRef> msg = adopt(JSStringCreateWithUTF8CString("Howdy"));
                JSRetainPtr<JSStringRef> msg2 = adopt(JSStringCreateWithUTF8CString("Partner"));
                JSRetainPtr<JSStringRef> msg3 = adopt(JSStringCreateWithUTF8CString("Wassup"));

                // Read /Users/max/Library/SampleCluster/settings.json into a string
                juce::File settingsFile = juce::File("/Users/max/Library/SampleCluster/settings.json");
                juce::String settingsText = settingsFile.loadFileAsString();
                JSRetainPtr<JSStringRef> settings = adopt(JSStringCreateWithUTF8CString(settingsText.toRawUTF8()));

                // Create a JSValueRef from our JS string, store it in a smart
                // pointer to release it when it goes out of scope.
                JSValueRef msgVal = JSValueMakeFromJSONString(ctx, settings.get());
                JSValueRef args[] = {msgVal};

                // Create our list of arguments (we only have one)
//                JSObjectRef array = JSObjectMakeArray(ctx, 0, NULL, NULL);
//                JSObjectSetPropertyAtIndex(ctx, array, 0, JSValueMakeString(ctx, msg.get()), NULL);
//                JSObjectSetPropertyAtIndex(ctx, array, 1, JSValueMakeString(ctx, msg2.get()), NULL);
//                JSObjectSetPropertyAtIndex(ctx, array, 2, JSValueMakeString(ctx, msg3.get()), NULL);
//                JSValueRef args[] = { array };

                // Count the number of arguments in the array.
                size_t num_args = 1;
                // Create a place to store an exception, if any
                JSValueRef exception = 0;
                // Call the ShowMessage() function with our list of arguments.
                JSValueRef result = JSObjectCallAsFunction(ctx, funcObj, 0, num_args, args, &exception);
                if (exception) {
                    // Handle any exceptions thrown from function here.
                }
                if (result) {
                    // Handle result (if any) here.
                }
            }
        }
    }


    void paint(juce::Graphics &g) override {
        g.fillAll(juce::Colours::black);

        std::string out;
        while (fileWatcherQueue.try_dequeue(out)) {
            // TODO: If multiple views, keep a map of files and their views
            view->Reload();
        }

        // Render all active Views (this updates the Surface for each View).
        AudioPluginAudioProcessor::RENDERER->Update();
        AudioPluginAudioProcessor::RENDERER->Render();

        // Get the Surface as a BitmapSurface (the default implementation).
        auto *surface = (BitmapSurface *) (view->surface());

        // Check if our Surface is dirty (pixels have changed).
        if (!surface->dirty_bounds().IsEmpty()) {
            // Get the pixel-buffer Surface for a View.
            RefPtr<Bitmap> bitmap = surface->bitmap();
            // Lock the Bitmap to retrieve the raw pixels.
            // The format is BGRA, 8-bpp, premultiplied alpha.
            void *pixels = bitmap->LockPixels();
            // Get the bitmap dimensions.
            uint32_t width = bitmap->width();
            uint32_t height = bitmap->height();
            uint32_t stride = bitmap->row_bytes();
            // Copy the raw pixels into a JUCE Image.
            // Always make sure that stride == width * 4
            // TODO find a better solution for this
            // The problem is probably that resize and repaint are called at the same time
            if (width * 4 == stride) {
                image = CopyPixelsToTexture(pixels, width, height, stride);
                if(inspectorModalWindow == nullptr){
                    inspectorModalWindow = std::make_unique<InspectorModalWindow>(inspectorView, inspectorImage, JUCE_SCALE);
                }
            }
            bitmap->UnlockPixels();
            // Clear the dirty bounds.
            if(width * 4 == stride)
                surface->ClearDirtyBounds();
        }

        // Draw the Image to the screen.
        g.drawImage(image,
                    0, 0, WIDTH, HEIGHT,
                    0, 0, static_cast<int>(WIDTH * JUCE_SCALE), static_cast<int>(HEIGHT * JUCE_SCALE));
    }

    void resized() override
    {
        // Resize the View to the new size of the window.
        if(view.get()){
            WIDTH = getParentWidth();
            HEIGHT = getParentHeight();
            view->Resize(static_cast<uint32_t>(WIDTH * JUCE_SCALE), static_cast<uint32_t>(HEIGHT * JUCE_SCALE));
            view->Reload();
            view->Focus();
            inspectorView->Focus();
        }
    }

    // ================================== Mouse events ==================================
    void mouseMove(const juce::MouseEvent& event) override
    {
        DBG("Mouse moved: " << event.x << ", " << event.y);
        MouseEvent evt{};
        evt.type = MouseEvent::kType_MouseMoved;
        evt.x = event.x;
        evt.y = event.y;
        evt.button = MouseEvent::kButton_None;
        view->FireMouseEvent(evt);
        repaint();
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        DBG("Mouse down: " << event.x << ", " << event.y);
        MouseEvent evt{};
        evt.type = MouseEvent::kType_MouseDown;
        evt.x = event.x;
        evt.y = event.y;
        evt.button = event.mods.isLeftButtonDown() ? MouseEvent::kButton_Left : MouseEvent::kButton_Right;
        view->FireMouseEvent(evt);
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        DBG("Mouse drag: " << event.x << ", " << event.y);
        MouseEvent evt{};
        evt.type = MouseEvent::kType_MouseMoved;
        evt.x = event.x;
        evt.y = event.y;
        evt.button = event.mods.isLeftButtonDown() ? MouseEvent::kButton_Left : MouseEvent::kButton_Right;
        view->FireMouseEvent(evt);
        repaint();
    }

    void mouseUp(const juce::MouseEvent& event) override
    {
        DBG("Mouse up: " << event.x << ", " << event.y);
        MouseEvent evt{};
        evt.type = MouseEvent::kType_MouseUp;
        evt.x = event.x;
        evt.y = event.y;
        evt.button = event.mods.isLeftButtonDown() ? MouseEvent::kButton_Left : MouseEvent::kButton_Right;
        view->FireMouseEvent(evt);
        repaint();
    }

    static juce::Image CopyPixelsToTexture(
            void* pixels,
            uint32_t width,
            uint32_t height,
            uint32_t stride)
    {
        juce::Image image(juce::Image::ARGB, width, height, false);
        juce::Image::BitmapData bitmapData(image, 0, 0, width, height, juce::Image::BitmapData::writeOnly);
        bitmapData.pixelFormat = juce::Image::ARGB;

        if(width * 4 == stride)
            std::memcpy(bitmapData.data, pixels, stride * height);

        // Overlay inspector image
//        juce::Graphics g(image);
//        g.drawImageAt(inspectorImage, 0, 0);

        return image;
    }

    void timerCallback() override
    {
        repaint();
        if(inspectorModalWindow.get() != nullptr && inspectorModalWindow->isActiveWindow()){
            inspectorModalWindow->repaint();
        }
    }

    void parameterChanged (const juce::String& parameterID, float newValue) override {
        if(!view.get())
            return;

        // Gain
        if(parameterID == "gain"){
            // Execute on the main thread
            juce::MessageManager::callAsync([this, newValue](){
                jsInterop->InvokeMethod("GainUpdate", newValue);
            });
        }
    }

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override
    {
        if (key.getTextCharacter() == 'i')
        {
            // "I" key is pressed
            // Hide/show inspector window
            if(inspectorModalWindow.get() == nullptr){
                inspectorModalWindow = std::make_unique<InspectorModalWindow>(inspectorView, inspectorImage, JUCE_SCALE);
            }
            else{
                inspectorModalWindow->setVisible(!inspectorModalWindow->isVisible());
            }
            return true; // Return true to indicate that the key press is consumed
        }

        return false; // Return false to allow the key press to propagate to other components
    }

    ~GUIMainComponent() override {
        // Stop timer -> no more redraws
        stopTimer();
        // Stop the file watcher
        fileWatcher->Stop();
        // Remove the load listener - removing this listener is important to avoid a crash on shutdown
        view->set_load_listener(nullptr);
        // Remove the APVTS parameter listener(s)
        audioParams.removeParameterListener("gain", this);
    }

    // ================================== Fields ==================================
    // APVTS
    juce::AudioProcessorValueTreeState& audioParams;

    // Main view
    RefPtr<View> view;
    RefPtr<View> inspectorView;

    // JS interop
    std::unique_ptr<JSInterop> jsInterop;

    // JUCE Image we render the ultralight UI to
    juce::Image image;
    juce::Image inspectorImage;

    // File watcher fields
    std::unique_ptr<FileWatcher> fileWatcher;
    moodycamel::ReaderWriterQueue<std::string> fileWatcherQueue;

    std::unique_ptr<InspectorModalWindow> inspectorModalWindow;

    // Scale multiplier from JUCE
    double JUCE_SCALE = 0;
};


#endif //ULTRALIGHTJUCE_GUIMAINCOMPONENT_H
