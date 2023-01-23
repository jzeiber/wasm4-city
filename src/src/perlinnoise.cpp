#include "perlinnoise.h"
#include "randommt.h"
#include "wasmmath.h"

PerlinNoise::PerlinNoise()
{
	for(int i=0; i<256; i++)
	{
		m_perm[i]=i;
		m_dx[i]=0;
		m_dy[i]=0;
	}
}

PerlinNoise::~PerlinNoise()
{

}

PerlinNoise &PerlinNoise::Instance()
{
	static PerlinNoise p;
	return p;
}

void PerlinNoise::Setup(const uint64_t seed)
{
	RandomMT rand;
	rand.Seed(seed);
	for(int i=0; i<256; i++)
	{
		m_perm[i]=i;
	}

	// shuffle elements
	for(int i=256-1; i>0; i--)
	{
		int j=rand.Next()%(i+1);
		int16_t temp=m_perm[j];
		m_perm[j]=m_perm[i];
		m_perm[i]=temp;
	}

	// calculate dirs
	for(int i=0; i<256; i++)
	{		 
		m_dx[i]=_cos(static_cast<double>(m_perm[i])*2.0*M_PI/256.0);
		m_dy[i]=_sin(static_cast<double>(m_perm[i])*2.0*M_PI/256.0);
	}
}

double PerlinNoise::Surflet(const double x, const double y, const int64_t per, int64_t gridx, int64_t gridy) const
{
	const double dgridx=static_cast<double>(gridx);
	const double dgridy=static_cast<double>(gridy);
	//double distx=_dabs(x-static_cast<double>(gridx));
	//double disty=_dabs(y-static_cast<double>(gridy));
	const double distx=_dabs(x-dgridx);
	const double disty=_dabs(y-dgridy);
	const double polyx=1.0-(6.0*_pow(distx,5))+(15.0*_pow(distx,4))-(10.0*_pow(distx,3));
	const double polyy=1.0-(6.0*_pow(disty,5))+(15.0*_pow(disty,4))-(10.0*_pow(disty,3));
	const int16_t hashed=m_perm[(m_perm[(gridx%per)%256ULL]+(gridy%per))%256ULL];
	const double grad=(x-dgridx)*m_dx[hashed]+(y-dgridy)*m_dy[hashed];
	return polyx*polyy*grad;
}

double PerlinNoise::Noise(double x, double y, const int64_t per) const
{
/*
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
			*/
	//trace("PerlinNoise::Noise start");
	/*
	auto surflet=[&x,&y,&per,*this,&debug](int64_t gridx, int64_t gridy) -> double 
	{ 
		const double dgridx=static_cast<double>(gridx);
		const double dgridy=static_cast<double>(gridy);
		//double distx=_dabs(x-static_cast<double>(gridx));
		//double disty=_dabs(y-static_cast<double>(gridy));
		const double distx=_dabs(x-dgridx);
		const double disty=_dabs(y-dgridy);
		const double polyx=1.0-(6.0*_pow(distx,5))+(15.0*_pow(distx,4))-(10.0*_pow(distx,3));
		const double polyy=1.0-(6.0*_pow(disty,5))+(15.0*_pow(disty,4))-(10.0*_pow(disty,3));
		if(debug)
		{
			//tracef("before hashed per=%d gx=%d gy=%d",per,gridx,gridy);
			//trace("debug");
			snprintf(global::buff,global::buffsize-1,"dx=%f dy=%f px=%f py=%f",distx,disty,polyx,polyy);
			global::buff[global::buffsize-1]='\0';
			trace(global::buff);
		}
		const int16_t hashed=m_perm[(m_perm[(gridx%per)%256ULL]+(gridy%per))%256ULL];
		if(debug)
		{
			//tracef("after hashed %d",hashed);
		}
		const double grad=(x-dgridx)*m_dx[hashed]+(y-dgridy)*m_dy[hashed];
		return polyx*polyy*grad;
	};
	*/
	
	const int64_t intx=static_cast<int64_t>(x);
	const int64_t inty=static_cast<int64_t>(y);
	//trace("PerlinNoise::Noise returning");
	//return surflet(intx+0,inty+0)+surflet(intx+1,inty+0)+surflet(intx+0,inty+1)+surflet(intx+1,inty+1);
	return Surflet(x,y,per,intx+0,inty+0)+Surflet(x,y,per,intx+1,inty+0)+Surflet(x,y,per,intx+0,inty+1)+Surflet(x,y,per,intx+1,inty+1);
}

double PerlinNoise::Get(const double x, const double y, const int64_t per, const int32_t oct) const
{
/*
	    val = 0
    for o in range(octs):
        val += 0.5**o * noise(x*2**o, y*2**o, per*2**o)
    return val
	*/

	double val=0;
	for(int i=0; i<oct; i++)
	{
		//trace("PerlinNoise::Get before");
		val+=_pow(0.5,i)*Noise(static_cast<double>(x)*_pow(2.0,i),static_cast<double>(y)*_pow(2.0,i),per*_pow(2.0,i));
		//trace("PerlinNoise::Get after");
	}
	return val;
}
