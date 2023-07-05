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
#include "JSInteropBase.h"
#include "JSInteropExample.h"
#include "JavaScriptCore/JavaScriptCore.h"
#include "ULHelper.h"
#include "Ultralight/RefPtr.h"
#include "PluginProcessor.h"
#include "InspectorModalWindow.h"
#include "ULHelper.h"

using namespace ultralight;

// Initial main window size
static int WIDTH = 1024;
static int HEIGHT = 700;

class GUIMainComponent :
        public juce::Component,
        public juce::AudioProcessorValueTreeState::Listener,
        public juce::Timer,
        public juce::KeyListener,
        public ultralight::LoadListener {
public:
    GUIMainComponent(juce::AudioProcessorValueTreeState &params) : audioParams(params) {
        // ================================== JUCE ========================================
        // Set component size
        setSize(WIDTH, HEIGHT);

        // Listen to APVTS changes
        audioParams.addParameterListener("gain", this);

        // Get the scale parameter of the main monitor.
        // If you use multiple screens with different DPIs, you need to handle the scaling transitions.
        auto scale = juce::Desktop::getInstance().getDisplays().displays[0].scale;
        JUCE_SCALE = scale;
        DBG("Current monitor scale: " << scale);

        // ================================== ULTRALIGHT ==================================
        // Create an HTML view that is WIDTH x HEIGHT
        // 1) JUCE handles scaling itself, Ultralight doesn't. So we need to scale our Views by the monitor scale.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // 2) VERY IMPORTANT: The third parameter of CreateView() is set to true, which means that the background
        // of the View will be transparent. This allows you to overlay the View on top of other JUCE components.
        // If you want an opaque background, set this to false.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        view = AudioPluginAudioProcessor::RENDERER->CreateView(
                static_cast<uint32_t>(WIDTH * JUCE_SCALE),
                static_cast<uint32_t>(HEIGHT * JUCE_SCALE),
                true, // Transparent background
                nullptr);
        // Create JS inspector View
        inspectorView = view->inspector();
        inspectorView->Resize(static_cast<uint32_t>(WIDTH * JUCE_SCALE), 500);

        // Set up JS interop for main View
        jsInterop = std::make_unique<JSInteropExample>(*view, audioParams, *this);
        // Tell ultralight that for this view, we want to use this JSInteropExample instance to handle the interop
        // Look into JSInteropExample.h for more info on JS interop
        view->set_load_listener(jsInterop.get());

        // Load HTML file from URL - this URL is relative to the resources folder set in
        // the createPluginFilter() method in PluginProcessor.cpp
        view->LoadURL("file:///index.html");

        // Notify the View it has input focus (updates appearance)
        view->Focus();

        // ================================== MISCELLANEOUS ==================================
        // Add file watcher to watch for changes to index.html and automatically hot-reload the View when
        // files (e.g., HTML, JS, CSS) in the given folder are changed
        fileWatcher = std::make_unique<FileWatcher>("/Users/max/CLionProjects/ultralight-juce/Resources");
        // Describe which files you want to watch
        fileWatcher->AddCallback("index.html", [this](const std::string &filename) {
            DBG("File changed: " << filename);
            // Adding a filename to this queue will enable hot-reloading when the file is changed
            fileWatcherQueue.enqueue(filename);
        });
        fileWatcher->AddCallback("script.js", [this](const std::string &filename) {
            DBG("File changed: " << filename);
            fileWatcherQueue.enqueue(filename);
        });
        // Start watching the files
        fileWatcher->Start();

        // Listen to keyboard presses
        addKeyListener(this);
        // Allows this window to receive keyboard presses
        setWantsKeyboardFocus(true);

        // Start timer to periodically redraw GUI
        startTimerHz(60);
    }

    /// \brief Paint method called by JUCE
    /// \param g JUCE Graphics context
    /// Here we draw all our JUCE components and Ultralight views
    void paint(juce::Graphics &g) override {
        g.fillAll(juce::Colours::black);

        // ================================== JUCE ========================================
        // This is where you can draw all your juce components (this project currently uses only Ultralight Views)

        // ================================== ULTRALIGHT ==================================
        std::string out;
        while (fileWatcherQueue.try_dequeue(out)) {
            // TODO: If multiple views, keep a map of files and their views
            view->Reload();
        }

        // Update and render all active Ultralight Views (this updates the Surface for each View).
        AudioPluginAudioProcessor::RENDERER->Update();
        AudioPluginAudioProcessor::RENDERER->Render();

        // Get the Surface as a BitmapSurface (the default implementation).
        auto *surface = (BitmapSurface *) (view->surface());

        // Check if our Surface is dirty (pixels have changed).
        if (!surface->dirty_bounds().IsEmpty()) {
            // Get the pixel-buffer Surface for a View.
            RefPtr<Bitmap> bitmap = surface->bitmap();
            // Lock the Bitmap to retrieve the raw pixels.
            // The format is ARGB, 4-bpp, premultiplied alpha.
            void *pixels = bitmap->LockPixels();
            // Get the bitmap dimensions.
            uint32_t width = bitmap->width();
            uint32_t height = bitmap->height();
            uint32_t stride = bitmap->row_bytes();
            // Copy the raw pixels into a JUCE Image.
            image = ULHelper::CopyPixelsToTexture(pixels, width, height, stride);

            // Spawn inspector if it doesn't exist
            if (inspectorModalWindow == nullptr) {
                inspectorModalWindow = std::make_unique<InspectorModalWindow>(inspectorView, inspectorImage,
                                                                              JUCE_SCALE);
                // Hide inspector initially
                inspectorModalWindow->setVisible(false);
            }

            // Unlock the Bitmap and mark the Surface as clean.
            bitmap->UnlockPixels();
            surface->ClearDirtyBounds();
        }

        // Draw the image we just got from Ultralight to the screen.
        g.drawImage(image,
                    0, 0, WIDTH, HEIGHT,
                    0, 0, static_cast<int>(WIDTH * JUCE_SCALE), static_cast<int>(HEIGHT * JUCE_SCALE));
    }

    /// \brief Called when the JUCE window is resized.
    void resized() override {
        // Resize the Component to the new size of the window.
        setSize(getParentWidth(), getParentHeight());

        // ================================== JUCE ========================================
        // This is where you would handle the layout of all your JUCE components
        // (this project currently uses only Ultralight Views)


        // ================================== ULTRALIGHT ==================================
        // Resize the Ultralight View to the new size of the window (if it exists).
        if (view.get()) {
            // Update our window sizes
            WIDTH = getParentWidth();
            HEIGHT = getParentHeight();
            view->Resize(static_cast<uint32_t>(WIDTH * JUCE_SCALE), static_cast<uint32_t>(HEIGHT * JUCE_SCALE));
            view->Focus();
        }
    }

    // ================================== Mouse events ==================================
    void mouseMove(const juce::MouseEvent &event) override {
//        DBG("Mouse moved: " << event.x << ", " << event.y);
        MouseEvent evt{};
        evt.type = MouseEvent::kType_MouseMoved;
        evt.x = event.x;
        evt.y = event.y;
        evt.button = MouseEvent::kButton_None;
        view->FireMouseEvent(evt);
        repaint();
    }

    void mouseDown(const juce::MouseEvent &event) override {
        DBG("Mouse down: " << event.x << ", " << event.y);
        MouseEvent evt{};
        evt.type = MouseEvent::kType_MouseDown;
        evt.x = event.x;
        evt.y = event.y;
        evt.button = event.mods.isLeftButtonDown() ? MouseEvent::kButton_Left : MouseEvent::kButton_Right;
        view->FireMouseEvent(evt);
        repaint();
    }

    void mouseDrag(const juce::MouseEvent &event) override {
//        DBG("Mouse drag: " << event.x << ", " << event.y);
        MouseEvent evt{};
        evt.type = MouseEvent::kType_MouseMoved;
        evt.x = event.x;
        evt.y = event.y;
        evt.button = event.mods.isLeftButtonDown() ? MouseEvent::kButton_Left : MouseEvent::kButton_Right;
        view->FireMouseEvent(evt);
        repaint();
    }

    void mouseUp(const juce::MouseEvent &event) override {
        DBG("Mouse up: " << event.x << ", " << event.y);
        MouseEvent evt{};
        evt.type = MouseEvent::kType_MouseUp;
        evt.x = event.x;
        evt.y = event.y;
        evt.button = event.mods.isLeftButtonDown() ? MouseEvent::kButton_Left : MouseEvent::kButton_Right;
        view->FireMouseEvent(evt);
        repaint();
    }

    /// \brief JUCE Timer callback
    /// We use it to periodically repaint the window and the inspector window if it is open
    void timerCallback() override {
        repaint();
        if (inspectorModalWindow != nullptr && inspectorModalWindow->isActiveWindow()) {
            inspectorModalWindow->repaint();
        }
    }

    /// \brief JUCE AudioProcessorValueTreeState::Listener callback
    /// \param parameterID The ID of the parameter that changed
    /// \param newValue The new value of the parameter
    /// We use it to send the APVTS as XML string to the JS side. That way we don't have to deal with individual
    /// parameters in JS, but can just use the APVTS XML and let JS decide which parameters it wants to use.
    /// Change this method if you want to send individual parameters to JS.
    void parameterChanged(const juce::String &parameterID, float newValue) override {
        if (!view.get())
            return;

        // Get the APVTS as XML string
        // Important: execute on the JUCE Message thread
        juce::MessageManager::callAsync([this]() {
            juce::String xml = audioParams.copyState().createXml()->toString();
            jsInterop->invokeMethod("APVTSUpdate", xml);
        });
    }

    // JUCE Key press event handler
    bool keyPressed(const juce::KeyPress &key, juce::Component *originatingComponent) override {
        if (key.getTextCharacter() == 'i') {
            // "I" key is pressed
            // Hide/show inspector window
            if (inspectorModalWindow == nullptr) {
                inspectorModalWindow = std::make_unique<InspectorModalWindow>(inspectorView, inspectorImage,
                                                                              JUCE_SCALE);
            } else {
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
    juce::AudioProcessorValueTreeState &audioParams;

    // Main view
    RefPtr<View> view;
    RefPtr<View> inspectorView;

    // JS interop
    std::unique_ptr<JSInteropExample> jsInterop;

    // JUCE Image we render the ultralight UI to
    juce::Image image;
    juce::Image inspectorImage;

    // File watcher fields
    std::unique_ptr<FileWatcher> fileWatcher;
    moodycamel::ReaderWriterQueue<std::string> fileWatcherQueue;

    // Inspector window
    std::unique_ptr<InspectorModalWindow> inspectorModalWindow;

    // Scale multiplier from JUCE
    double JUCE_SCALE = 0;
};


#endif //ULTRALIGHTJUCE_GUIMAINCOMPONENT_H
