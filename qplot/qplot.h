#pragma once

#include <unordered_map>
#include <tuple>
#include <type_traits>
#include <variant>
#include <any>
#include <ostream>

#include "process.h"
#include "util.h"

// todo
// #include "examples/styles.h"

// These non-members are called for every object to plot, along with the it's currently associated style
// Overload to completely govern how this type is plotted

// Plotting styles associated to objects

template <typename P>
class Qplot;

template <typename P, typename Style, typename T,
          std::enable_if_t<!has_plot_member<Style, void(Qplot<P>&,const T&)>::value, int> = 0>
void plotObject(Qplot<P>& process, const Style& style, const T& obj)
{
    // Do nothing where no plot member exists for this Process<P>
}

// Note that this will be instantiated for all style variants associated with the obj type (regardless of the actual obj type)
template <typename P, typename Style, typename T,
          std::enable_if_t<has_plot_member<Style, void(Qplot<P>&,const T&)>::value, int> = 0>
void plotObject(Qplot<P>& process, const Style& style, const T& obj)
{
    style.plot(process, obj);
}

// Plotting styles with no associated object

template <typename P, typename Style,
          std::enable_if_t<!has_plot_member<Style, void(Qplot<P>&)>::value, int> = 0>
void plotStyle(Qplot<P>& process, const Style& style)
{
    // Do nothing where no plot member exists for this Process<P>
}

template <typename P, typename Style,
          std::enable_if_t<has_plot_member<Style, void(Qplot<P>&)>::value, int> = 0>
void plotStyle(Qplot<P>& process, const Style& style)
{
    style.plot(process);
}


using PlotStyles = std::unordered_map<size_t, std::any>;
// using PlotStyles = std::array<std::any, MAX_STYLES>;

// Specializations in client code
template <typename T> struct plot_traits;

template <typename P>
class Qplot
{
    std::unique_ptr<Process<P>> process_ = nullptr;
    std::ostringstream header_;

public:
    template <typename... Ts>
	Qplot(Ts&&... args) : process_(std::make_unique<Process<P>>())
    {
        // Everything to cfout goes to our local ostringstream
        auto cout_buffer = process_->cfout().rdbuf();
        process_->cfout().rdbuf(header_.rdbuf());

        // Capture in local header_ buffer
        processArgs(args...);

        // Swap back
        process_->cfout().rdbuf(cout_buffer);
    }

    PlotStyles plotStyles_;

    void processArgs() {}

	// Arg is style (for an object)
    template <typename T, typename... Ts,
              std::enable_if_t<is_style<T>::value &&
                               has_supported_types<T>::value, int> = 0>
    void processArgs(const T& style, const Ts&... args)
    {
		// Nothing to plot now, but update our plotStyles_ with this style where variants that support it
		using ObjTypes = typename T::supported_types;
        constexpr size_t NumObjects = std::tuple_size_v<ObjTypes>;
		MapUpdater<T, PlotStyles, ObjTypes, NumObjects>::update(plotStyles_, style);
	
		processArgs(args...);
    }

	// Arg is a style (for the canvas)
    template <typename T, typename... Ts,
              std::enable_if_t<is_style<T>::value &&
                               !has_supported_types<T>::value, int> = 0>
    void processArgs(const T& style, const Ts&... args)
    {
        plotStyle(*this, style);

		processArgs(args...);
    }

	// Arg is an object to plot
    template <typename T, typename... Ts,
              std::enable_if_t<!is_style<T>::value, int> = 0>
    void processArgs(const T& obj, const Ts&... args)
    {
		// Get the style variant that's currently associated with the arg type T
        size_t key = plot_traits<T>::type::id;
        using StyleVariant = typename plot_traits<T>::type::supported_styles;
		auto styleVar = std::any_cast<StyleVariant>(plotStyles_[key]);

		// Plot this object
		std::visit([this,&obj](auto&& arg) {
			plotObject(*this, arg, obj);
		}, styleVar);

		processArgs(args...);
    }

    template <typename... Ts>
	void plot(const Ts&... args)
	{
        // Process header
        *this << header_.str();

        // Recuse into arguments
		processArgs(args...);

        // Finally, close this process and reopen with a fresh instance
        // + This ends out code stream to the process, which e.g. for python alows the process to start executin
        // + Also important that each call to plot (aside from the intentional header) is stateless
        process_.reset();  // destory first
        process_ = std::make_unique<Process<P>>();
	}

    // cf_ostream& cfout() { return *cfout_; }
    fd_ostream& fdout() { return process_->fdout(); }

    int fd_r() { return process_->fd_r(); }
    int fd_w() { return process_->fd_w(); }

    template <typename U>
    friend Qplot& operator<<(Qplot& qplot, const U& str)
    {
        *qplot.process_ << str;
        return qplot;
    }
};

// Main convenience method for one-liner plotting
// template <typename... Ts>
// void qplot(const Ts&... args)
// {
//     static Qplot qp;
//     qp.plot(args...);
// }