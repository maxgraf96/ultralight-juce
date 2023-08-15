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
            uint32_t stride) {
        // Create a JUCE Image with the same dimensions as the Ultralight BitmapSurface
        juce::Image image(juce::Image::ARGB, static_cast<int>(width), static_cast<int>(height), false);
        // Create a BitmapData object to access the raw pixels of the JUCE Image
        juce::Image::BitmapData bitmapData(image, 0, 0, static_cast<int>(width), static_cast<int>(height),
            juce::Image::BitmapData::writeOnly);
        // Set the pixel format to ARGB (same as Ultralight)
        bitmapData.pixelFormat = juce::Image::ARGB;

        // Normal case: the stride is the same as the width * 4 (4 bytes per pixel)
        // In this case, we can just memcpy the whole image
        if (width * 4 == stride) {
            std::memcpy(bitmapData.data, pixels, stride * height);
        }
        // Special case: the stride is different from the width * 4
        // In this case, we need to copy the image line by line
        // The reason for this is that in some cases, the stride is not the same as the width * 4,
        // for example when the JUCE window width is uneven (e.g. 1001px)
        else {
            for (uint32_t y = 0; y < height; ++y)
                std::memcpy(bitmapData.getLinePointer(static_cast<int>(y)), static_cast<uint8_t*>(pixels) + y * stride, width * 4);
        }

        return image;
    }
};

#endif //ULTRALIGHTJUCE_ULHELPER_H
