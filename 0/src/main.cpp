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

void fullTriangle(int minX, int minY, int midX, int midY, int maxX, int maxY, TGAImage& image, TGAColor color)
{
	if (minY == maxY) return;

	for (int y = maxY; y >= minY; --y)
	{
		float t1 = (float)(y - minY) / (maxY - minY);
		int x1 = minX + (maxX - minX) * t1;

		int x2 = x1; 

		if (y > midY)
		{
			if (maxY != midY)
			{
				float t2 = (float)(y - midY) / (maxY - midY);
				x2 = midX + (maxX - midX) * t2;
			}
		}
		else
		{
			if (midY != minY) 
			{
				float t3 = (float)(y - minY) / (midY - minY);
				x2 = minX + (midX - minX) * t3;
			}
		}

		line(x1, y, x2, y, image, color);
	}
}

void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage& image, TGAColor color)
{
	int minX = ax;
	int minY = ay;
	int midX = bx;
	int midY = by;
	int maxX = cx;
	int maxY = cy;
	if (maxY < midY)
	{
		std::swap(maxY, midY);
		std::swap(maxX, midX);
	}
	if (maxY < minY)
	{
		std::swap(maxY, minY);
		std::swap(maxX, minX);
	}
	if (midY < minY)
	{
		std::swap(midY, minY);
		std::swap(midX, minX);
	}
	fullTriangle(minX, minY, midX, midY, maxX, maxY, image, color);
	line(ax, ay, bx, by, image, color);
	line(bx, by, cx, cy, image, color);
	line(cx, cy, ax, ay, image, color);
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

