//
// Created by Max on 24/05/2023.
//
#pragma once

#include "ULHelper.h"
#ifndef ULTRALIGHTJUCE_INSPECTORMODALWINDOW_H
#define ULTRALIGHTJUCE_INSPECTORMODALWINDOW_H

#include <codecvt>
#include <string>
#include <regex>
#include <JuceHeader.h>
#include <Ultralight/Ultralight.h>

#include "Ultralight/KeyEvent.h"
#include "Ultralight/String.h"
#include "GUIMainComponent.h"

class ImageComponent : public juce::Component
{
public:
    ImageComponent(juce::Image& imageRef, ultralight::RefPtr<ultralight::View>& inspectorViewIn, double& scale) :
    image(imageRef), inspectorView(inspectorViewIn), JUCE_SCALE(scale)
    {
        // Set the size of the component based on the image size
        setSize(inspectorView->width(), inspectorView->height());
    }

    // Same as in GUIMainComponent.h, see there for more details
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
            image = ULHelper::CopyPixelsToTexture(pixels, width, height, stride);
            bitmap->UnlockPixels();
            // Clear the dirty bounds.
            surface->ClearDirtyBounds();
        }

        // Draw the image onto the component
        g.drawImage(image,
                    0, 0, getWidth(), getHeight(),
                    0, 0, static_cast<int>(getWidth() * JUCE_SCALE), static_cast<int>(getHeight() * JUCE_SCALE));

    }

    void mouseMove(const juce::MouseEvent& event) override
    {
//        DBG("Inspector Mouse moved: " << event.x << ", " << event.y);
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
//        DBG("Inspector Mouse down: " << event.x << ", " << event.y);
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
//        DBG("Inspector Mouse drag: " << event.x << ", " << event.y);
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
//        DBG("Inspector Mouse up: " << event.x << ", " << event.y);
        ultralight::MouseEvent evt{};
        evt.type = ultralight::MouseEvent::kType_MouseUp;
        evt.x = event.x;
        evt.y = event.y;
        evt.button = event.mods.isLeftButtonDown() ? ultralight::MouseEvent::kButton_Left : ultralight::MouseEvent::kButton_Right;
        inspectorView->FireMouseEvent(evt);
        repaint();
    }

    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override
    {
        // Handle the scroll event here
        const float scrollDeltaX = wheel.deltaX;
        const float scrollDeltaY = wheel.deltaY;

        // Perform actions based on the scroll direction and amount
        ultralight::ScrollEvent evt{};
        evt.type = ultralight::ScrollEvent::kType_ScrollByPixel;
        // Scroll Y
        evt.delta_x = static_cast<int>(scrollDeltaX * 100.0);
        evt.delta_y = static_cast<int>(scrollDeltaY * 1000.0);
//        DBG("Inspector Mouse wheel x: " << evt.delta_x << ", y:" << evt.delta_y);
        inspectorView->FireScrollEvent(evt);
    }

private:
    juce::Image& image;
    ultralight::RefPtr<ultralight::View>& inspectorView;
    double& JUCE_SCALE;
};

class InspectorModalWindow :
        public juce::DocumentWindow,
        public juce::KeyListener {
public:
    InspectorModalWindow(ultralight::RefPtr<ultralight::View>& inspectorViewIn, juce::Image& imageRef, double& scale)
    : juce::DocumentWindow("Inspector", juce::Colours::white, TitleBarButtons::allButtons, true),
    JUCE_SCALE(scale),
    inspectorView(inspectorViewIn)
    {
        // Create the image component
        imageComponent = std::make_unique<ImageComponent>(imageRef, inspectorViewIn, scale);

        // Set the content component of the modal window
        setContentOwned(imageComponent.get(), false);
        setUsingNativeTitleBar(true);
        setOpaque(true);

        // Set the size of the modal window
        setSize(1024, 768);

        setDraggable(true);
        setResizable(true, true);
        // Show the modal window
        setVisible(true);
        setWantsKeyboardFocus(true);
        addKeyListener(this);
    }

    void resized() override
    {
        auto w = getWidth();
        auto h = getHeight();
        // Update JUCE modal window size
        setSize(w, h);

        if (inspectorView.get() != nullptr && imageComponent != nullptr){
            // Update Ultralight view size
            inspectorView->Resize(static_cast<uint32_t>(w * JUCE_SCALE), static_cast<uint32_t>(h * JUCE_SCALE));
            inspectorView->Focus();
            // Update JUCE image component size
            imageComponent->setSize(static_cast<int>(w * JUCE_SCALE), static_cast<int>(h * JUCE_SCALE));
        }
    }

    void activeWindowStatusChanged() override
    {
        if (isActiveWindow())
        {
            // The main window has gained focus
            // Perform actions when the main window gets focus
            inspectorView->Focus();
            setWantsKeyboardFocus(true);
        }
        else
        {
            // The main window has lost focus
            // Perform actions when the main window loses focus
        }
    }

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override
    {
        // Check if key character contains only single letter or number
        std::regex regexPattern(R"([A-Za-z0-9`\-=[\\\];',./~!@#$%^&*()_+{}|:"<>?\u0020])");
        // Convert from wchar_t to string
        auto wstr = std::wstring(1, key.getTextCharacter()).c_str();
        std::string str = converter.to_bytes(wstr);

        ultralight::KeyEvent evt;
        if(std::regex_match(str, regexPattern)){
            // Single character
            evt.type = ultralight::KeyEvent::kType_Char;
            ultralight::String keyStr(str.c_str());
            evt.text = keyStr;
            evt.native_key_code = 0;
            evt.modifiers = 0;
            evt.unmodified_text = evt.text; // If not available, set to same as evt.text
        } else {
            // Special keys
            evt.type = ultralight::KeyEvent::kType_RawKeyDown;
            evt.virtual_key_code = mapJUCEKeyCodeToUltralight(key.getKeyCode());
            evt.native_key_code = 0;
            evt.modifiers = 0;
            GetKeyIdentifierFromVirtualKeyCode(evt.virtual_key_code, evt.key_identifier);
        }

        inspectorView->FireKeyEvent(evt);

        return true; // Indicate that the key press is consumed
    }

    void closeButtonPressed() override
    {
        // Close the modal window
        setVisible(false);
    }

    int mapJUCEKeyCodeToUltralight(int juceKeyCode)
    {
        if (juceKeyCode == juce::KeyPress::backspaceKey) return ultralight::KeyCodes::GK_BACK;
        else if (juceKeyCode == juce::KeyPress::tabKey) return ultralight::KeyCodes::GK_TAB;
        else if (juceKeyCode == juce::KeyPress::returnKey) return ultralight::KeyCodes::GK_RETURN;
        else if (juceKeyCode == juce::KeyPress::escapeKey) return ultralight::KeyCodes::GK_ESCAPE;
        else if (juceKeyCode == juce::KeyPress::spaceKey) return ultralight::KeyCodes::GK_SPACE;
        else if (juceKeyCode == juce::KeyPress::deleteKey) return ultralight::KeyCodes::GK_DELETE;
        else if (juceKeyCode == juce::KeyPress::homeKey) return ultralight::KeyCodes::GK_HOME;
        else if (juceKeyCode == juce::KeyPress::endKey) return ultralight::KeyCodes::GK_END;
        else if (juceKeyCode == juce::KeyPress::pageUpKey) return ultralight::KeyCodes::GK_PRIOR;
        else if (juceKeyCode == juce::KeyPress::pageDownKey) return ultralight::KeyCodes::GK_NEXT;
        else if (juceKeyCode == juce::KeyPress::leftKey) return ultralight::KeyCodes::GK_LEFT;
        else if (juceKeyCode == juce::KeyPress::rightKey) return ultralight::KeyCodes::GK_RIGHT;
        else if (juceKeyCode == juce::KeyPress::upKey) return ultralight::KeyCodes::GK_UP;
        else if (juceKeyCode == juce::KeyPress::downKey) return ultralight::KeyCodes::GK_DOWN;
        else if (juceKeyCode == juce::KeyPress::F1Key) return ultralight::KeyCodes::GK_F1;
        else if (juceKeyCode == juce::KeyPress::F2Key) return ultralight::KeyCodes::GK_F2;
        else if (juceKeyCode == juce::KeyPress::F3Key) return ultralight::KeyCodes::GK_F3;
        else if (juceKeyCode == juce::KeyPress::F4Key) return ultralight::KeyCodes::GK_F4;
        else if (juceKeyCode == juce::KeyPress::F5Key) return ultralight::KeyCodes::GK_F5;
        else if (juceKeyCode == juce::KeyPress::F6Key) return ultralight::KeyCodes::GK_F6;
        else if (juceKeyCode == juce::KeyPress::F7Key) return ultralight::KeyCodes::GK_F7;
        else if (juceKeyCode == juce::KeyPress::F8Key) return ultralight::KeyCodes::GK_F8;
        else if (juceKeyCode == juce::KeyPress::F9Key) return ultralight::KeyCodes::GK_F9;
        else if (juceKeyCode == juce::KeyPress::F10Key) return ultralight::KeyCodes::GK_F10;
        else if (juceKeyCode == juce::KeyPress::F11Key) return ultralight::KeyCodes::GK_F11;
        else if (juceKeyCode == juce::KeyPress::F12Key) return ultralight::KeyCodes::GK_F12;
            // Add more if-else conditions for other JUCE keycodes as needed
        else return -1; // Return -1 if there's no matching Ultralight keycode
    }

private:
    std::unique_ptr<ImageComponent> imageComponent;
    ultralight::RefPtr<ultralight::View>& inspectorView;
    double& JUCE_SCALE;

    // For string conversions
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

};
#endif //ULTRALIGHTJUCE_INSPECTORMODALWINDOW_H
