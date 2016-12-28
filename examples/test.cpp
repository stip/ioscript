#include <iostream>
#include <vector>

#include "qplot/qplot.h"

using namespace std;

struct CanvasStyle
{
	using supported_types = std::tuple<>;

	CanvasStyle(const std::string& filename) : filename(filename) {}

	void plot(Process<Gnuplot>& gnuplot) const
	{
        gnuplot << "set output '" << filename << ".png'\n"
                << "set terminal pngcairo size 500, 500\n";
    }
	std::string filename;
};

struct ObjectStyle
{
	using supported_types = std::tuple<std::vector<int>,std::vector<float>>;

	template<typename T>
	void plot(Process<Gnuplot>& gnuplot, const T& obj) const
	{
        gnuplot << "plot '-' using 1:2\n";
        sendData(gnuplot, obj);
        gnuplot << "e\n";
	}
};

struct DataObject
{
};


void test()
{
	assert(  is_style<CanvasStyle>::value );
	assert(  is_style<ObjectStyle>::value );
	assert( !is_style<DataObject>::value );

	assert( !has_supported_types<CanvasStyle>::value );
	assert(  has_supported_types<ObjectStyle>::value );
	assert( !has_supported_types<DataObject>::value );


}
