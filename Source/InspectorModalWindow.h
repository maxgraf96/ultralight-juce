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
    ImageComponent(juce::Image& imageRef) :
    image(imageRef)
    {
        // Set the size of the component based on the image size
        setSize(image.getWidth(), image.getHeight());
    }

    void paint(juce::Graphics& g) override
    {
        // Draw the image onto the component
        g.drawImageAt(image, 0, 0);
    }

    void mouseMove(const juce::MouseEvent& event) override
    {
        juce::Component::mouseMove(event);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        juce::Component::mouseDown(event);
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        juce::Component::mouseDrag(event);
    }

    void mouseUp(const juce::MouseEvent& event) override
    {
        juce::Component::mouseUp(event);
    }

private:
    juce::Image& image;
};

class InspectorModalWindow : public juce::DialogWindow {
public:
    InspectorModalWindow(ultralight::RefPtr<ultralight::View>& inspectorViewIn, juce::Image& imageRef)
    : juce::DialogWindow("Modal Window", juce::Colours::white, true, true),
    inspectorView(inspectorViewIn)
    {
        // Create the image component
        imageComponent = std::make_unique<ImageComponent>(imageRef);

        // Set the content component of the modal window
        setContentOwned(imageComponent.get(), true);

        // Set the size of the modal window
        setSize(imageComponent->getWidth(), imageComponent->getHeight());

        setDraggable(true);
        // Show the modal window
        setVisible(true);
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

    void closeButtonPressed() override
    {
        // Close the modal window
        exitModalState(0);
    }
private:
    std::unique_ptr<ImageComponent> imageComponent;
    ultralight::RefPtr<ultralight::View>& inspectorView;

};
#endif //ULTRALIGHTJUCE_INSPECTORMODALWINDOW_H
