#include<cmath>
#include<tuple>

#include "model.h"
#include "tgaimage.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(255, 128,  64, 255);
const TGAColor yellow = TGAColor(0, 200, 255, 255);

constexpr int width = 128;
constexpr int height = 128;

void line(int xOri, int yOri, int xEnd, int yEnd, TGAImage& image,TGAColor color)
{
	bool steep = std::abs(xEnd - xOri) < std::abs(yEnd - yOri);

	if (steep)
	{
		std::swap(xOri, yOri);
		std::swap(xEnd, yEnd);
	}

	if (xOri > xEnd)
	{
		std::swap(xOri, xEnd);
		std::swap(yOri, yEnd);
	}

	int y = yOri;
	int iError = 0;

	for (int x = xOri; x <= xEnd; x += 1)
	{
		if (steep)
		{
			image.set(y, x, color);
		}
		else
		{
			image.set(x, y, color);
		}
		iError += 2 * std::abs(yEnd - yOri);
		if (iError > (xEnd - xOri))
		{
			y += yOri > yEnd ? -1 : 1;
			iError -= 2 * (xEnd - xOri);
		}
	}
}


void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage& image, TGAColor color)
{
	if (ay < by)
	{
		std::swap(ay, by);
		std::swap(ax, bx);
	}
	if (ay < cy)
	{
		std::swap(ay, cy);
		std::swap(ax, cx);
	}
	if (by < cy)
	{
		std::swap(by, cy);
		std::swap(bx, cx);
	}

	int totalHeight = ay - cy;
	if (by != ay)
	{
		int segmentHeight = ay - by;
		for (int y = by; y <= ay; ++y)
		{
			int x1 = cx + ((ax - cx) * (y - cy)) / totalHeight;
			int x2 = bx + ((ax - bx) * (y - by)) / segmentHeight;
			for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x)
			{
				image.set(x, y, color);
			}
		}
	}

	if (cy != by)
	{
		int segmentHeight = by - cy;
		for (int y = cy; y <= by; ++y)
		{
			int x1 = cx + ((ax - cx) * (y - cy)) / totalHeight;
			int x2 = cx + ((bx - cx) * (y - cy)) / segmentHeight;
			for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x)
			{
				image.set(x, y, color);
			}
		}
	}
}


std::tuple<int, int> project(vec3 v)
{
	return { (v.x + 1.) * width / 2,
		(v.y + 1.) * height / 2 };
}

int main(int argc, char** argv) 
{
	TGAImage image(width, height, TGAImage::RGB);

	triangle(7, 45, 35, 100, 45, 60, image, red);
	triangle(120, 35, 90, 5, 45, 110, image, white);
	triangle(115, 83, 80, 90, 85, 120, image, green);
	

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");
	return 0;
}

