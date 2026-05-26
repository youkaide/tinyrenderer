#include "tgaimage.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(255, 128,  64, 255);
const TGAColor yellow = TGAColor(0, 200, 255, 255);

void line(int xOri, int yOri, int xEnd, int yEnd, TGAImage& image,TGAColor color)
{
	if(std::fabs)

	if (xOri > xEnd)
	{
		std::swap(xOri, xEnd);
		std::swap(yOri, yEnd);
	}

	for (float x = xOri; x <= xEnd; x += 1.)
	{
		float t = (float)(x - xOri) / (float)(xEnd - xOri);
		float y = yOri + t * (yEnd - yOri);
		image.set(x, y, color);
	}
}

int main(int argc, char** argv) 
{
	constexpr int width = 64;
	constexpr int height = 64;
	TGAImage image(width, height, TGAImage::RGB);

	int ax = 7, ay = 3;
	int bx = 12, by = 37;
	int cx = 62, cy = 53;

	line(ax, ay, bx, by, image, blue);
	line(cx, cy, bx, by, image, green);
	line(cx, cy, ax, ay, image, yellow);
	line(ax, ay, cx, cy, image, red);

	image.set(ax, ay, white);
	image.set(bx, by, white);
	image.set(cx, cy, white);

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");
	return 0;
}

