// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#include "helpers/GmpiPluginEditor.h"
#include "helpers/BitmapMask.h"

using namespace gmpi;
using namespace gmpi::editor;
using namespace gmpi::drawing;

// Outputs get a light title (the TiDE panel paints their black backing), inputs a dark one.
template<bool isOutput>
class PatchPointGui final : public PluginEditor, public gmpi::api::IDrawingLayer
{
	// Sized like a VCV Fundamental output cell (one jack's share of VCO's output box). Must match <PatchPoint center="15,25"> in PatchPoint.cpp.
	static constexpr Size moduleSize{ 30.0f, 40.0f };
	static constexpr float jackBottomMargin = 15.0f; // layout rect bottom to jack centre

	// Radius of the clickable disc, and of the debug outline.
	static constexpr float radius = 9.0f;

	// VCV Fundamental's label size.
	static constexpr float titleCapHeight = 4.3f;
	static constexpr float titleGap = 2.0f;

	// The socket's black moulded body (TiDEPanel's kJackBodyMm = 8 mm); the title stops above it.
	static constexpr float jackBodyRadius = 4.0f * 75.0f / 25.4f;

	Pin<std::string> pinTitle;
	Pin<float> pinLevel; // signed peak from the DSP; 1.0 = 10 V

	TextFormat titleFormat;

	// The LED in the middle of the jack, as VCV Rack's plug lights: green positive, red negative,
	// brightness |V| / 10. Drawn like SE's LED (LEDGui.cpp): a lit core, plus an additive glow on layer 1.
	static constexpr float ledRadius = 2.0f;
	static constexpr float ledGlowSize = 6.0f;
	static constexpr uint32_t ledPositiveColor = 0x90C73Eu; // VCV SCHEME_GREEN
	static constexpr uint32_t ledNegativeColor = 0xED2C24u; // VCV SCHEME_RED
	Bitmap ledGlow[2]; // [0] positive, [1] negative

	float ledBrightness() const
	{
		return (std::min)(1.0f, std::fabs(pinLevel.value));
	}

	Color ledColor() const
	{
		return colorFromHex(pinLevel.value < 0.0f ? ledNegativeColor : ledPositiveColor);
	}

	Rect ledGlowRect() const
	{
		const auto c = jackCenter();
		const float r = ledRadius + ledGlowSize;
		return { c.x - r, c.y - r, c.x + r, c.y + r };
	}

	// Purely additive glow pixels (alpha 0), as LEDGui builds them, without its star spokes.
	Bitmap& glowBitmap(Graphics& g)
	{
		auto& bitmap = ledGlow[pinLevel.value < 0.0f ? 1 : 0];
		if (bitmap)
			return bitmap;

		const int32_t size = static_cast<int32_t>(std::ceil(2.0f * (ledRadius + ledGlowSize)));
		bitmap = g.getFactory().createImage(size, size);

		auto pixels = bitmap.lockPixels(BitmapLockFlags::Write);
		if (!pixels)
			return bitmap;

		constexpr float falloff = 0.3f; // bigger = steeper (less glow)
		constexpr float taperZoneStart = 0.5f;
		constexpr float taperGradient = 1.0f / (1.0f - taperZoneStart);
		const float centre = 0.5f * static_cast<float>(size);
		const auto colour = ledColor();

		for (auto& it : pixelIterator<RgbaHalfPixel>(pixels))
		{
			const auto dx = (it.x + 0.5f) - centre;
			const auto dy = (it.y + 0.5f) - centre;
			const auto distance = std::sqrt(dx * dx + dy * dy);
			auto glow = 0.5f / (std::max)(1.0f, falloff * (distance - ledRadius));
			glow *= (std::clamp)(1.0f - taperGradient * (distance - centre * taperZoneStart) / centre, 0.0f, 1.0f);

			it->setR(colour.r * glow);
			it->setG(colour.g * glow);
			it->setB(colour.b * glow);
			it->setA(0.0f);
		}
		return bitmap;
	}

	void onLevelChanged()
	{
		if (drawingHost)
		{
			const auto r = offsetRect(ledGlowRect(), { bounds.left, bounds.top });
			drawingHost->invalidateRect(&r);
		}
	}

	// VCV Fundamental's panel colours.
	static constexpr uint32_t darkColor = 0x1F1F1Fu;
	static constexpr uint32_t lightColor = 0xF0F0F0u;

	// Local coords. The jack sits near the bottom of the layout rect, the title above it.
	Point jackCenter() const
	{
		return { getWidth(bounds) * 0.5f, getHeight(bounds) - jackBottomMargin };
	}

	Rect titleRect() const
	{
		return { 0.0f, 0.0f, getWidth(bounds), jackCenter().y - jackBodyRadius - titleGap };
	}

	void onTitleChanged()
	{
		if (!pinTitle.value.empty() && !titleFormat && drawingHost)
		{
			gmpi::shared_ptr<gmpi::api::IUnknown> unk;
			drawingHost->getDrawingFactory(unk.put());
			Factory factory;
			if (unk)
				unk->queryInterface(&drawing::api::IFactory::guid, AccessPtr::put_void(factory));

			if (AccessPtr::get(factory))
			{
				titleFormat = factory.createTextFormat(titleCapHeight, {}, FontWeight::Bold, FontStyle::Normal, FontStretch::Normal, FontFlags::CapHeight);
				titleFormat.setTextAlignment(TextAlignment::Center);
				titleFormat.setParagraphAlignment(ParagraphAlignment::Far);
				titleFormat.setWordWrapping(WordWrapping::NoWrap);
			}
		}

		if (drawingHost)
			drawingHost->invalidateRect(&bounds);
	}

public:
	PatchPointGui()
	{
		pinTitle.onUpdate = [this](PinBase*) { onTitleChanged(); };
		pinLevel.onUpdate = [this](PinBase*) { onLevelChanged(); };
	}

	// Layer 4 = editor guide (see IDrawingLayer in NativeUi.h): a design-time-only
	// overlay pass, so the debug outline still needs its own _DEBUG guard to stay
	// out of Release-configuration modules loaded into the same editor. Once a
	// plugin implements IDrawingLayer, render() is never called for layer 0 either
	// -- so all drawing, not just this guide, lives here now.
	ReturnCode renderLayer(gmpi::drawing::api::IDeviceContext* drawingContext, int32_t layer) override
	{
		if (layer == 0)
		{
			Graphics g(drawingContext);

			if (titleFormat && !pinTitle.value.empty())
			{
				const auto r = titleRect();
				g.pushAxisAlignedClip(r);
				g.drawTextU(pinTitle.value, titleFormat, r, g.createSolidColorBrush(colorFromHex(isOutput ? lightColor : darkColor)));
				g.popAxisAlignedClip();
			}

			if (const float brightness = ledBrightness(); brightness > 0.0f)
			{
				const auto c = jackCenter();
				auto core = interpolateColor(ledColor(), Colors::White, 0.5f * brightness);
				core.a = brightness;
				g.fillEllipse({ c, ledRadius, ledRadius }, g.createSolidColorBrush(core));
			}

			return ReturnCode::Ok;
		}

		if (layer == 1)
		{
			const float brightness = ledBrightness();
			if (brightness <= 0.0f)
				return ReturnCode::Ok;

			Graphics g(drawingContext);
			auto& glow = glowBitmap(g);
			if (!glow)
				return ReturnCode::Ok;

			const auto size = glow.getSize();
			const Rect srcRect{ 0.0f, 0.0f, static_cast<float>(size.width), static_cast<float>(size.height) };
			g.drawBitmap(glow, ledGlowRect(), srcRect, brightness);
			return ReturnCode::Ok;
		}

		if (layer == 4)
		{
			Graphics g(drawingContext);

			StrokeStyleProperties strokeStyleProperties{};
			strokeStyleProperties.lineCap = CapStyle::Round; // Flat caps don't draw dots on Windows.
			strokeStyleProperties.dashStyle = DashStyle::Dot;
			auto dottedStroke = g.getFactory().createStrokeStyle(strokeStyleProperties);

			g.drawEllipse({ jackCenter(), radius + 0.5f, radius + 0.5f }, g.createSolidColorBrush(Colors::Orange), 1.0f, dottedStroke);

			return ReturnCode::Ok;
		}
		return ReturnCode::NoSupport;
	}

	// Fixed size: return the same constant regardless of availableSize. That is how
	// an editor declares it does not resize (see PluginEditor::measure).
	ReturnCode measure([[maybe_unused]] const Size* availableSize, Size* returnDesiredSize) override
	{
		*returnDesiredSize = moduleSize;
		return ReturnCode::Ok;
	}

	// Ok = hit, Unhandled/Fail = miss.
	// The base class defaults to Ok so the user can select by clicking; here we
	// narrow the hit area to the disc so the rest of the module falls through.
	// point will always be within the bounding rect.
	ReturnCode hitTest(Point point, [[maybe_unused]] int32_t flags) override
	{
		const auto center = jackCenter();
		const float dx = point.x - center.x;
		const float dy = point.y - center.y;

		return dx * dx + dy * dy <= radius * radius ? ReturnCode::Ok : ReturnCode::Fail;
	}

	int32_t addRef() override
	{
		return PluginEditor::addRef();
	}

	int32_t release() override
	{
		return PluginEditor::release();
	}

	ReturnCode queryInterface(const gmpi::api::Guid* iid, void** returnInterface) override
	{
		*returnInterface = {};

		if ((*iid) == gmpi::api::IDrawingLayer::guid)
		{
			*returnInterface = static_cast<gmpi::api::IDrawingLayer*>(this);
			PluginEditor::addRef();
			return ReturnCode::Ok;
		}

		return PluginEditor::queryInterface(iid, returnInterface);
	}
};

namespace
{
	// One editor serves both patch-point plugins. These ids must match the
	// <Plugin id="..."> attributes in PatchPoint.cpp exactly, or the host finds
	// no editor for the plugin. The factory keys on {subtype, id}, so registering
	// the same class twice under two ids is fine -- SDK3 needed a second subclass
	// here only because its macro keyed on the type.
	auto rIn = gmpi::Register<PatchPointGui<false>>::withId("TiDE Patch Point In");
	auto rOut = gmpi::Register<PatchPointGui<true>>::withId("TiDE Patch Point Out");
}
