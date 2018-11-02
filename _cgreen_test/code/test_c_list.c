#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

// The file under test
#include "c_list.h"



Ensure(add_element_to_end)
{
    c_list_ptr list = c_list_create();
    c_list_insert_elm_end(list, (void*) 1);
    assert_that(c_list_node_count(list), is_equal_to(1));
}

TestSuite *c_list_suite()
{
    TestSuite *suite = create_test_suite();
    add_test(suite, add_element_to_end);
    return suite;
}