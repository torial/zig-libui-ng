// libui-ng Haiku backend — uiImage (a set of BBitmap representations).
// libui hands pixels as tightly-or-strided RGBA8; Haiku's B_RGBA32 is BGRA in memory (little-endian),
// so each pixel is byte-swapped on import. uiprivImageBestBitmap() returns a representation for the
// table/area code to draw.
#include <Bitmap.h>
#include "uipriv_haiku.h"

struct uiImage {
	double width, height;
	BList *bitmaps;		// BBitmap*
};

uiImage *uiNewImage(double width, double height)
{
	uiImage *i = uiprivNew(uiImage);
	i->width = width;
	i->height = height;
	i->bitmaps = new BList();
	return i;
}

void uiFreeImage(uiImage *i)
{
	for (int32 j = 0; j < i->bitmaps->CountItems(); j++)
		delete (BBitmap *) i->bitmaps->ItemAt(j);
	delete i->bitmaps;
	uiprivFree(i);
}

void uiImageAppend(uiImage *i, void *pixels, int pixelWidth, int pixelHeight, int byteStride)
{
	BBitmap *bmp = new BBitmap(BRect(0, 0, pixelWidth - 1, pixelHeight - 1), B_RGBA32, false);
	uint8 *src = (uint8 *) pixels;
	uint8 *dst = (uint8 *) bmp->Bits();
	int32 dstStride = bmp->BytesPerRow();
	for (int y = 0; y < pixelHeight; y++) {
		uint8 *s = src + (size_t) y * byteStride;
		uint8 *d = dst + (size_t) y * dstStride;
		for (int x = 0; x < pixelWidth; x++) {
			d[0] = s[2];	// B
			d[1] = s[1];	// G
			d[2] = s[0];	// R
			d[3] = s[3];	// A
			s += 4;
			d += 4;
		}
	}
	i->bitmaps->AddItem(bmp);
}

// Returns the largest representation (good enough; libui would pick by DPI). NULL if none.
BBitmap *uiprivImageBestBitmap(uiImage *i)
{
	BBitmap *best = NULL;
	float bestArea = -1;
	for (int32 j = 0; j < i->bitmaps->CountItems(); j++) {
		BBitmap *b = (BBitmap *) i->bitmaps->ItemAt(j);
		BRect r = b->Bounds();
		float area = r.Width() * r.Height();
		if (area > bestArea) {
			bestArea = area;
			best = b;
		}
	}
	return best;
}
