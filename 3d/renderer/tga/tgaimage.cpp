#include <iostream>
#include <fstream>
#include <string.h>
#include <time.h>
#include <math.h>
#include <utility>
#include "tgaimage.hpp"

// original: https://github.com/ssloy/tinyrenderer/blob/909fe20934ba5334144d2c748805690a1fa4c89f/tgaimage.cpp

TGAImage::TGAImage(): width(0), height(0), bytespp(0) {}

TGAImage::TGAImage(int w, int h, int bpp):data(w*h*bpp,0), width(w), height(h), bytespp(bpp){}

TGAImage::TGAImage(const TGAImage &img): data(img.width*img.height*img.bytespp), width(img.width), height(img.height), bytespp(img.bytespp) {
    std::copy(img.data.begin(), img.data.end(), data.begin());
}

void TGAImage::swap(TGAImage& rhs) noexcept {
    std::swap(rhs.width, width);
    std::swap(rhs.height, height);
    std::swap(rhs.bytespp, bytespp);
    std::swap(rhs.data, data);
}

TGAImage::TGAImage(TGAImage&& img) noexcept: 
    width(std::exchange(img.width, 0)), 
    height(std::exchange(img.height, 0)), 
    bytespp(std::exchange(img.bytespp, 0)), 
    data(std::move(img.data))  {
}

TGAImage& TGAImage::operator=(TGAImage&& img) noexcept {
	if (this != &img) {
        width = std::exchange(img.width, 0);
        height = std::exchange(img.height, 0); 
        bytespp = std::exchange(img.bytespp, 0);
        data = std::move(img.data);
	}
	return *this;
}

TGAImage::~TGAImage() {}

TGAImage& TGAImage::operator=(TGAImage img) {
	if (this != &img) {
        swap(img);
	}
	return *this;
}

bool TGAImage::read_tga_file(const char *filename) {
	data.clear();
	std::ifstream in;
	in.open(filename, std::ios::binary);
	if (!in.is_open()) {
		std::cerr << "can't open file " << filename << "\n";
		in.close();
		return false;
	}
	TGA_Header header;
	in.read((char *)&header, sizeof(header));
	if (!in.good()) {
		in.close();
		std::cerr << "an error occured while reading the header\n";
		return false;
	}
	width   = header.width;
	height  = header.height;
	bytespp = header.bitsperpixel>>3;
	if (width<=0 || height<=0 || (bytespp!=GRAYSCALE && bytespp!=RGB && bytespp!=RGBA)) {
		in.close();
		std::cerr << "bad bpp (or width/height) value\n";
		return false;
	}
	unsigned long nbytes = bytespp*width*height;
    data.resize(bytespp*width*height);
	if (3==header.datatypecode || 2==header.datatypecode) {
		in.read((char *)data.data(), nbytes);
		if (!in.good()) {
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	} else if (10==header.datatypecode||11==header.datatypecode) {
		if (!load_rle_data(in)) {
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	} else {
		in.close();
		std::cerr << "unknown file format " << (int)header.datatypecode << "\n";
		return false;
	}
	if (!(header.imagedescriptor & 0x20)) {
		flip_vertically();
	}
	if (header.imagedescriptor & 0x10) {
		flip_horizontally();
	}
	std::cerr << width << "x" << height << "/" << bytespp*8 << "\n";
	in.close();
	return true;
}

bool TGAImage::load_rle_data(std::ifstream &in) {
	unsigned long pixelcount = width*height;
	unsigned long currentpixel = 0;
	unsigned long currentbyte  = 0;
	TGAColor colorbuffer;
	do {
		unsigned char chunkheader = 0;
		chunkheader = in.get();
		if (!in.good()) {
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
		if (chunkheader<128) {
			chunkheader++;
			for (int i=0; i<chunkheader; i++) {
				in.read((char *)colorbuffer.raw, bytespp);
				if (!in.good()) {
					std::cerr << "an error occured while reading the header\n";
					return false;
				}
				for (int t=0; t<bytespp; t++)
					data[currentbyte++] = colorbuffer.raw[t];
				currentpixel++;
				if (currentpixel>pixelcount) {
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		} else {
			chunkheader -= 127;
			in.read((char *)colorbuffer.raw, bytespp);
			if (!in.good()) {
				std::cerr << "an error occured while reading the header\n";
				return false;
			}
			for (int i=0; i<chunkheader; i++) {
				for (int t=0; t<bytespp; t++)
					data[currentbyte++] = colorbuffer.raw[t];
				currentpixel++;
				if (currentpixel>pixelcount) {
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		}
	} while (currentpixel < pixelcount);
	return true;
}

bool TGAImage::write_tga_file(const char *filename, bool rle) {
	unsigned char developer_area_ref[4]{};
	unsigned char extension_area_ref[4]{};
	unsigned char footer[18] = {'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0'};
	std::ofstream out;
	out.open(filename, std::ios::binary);
	if (!out.is_open()) {
		std::cerr << "can't open file " << filename << "\n";
		out.close();
		return false;
	}
	TGA_Header header;
	memset((void *)&header, 0, sizeof(header));
	header.bitsperpixel = bytespp<<3;
	header.width  = width;
	header.height = height;
	header.datatypecode = (bytespp==GRAYSCALE?(rle?11:3):(rle?10:2));
	header.imagedescriptor = 0x20; // top-left origin
	out.write((char *)&header, sizeof(header));
	if (!out.good()) {
		out.close();
		std::cerr << "can't dump the tga file\n";
		return false;
	}
	if (!rle) {
		out.write((char *)data.data(), width*height*bytespp);
		if (!out.good()) {
			std::cerr << "can't unload raw data\n";
			out.close();
			return false;
		}
	} else {
		if (!unload_rle_data(out)) {
			out.close();
			std::cerr << "can't unload rle data\n";
			return false;
		}
	}
	out.write((char *)developer_area_ref, sizeof(developer_area_ref));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.write((char *)extension_area_ref, sizeof(extension_area_ref));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.write((char *)footer, sizeof(footer));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.close();
	return true;
}

// TODO: it is not necessary to break a raw chunk for two equal pixels (for the matter of the resulting size)
bool TGAImage::unload_rle_data(std::ofstream &out) {
	const unsigned char max_chunk_length = 128;
	unsigned long npixels = width*height;
	unsigned long curpix = 0;
	while (curpix < npixels) {
		unsigned long chunkstart = curpix*bytespp;
		unsigned long curbyte = curpix*bytespp;
		unsigned char run_length = 1;
		bool raw = true;
		while (curpix+run_length<npixels && run_length<max_chunk_length) {
			bool succ_eq = true;
			for (int t=0; succ_eq && t<bytespp; t++) {
				succ_eq = (data[curbyte+t]==data[curbyte+t+bytespp]);
			}
			curbyte += bytespp;
			if (1==run_length) {
				raw = !succ_eq;
			}
			if (raw && succ_eq) {
				run_length--;
				break;
			}
			if (!raw && !succ_eq) {
				break;
			}
			run_length++;
		}
		curpix += run_length;
		out.put(raw?run_length-1:run_length+127);
		if (!out.good()) {
			std::cerr << "can't dump the tga file\n";
			return false;
		}
		out.write((char *)(data.data()+chunkstart), (raw?run_length*bytespp:bytespp));
		if (!out.good()) {
			std::cerr << "can't dump the tga file\n";
			return false;
		}
	}
	return true;
}

TGAColor TGAImage::get(int x, int y) {
	if (data.empty() || x<0 || y<0 || x>=width || y>=height) {
		return TGAColor();
	}
	return TGAColor(data.data()+(x+y*width)*bytespp, bytespp);
}

bool TGAImage::set(int x, int y, const TGAColor& c) {
	if (data.empty() || x<0 || y<0 || x>=width || y>=height) {
		return false;
	}
	memcpy(data.data()+(x+y*width)*bytespp, c.raw, bytespp);
	return true;
}

int TGAImage::get_bytespp() {
	return bytespp;
}

int TGAImage::get_width() {
	return width;
}

int TGAImage::get_height() {
	return height;
}

bool TGAImage::flip_horizontally() {
	if (data.empty()) return false;
	int half = width>>1;
    TGAColor c1, c2;
	for (int i=0; i<half; i++) {
		for (int j=0; j<height; j++) {
			c1 = get(i, j);
			c2 = get(width-1-i, j);
			set(i, j, c2);
			set(width-1-i, j, c1);
		}
	}
	return true;
}

bool TGAImage::flip_vertically() {
    if (data.empty()) return false;
	unsigned long bytes_per_line = width*bytespp;
    std::vector<unsigned char> line(bytes_per_line);
	int half = height>>1;
	for (int j=0; j < half; ++j) {
		unsigned long l1 = j*bytes_per_line;
		unsigned long l2 = (height-1-j)*bytes_per_line;
        std::copy(data.begin() + l1, data.begin() + l1 + bytes_per_line, line.begin());
        std::copy(data.begin() + l2, data.begin() + l2 + bytes_per_line, data.begin() + l1);
        std::copy(line.begin(), line.begin() + bytes_per_line, data.begin() + l2);
	}
	return true;
}

unsigned char *TGAImage::buffer() {
	return data.data();
}

void TGAImage::clear() {
	data.clear();
}

bool TGAImage::scale(int w, int h) {
	if (w<=0 || h<=0 || data.empty()) return false;
    TGAImage tmp(w, h, bytespp);
	int nscanline = 0;
	int oscanline = 0;
	int erry = 0;
	unsigned long nlinebytes = w*bytespp;
	unsigned long olinebytes = width*bytespp;
	for (int j=0; j<height; j++) {
		int errx = width-w;
		int nx   = -bytespp;
		int ox   = -bytespp;
		for (int i=0; i<width; i++) {
			ox += bytespp;
			errx += w;
			while (errx>=(int)width) {
				errx -= width;
				nx += bytespp;
                std::copy(tmp.data.begin()+ oscanline + ox, tmp.data.begin() + oscanline + ox + bytespp, tmp.data.begin() + nscanline + nx);
			}
		}
		erry += h;
		oscanline += olinebytes;
		while (erry>=(int)height) {
			if (erry>=(int)height<<1) // it means we jump over a scanline
                std::copy(tmp.data.begin() + nscanline, tmp.data.begin() + nscanline + nlinebytes, tmp.data.begin()+nscanline+nlinebytes);
			erry -= height;
			nscanline += nlinebytes;
		}
	}
    swap(tmp);
	return true;
}