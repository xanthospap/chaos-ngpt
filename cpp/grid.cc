#ifndef __NGPT_GRID_HPP__
#define __NGPT_GRID_HPP__

#include <cstddef>
#include <utility>
#ifdef RANGE_CHECK
#include <cassert>
#include <limits>
#endif
#ifdef DEBUG
#include <iostream>
#include <iomanip>
#include <random>
#endif

namespace ngpt
{

/// The tick_axis is inlusive! Given e.g. the range [90, -90], both 90 and -90
/// will be valid nodes.
template<class T>
    class tick_axis
{
public:

    /// Constructor. Set start, stop and step. This can never fail (i.e. throw)
    /// but the given parameters could be wrong. Use the tick_axis::validate
    /// method to check if the tick_axis is constructed correctly.
    explicit
    tick_axis(T start, T stop, T step = T{1}) noexcept
    : _start{start},
      _stop{stop},
      _step{step}
    {}

    /// Validate that this tick_axis is actualy valid (i.e. its parameters are
    /// set correctly). The function will return false if something is wrong.
    bool
    validate() const noexcept
    { return static_cast<long>((_stop-_start)/_step) >= 0; }

    /// Check if the tick_axis is in ascending order.
    bool
    is_ascending() const noexcept
    { return _stop > _start; }

    /// Return the number of ticks on the axis.
    std::size_t
    num_pts() const noexcept
    { return static_cast<std::size_t>((_stop - _start) / _step) + 1; }

    /// Given a value, return the index of the nearest **left** axis tick.
    std::size_t
    index(T val) const noexcept
    { return static_cast<std::size_t>((val-_start)/_step); }

    /// Given a value, return the index of the nearest axis tick.
    std::size_t
    nearest_neighbor(T val) const noexcept
    {
        std::size_t idx { this->index(val) };
        // Note: the division should always be with a float/double 2; just
        //       imagine the case where the template parameter T is int!
        return  std::abs(val-this->val_at_index(idx)) > std::abs(_step)/2e0
                ? ( (idx == num_pts()-1) ? idx : ++idx )
                : idx;
    }
    
    /// Given an index of a tick, return it's value.
    T
    val_at_index(std::size_t idx) const
    {
#ifdef RANGE_CHECK
        if ( idx >= num_pts() ) {
            throw std::out_of_range("tick_axis::val_at_index() invalid index.");
        }
#endif
        return _start + idx*_step;
    }

    /// Given an index of a tick, return it's value. No check is performed on
    /// on the validity of the given index (meaning it could be out of range)
    T
    operator()(std::size_t idx) const noexcept
    { return _start + idx * _step; }

    /// Maximum value on the tick_axis
    T
    max_val() const noexcept
    { return this->is_ascending() ? _stop : _start; }

    /// Minimum value on the tick_axis
    T
    min_val() const noexcept
    { return this->is_ascending() ? _start : _stop; }

    /// The (value) of the starting (i.e. leftmost) tick.
    T
    start() const noexcept { return _start; }

    /// The (value) of the ending (i.e. rightmost) tick.
    T
    stop() const noexcept { return _stop; }

    /// The step value.
    T
    step() const noexcept { return _step; }

private:
    T _start,
      _stop,
      _step;
}; // class tick_axis

enum class storage_order : char { col_wise, row_wise };

// forward decleration
template<typename T, storage_order S> class gnode;

template<typename T, storage_order S>
    class two_dim_grid
{
public:
    using grid_node  = std::pair<std::size_t, std::size_t>;
    using grid_value = std::pair<T, T>;

    explicit
    two_dim_grid(T xstart, T xstop, T xstep, T ystart, T ystop, T ystep)
    noexcept
    : _xaxis{xstart, xstop, xstep},
      _yaxis{ystart, ystop, ystep}
    {}

    std::size_t
    num_pts() const noexcept
    { return _xaxis.num_pts() * _yaxis.num_pts(); }

    std::size_t
    x_pts() const noexcept { return _xaxis.num_pts(); }
    
    std::size_t
    y_pts() const noexcept { return _yaxis.num_pts(); }

    grid_value
    val_at_index(const grid_node& node) const
    {
        return grid_value{_xaxis.val_at_index(node.first),
                          _yaxis.val_at_index(node.second)};
    }

    grid_value
    operator()(const grid_node& node) const noexcept
    {
        return grid_value{_xaxis(node.first), _yaxis(node.second)};
    }

    grid_value
    val_at_index(std::size_t xi, std::size_t yi) const
    { return val_at_index(grid_node{xi, yi}); }
    
    grid_value
    operator()(std::size_t xi, std::size_t yi) const noexcept
    { return this->operator()(grid_node{xi, yi}); }

    grid_node
    nearest_neighbor(T x, T y) const noexcept
    {
        auto n = grid_node{_xaxis.nearest_neighbor(x),
                           _yaxis.nearest_neighbor(y)};
        return n;
    }

    gnode<T,S>
    begin() const noexcept
    {
        return gnode<T,S>{0, 0, this};
    }

    gnode<T,S>
    end() const noexcept
    {
        return gnode<T,S>{_xaxis.num_pts(), _yaxis.num_pts(), this};
    }

    /// last (valid) node on row (i.e. y-axis row) with index yi.
    gnode<T,S>
    end_of_row(std::size_t yi) const noexcept
    { return gnode<T,S>{_xaxis.num_pts(), yi, this}; }
    
    /// first (valid) node on row (i.e. y-axis row) with index yi.
    gnode<T,S>
    start_of_row(std::size_t yi) const noexcept
    { return gnode<T,S>{0, yi, this}; }
    
    /// last (valid) node on column (i.e. x-axis row) with index xi.
    gnode<T,S>
    end_of_col(std::size_t xi) const noexcept
    { return gnode<T,S>{xi, _yaxis.num_pts(), this}; }
    
    /// first (valid) node on column (i.e. x-axis row) with index xi.
    gnode<T,S>
    start_of_col(std::size_t xi) const noexcept
    { return gnode<T,S>{xi, 0, this}; }

private:
    tick_axis<T> _xaxis, _yaxis;

    std::size_t
    node2index_cw(const grid_node& node) const noexcept
    {
        auto x = node.first;
        auto y = node.second;
        return _yaxis.num_pts()*x + y;
    }

    std::size_t
    node2index_rw(const grid_node& node) const noexcept
    {
        auto x = node.first;
        auto y = node.second;
        return _xaxis.num_pts()*y + x;
    }
};// class two_dim_grid

template<typename T, storage_order S>
    class inode
{
public:
    inode() noexcept : _index(0), _grid(nullptr) {};
    explicit
    inode(const two_dim_grid<T,S>* t) noexcept : _index{0}, _grid{t} {};
    void
    attach_to_grid(const two_dim_grid<T,S>* t) noexcept { _grid = t; }

private:
    std::size_t              _index;
    const two_dim_grid<T,S>* _grid; ///< ptr to the actual grid
}; //class inode

template<typename T, storage_order S>
    class gnode
{
public:

    /// Default Constructor; the created gnode is hanging(!), i.e. it does not
    /// have an underlying grid.
    gnode() noexcept
    : _x{0},
      _y{0},
      _grid{nullptr}
    {};
    
    /// Constructor using a valid grid; the gnode will point to the first node
    /// of the grid.
    explicit
    gnode(const two_dim_grid<T,S>* t) noexcept
    : _x{0},
      _y{0},
      _grid{t}
    {};
    
    /// Constructor using a valid grid and axis indexes.
    explicit
    gnode(std::size_t xi, std::size_t yi, const two_dim_grid<T,S>* t) noexcept
    : _x{xi},
      _y{yi},
      _grid{t}
    {};

    /// Equality operator; two gnodes are the same if they are in the same grid
    /// and point to the same node.
    bool
    operator==(const gnode& node) const noexcept
    {
        return _grid == node._grid && (_x == node._x && _y == node._y);
    }

    /// Inequality operator. See gnode::operator==()
    bool
    operator!=(const gnode& node) const noexcept
    { return !(this->operator==(node)); }

    /// Return the next node; there are a number of different options here,
    /// depending on the current (i.e. calling) node:
    /// if the calling node is the last on the grid, then grid::end() is
    /// returned else, the next node is returned (which may be in a different
    /// column or/and row).
    gnode
    next() const noexcept
    {
        if ( _x == _grid->x_pts() ) {
            if ( _y == _grid->y_pts() ) {
                return _grid->end();
            }
            return _grid->start_of_row(this->_y + 1);
        }
        return gnode {_x+1, _y, _grid};
    }

    gnode
    next_right() const noexcept
    {
        if ( this->is_end_of_row() ) {
            if ( this->is_end_of_col() ) {
                return _grid->end();
            }
            return _grid->end_of_row(this->_y);
        }
        return gnode{_x+1, _y, _grid};
    }

    gnode
    next_up() const noexcept
    {

    }

private:
    std::size_t              _x,    ///< index of x axis
                             _y;    ///< index of y axis
    const two_dim_grid<T,S>* _grid; ///< ptr to the actual grid

    bool
    is_end_of_row() const noexcept { return _x >= _grid->x_pts(); }

    bool
    is_end_of_col() const noexcept { return _y >= _grid->y_pts(); }

    gnode
    next_on_right_colwise() const noexcept
    {
        gnode r {*this};
        ++r._x;
        return r;
    }
    
    gnode
    on_right_rowwise() const noexcept
    {
        gnode r {*this};
        r._x += 1;
        return r;
    }
};// class gnode

}

using ngpt::tick_axis;

int main()
{
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 eng(rd()); // seed the generator
    std::uniform_real_distribution<double> distr; // define the distribution
    std::cout << std::fixed << std::setfill('0') << std::setprecision(3);
    double t;
    
    std::cout<<"\nGrid from -90 to 90, with step = -5. (INVALID!)";
    tick_axis<double> te1(-90, 90, -5);
    std::cout<<"\n\tGrid is valid? "<< std::boolalpha << te1.validate();
    std::cout<<"\nGrid from 90 to -90, with step = 5. (INVALID!)";
    tick_axis<double> te2(90, -90, 5);
    std::cout<<"\n\tGrid is valid? "<< std::boolalpha << te2.validate();
    std::cout<<"\nGrid from 90 to 180, with step = -5. (INVALID!)";
    tick_axis<double> te3(90, 180, -5);
    std::cout<<"\n\tGrid is valid? "<< std::boolalpha << te3.validate();

    std::cout<<"\nGrid from -90 to 90, with step = 5.";
    tick_axis<double> t1(-90, 90, 5);
    std::cout<<"\n\tAxis is ascending?"<< std::boolalpha << t1.is_ascending();
    std::cout<<"\n\tNum of points:    "<<t1.num_pts();
    // set the range for the random generator
    std::uniform_real_distribution<double>::param_type new_range1 {-90, 90};
    distr.param(new_range1);
    for (int i=0; i<10; ++i) {
        t = distr(eng);
        std::cout<<"\n\tRanom Number "<<t<<" has index: "<<t1.index(t)
        <<" which is node: " <<t1.val_at_index(t1.index(t))
        <<" distance from node=" <<std::abs(t1.val_at_index(t1.index(t))-t)
        <<" nearest node: "<<t1.nearest_neighbor(t)<<" ("<<t1.val_at_index(t1.nearest_neighbor(t))<<")";
    }
    t = -90e0;
    std::cout<<"\n\tRanom Number "<<t<<" has index: "<<t1.index(t)
    <<" which is node: " <<t1.val_at_index(t1.index(t))
    <<" distance from node=" <<std::abs(t1.val_at_index(t1.index(t))-t)
    <<" nearest node: "<<t1.nearest_neighbor(t)<<" ("<<t1.val_at_index(t1.nearest_neighbor(t))<<")";
    t = 90e0;
    std::cout<<"\n\tRanom Number "<<t<<" has index: "<<t1.index(t)
    <<" which is node: " <<t1.val_at_index(t1.index(t))
    <<" distance from node=" <<std::abs(t1.val_at_index(t1.index(t))-t)
    <<" nearest node: "<<t1.nearest_neighbor(t)<<" ("<<t1.val_at_index(t1.nearest_neighbor(t))<<")";

    std::cout<<"\nGrid from 0 to 180, with step = 5.";
    tick_axis<double> t2(0, 180, 5);
    std::cout<<"\n\tAxis is ascending?"<< std::boolalpha << t2.is_ascending();
    std::cout<<"\n\tNum of points: "<<t2.num_pts();
    // set the range for the random generator
    std::uniform_real_distribution<double>::param_type new_range2 {0, 180};
    distr.param(new_range2);
    for (int i=0; i<10; ++i) {
        t = distr(eng);
        std::cout<<"\n\tRanom Number "<<t<<" has index: "<<t2.index(t)
        <<" which is node: " <<t2.val_at_index(t2.index(t))
        <<" distance from node=" <<std::abs(t2.val_at_index(t2.index(t))-t)
        <<" nearest node: "<<t2.nearest_neighbor(t)<<" ("<<t2.val_at_index(t2.nearest_neighbor(t))<<")";
    }
    t = 0e0;
    std::cout<<"\n\tRanom Number "<<t<<" has index: "<<t2.index(t)
    <<" which is node: " <<t2.val_at_index(t2.index(t))
    <<" distance from node=" <<std::abs(t2.val_at_index(t2.index(t))-t)
    <<" nearest node: "<<t2.nearest_neighbor(t)<<" ("<<t2.val_at_index(t2.nearest_neighbor(t))<<")";
    t = 180e0;
    std::cout<<"\n\tRanom Number "<<t<<" has index: "<<t2.index(t)
    <<" which is node: " <<t2.val_at_index(t2.index(t))
    <<" distance from node=" <<std::abs(t2.val_at_index(t2.index(t))-t)
    <<" nearest node: "<<t2.nearest_neighbor(t)<<" ("<<t2.val_at_index(t2.nearest_neighbor(t))<<")";
    
    std::cout<<"\nGrid from 180 to 0, with step = 5.";
    tick_axis<double> t3(180, 0, -5);
    std::cout<<"\n\tAxis is ascending?"<< std::boolalpha << t3.is_ascending();
    std::cout<<"\n\tNum of points: "<<t3.num_pts();
    // set the range for the random generator
    std::uniform_real_distribution<double>::param_type new_range3 {0, 180};
    distr.param(new_range3);
    for (int i=0; i<10; ++i) {
        t = distr(eng);
        std::cout<<"\n\tRanom Number "<<t<<" has index: "<<t3.index(t)
        <<" which is node: " <<t3.val_at_index(t3.index(t))
        <<" distance from node=" <<std::abs(t3.val_at_index(t3.index(t))-t)
        <<" nearest node: "<<t3.nearest_neighbor(t)<<" ("<<t3.val_at_index(t3.nearest_neighbor(t))<<")";
    }
    t = 180e0;
    std::cout<<"\n\tRanom Number "<<t<<" has index: "<<t3.index(t)
    <<" which is node: " <<t3.val_at_index(t3.index(t))
    <<" distance from node=" <<std::abs(t3.val_at_index(t3.index(t))-t)
    <<" nearest node: "<<t3.nearest_neighbor(t)<<" ("<<t3.val_at_index(t3.nearest_neighbor(t))<<")";
    t = 0e0;
    std::cout<<"\n\tRanom Number "<<t<<" has index: "<<t3.index(t)
    <<" which is node: " <<t3.val_at_index(t3.index(t))
    <<" distance from node=" <<std::abs(t3.val_at_index(t3.index(t))-t)
    <<" nearest node: "<<t3.nearest_neighbor(t)<<" ("<<t3.val_at_index(t3.nearest_neighbor(t))<<")";
    
    std::cout<<"\nGrid from 90 to -90, with step = 5.";
    tick_axis<double> t4(90, -90, -5);
    std::cout<<"\n\tAxis is ascending?"<< std::boolalpha << t4.is_ascending();
    std::cout<<"\n\tNum of points: "<<t4.num_pts();
    // set the range for the random generator
    std::uniform_real_distribution<double>::param_type new_range4 {90, -90};
    distr.param(new_range4);
    for (int i=0; i<10; ++i) {
        t = distr(eng);
        std::cout<<"\n\tRanom Number "<<t<<" has index: "<<t4.index(t)
        <<" which is node: " <<t4.val_at_index(t4.index(t))
        <<" distance from node=" <<std::abs(t4.val_at_index(t4.index(t))-t)
        <<" nearest node: "<<t4.nearest_neighbor(t)<<" ("<<t4.val_at_index(t4.nearest_neighbor(t))<<")";
    }
    t = 90e0;
    std::cout<<"\n\tRanom Number "<<t<<" has index: "<<t4.index(t)
    <<" which is node: " <<t4.val_at_index(t4.index(t))
    <<" distance from node=" <<std::abs(t4.val_at_index(t4.index(t))-t)
    <<" nearest node: "<<t4.nearest_neighbor(t)<<" ("<<t4.val_at_index(t4.nearest_neighbor(t))<<")";
    t = -90e0;
    std::cout<<"\n\tRanom Number "<<t<<" has index: "<<t4.index(t)
    <<" which is node: " <<t4.val_at_index(t4.index(t))
    <<" distance from node=" <<std::abs(t4.val_at_index(t4.index(t))-t)
    <<" nearest node: "<<t4.nearest_neighbor(t)<<" ("<<t4.val_at_index(t4.nearest_neighbor(t))<<")";

    float j = 90;
    std::cout<<"\n";
    for (int i=0; i<t4.num_pts(); i++) {
        std::cout<<" " << j+i*t4.step();
    }

    std::cout<<"\n";
    return 0;
}

#endif
