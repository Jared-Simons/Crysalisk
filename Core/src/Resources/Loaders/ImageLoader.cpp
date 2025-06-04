//=============================================================================
// Author: Jared Simons
// Date Created: 5/18/2025
// 
// Purpose: Image resource loading utilities.
//=============================================================================
#include "ImageLoader.h"

#include "Containers/CVector.h"
#include "Core/Logging.h"
#include "Platform/FileSystem.h"

#define STB_IMAGE_IMPLEMENTATION
#include "Vendor/stb_image.h"

namespace ImageLoader
{
	// @Multithreading: This function is a prime candidate for threading.
	bool LoadImage(const char* pszImagePath, ImageResource_t& pOutResource)
	{
		const int kRequiredChannelCount = 4;
		stbi_set_flip_vertically_on_load(true);
		if (!FileSystem::FileExists(pszImagePath))
		{
			LOG_ERROR("Image '%s' could not be found. Image load failed.", pszImagePath);
			return false;
		}

		// Get the size of the image data to be loaded.
		FileHandle_t tempFileHandle{};
		if (!FileSystem::OpenFile(pszImagePath, eFileMode::FILEMODE_READ, true, &tempFileHandle))
		{
			return false;
		}

		size_t imageDataSize = FileSystem::FileSize(&tempFileHandle);
		size_t bytesRead{ 0 };
		Utl::CVector<byte> rawImageData(imageDataSize);
		FileSystem::ReadWholeBinaryFile(rawImageData.Data(), imageDataSize, &bytesRead, &tempFileHandle);
		
		if (bytesRead != imageDataSize)
		{
			LOG_WARN("Data size mismatch on reading image '%s'. Expected %d bytes, read %d bytes. Image data loss is likely.", pszImagePath, imageDataSize, bytesRead);
		}

		FileSystem::CloseFile(&tempFileHandle);

		// Load the image data into our buffer. (This must be freed by the caller).
		// NOTE: We are forcing each image to be loaded as 4 channels because support for R8G8B8 images on some hardware is dodgy, whereas R8G8B8A8 is seemingly universal.
		pOutResource.pPixelData = stbi_load_from_memory(rawImageData.Data(), bytesRead, &pOutResource.Width, &pOutResource.Height, (int*)&pOutResource.NumChannels, kRequiredChannelCount);

		// Check the image for transparency.
		// We (currently) only care about this if there is an alpha channel.
		if (pOutResource.NumChannels == 4)
		{
			byte* pCurrentPixel = pOutResource.pPixelData;

			for (int x = 0; x < pOutResource.Width; x++)
			{
				// If we're already marked as transparent, break out now.
				if (pOutResource.bIsTransparent)
					break;

				for (int y = 0; y < pOutResource.Height; y++)
				{
					byte alpha = pCurrentPixel[3];
					if (alpha < 255)
					{
						// The texture is transparent, mark it as such.
						pOutResource.bIsTransparent;
						break;
					}

					pCurrentPixel += 4; // We are currently assuming that each image with alpha channels are 4 bytes per pixel.
				}
			}
		}

		// Force this to be 4, even if we don't have 4 channels, otherwise the image data won't be properly rendered.
		pOutResource.NumChannels = kRequiredChannelCount;

		return true;
	}

	void ReleaseImageResource(ImageResource_t& pResource)
	{
		// For now, all we care about is freeing the pixel array.
		if (pResource.pPixelData)
		{
			stbi_image_free(pResource.pPixelData);
		}
	}
}