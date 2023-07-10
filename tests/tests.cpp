#include "CppUTest/TestHarness.h"

extern "C"
{
}

TEST_GROUP(ExampleTestGroup){
    void setup(){}

    void teardown(){}};

TEST(ExampleTestGroup, PassingTest)
{
  CHECK_EQUAL(1, 1);
}

TEST(ExampleTestGroup, FailingTest)
{
  CHECK_EQUAL(1, 0);
}

#include "CppUTest/CommandLineTestRunner.h"

int main(int argc, char **argv)
{
  return RUN_ALL_TESTS(argc, argv);
}
