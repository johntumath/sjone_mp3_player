#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

using namespace cgreen;

// The file under test
#include "gps_parse.h"



Ensure(test_get_long)
{
    gps_handle_string("123.123,456.789");
    assert_equal(get_long(), 123.123);
}

TestSuite *gps_test_suite()
{
    TestSuite *suite = create_test_suite();
    add_test(suite, test_get_long);
    return suite;
}
