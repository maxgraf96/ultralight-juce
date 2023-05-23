//
// Created by Max on 22/05/2023.
//

#ifndef ULTRALIGHTJUCE_GUIMAINCOMPONENT_H
#define ULTRALIGHTJUCE_GUIMAINCOMPONENT_H

#include <memory>
#include <string>

#include <JuceHeader.h>
#include <juce_opengl/juce_opengl.h>
#include <Ultralight/Ultralight.h>
#include <AppCore/Platform.h>
#include <readerwriterqueue.h>

#include "FileWatcher.hpp"
#include "Ultralight/RefPtr.h"

using namespace ultralight;

static int WIDTH = 1024;
static int HEIGHT = 700;

class GUIMainComponent :
        public juce::Component,
        public juce::Timer {
public:

    GUIMainComponent() {
        setSize(WIDTH, HEIGHT);

        auto scale = juce::Desktop::getInstance().getDisplays().displays[0].scale;
        auto dpi = juce::Desktop::getInstance().getDisplays().displays[0].dpi;
        JUCE_SCALE = scale;
        DBG("Current DPI: " << dpi << " Scale: " << scale);

        // ================================== ULTRALIGHT ==================================
        Config config;

        // We need to tell config where our resources are so it can
        // load our bundled SSL certificates to make HTTPS requests.
        config.resource_path = "./Resources/";

        // The GPU renderer should be disabled to render Views to a
        // pixel-buffer (Surface).
        config.use_gpu_renderer = false;

        // You can set a custom DPI scale here. Default is 1.0 (100%)
        config.device_scale = JUCE_SCALE;

        // Pass our configuration to the Platform singleton so that
        // the library can use it.
        Platform::instance().set_config(config);

        // Use the OS's native font loader
        Platform::instance().set_font_loader(GetPlatformFontLoader());

        // Use the OS's native file loader, with a base directory of "."
        // All file:// URLs will load relative to this base directory.
        Platform::instance().set_file_system(GetPlatformFileSystem("/Users/max/CLionProjects/ultralight-juce/Resources"));

        // Use the default logger (writes to a log file)
        Platform::instance().set_logger(GetDefaultLogger("ultralight.log"));

        // Create our Renderer (call this only once per application).
        // The Renderer singleton maintains the lifetime of the library
        // and is required before creating any Views.
        // You should set up the Platform handlers before this.
        renderer = Renderer::Create();

        // Create an HTML view that is WIDTH x HEIGHT
        view = renderer->CreateView(WIDTH * JUCE_SCALE, HEIGHT * JUCE_SCALE, true, nullptr);

        // Load a raw string of HTML.
        // Load text from html file
        juce::String filePath = "/Users/max/CLionProjects/ultralight-juce/Resources/index.html";
        juce::File htmlFile = juce::File(filePath);
        juce::String htmlText = htmlFile.loadFileAsString();
//        view->LoadHTML(htmlText.toStdString().c_str());
        view->LoadURL("file:///index.html");

        // Notify the View it has input focus (updates appearance).
        view->Focus();

        fileWatcher = std::make_unique<FileWatcher>("/Users/max/CLionProjects/ultralight-juce/Resources");

        fileWatcher->AddCallback("index.html", [this](const std::string& filename) {
            DBG("File changed: " << filename);
            queue.enqueue(filename);
        });

        fileWatcher->Start();

        startTimerHz(30);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);

        std::string out;
        while(queue.try_dequeue(out)){
            view->Reload();
        }

        // Render all active Views (this updates the Surface for each View).
        renderer->Update();
        renderer->Render();

        // Get the Surface as a BitmapSurface (the default implementation).
        BitmapSurface* surface = (BitmapSurface*)(view->surface());

        // Check if our Surface is dirty (pixels have changed).
        if (!surface->dirty_bounds().IsEmpty()){
            // Get the pixel-buffer Surface for a View.
            RefPtr<Bitmap> bitmap = surface->bitmap();
            // Lock the Bitmap to retrieve the raw pixels.
            // The format is BGRA, 8-bpp, premultiplied alpha.
            void* pixels = bitmap->LockPixels();
            // Get the bitmap dimensions.
            uint32_t width = bitmap->width();
            uint32_t height = bitmap->height();
            uint32_t stride = bitmap->row_bytes();

            image = CopyPixelsToTexture(pixels, width, height, stride);
            bitmap->UnlockPixels();

            // Clear the dirty bounds.
            surface->ClearDirtyBounds();
        }

        g.drawImage(image,
                    0, 0, WIDTH, HEIGHT,
                    0, 0, static_cast<int>(WIDTH * JUCE_SCALE), static_cast<int>(HEIGHT * JUCE_SCALE));
    }

    void mouseMove(const juce::MouseEvent& event) override
    {
        DBG("Mouse moved: " << event.x << ", " << event.y);
        MouseEvent evt;
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
        MouseEvent evt;
        evt.type = MouseEvent::kType_MouseDown;
        evt.x = event.x;
        evt.y = event.y;
        evt.button = MouseEvent::kButton_Left;

        view->FireMouseEvent(evt);

        repaint();
    }

    void resized() override
    {
        // Resize the View to the new size of the window.
//        if(view)
//            view->Resize(static_cast<uint32_t>(getWidth()), static_cast<uint32_t>(getHeight()));
    }

    juce::Image CopyPixelsToTexture(void* pixels, uint32_t width, uint32_t height, uint32_t stride)
    {
        juce::Image image(juce::Image::ARGB, width, height, false);
        juce::Image::BitmapData bitmapData(image, 0, 0, width, height, juce::Image::BitmapData::writeOnly);
        bitmapData.pixelFormat = juce::Image::ARGB;

        std::memcpy(bitmapData.data, pixels, stride * height);

//        // Save image to PNG file
//        juce::File outputFile("/Users/max/Desktop/test.png");
//        // Delete the output file if it already exists
//        outputFile.deleteFile();
//        // Create a FileOutputStream for the output file
//        juce::FileOutputStream outputStream(outputFile);
//        // Check if the FileOutputStream was successfully created
//        if (outputStream.openedOk())
//        {
//            // Export the image to PNG
//            juce::PNGImageFormat pngFormat;
//            pngFormat.writeImageToStream(image, outputStream);
//            // Close the output stream
//            outputStream.flush();
//        }

        return image;
    }

    void timerCallback() override
    {
        repaint();
    }

    RefPtr<Renderer> renderer;
    RefPtr<View> view;
    std::unique_ptr<FileWatcher> fileWatcher;
    double JUCE_SCALE = 0;

    juce::Image image;

    moodycamel::ReaderWriterQueue<std::string> queue;

};


#endif //ULTRALIGHTJUCE_GUIMAINCOMPONENT_H
