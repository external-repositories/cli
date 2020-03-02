/*******************************************************************************
 * CLI - A simple command line interface.
 * Copyright (C) 2019 Daniele Pallastrelli
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

#include <boost/test/unit_test.hpp>
#include "cli/history.h"

using namespace cli;
using namespace cli::detail;

BOOST_AUTO_TEST_SUITE(HistorySuite)

BOOST_AUTO_TEST_CASE(NotFull)
{
    History history(10);
    history.NewCommand("item1");
    history.NewCommand("item2");
    history.NewCommand("item3");
    history.NewCommand("item4");

    BOOST_CHECK_EQUAL(history.Next(), "");
    BOOST_CHECK_EQUAL(history.Previous(""), "item4");
    BOOST_CHECK_EQUAL(history.Next(), "");
    BOOST_CHECK_EQUAL(history.Previous(""), "item4");
    BOOST_CHECK_EQUAL(history.Previous("item4"), "item3");
    BOOST_CHECK_EQUAL(history.Previous("item3"), "item2");
    BOOST_CHECK_EQUAL(history.Previous("item2"), "item1");
    BOOST_CHECK_EQUAL(history.Previous("item1"), "item1");
}

BOOST_AUTO_TEST_CASE(Full)
{
    History history(3);
    history.NewCommand("item1");
    history.NewCommand("item2");
    history.NewCommand("item3");
    history.NewCommand("item4");

    BOOST_CHECK_EQUAL(history.Previous(""), "item4");
    BOOST_CHECK_EQUAL(history.Next(), "");
    BOOST_CHECK_EQUAL(history.Previous(""), "item4");
    BOOST_CHECK_EQUAL(history.Previous("item4"), "item3");
    BOOST_CHECK_EQUAL(history.Previous("item3"), "item3");
    BOOST_CHECK_EQUAL(history.Previous("item3"), "item3");
    BOOST_CHECK_EQUAL(history.Previous("item3"), "item3");
    BOOST_CHECK_EQUAL(history.Next(), "item4");
    BOOST_CHECK_EQUAL(history.Next(), "");
}

BOOST_AUTO_TEST_CASE(Insertion)
{
    History history(10);
    history.NewCommand("item1");
    history.NewCommand("item2");
    history.NewCommand("item3");
    history.NewCommand("item4");

    BOOST_CHECK_EQUAL(history.Previous(""), "item4");
    BOOST_CHECK_EQUAL(history.Previous("item4"), "item3");
    BOOST_CHECK_EQUAL(history.Previous("foo"), "item2");
    BOOST_CHECK_EQUAL(history.Next(), "foo");
    BOOST_CHECK_EQUAL(history.Next(), "item4");
    BOOST_CHECK_EQUAL(history.Previous("item4"), "foo");
    BOOST_CHECK_EQUAL(history.Previous("foo"), "item2");

    history.NewCommand("item5");

    BOOST_CHECK_EQUAL(history.Previous(""), "item5");
    BOOST_CHECK_EQUAL(history.Previous("item5"), "item4");
    BOOST_CHECK_EQUAL(history.Next(), "item5");
    BOOST_CHECK_EQUAL(history.Next(), "");
}

BOOST_AUTO_TEST_CASE(InsertionIgnoreRepeat)
{
    History history(10);

    history.NewCommand("item1");
    history.NewCommand("item2");
    history.NewCommand("item2");
    history.NewCommand("item1");
    history.NewCommand("item1");
    history.NewCommand("item3");
    history.NewCommand("item3");
    history.NewCommand("item3");
    history.NewCommand("item1");
    history.NewCommand("item1");
    history.NewCommand("item1");

    BOOST_CHECK_EQUAL(history.Previous(""), "item1");
    BOOST_CHECK_EQUAL(history.Previous("item1"), "item3");
    BOOST_CHECK_EQUAL(history.Previous("item3"), "item1");
    BOOST_CHECK_EQUAL(history.Previous("item1"), "item2");
    BOOST_CHECK_EQUAL(history.Previous("item2"), "item1");
    BOOST_CHECK_EQUAL(history.Next(), "item2");
    BOOST_CHECK_EQUAL(history.Next(), "item1");
    BOOST_CHECK_EQUAL(history.Next(), "item3");
    BOOST_CHECK_EQUAL(history.Next(), "item1");
}

BOOST_AUTO_TEST_CASE(Empty)
{
    History history(10);

    BOOST_CHECK_EQUAL(history.Next(), "");
    BOOST_CHECK_EQUAL(history.Previous(""), "");

    History history2(10);

    BOOST_CHECK_EQUAL(history2.Previous(""), "");
    BOOST_CHECK_EQUAL(history2.Next(), "");

    History history3(10);

    BOOST_CHECK_EQUAL(history3.Previous(""), "");
    history3.NewCommand("item1");
    BOOST_CHECK_EQUAL(history3.Next(), "");
    BOOST_CHECK_EQUAL(history3.Previous(""), "item1");
}

BOOST_AUTO_TEST_CASE(Copies)
{
    History history(10);

    const std::vector<std::string> v = { "item1", "item2", "item3" };
    history.LoadCommands(v);

    BOOST_CHECK_EQUAL(history.Previous(""), "item3");
    BOOST_CHECK_EQUAL(history.Previous("item3"), "item2");
    BOOST_CHECK_EQUAL(history.Previous("item2"), "item1");
    BOOST_CHECK_EQUAL(history.Previous("item1"), "item1");

    history.NewCommand("itemA");
    history.NewCommand("itemB");

    BOOST_CHECK_EQUAL(history.Previous(""), "itemB");
    BOOST_CHECK_EQUAL(history.Previous("itemB"), "itemA");
    BOOST_CHECK_EQUAL(history.Previous("itemA"), "item3");
    BOOST_CHECK_EQUAL(history.Previous("item3"), "item2");
    BOOST_CHECK_EQUAL(history.Previous("item2"), "item1");

    auto cmds = history.GetCommands();
    const std::vector<std::string> expected = { "itemA", "itemB" };
    BOOST_CHECK_EQUAL_COLLECTIONS(cmds.begin(), cmds.end(), expected.begin(), expected.end());



    History history1(3);

    const std::vector<std::string> v1 = { "item1", "item2", "item3" };
    history1.LoadCommands(v1);

    BOOST_CHECK_EQUAL(history1.Previous(""), "item3");
    BOOST_CHECK_EQUAL(history1.Previous("item3"), "item2");
    BOOST_CHECK_EQUAL(history1.Previous("item2"), "item2");

    history1.NewCommand("itemA");
    history1.NewCommand("itemB");

    BOOST_CHECK_EQUAL(history1.Previous(""), "itemB");
    BOOST_CHECK_EQUAL(history1.Previous("itemB"), "itemA");
    BOOST_CHECK_EQUAL(history1.Previous("itemA"), "itemA");

    auto cmds1 = history1.GetCommands();
    const std::vector<std::string> expected1 = { "itemA", "itemB" };
    BOOST_CHECK_EQUAL_COLLECTIONS(cmds1.begin(), cmds1.end(), expected1.begin(), expected1.end());


    History history2(3);

    history2.NewCommand("itemA");
    history2.NewCommand("itemB");
    history2.NewCommand("itemC");
    history2.NewCommand("itemD");
    history2.NewCommand("itemE");

    auto cmds2 = history2.GetCommands();
    const std::vector<std::string> expected2 = { "itemC", "itemD", "itemE" };
    BOOST_CHECK_EQUAL_COLLECTIONS(cmds2.begin(), cmds2.end(), expected2.begin(), expected2.end());
}

BOOST_AUTO_TEST_SUITE_END()