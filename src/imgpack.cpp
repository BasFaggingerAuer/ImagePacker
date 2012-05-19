/*
Copyright 2012, Bas Fagginger Auer.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
Directly based on
'Fast Optimizing Rectangle Packing Algorithm for Building CSS Sprites' by Matt Perdeck (http://www.codeproject.com/Articles/210979/Fast-optimizing-rectangle-packing-algorithm-for-bu).
*/
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <algorithm>

#include <cstdlib>
#include <cassert>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

using namespace std;

class ivec2
{
	public:
		ivec2();
		ivec2(const int &, const int &);
		~ivec2();
		
		bool operator < (const ivec2 &) const;
		
		int x, y;
};

ivec2::ivec2() :
	x(0),
	y(0)
{

}

ivec2::ivec2(const int &_x, const int &_y) :
	x(_x),
	y(_y)
{

}

ivec2::~ivec2()
{

}

bool ivec2::operator < (const ivec2 &a) const
{
	return (y != a.y ? y < a.y : x < a.x);
}

class Image
{
	public:
		Image(const std::string &, SDL_Surface *);
		~Image();
		
		std::string name;
		ivec2 pos, size;
		int width, height;
		SDL_Surface *data;
};

Image::Image(const string &_name, SDL_Surface *_data) :
	name(_name),
	pos(0, 0),
	size(0, 0),
	data(_data)
{
	if (data) size = ivec2(data->w, data->h);
}

Image::~Image()
{
	if (data) SDL_FreeSurface(data);
}

struct SortImages
{
	bool operator () (const Image *a, const Image *b) const
	{
		return b->size < a->size;
	};
};

bool fitImages(vector<Image *> &images, const int &outputWidth, const int &outputHeight)
{
	//Attempts to create positions for all images in the provided list within the desired rectangle.
	int totalArea = 0;
	
	for (vector<Image *>::const_iterator i = images.begin(); i != images.end(); ++i) totalArea += (*i)->size.x*(*i)->size.y;
	
	if (totalArea > outputWidth*outputHeight)
	{
		cerr << "Total area of the " << images.size() << " images equals " << totalArea << " > " << outputWidth*outputHeight << "!" << endl;
		return false;
	}
	
	cerr << "Trying to fit " << images.size() << " images with total area " << totalArea << " into " << outputWidth*outputHeight << " pixels..." << endl;
	
	//Create a map storing for each cell whether or not it is occupied.
	map<ivec2, bool> occupied;
	list<int> xBounds, yBounds;
	
	//Start with a big free cell.
	xBounds.push_back(0);
	xBounds.push_back(outputWidth);
	yBounds.push_back(0);
	yBounds.push_back(outputHeight);
	occupied[ivec2(0, 0)] = false;
	occupied[ivec2(outputWidth, 0)] = true;
	occupied[ivec2(0, outputHeight)] = true;
	occupied[ivec2(outputWidth, outputHeight)] = true;
	
	//Sort images by size.
	sort(images.begin(), images.end(), SortImages());
	
	//Subdivide cells.
	for (vector<Image *>::iterator i = images.begin(); i != images.end(); ++i)
	{
		Image * const img = *i;
		bool found = false;
		
		cerr << "\r" << 1 + (i - images.begin()) << "/" << images.size();
		cerr.flush();
		
		//Find unoccupied cells to store the image.
		for (list<int>::iterator x0 = xBounds.begin(); x0 != xBounds.end() && !found; ++x0)
		{
			for (list<int>::iterator y0 = yBounds.begin(); y0 != yBounds.end() && !found; ++y0)
			{
				const ivec2 pos0(*x0, *y0);
				
				assert(occupied.find(pos0) != occupied.end());
				
				if (!occupied[pos0])
				{
					//This cell is free, start expanding it until it encompasses the image.
					list<int>::iterator x1 = x0, y1 = y0;
					
					while (x1 != xBounds.end())
					{
						if (*x1 - *x0 < img->size.x) ++x1;
						else break;
					}
					
					while (y1 != yBounds.end())
					{
						if (*y1 - *y0 < img->size.y) ++y1;
						else break;
					}
					
					if (x1 != xBounds.end() && y1 != yBounds.end())
					{
						bool blocked = false;
						
						//Verify that all encountered cells are not occupied.
						for (list<int>::iterator x2 = x0; x2 != x1 && !blocked; ++x2)
						{
							for (list<int>::iterator y2 = y0; y2 != y1 && !blocked; ++y2)
							{
								const ivec2 pos2(*x2, *y2);
								
								assert(occupied.find(pos2) != occupied.end());
								
								if (occupied[pos2]) blocked = true;
							}
						}
						
						if (!blocked)
						{
							//We found a spot where we can put our image.
							found = true;
							img->pos = pos0;
							
							//cerr << "Stored '" << img->name << "' at " << img->pos.x << ", " << img->pos.y << "." << endl;
							
							//Split cells if required.
							if (*x1 - *x0 > img->size.x)
							{
								const int xNew = *x0 + img->size.x;
								
								xBounds.insert(x1, xNew);
								--x1;
								
								list<int>::iterator x2 = x1;
								
								--x2;
								assert(*x1 == xNew);
								assert(*x2 < xNew);
								
								for (list<int>::iterator y2 = yBounds.begin(); y2 != yBounds.end(); ++y2) occupied[ivec2(*x1, *y2)] = occupied[ivec2(*x2, *y2)];
							}
							
							if (*y1 - *y0 > img->size.y)
							{
								const int yNew = *y0 + img->size.y;
								
								yBounds.insert(y1, yNew);
								--y1;
								
								list<int>::iterator y2 = y1;
								
								--y2;
								assert(*y1 == yNew);
								assert(*y2 < yNew);
								
								for (list<int>::iterator x2 = xBounds.begin(); x2 != xBounds.end(); ++x2) occupied[ivec2(*x2, *y1)] = occupied[ivec2(*x2, *y2)];
							}
							
							//Mark all occupied cells.
							for (list<int>::iterator x2 = x0; x2 != x1; ++x2)
							{
								for (list<int>::iterator y2 = y0; y2 != y1; ++y2)
								{
									occupied[ivec2(*x2, *y2)] = true;
								}
							}
						}
					}
				}
			}
		}
		
		if (!found)
		{
			cerr << "Unable to fit '" << img->name << "' into the image!" << endl;
			return false;
		}
	}
	
	cerr << endl;
	
	return true;
}

SDL_Surface *blitImages(const vector<Image *> &images, const int &outputWidth, const int &outputHeight)
{
	//Blits all images at their specified positions.
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	const Uint32 rMask = 0xff000000, gMask = 0x00ff0000, bMask = 0x0000ff00, aMask = 0x000000ff;
#else
	const Uint32 rMask = 0x000000ff, gMask = 0x0000ff00, bMask = 0x00ff0000, aMask = 0xff000000;
#endif
	SDL_Surface *outputSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, outputWidth, outputHeight, 32, rMask, gMask, bMask, aMask);
	
	if (!outputSurface)
	{
		cerr << "Unable to create output surface: " << SDL_GetError() << "!" << endl;
		return 0;
	}
	
	//Blit all images at the correct position.
	SDL_FillRect(outputSurface, 0, 0);
	SDL_SetAlpha(outputSurface, 0, 255);
	
	for (vector<Image *>::const_iterator i = images.begin(); i != images.end(); ++i)
	{
		const Image * const img = *i;
		SDL_Rect srcRect;
		SDL_Rect dstRect;
		
		srcRect.x = 0; srcRect.y = 0; srcRect.w = img->size.x; srcRect.h = img->size.y;
		dstRect.x = img->pos.x; dstRect.y = img->pos.y; dstRect.w = img->size.x; dstRect.h = img->size.y;
		
		if (SDL_BlitSurface(img->data, &srcRect, outputSurface, &dstRect) < 0)
		{
			cerr << "Unable to blit '" << img->name << "': " << SDL_GetError() << "!" << endl;
			SDL_FreeSurface(outputSurface);
			return 0;
		}
	}
	
	return outputSurface;
}

bool saveStatistics(const vector<Image *> &images, const char *xmlFileName, const char *outputFileName, const int &outputWidth, const int &outputHeight)
{
	//Store positions and sizes of all images in an XML file.
	assert(xmlFileName);
	assert(outputFileName);
	
	ofstream file(xmlFileName);
	
	if (!file.good())
	{
		cerr << "Unable to open '" << outputFileName << "' for writing!" << endl;
		return false;
	}
	
	file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	file << "<imagemap file=\"" << outputFileName << "\" w=\"" << outputWidth << "\" h=\"" << outputHeight << "\">" << endl;
	
	for (vector<Image *>::const_iterator i = images.begin(); i != images.end(); ++i)
	{
		const Image * const img = *i;
		
		file << "\t<image name=\"" << img->name << "\" x=\"" << img->pos.x << "\" y=\"" << img->pos.y << "\" w=\"" << img->size.x << "\" h=\"" << img->size.y << "\"/>" << endl;
	}
	
	file << "</imagemap>" << endl;
	file.close();
	
	return true;
}

int main(int argc, char **argv)
{
	//Display help message if required.
	if (argc < 6)
	{
		cerr << "Usage: " << endl;
		cerr << "    To store input1.png, input2.png, ... into output.bmp: " << argv[0] << " output.xml output.bmp 1024 1024 input1.png input2.png ..." << endl;
		cerr << "    To create output.bmp containing the font input.ttf  : " << argv[0] << " output.xml output.bmp 1024 1024 input.ttf 20" << endl;
		cerr << endl;
		return -1;
	}
	
	//Do we want to create a font?
	bool createFont = false;
	int outputWidth = atoi(argv[3]), outputHeight = atoi(argv[4]);
	int fontSize = 0;
	
	if (argc == 7)
	{
		if (string(argv[5]).find(".ttf") || string(argv[5]).find(".TTF"))
		{
			createFont = true;
			fontSize = atoi(argv[6]);
		}
	}
	
	//Verify data.
	if (outputWidth <= 0 ||
		outputHeight <= 0 ||
		(createFont && fontSize <= 0))
	{
		cerr << "Invalid image dimensions or font size!" << endl;
		return -1;
	}
	
	//Initialise all libraries.
	if (SDL_Init(0) < 0)
	{
		cerr << "Unable to initialise SDL: " << SDL_GetError() << "!" << endl;
		return -1;
	}
	
	if (IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) != (IMG_INIT_JPG | IMG_INIT_PNG))
	{
		cerr << "Unable to enable SDL_image JPEG and PNG support: " << IMG_GetError() << "!" << endl;
		return -1;
	}
	
	if (createFont)
	{
		if (TTF_Init() < 0)
		{
			cerr << "Unable to initialise SDL_ttf: " << TTF_GetError() << "!" << endl;
			return -1;
		}
	}
	
	//Read/generate all images.
	vector<Image *> images;
	
	if (createFont)
	{
		cerr << "Creating font images..." << endl;
		
		TTF_Font *font = TTF_OpenFont(argv[5], fontSize);
		char text[2] = {' ', '\0'};
		SDL_Color color;
		
		color.r = 255; color.g = 255; color.b = 255;
		
		if (!font)
		{
			cerr << "Unable to read '" << argv[5] << "' as font!" << endl;
			return -1;
		}
		
		for (int i = 1; i < 256; ++i)
		{
			cerr << "\r" << i + 1 << "/" << 256;
			cerr.flush();
			
			text[0] = i;
			SDL_Surface *data = TTF_RenderText_Blended(font, text, color);
			
			if (!data)
			{
				cerr << "Unable to render character " << i << "!" << endl;
			}
			else
			{
				stringstream tmp;
				
				tmp << i;
				
				images.push_back(new Image(tmp.str(), data));
			}
		}
		
		TTF_CloseFont(font);
	}
	else
	{
		cerr << "Reading images..." << endl;
		
		for (int i = 5; i < argc; ++i)
		{
			cerr << "\r" << i - 4 << "/" << argc - 5;
			cerr.flush();
			
			SDL_Surface *data = IMG_Load(argv[i]);
			
			if (!data)
			{
				cerr << "Unable to read '" << argv[i] << "'!" << endl;
				return -1;
			}
			
			if (data->w <= 0 || data->h <= 0)
			{
				cerr << "Image '" << argv[i] << "' is contains no data!" << endl;
				return -1;
			}
			
			string name(argv[i]);
			size_t extPos = name.find_last_of(".");
			size_t dirPos = name.find_last_of("/\\", extPos);
			
			if (dirPos == string::npos)
			{
				//No subdirectory present.
				name = name.substr(0, extPos);
			}
			else
			{
				name = name.substr(dirPos + 1, extPos - dirPos - 1);
			}
			
			images.push_back(new Image(name, data));
		}
	}
	
	cerr << endl;
	
	//Fit all images into the big image.
	if (!fitImages(images, outputWidth, outputHeight)) return -1;
	
	//Create image.
	cerr << "Creating output image..." << endl;
	
	SDL_Surface *outputSurface = blitImages(images, outputWidth, outputHeight);
	
	if (!outputSurface) return -1;
	
	//Save image.
	//FIXME: SaveBMP does not store the alpha channel.
	if (SDL_SaveBMP(outputSurface, argv[2]) < 0)
	{
		cerr << "Unable to store '" << argv[2] << "': " << SDL_GetError() << "!" << endl;
		return -1;
	}
	
	//Save XML.
	if (!saveStatistics(images, argv[1], argv[2], outputWidth, outputHeight)) return -1;
	
	//Clean up.
	cerr << "Freeing data..." << endl;
	
	SDL_FreeSurface(outputSurface);
	
	for (vector<Image *>::iterator i = images.begin(); i != images.end(); ++i) delete *i;
	
	if (createFont) TTF_Quit();
	IMG_Quit();
	SDL_Quit();
	
	cerr << "Done." << endl;
	
	return 0;
}

