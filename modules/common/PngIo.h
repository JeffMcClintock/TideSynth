// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// PNG read and write in ~200 lines, for the DEVELOPER PREVIEW TOOL AND THE
// REGRESSION TEST ONLY. The shipping modules never call this — they hand pixels
// straight to a GMPI bitmap. It exists so a render can be eyeballed without a
// plugin host, and so a reference image can be committed and compared.
//
// WHY NOT stb_image / libpng: the whole point of this renderer is zero
// dependencies, and a preview tool that drags in a third-party image library
// undoes that for the one target most likely to be built casually.
//
// The trick that makes it short: PNG's IDAT payload is a zlib stream, and zlib
// permits STORED (uncompressed) deflate blocks. So no deflate encoder and no
// inflate decoder is needed — just a 2-byte zlib header, the data cut into
// <=65535-byte stored blocks, and an Adler-32. The file is roughly as large as
// the raw pixels, which for a few-hundred-pixel preview is irrelevant, and
// every PNG reader accepts it.
//
// THE READER IS NOT A GENERAL PNG DECODER. It reads back exactly what the
// writer here produces: 8-bit RGBA, filter 0, stored deflate blocks. That is
// enough to compare against our own committed references, and anything else is
// rejected with a message saying so rather than decoded incorrectly. If a
// reference is ever re-saved by an image editor it will use real deflate and
// stop loading — regenerate it with the preview tool instead.
namespace tide::png
{
namespace detail
{
inline uint32_t crc32Of(const uint8_t* data, size_t len, uint32_t crc = 0xFFFFFFFFu)
{
	// Table built on first use rather than hard-coded: 256 magic numbers in a
	// source file are unreviewable, and this costs microseconds once.
	static const auto table = []
	{
		std::vector<uint32_t> t(256);
		for (uint32_t n = 0; n < 256; ++n)
		{
			uint32_t c = n;
			for (int k = 0; k < 8; ++k)
				c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
			t[n] = c;
		}
		return t;
	}();

	for (size_t i = 0; i < len; ++i)
		crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);

	return crc;
}

inline void put32(std::vector<uint8_t>& out, uint32_t v)
{
	out.push_back((uint8_t)(v >> 24));
	out.push_back((uint8_t)(v >> 16));
	out.push_back((uint8_t)(v >> 8));
	out.push_back((uint8_t)v);
}

inline uint32_t read32(const uint8_t* p)
{
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

// A PNG chunk is length, type, payload, then a CRC over TYPE AND PAYLOAD but
// NOT the length — a detail that silently produces unreadable files if missed.
inline void putChunk(std::vector<uint8_t>& out, const char type[4], const std::vector<uint8_t>& payload)
{
	put32(out, (uint32_t)payload.size());

	const size_t crcStart = out.size();
	out.insert(out.end(), type, type + 4);
	out.insert(out.end(), payload.begin(), payload.end());

	put32(out, crc32Of(out.data() + crcStart, out.size() - crcStart) ^ 0xFFFFFFFFu);
}

inline const uint8_t kSignature[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };

// The one place that knows how to open a file portably. MSVC deprecates plain
// fopen; keeping the #ifdef in a single helper means a future fix (wide-char
// paths, error reporting) cannot land in the writer and miss the reader.
inline FILE* openFile(const std::string& path, const char* mode)
{
	FILE* f = nullptr;
#ifdef _MSC_VER
	fopen_s(&f, path.c_str(), mode);
#else
	f = std::fopen(path.c_str(), mode);
#endif
	return f;
}
}

// `rgba` is 8-bit, NON-premultiplied, width*height*4 bytes, top row first.
//
// NON-premultiplied is deliberate and is the opposite of what the GMPI bitmaps
// want: PNG's spec stores unassociated alpha, so premultiplied data written
// here would look correct only where alpha is 1 and would darken everywhere
// else. The caller un-premultiplies before arriving.
inline bool write(const std::string& path, const uint8_t* rgba, int width, int height)
{
	if (width <= 0 || height <= 0 || !rgba)
		return false;

	using namespace detail;

	std::vector<uint8_t> file(kSignature, kSignature + 8);

	std::vector<uint8_t> ihdr;
	put32(ihdr, (uint32_t)width);
	put32(ihdr, (uint32_t)height);
	ihdr.push_back(8); // bit depth
	ihdr.push_back(6); // colour type 6 = truecolour with alpha
	ihdr.push_back(0); // deflate
	ihdr.push_back(0); // adaptive filtering
	ihdr.push_back(0); // no interlace
	putChunk(file, "IHDR", ihdr);

	// Scanlines, each prefixed by its filter byte. Filter 0 (None) everywhere:
	// filtering exists to help the compressor, and there is no compressor here.
	std::vector<uint8_t> raw;
	raw.reserve((size_t)height * (1 + (size_t)width * 4));
	for (int y = 0; y < height; ++y)
	{
		raw.push_back(0);
		const uint8_t* row = rgba + (size_t)y * (size_t)width * 4;
		raw.insert(raw.end(), row, row + (size_t)width * 4);
	}

	// Adler-32 runs over the UNCOMPRESSED bytes, so it is computed from `raw`
	// and not from the stored-block stream that wraps it.
	uint32_t a = 1, b = 0;
	for (uint8_t byte : raw)
	{
		a = (a + byte) % 65521u;
		b = (b + a) % 65521u;
	}

	std::vector<uint8_t> zlib{ 0x78, 0x01 }; // CM=8, CINFO=7, FCHECK making it %31==0

	size_t offset = 0;
	do
	{
		const size_t n = (raw.size() - offset < 65535u) ? (raw.size() - offset) : 65535u;
		const bool last = (offset + n) >= raw.size();

		zlib.push_back(last ? 1 : 0);            // BFINAL, BTYPE=00 (stored)
		zlib.push_back((uint8_t)(n & 0xFF));     // LEN, little endian
		zlib.push_back((uint8_t)(n >> 8));
		zlib.push_back((uint8_t)(~n & 0xFF));    // NLEN, the ones complement
		zlib.push_back((uint8_t)((~n >> 8) & 0xFF));
		zlib.insert(zlib.end(), raw.begin() + offset, raw.begin() + offset + n);

		offset += n;
	}
	while (offset < raw.size()); // do/while so a zero-byte payload still emits a final block

	put32(zlib, (b << 16) | a);
	putChunk(file, "IDAT", zlib);
	putChunk(file, "IEND", {});

	FILE* f = detail::openFile(path, "wb");
	if (!f)
		return false;

	const bool ok = std::fwrite(file.data(), 1, file.size(), f) == file.size();
	std::fclose(f);
	return ok;
}

// Reads a PNG written by `write` above. On failure `error` says why and the
// output is left untouched.
inline bool read(const std::string& path, std::vector<uint8_t>& rgba,
	int& width, int& height, std::string& error)
{
	using namespace detail;

	FILE* f = detail::openFile(path, "rb");
	if (!f)
	{
		error = "cannot open " + path;
		return false;
	}

	std::vector<uint8_t> file;
	uint8_t buffer[4096];
	for (size_t n; (n = std::fread(buffer, 1, sizeof(buffer), f)) > 0; )
		file.insert(file.end(), buffer, buffer + n);
	std::fclose(f);

	if (file.size() < 8 || std::memcmp(file.data(), kSignature, 8) != 0)
	{
		error = path + ": not a PNG";
		return false;
	}

	int w = 0, h = 0;
	std::vector<uint8_t> idat;

	size_t pos = 8;
	while (pos + 8 <= file.size())
	{
		const uint32_t length = read32(&file[pos]);
		const char* type = (const char*)&file[pos + 4];
		const size_t payload = pos + 8;
		if (payload + length + 4 > file.size())
		{
			error = path + ": truncated chunk";
			return false;
		}

		if (std::memcmp(type, "IHDR", 4) == 0 && length >= 13)
		{
			w = (int)read32(&file[payload]);
			h = (int)read32(&file[payload + 4]);
			const uint8_t depth = file[payload + 8];
			const uint8_t colour = file[payload + 9];
			const uint8_t interlace = file[payload + 12];
			if (depth != 8 || colour != 6 || interlace != 0)
			{
				error = path + ": only 8-bit RGBA non-interlaced is supported";
				return false;
			}
		}
		else if (std::memcmp(type, "IDAT", 4) == 0)
		{
			idat.insert(idat.end(), &file[payload], &file[payload] + length);
		}
		else if (std::memcmp(type, "IEND", 4) == 0)
		{
			break;
		}

		pos = payload + length + 4;
	}

	if (w <= 0 || h <= 0 || idat.size() < 2)
	{
		error = path + ": missing IHDR or IDAT";
		return false;
	}

	// Unwrap the zlib stream: skip the 2-byte header, then walk stored blocks.
	std::vector<uint8_t> raw;
	size_t z = 2;
	bool sawFinal = false;
	while (z + 5 <= idat.size() && !sawFinal)
	{
		const uint8_t header = idat[z];
		const uint8_t blockType = (header >> 1) & 3u;
		sawFinal = (header & 1u) != 0;
		if (blockType != 0)
		{
			error = path + ": compressed PNG (only stored/uncompressed blocks are"
				" supported). Regenerate it with tide_render_preview.";
			return false;
		}

		const size_t n = (size_t)idat[z + 1] | ((size_t)idat[z + 2] << 8);
		if (z + 5 + n > idat.size())
		{
			error = path + ": truncated deflate block";
			return false;
		}

		raw.insert(raw.end(), &idat[z + 5], &idat[z + 5] + n);
		z += 5 + n;
	}

	const size_t stride = 1 + (size_t)w * 4;
	if (raw.size() < stride * (size_t)h)
	{
		error = path + ": short pixel data";
		return false;
	}

	// Decoded into a LOCAL buffer and swapped in only on success. The contract
	// above says failure leaves the outputs untouched, and an earlier version
	// broke it on exactly one path: a bad filter byte fired after rgba had
	// already been resized and partly filled, leaving the caller's buffer sized
	// for the FAILED file while width/height still described the old contents.
	std::vector<uint8_t> pixels((size_t)w * (size_t)h * 4);
	for (int y = 0; y < h; ++y)
	{
		const uint8_t* row = &raw[(size_t)y * stride];
		if (row[0] != 0)
		{
			error = path + ": only filter type 0 is supported";
			return false;
		}
		std::memcpy(&pixels[(size_t)y * (size_t)w * 4], row + 1, (size_t)w * 4);
	}

	rgba = std::move(pixels);
	width = w;
	height = h;
	return true;
}
}
