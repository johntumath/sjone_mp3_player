#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

// Include for C++
using namespace cgreen;

// The file under test
#include "foo.hpp"

// Files we need to include
#include "turtle.hpp"



int Turtle::turtle_draw(void) { return mock(); }

Ensure(testing_call_to_turtle)
{
    Foo foo;
    
    expect(turtle_draw, will_return(20));
    assert_that(foo.add(1), is_equal_to(21));
}

TestSuite *foo_suite()
{
    TestSuite *suite = create_test_suite();
    add_test(suite, testing_call_to_turtle);
    return suite;
}
