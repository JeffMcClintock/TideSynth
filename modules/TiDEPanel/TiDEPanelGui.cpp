// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#include "helpers/GmpiPluginEditor.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
// The blur behind the caption's raised edge. This is the same filter
// gmpi_ui's `cachedBlur` uses (helpers/CachedBlur.h, as seen in
// Controls/BumpGui.cpp); cachedBlur itself composites a tinted bitmap onto a
// Graphics, whereas the highlight below needs the blurred MASK to subtract
// with, so it calls one level down.
#include "helpers/GinBlur.h"

using namespace gmpi;
using namespace gmpi::editor;
using namespace gmpi::drawing;

// The rack-module faceplate. BACKLOG E15 — this replaces `SE Rectangle XP` as
// the background behind every rack-module prefab.
//
// ONLY TWO PINS, on purpose. The appearance is TIDE's, not the patch author's:
// PLAN constraint 8 ships one look and forbids user skins, so the faceplate's
// colours, corner radius, grain and glow are compile-time constants below
// rather than pins. Tune them here and every rack module moves together, which
// is the point. `SE Rectangle XP` exposed all of these as pins; deliberately
// not carried over.
//
// TWO CACHED BITMAPS, and both exist for the same reason: the effects need
// per-pixel access, which means a CPU-readable offscreen target.
//
//   face    — gradient plus material grain. Regenerating it per frame would be
//             both slow and WRONG: fresh random numbers every redraw would make
//             the surface crawl.
//   caption — the rotated legend and its inner glow. Rendering into a
//             bounds-sized bitmap also clips it for free.
//
// Per the design language (docs/ui-design-language.md), panels are flat in the
// sense that matters — no bevel, no drop shadow, no glow on the panel itself.
// The gradient and grain are material, not decoration, and both are subtle
// enough to be felt rather than seen.
namespace
{
// --- the faceplate's look. One set of values for the whole rack. ------------
constexpr Color kTopColor    = colorFromHex(0xD8D8DCu);
constexpr Color kBottomColor = colorFromHex(0xB4B4BEu);
constexpr float kCornerRadius = 4.0f;

// Grain amplitude in 8-bit levels. kNoiseMono is MONOCHROME — the same delta on
// all three channels, so it varies brightness and leaves hue alone; that is what
// reads as a material surface. kNoiseRgb is INDEPENDENT per channel, which reads
// as coloured speckle (sensor grain); it is off, and adds to the monochrome
// grain rather than replacing it, so a mix is available if wanted.
constexpr float kNoiseMono = 3.0f;
constexpr float kNoiseRgb[3] = { 0.0f, 0.0f, 0.0f };

// The caption's raised-paint highlight: a light edge along the TOP and LEFT of
// every glyph, as if lit from the top-left.
//
// Both values are in DIPs and scale to device pixels, so the edge holds its
// apparent weight at any zoom instead of thinning out on a HiDPI screen.
// kEdgeOffset is how far the occluding copy of the glyph slides down-right —
// it decides WHICH edges light. kEdgeBlur is the softness of the falloff — it
// decides how much the result looks lit rather than outlined.
// TEMPORARILY HEAVY so the effect can be judged; expect to halve both once the
// look is settled.
constexpr float kGlow = 1.0f;
constexpr float kEdgeOffsetDips = 2.0f;
constexpr float kEdgeBlurDips = 2.0f;
// Concentrates the highlight at the edge. Without it a blurred occluder never
// fully covers the middle of a THIN stroke, so a few percent of lightening
// lands on every pixel and the whole caption goes milky instead of gaining an
// edge. Raising this pushes the mid-tones down hard while leaving the lit edge
// itself untouched.
constexpr float kEdgeFalloff = 2.5f;

// createTextFormat's default body height is 12; the caption is five times that.
// Inset from the bottom edge so it starts NEAR the bottom, not on it.
constexpr float kCaptionBodyHeight = 60.0f;
constexpr float kCaptionInset = 4.0f;
}

class TiDEPanelGui final : public PluginEditor
{
	Pin<std::string> pinText;
	Pin<std::string> pinTextColor;

	Bitmap faceBitmap;
	Bitmap captionBitmap;
	SizeU bitmapSize{};      // DEVICE pixels, not DIPs — see getDeviceScale
	float bitmapScale = 0.0f;
	bool faceDirty = true;
	bool captionDirty = true;

	// CpuReadable is what makes lockPixels() work at all, and SRGBPixels is what
	// makes the result 8-bit rather than the default 64bpp half-float. Both are
	// needed: on DirectX the 8-bit face is gated behind CpuReadable, so
	// SRGBPixels alone still yields a target that cannot be locked
	// (gmpi_ui/GmpiApiDrawing.h:112-122).
	static constexpr int32_t kBitmapFlags = (int32_t)BitmapRenderTargetFlags::SRGBPixels
		| (int32_t)BitmapRenderTargetFlags::CpuReadable;

	void invalidate()
	{
		if (drawingHost)
			drawingHost->invalidateRect(nullptr);
	}

	void onCaptionChanged()
	{
		captionDirty = true;
		invalidate();
	}

	// Magnitude of a transform's linear (scale) part.
	static float transformScale(const Matrix3x2& m)
	{
		const float s = std::sqrt(m._11 * m._11 + m._12 * m._12);
		return (s > 0.0f) ? s : 1.0f;
	}

	// PHYSICAL pixels per logical unit. `bounds` is in DIPs, so a bitmap sized
	// from it directly is generated at 1 pixel per DIP and then STRETCHED to the
	// display by drawBitmap — soft on any HiDPI screen and softer still at panel
	// zoom. The host's rasterization scale covers the display; the transform's
	// scale covers the zoom; the texture needs both.
	// Precedent: SynthEditLib/modules/Controls/Scope4Gui.cpp:353.
	float getDeviceScale(Graphics& g) const
	{
		const float dpiScale = drawingHost.get() ? drawingHost->getRasterizationScale() : 1.0f;
		return (std::max)(0.01f, dpiScale * transformScale(g.getTransform()));
	}

	// The hex-string -> Color conversion is gmpi_ui's own (Drawing.h:654). It
	// already implements SynthEdit's convention exactly: AARRGGBB, alpha taken
	// from the high byte only when the string is longer than 6 digits, and RGB
	// decoded through the sRGB transfer function. That matches what
	// SubControlsXp/RectangleGui.cpp does by hand with FastGamma.
	Color getTextColor() const
	{
		return pinTextColor.value.empty() ? Colors::Black : colorFromHexString(pinTextColor.value);
	}

	static float cornerRadius(const Size& size)
	{
		return (std::clamp)(kCornerRadius, 0.0f, 0.5f * (std::min)(size.width, size.height));
	}

	// Deterministic value noise: a hash of the pixel coordinate plus a salt, so
	// the grain is identical every regeneration and needs no stored state or
	// seeded engine. The salt is what makes the per-channel noise INDEPENDENT
	// rather than three copies of one pattern. Returns -1.0 .. +1.0.
	static float pixelNoise(uint32_t x, uint32_t y, uint32_t salt)
	{
		uint32_t h = x * 0x8DA6B343u ^ y * 0xD8163841u ^ (salt + 1u) * 0x1B56C4E9u;
		h ^= h >> 15;
		h *= 0x2C1B3C6Du;
		h ^= h >> 12;
		h *= 0x297A2D39u;
		h ^= h >> 15;
		return (float)(int32_t)(h & 0xFFFFu) * (1.0f / 32767.5f) - 1.0f;
	}

	// Adds grain to an 8-bit face in place, one dot per DEVICE pixel.
	//
	// Channel order does not matter for the monochrome grain but DOES for the
	// per-channel grain, so the layout is read from the pixel format rather than
	// assumed: both 8-bit formats the backends produce put alpha last, but one
	// is BGRA (Windows) and the other RGBA (macOS), so R and B swap.
	//
	// Anything that is not 4-byte integer is left ungrained rather than
	// corrupted — a backend that declined SRGBPixels hands back half-float, and
	// blindly writing bytes into that would produce garbage, not a subtle miss.
	static void addNoise(Bitmap& bitmap)
	{
		if constexpr (kNoiseMono <= 0.0f && kNoiseRgb[0] <= 0.0f
			&& kNoiseRgb[1] <= 0.0f && kNoiseRgb[2] <= 0.0f)
			return;

		auto pixels = bitmap.lockPixels(BitmapLockFlags::ReadWrite);
		if (!pixels)
			return;

		// From the pixel format, never from bytesPerRow/width: rows are padded.
		if (pixels.getBytesPerPixel() != 4 || !pixels.isInteger())
			return;

		// Byte index of R, G, B for this bitmap's actual layout (0=BGRA, 1=RGBA).
		const bool isRgba = pixels.channelLayout() == 1;
		const int channelByte[3] = { isRgba ? 0 : 2, 1, isRgba ? 2 : 0 };

		uint8_t* const data = pixels.getAddress();
		const int32_t bytesPerRow = pixels.getBytesPerRow();
		const auto size = pixels.getSize();

		for (uint32_t y = 0; y < size.height; ++y)
		{
			uint8_t* row = data + (size_t)y * bytesPerRow;
			for (uint32_t x = 0; x < size.width; ++x)
			{
				uint8_t* px = row + (size_t)x * 4;

				// Channels are PREmultiplied, so the delta has to be scaled by
				// alpha or grain would bleed outside the rounded corners.
				const float alpha = px[3] * (1.0f / 255.0f);
				if (alpha <= 0.0f)
					continue;

				const float monoDelta = pixelNoise(x, y, 0) * kNoiseMono;

				for (int c = 0; c < 3; ++c)
				{
					const float chroma = kNoiseRgb[c] > 0.0f
						? pixelNoise(x, y, 1u + (uint32_t)c) * kNoiseRgb[c]
						: 0.0f;
					uint8_t& channel = px[channelByte[c]];
					channel = (uint8_t)(std::clamp)(
						channel + (monoDelta + chroma) * alpha + 0.5f, 0.0f, (float)px[3]);
				}
			}
		}
	}

	// A white highlight along the TOP and LEFT inside edge of every glyph —
	// raised paint catching a light from the top-left.
	//
	// This is BumpGui.cpp's inner-highlight recipe applied to glyphs instead of
	// a rounded rect: take a copy of the shape, slide it AWAY from the light,
	// blur it, and light whatever the slid copy fails to cover. Because the copy
	// moves down-right, the band it vacates is the top-left inside edge, and
	// because it is blurred the band fades inward instead of stopping dead.
	//
	// The first version of this eroded the alpha by one pixel in the up/left
	// direction and lit the shortfall. That is a hard-edged rim, not lighting —
	// it aliases along every diagonal and reads as an outline. The blur is what
	// makes it look like a lit surface, and it is the SAME blur the rest of the
	// codebase uses (ginSingleChannel, via cachedBlur).
	//
	// The blend runs in sRGB space, deliberately. Premultiplied white is simply
	// "every channel equals alpha", so lerping a channel toward alpha needs no
	// colour-space conversion at all. Physically it should be done in linear
	// light, but for a highlight on a glyph the difference is invisible and the
	// conversion would cost a decode/encode per pixel.
	static void addRaisedEdge(Bitmap& bitmap, float deviceScale)
	{
		if constexpr (kGlow <= 0.0f)
			return;

		auto pixels = bitmap.lockPixels(BitmapLockFlags::ReadWrite);
		if (!pixels)
			return;

		if (pixels.getBytesPerPixel() != 4 || !pixels.isInteger())
			return;

		uint8_t* const data = pixels.getAddress();
		const int32_t bytesPerRow = pixels.getBytesPerRow();
		const auto size = pixels.getSize();
		const int32_t w = (int32_t)size.width;
		const int32_t h = (int32_t)size.height;

		const int32_t offset = (std::max)(1, (int32_t)std::lround(kEdgeOffsetDips * deviceScale));
		const unsigned radius = (unsigned)(std::clamp)(
			(int32_t)std::lround(kEdgeBlurDips * deviceScale), 1, 254);

		// The occluder: the glyphs' own coverage, slid down-right, away from the
		// light. Single channel, tightly packed, which is what ginSingleChannel
		// wants.
		std::vector<uint8_t> occluder((size_t)w * h, 0);
		for (int32_t y = offset; y < h; ++y)
		{
			const uint8_t* src = data + (size_t)(y - offset) * bytesPerRow + 3;
			uint8_t* dst = occluder.data() + (size_t)y * w + offset;
			for (int32_t x = offset; x < w; ++x, ++dst, src += 4)
				*dst = *src;
		}

		ginSingleChannel(occluder.data(), (unsigned)w, (unsigned)h, radius, (unsigned)w);

		for (int32_t y = 0; y < h; ++y)
		{
			uint8_t* row = data + (size_t)y * bytesPerRow;
			const uint8_t* occ = occluder.data() + (size_t)y * w;
			for (int32_t x = 0; x < w; ++x)
			{
				uint8_t* px = row + (size_t)x * 4;
				const int32_t a = px[3];
				if (a == 0)
					continue;

				// Uncovered by the slid copy == facing the light.
				const float exposure = (255 - occ[x]) * (1.0f / 255.0f);
				if (exposure <= 0.0f)
					continue;

				const float t = kGlow * std::pow(exposure, kEdgeFalloff);

				for (int c = 0; c < 3; ++c)
					px[c] = (uint8_t)(px[c] + t * (a - px[c]) + 0.5f);
			}
		}
	}

	// `size` is LOGICAL (DIPs) and `pixels` is PHYSICAL. Everything below draws
	// in DIPs and lets the scale transform map it to device pixels, so the corner
	// radius and gradient stay specified in the units they are written in.
	static Bitmap renderFace(Graphics& g, const Size& size, const SizeU& pixels, float deviceScale)
	{
		const Rect localBounds{ 0.0f, 0.0f, size.width, size.height };
		const auto radius = cornerRadius(size);
		const RoundedRect shape{ localBounds, radius, radius };

		auto rt = g.getFactory().createCpuRenderTarget(pixels, kBitmapFlags);
		rt.beginDraw();
		rt.clear(Color{ 0.0f, 0.0f, 0.0f, 0.0f });
		{
			TempTransform toDevice(rt, makeScale(deviceScale));

			auto brush = rt.createLinearGradientBrush(
				Point{ 0.0f, 0.0f }, Point{ 0.0f, size.height }, kTopColor, kBottomColor);
			rt.fillRoundedRectangle(shape, brush);
		}
		rt.endDraw();

		auto bitmap = rt.getBitmap();
		addNoise(bitmap);
		return bitmap;
	}

	// The caption runs UP the panel — rotated a quarter-turn anti-clockwise,
	// reading bottom to top, the way a Eurorack module is legended.
	//
	// THE SIGN IS THE THING TO GET RIGHT. makeRotation takes RADIANS and screen
	// y points DOWN, so a POSITIVE angle turns clockwise on screen. Anti-
	// clockwise is therefore -pi/2, not +pi/2.
	//
	// Rotating about the panel's centre means the drawing box in the rotated
	// frame is the panel TRANSPOSED — height by width — about that same centre.
	// In that frame the two alignments map to panel edges: Leading along the
	// baseline is the panel's BOTTOM, and Near across it is the panel's LEFT.
	// Deliberately NOT scaled to fit: a tall caption on a short module runs off
	// the top, and Jeff ruled that acceptable. It is clipped rather than
	// overflowing only because this renders into a bounds-sized bitmap — drawn
	// straight to the screen it would cover the neighbouring rack module.
	Bitmap renderCaption(Graphics& g, const Size& size, const SizeU& pixels, float deviceScale) const
	{
		const Point centre{ size.width * 0.5f, size.height * 0.5f };

		auto rt = g.getFactory().createCpuRenderTarget(pixels, kBitmapFlags);
		rt.beginDraw();
		rt.clear(Color{ 0.0f, 0.0f, 0.0f, 0.0f });
		{
			// Composes with the rotation below: a point is rotated in DIP space
			// first, then scaled to device pixels.
			TempTransform toDevice(rt, makeScale(deviceScale));

			auto textFormat = rt.getFactory().createTextFormat(
				kCaptionBodyHeight,
				{},
				FontWeight::Bold);
			textFormat.setWordWrapping(WordWrapping::NoWrap);
			textFormat.setTextAlignment(TextAlignment::Leading);        // start at the bottom
			textFormat.setParagraphAlignment(ParagraphAlignment::Near); // sit on the left edge

			auto brush = rt.createSolidColorBrush(getTextColor());

			// The panel, transposed about its own centre.
			const Rect rotatedBounds{
				centre.x - size.height * 0.5f + kCaptionInset,
				centre.y - size.width * 0.5f,
				centre.x + size.height * 0.5f,
				centre.y + size.width * 0.5f
			};

			constexpr float quarterTurnAntiClockwise = -1.57079632679489661923f;
			TempTransform rotate(rt, makeRotation(quarterTurnAntiClockwise, centre));
			rt.drawTextU(pinText.value.c_str(), textFormat, rotatedBounds, brush);
		}
		rt.endDraw();

		auto bitmap = rt.getBitmap();
		addRaisedEdge(bitmap, deviceScale);
		return bitmap;
	}

	void updateBitmaps(Graphics& g, const Size& size, float deviceScale)
	{
		const SizeU pixels{
			(uint32_t)(std::max)(1.0f, std::ceil(size.width * deviceScale)),
			(uint32_t)(std::max)(1.0f, std::ceil(size.height * deviceScale))
		};

		// The device scale is part of the cache key: a window dragged to a
		// different-DPI monitor, or a panel zoom, changes it without changing
		// the logical size, and a stale bitmap would then be resampled.
		if (bitmapSize != pixels || bitmapScale != deviceScale)
		{
			faceDirty = true;
			captionDirty = true;
			bitmapSize = pixels;
			bitmapScale = deviceScale;
		}

		if (faceDirty || !faceBitmap)
		{
			faceBitmap = renderFace(g, size, pixels, deviceScale);
			faceDirty = false;
		}

		if (captionDirty || !captionBitmap)
		{
			captionBitmap = pinText.value.empty()
				? Bitmap{}
				: renderCaption(g, size, pixels, deviceScale);
			captionDirty = false;
		}
	}

public:
	TiDEPanelGui()
	{
		pinText.onUpdate      = [this](PinBase*) { onCaptionChanged(); };
		pinTextColor.onUpdate = [this](PinBase*) { onCaptionChanged(); };
	}

	ReturnCode render(gmpi::drawing::api::IDeviceContext* drawingContext) override
	{
		Graphics g(drawingContext);

		const Size size{ getWidth(bounds), getHeight(bounds) };
		updateBitmaps(g, size, getDeviceScale(g));

		// Source is the bitmap's own PIXEL rect; destination is the DIP bounds.
		// Because the bitmap was built at device resolution, that pairing is a
		// 1:1 blit on screen rather than an upscale.
		const Rect source{ 0.0f, 0.0f, (float)bitmapSize.width, (float)bitmapSize.height };

		if (faceBitmap)
			g.drawBitmap(faceBitmap, bounds, source);

		if (captionBitmap)
			g.drawBitmap(captionBitmap, bounds, source);

		return ReturnCode::Ok;
	}
};

namespace
{
auto r = Register<TiDEPanelGui>::withXml(R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<Plugin id="SE TiDE:Panel" name="Panel" category="TiDE">
    <GUI graphicsApi="GmpiUi">
        <Pin name="Text"       datatype="string_utf8" default=""/>
        <Pin name="Text Color" datatype="string_utf8" default="FF101010"/>
    </GUI>
</Plugin>
)XML");
}
