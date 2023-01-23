#pragma once

#include <stdint.h>

/*
import random
import math
from PIL import Image

perm = range(256)
random.shuffle(perm)
perm += perm
dirs = [(math.cos(a * 2.0 * math.pi / 256),
         math.sin(a * 2.0 * math.pi / 256))
         for a in range(256)]

def noise(x, y, per):
    def surflet(gridX, gridY):
        distX, distY = abs(x-gridX), abs(y-gridY)
        polyX = 1 - 6*distX**5 + 15*distX**4 - 10*distX**3
        polyY = 1 - 6*distY**5 + 15*distY**4 - 10*distY**3
        hashed = perm[perm[int(gridX)%per] + int(gridY)%per]
        grad = (x-gridX)*dirs[hashed][0] + (y-gridY)*dirs[hashed][1]
        return polyX * polyY * grad
    intX, intY = int(x), int(y)
    return (surflet(intX+0, intY+0) + surflet(intX+1, intY+0) +
            surflet(intX+0, intY+1) + surflet(intX+1, intY+1))
			
def fBm(x, y, per, octs):
    val = 0
    for o in range(octs):
        val += 0.5**o * noise(x*2**o, y*2**o, per*2**o)
    return val
	
size, freq, octs, data = 128, 1/32.0, 5, []
for y in range(size):
    for x in range(size):
        data.append(fBm(x*freq, y*freq, int(size*freq), octs))
im = Image.new("L", (size, size))
im.putdata(data, 128, 128)
im.save("noise.png")
*/

class PerlinNoise
{
public:
	PerlinNoise();
	~PerlinNoise();
	
	static PerlinNoise &Instance();
	
	void Setup(const uint64_t seed);
	
	double Get(const double x, const double y, const int64_t per, const int32_t oct) const;
	
private:
	int16_t m_perm[256];
	float m_dx[256];
	float m_dy[256];
	
	double Surflet(const double x, const double y, const int64_t per, int64_t gridx, int64_t gridy) const;
	
	double Noise(double x, double y, const int64_t per) const;
	
	
};