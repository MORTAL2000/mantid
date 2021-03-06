// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <string.h>
#include <memory.h>

//
// This test suite shows what to do when your test case
// class cannot be instantiated statically.
// As an example, this test suite requires a non-default constructor.
//

class CreatedTest : public CxxTest::TestSuite
{
    char *_buffer;
public:
    CreatedTest( unsigned size ) : _buffer( new char[size] ) {}
    virtual ~CreatedTest() { delete [] _buffer; }
    
    static CreatedTest *createSuite() { return new CreatedTest( 16 ); }
    static void destroySuite( CreatedTest *suite ) { delete suite; }

    void test_nothing()
    {
        TS_FAIL( "Nothing to test" );
    }
};
