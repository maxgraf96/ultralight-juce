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

        // Listen to APVTS changes
        audioParams.addParameterListener("gain", this);

        auto scale = juce::Desktop::getInstance().getDisplays().displays[0].scale;
        JUCE_SCALE = scale;
        DBG("Current monitor scale: " << scale);

        // ================================== ULTRALIGHT ==================================
        // Create an HTML view that is WIDTH x HEIGHT
        view = AudioPluginAudioProcessor::RENDERER->CreateView(WIDTH * JUCE_SCALE, HEIGHT * JUCE_SCALE, true, nullptr);
        inspectorView = view->inspector();
        inspectorView->Resize(WIDTH * JUCE_SCALE, 500);

        // Set up JS interop for view
        jsInterop = std::make_unique<JSInteropExample>(*view, audioParams, *this);
        // Tell ultralight that for this view, we want to use this JSInteropExample instance to handle the interop
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

        // Listen to keyboard presses
        addKeyListener(this);
        setWantsKeyboardFocus(true);

        // Start timer to periodically redraw GUI
        startTimerHz(60);
    }

    void paint(juce::Graphics &g) override {
        g.fillAll(juce::Colours::black);

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
            // The format is BGRA, 8-bpp, premultiplied alpha.
            void *pixels = bitmap->LockPixels();
            // Get the bitmap dimensions.
            uint32_t width = bitmap->width();
            uint32_t height = bitmap->height();
            uint32_t stride = bitmap->row_bytes();
            // Copy the raw pixels into a JUCE Image.
            image = CopyPixelsToTexture(pixels, width, height, stride);

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
//            DBG("JUCE WIDTH: " << WIDTH << " HEIGHT: " << HEIGHT << " IMAGE WIDTH: " << (int) width << " HEIGHT: " << (int) height << " STRIDE: " << (int) stride);
        }

        // Draw the image we just got from Ultralight to the screen.
        g.drawImage(image,
                    0, 0, WIDTH, HEIGHT,
                    0, 0, static_cast<int>(WIDTH * JUCE_SCALE), static_cast<int>(HEIGHT * JUCE_SCALE));
    }

    void resized() override {
        setSize(getParentWidth(), getParentHeight());

        // Resize the View to the new size of the window.
        if (view.get()) {
            WIDTH = getParentWidth();
            HEIGHT = getParentHeight();
            view->Resize(static_cast<uint32_t>(WIDTH * JUCE_SCALE), static_cast<uint32_t>(HEIGHT * JUCE_SCALE));
            view->Focus();
        }
    }

    // ================================== Mouse events ==================================
    void mouseMove(const juce::MouseEvent &event) override {
        DBG("Mouse moved: " << event.x << ", " << event.y);
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
        DBG("Mouse drag: " << event.x << ", " << event.y);
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

    static juce::Image CopyPixelsToTexture(
            void *pixels,
            uint32_t width,
            uint32_t height,
            uint32_t stride) {
        juce::Image image(juce::Image::ARGB, static_cast<int>(width), static_cast<int>(height), false);
        juce::Image::BitmapData bitmapData(image, 0, 0, static_cast<int>(width), static_cast<int>(height),
                                           juce::Image::BitmapData::writeOnly);
        bitmapData.pixelFormat = juce::Image::ARGB;

        // Normal case: the stride is the same as the width * 4 (4 bytes per pixel)
        // In this case, we can just memcpy the image
        if (width * 4 == stride){
            std::memcpy(bitmapData.data, pixels, stride * height);
        }
        // Special case: the stride is different from the width * 4
        // In this case, we need to copy the image line by line
        // The reason for this special case is that in some cases, the stride is not the same as the width * 4,
        // for example when the JUCE window width is uneven (e.g. 1001px)
        else{
            for (uint32_t y = 0; y < height; ++y)
                std::memcpy(bitmapData.getLinePointer(static_cast<int>(y)), static_cast<uint8_t *>(pixels) + y * stride, width * 4);
        }

        return image;
    }

    void timerCallback() override {
        repaint();
        if (inspectorModalWindow != nullptr && inspectorModalWindow->isActiveWindow()) {
            inspectorModalWindow->repaint();
        }
    }

    void parameterChanged(const juce::String &parameterID, float newValue) override {
        if (!view.get())
            return;

        // Get the APVTS as XML string
        // Important: execute on the JUCE Message thread
        juce::MessageManager::callAsync([this]() {
            juce::String xml = audioParams.copyState().createXml()->toString();
            jsInterop->InvokeMethod("APVTSUpdate", xml);
        });
    }

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

    // Helpers
    bool isResizing = false;

    // Scale multiplier from JUCE
    double JUCE_SCALE = 0;
};


#endif //ULTRALIGHTJUCE_GUIMAINCOMPONENT_H
