//
// Created by Max on 05/07/2023.
//

#ifndef ULTRALIGHTJUCE_ULHELPER_H
#define ULTRALIGHTJUCE_ULHELPER_H

#include <JuceHeader.h>
#include <juce_opengl/juce_opengl.h>

class ULHelper {
public:
    /// \brief Copies the raw pixels from the Ultralight rendered BitmapSurface to a JUCE Image
    /// \param pixels The raw pixels from the Ultralight BitmapSurface
    /// \param width The width of the image
    /// \param height The height of the image
    /// \param stride The stride of the image (approximately number of bytes per row, but there are some edge case - see below)
    static juce::Image CopyPixelsToTexture(
            void *pixels,
            uint32_t width,
            uint32_t height,
            uint32_t stride);
};


#endif //ULTRALIGHTJUCE_ULHELPER_H
