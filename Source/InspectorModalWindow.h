//
// Created by Max on 24/05/2023.
//
#pragma once

#ifndef ULTRALIGHTJUCE_INSPECTORMODALWINDOW_H
#define ULTRALIGHTJUCE_INSPECTORMODALWINDOW_H

#include <JuceHeader.h>
#include <Ultralight/Ultralight.h>

class ImageComponent : public juce::Component
{
public:
    ImageComponent(juce::Image& imageRef, ultralight::RefPtr<ultralight::View>& inspectorViewIn, double& scale) :
    image(imageRef), inspectorView(inspectorViewIn), JUCE_SCALE(scale)
    {
        // Set the size of the component based on the image size
        setSize(image.getWidth(), image.getHeight());
    }

    void paint(juce::Graphics& g) override
    {
        AudioPluginAudioProcessor::RENDERER->Update();
        AudioPluginAudioProcessor::RENDERER->Render();

        auto *surface = (ultralight::BitmapSurface *) (inspectorView->surface());

        if (!surface->dirty_bounds().IsEmpty()) {
            // Get the pixel-buffer Surface for a View.
            ultralight::RefPtr <ultralight::Bitmap> bitmap = surface->bitmap();
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
            if (width * 4 == stride && width * 4 == stride) {
                image = CopyPixelsToTexture(pixels, width, height, stride);
            }
            bitmap->UnlockPixels();
            // Clear the dirty bounds.
            if (width * 4 == stride)
                surface->ClearDirtyBounds();
        }

        // Draw the image onto the component
        g.drawImage(image,
                    0, 0, getWidth(), getHeight(),
                    0, 0, static_cast<int>(getWidth() * JUCE_SCALE), static_cast<int>(getHeight() * JUCE_SCALE));

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

        return image;
    }

    void mouseMove(const juce::MouseEvent& event) override
    {
        DBG("Inspector Mouse moved: " << event.x << ", " << event.y);
        ultralight::MouseEvent evt{};
        evt.type = ultralight::MouseEvent::kType_MouseMoved;
        evt.x = event.x;
        evt.y = event.y;
        evt.button = ultralight::MouseEvent::kButton_None;
        inspectorView->FireMouseEvent(evt);
        repaint();
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        DBG("Inspector Mouse down: " << event.x << ", " << event.y);
        ultralight::MouseEvent evt{};
        evt.type = ultralight::MouseEvent::kType_MouseDown;
        evt.x = event.x;
        evt.y = event.y;
        evt.button = event.mods.isLeftButtonDown() ? ultralight::MouseEvent::kButton_Left : ultralight::MouseEvent::kButton_Right;
        inspectorView->FireMouseEvent(evt);
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        DBG("Inspector Mouse drag: " << event.x << ", " << event.y);
        ultralight::MouseEvent evt{};
        evt.type = ultralight::MouseEvent::kType_MouseMoved;
        evt.x = event.x;
        evt.y = event.y;
        evt.button = event.mods.isLeftButtonDown() ? ultralight::MouseEvent::kButton_Left : ultralight::MouseEvent::kButton_Right;
        inspectorView->FireMouseEvent(evt);
        repaint();
    }

    void mouseUp(const juce::MouseEvent& event) override
    {
        DBG("Inspector Mouse up: " << event.x << ", " << event.y);
        ultralight::MouseEvent evt{};
        evt.type = ultralight::MouseEvent::kType_MouseUp;
        evt.x = event.x;
        evt.y = event.y;
        evt.button = event.mods.isLeftButtonDown() ? ultralight::MouseEvent::kButton_Left : ultralight::MouseEvent::kButton_Right;
        inspectorView->FireMouseEvent(evt);
        repaint();
    }

private:
    juce::Image& image;
    ultralight::RefPtr<ultralight::View>& inspectorView;
    double& JUCE_SCALE;
};

class InspectorModalWindow : public juce::DocumentWindow {
public:
    InspectorModalWindow(ultralight::RefPtr<ultralight::View>& inspectorViewIn, juce::Image& imageRef, double& scale)
    : juce::DocumentWindow("Inspector", juce::Colours::white, TitleBarButtons::allButtons, true),
    inspectorView(inspectorViewIn)
    {
        // Create the image component
        imageComponent = std::make_unique<ImageComponent>(imageRef, inspectorViewIn, scale);

        // Set the content component of the modal window
        setContentOwned(imageComponent.get(), true);
        setUsingNativeTitleBar(true);
        setOpaque(true);

        // Set the size of the modal window
        setSize(imageComponent->getWidth() / scale, imageComponent->getHeight() / scale);

        setDraggable(true);
        setResizable(true, true);
        // Show the modal window
        setVisible(true);
    }

    void activeWindowStatusChanged() override
    {
        if (isActiveWindow())
        {
            // The main window has gained focus
            // Perform actions when the main window gets focus
            inspectorView->Focus();
        }
        else
        {
            // The main window has lost focus
            // Perform actions when the main window loses focus
        }
    }

    void closeButtonPressed() override
    {
        // Close the modal window
        delete this;
    }
private:
    std::unique_ptr<ImageComponent> imageComponent;
    ultralight::RefPtr<ultralight::View>& inspectorView;

};
#endif //ULTRALIGHTJUCE_INSPECTORMODALWINDOW_H
