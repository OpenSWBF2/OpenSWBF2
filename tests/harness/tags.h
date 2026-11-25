#pragma once
#include "third_party/doctest/doctest.h"
#include "harness/runner.h"

#define TEST_CASE_NAMED(ClassName, PrettyName, Tags) TEST_CASE(PrettyName * doctest::test_suite(Tags))
#define COMPLEX_TEST_CASE_NAMED(ClassName, PrettyName, Tags, GetTests)                                                  \
    static ComplexTestRegistrar ClassName##Registrar(PrettyName, GetTests);                                             \
    TEST_CASE(PrettyName * doctest::test_suite(Tags))
