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
#include "Strings/String_Utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "Vendor/stb_image.h"

namespace ImageLoader
{
	//-------------------------------------------------------------------------
	// Purpose: The entrypoint for ImageLoader jobs.
	// 
	// Input:	pParams - An opaque pointer to be cast to an ImageLoaderParams_t struct.
	//			pJobResults - An opaque pointer that will hold the results of this job. 
	//				(In this case, the results are held in the same struct as the params).
	// Output:	i32 - 0 indicates the job failed, 1 indicates the job completed successfully.
	//-------------------------------------------------------------------------
	i32 LoadImage_Threaded(void* pParams, void** pJobResults)
	{
		if (!pParams)
		{
			LOG_ERROR("LoadImage_Threaded was provided with no parameters. Image load failed.");
			return 0;
		}

		// Job results for this job are the same as the params.
		*pJobResults = pParams;

		ImageLoaderParams_t* pTypedParams = reinterpret_cast<ImageLoaderParams_t*>(pParams);

		if (!LoadImage(pTypedParams->pszImageName, *pTypedParams->pOutResource))
		{
			return 0;
		}

		return 1;
	}

	//-------------------------------------------------------------------------
	// Purpose: Attempts to load an existing image asset from disk.
	// 
	// Input:	pszImagePath - The path to the image to be loaded.
	//			pOutResource - The loaded resource data.
	// Output:	bool - Returns true on successful resource acquisition, otherwise false.
	//-------------------------------------------------------------------------
	bool LoadImage(const char* pszImagePath, ImageResource_t& pOutResource)
	{
		const i32 kRequiredChannelCount = 4;
		stbi_set_flip_vertically_on_load_thread(true);
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

		u64 imageDataSize = FileSystem::FileSize(&tempFileHandle);
		u64 bytesRead{ 0 };
		Utl::CVector<byte> rawImageData(imageDataSize);
		FileSystem::ReadWholeBinaryFile(rawImageData.Data(), imageDataSize, &bytesRead, &tempFileHandle);
		
		if (bytesRead != imageDataSize)
		{
			LOG_WARN("Data size mismatch on reading image '%s'. Expected %d bytes, read %d bytes. Image data loss is likely.", pszImagePath, imageDataSize, bytesRead);
		}

		FileSystem::CloseFile(&tempFileHandle);

		// Load the image data into our buffer. (This must be freed by the caller).
		// NOTE: We are forcing each image to be loaded as 4 channels because support for R8G8B8 images on some hardware is dodgy, whereas R8G8B8A8 is seemingly universal.
		pOutResource.pPixelData = stbi_load_from_memory(rawImageData.Data(), (i32)bytesRead, &pOutResource.Width, &pOutResource.Height, (i32*)&pOutResource.NumChannels, kRequiredChannelCount);

		// Check the image for transparency.
		// We (currently) only care about this if there is an alpha channel.
		if (pOutResource.NumChannels == 4)
		{
			byte* pCurrentPixel = pOutResource.pPixelData;

			for (i32 x = 0; x < pOutResource.Width; x++)
			{
				// If we're already marked as transparent, break out now.
				if (pOutResource.bIsTransparent)
					break;

				for (i32 y = 0; y < pOutResource.Height; y++)
				{
					byte alpha = pCurrentPixel[3];
					if (alpha < 255)
					{
						// The texture is transparent, mark it as such.
						pOutResource.bIsTransparent = true;
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

	//-------------------------------------------------------------------------
	// Purpose: Releases resources allocated during image resource acquisition.
	// 
	// Input:	pResource - Contains the resources to be released.
	//-------------------------------------------------------------------------
	void ReleaseImageResource(ImageResource_t& pResource)
	{
		// For now, all we care about is freeing the pixel array.
		if (pResource.pPixelData)
		{
			stbi_image_free(pResource.pPixelData);
			pResource.pPixelData = nullptr;
		}
	}
}