#include <vector>
#include <tuple>
#include <fstream>

#include "qplot/qplot.h"

using namespace std;

constexpr int NI = 20;
constexpr int NJ = 20;

using Array2d = std::array<std::array<int, NJ>, NI>;

void sendData(Subprocess<Gnuplot>& gnuplot, const Array2d& arr)
{
    for (int i=0; i<arr.size(); i++)
    {
        for (int j=0; j<arr[0].size(); j++)
        {
            gnuplot << i << " " << j << " " << arr[i][j] << '\n';
        }
    }
    gnuplot << "e\n";  // 'e' is Gnuplot's terminating character
}

struct HeatMap
{
	using supported_types = std::tuple<Array2d>;

	template<typename T>
	void operator()(Subprocess<Gnuplot>& gnuplot, const T& obj) const
	{
    	gnuplot <<  "plot '-' using 1:2:3 with image\n";
        sendData(gnuplot, obj);
	}
};

struct NumberGrid
{
	using supported_types = std::tuple<Array2d>;

	template<typename T>
	void operator()(Subprocess<Gnuplot>& gnuplot, const T& obj) const
	{
    	gnuplot <<  "plot '-' using 1:2:3 with image"
    	        <<     ", '-' using 1:2:3 with labels font \"PTMono,8\""
                << '\n';

        // Gnuplot requires the data is resent for additional plots
        sendData(gnuplot, obj);
    	sendData(gnuplot, obj);
	}
};

struct ContourPlot
{
    using supported_types = std::tuple<Array2d>;

    template<typename T>
    void operator()(Subprocess<Gnuplot>& gnuplot, const T& obj) const
    {
        gnuplot << "set dgrid3d " << NI << ", " << NJ << "\n"
                << "set contour surface\n"
                << "splot '-' using 1:2:3 with lines linetype 2 linewidth 1\n";

        sendData(gnuplot, obj);

        gnuplot << "unset contour\n"
                << "unset dgrid3d\n";
    }
};

struct Header
{
    void operator()(Subprocess<Gnuplot>& gnuplot) const {
        gnuplot << "set terminal png size 640, 480\n"
                << "set output 'output.png'\n";
    }
};

struct Filename
{
    void operator()(Subprocess<Gnuplot>& gnuplot) const {
        gnuplot << "set output '" << filename << ".png'\n";
    }
    std::string filename = "output.png";
};

struct ImageSize
{
    void operator()(Subprocess<Gnuplot>& gnuplot) const {
        gnuplot << "set terminal png size " << size_x << ", " << size_y << "\n";
    }
    unsigned size_x = 640;
    unsigned size_y = 480;
};

struct Colours
{
    enum Palette {
        OCEAN,
        RAINBOW,
        HOT
    };

    Colours(Palette palette) : palette(palette) {}

    void operator()(Subprocess<Gnuplot>& gnuplot) const {
        switch (palette)
        {
            case OCEAN:    gnuplot << "set palette rgbformulae 23,28,3 \n";  break;
            case RAINBOW:  gnuplot << "set palette rgbformulae 33,13,10\n";  break;
            case HOT:      gnuplot << "set palette rgbformulae 21,22,23\n";  break;
        }
    }

    Palette palette;
};

// id must be unique (so here, just within this translation unit)
// struct Scalar2d { static constexpr size_t id = 0; using supported_styles = std::variant<HeatMap, NumberGrid, ContourPlot>; };

// template <> struct associated_styles<Array2d> { using type = Scalar2d; };

using Scalar2D = std::variant<HeatMap, NumberGrid, ContourPlot>;

template <> struct Styles<void> { using tuple = std::tuple<Scalar2D>; tuple t; };

template <> struct style_variant<Array2d> { using type = Scalar2D; };

void example_gnuplot()
{
	Array2d array;
	for (int i=0; i<array.size(); i++)
    {
    	for (int j=0; j<array[0].size(); j++)
  		{
			array[i][j] = pow(i-NI/2.f,2) - pow(j-NJ/2.f,2);
    	}
    }

	Qplot<Gnuplot> qplot(Header{}, HeatMap{}, Colours{Colours::RAINBOW});
    qplot.plot(Filename{"Grid1"}, array);
    qplot.plot(Filename{"Grid2"}, ContourPlot{}, array);
    qplot.plot(Filename{"Grid3"}, ImageSize{800,600}, array);     // Same as Grid1, but larger (todo: broken!)

    qplot.addToHeader(Colours{Colours::OCEAN});
    qplot.plot(array);
}
