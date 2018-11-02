#include "foo.hpp"
#include "turtle.hpp"



int Foo::add(int x)
{
	Turtle turtle;
	return x + turtle.turtle_draw();
}