#include <iostream>
#include <sstream>
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK  // IMPROVE: why do I need this?
#include <boost/test/unit_test.hpp>

#include "gtp.hpp"
#include "gtp_gogui.hpp"

using std::endl;
using std::string;
using std::stringstream;

// -----------------------------------------------------------------------------

void DoGfx(Gtp::Io&) {}

class DummyAnalyzeUser {
public:
    void DoBlah(Gtp::Io&) {}
};

// Environement of all unit tests.
struct Fixture {
    stringstream in;
    stringstream out;
    stringstream expected_out;

    Gtp::ReplWithGogui gtp;
    DummyAnalyzeUser dau;

    int i;
    float f;
    string s;
    // TODO try custom type with << >> operators

    Fixture() : i(1), f(1.5), s("one_and_half") {
        out << endl;
        expected_out << endl;
        gtp.RegisterGfx("do_gfx", "all", &DoGfx);
        gtp.RegisterGfx("do_gfx", "half", &DoGfx);
        gtp.RegisterGfx("do_gfx", "", &DoGfx);
        gtp.RegisterGfx("do_blah", "", &dau, &DummyAnalyzeUser::DoBlah);
        gtp.RegisterParam("test.params", "int", &i);
        gtp.RegisterParam("test.params", "float", &f);
        gtp.RegisterParam("test.params", "str", &s);
        gtp.RegisterParam("test.params2", "same_str", &s);
    }
};

// -----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(f, Fixture)

BOOST_AUTO_TEST_CASE(ListCommands) {
    in << "help" << endl;

    expected_out << "= "
                 << "do_blah" << endl
                 << "do_gfx" << endl
                 << "gogui_analyze_commands" << endl
                 << "gtpfile" << endl
                 << "help" << endl
                 << "known_command" << endl
                 << "list_commands" << endl
                 << "quit" << endl
                 << "test.params" << endl
                 << "test.params2" << endl
                 << endl;

    gtp.Run(in, out);
    BOOST_CHECK_EQUAL(out.str(), expected_out.str());
}

BOOST_AUTO_TEST_CASE(GoguiAnalyzeCommands) {
    in << "gogui_analyze_commands" << endl << "do_gfx" << endl << "do_blah" << endl;
    expected_out << "= "
                 << "gfx/do_gfx all/do_gfx all" << endl
                 << "gfx/do_gfx half/do_gfx half" << endl
                 << "gfx/do_gfx/do_gfx" << endl
                 << "gfx/do_blah/do_blah" << endl
                 << "param/test.params/test.params" << endl
                 << "param/test.params2/test.params2" << endl
                 << endl
                 << "= " << endl
                 << endl
                 << "= " << endl
                 << endl;
    gtp.Run(in, out);
    BOOST_CHECK_EQUAL(out.str(), expected_out.str());
}

BOOST_AUTO_TEST_CASE(GfxVariable1) {
    in << "test.params" << endl
       << "test.params2" << endl
       << "test.params x" << endl
       << "test.params float" << endl;

    expected_out

        << "= "
        << "[string] float 1.5" << endl
        << "[string] int 1" << endl
        << "[string] str one_and_half" << endl
        << endl

        << "= "
        << "[string] same_str one_and_half" << endl
        << endl

        << "? unknown variable: \"x\"" << endl
        << endl

        << "= 1.5" << endl
        << endl;
    gtp.Run(in, out);
    BOOST_CHECK_EQUAL(out.str(), expected_out.str());
}

BOOST_AUTO_TEST_CASE(GfxVariable2) {
    in << "test.params float NotFloat" << endl
       << "test.params float 11.5 " << endl
       << "test.params float" << endl
       << "test.params str string_with_no_spaces" << endl
       << "test.params2 same_str" << endl;

    expected_out

        << "? syntax error" << endl
        << endl

        << "= " << endl
        << endl

        << "= 11.5" << endl
        << endl

        << "= " << endl
        << endl

        << "= string_with_no_spaces" << endl
        << endl;
    gtp.Run(in, out);
    BOOST_CHECK_EQUAL(out.str(), expected_out.str());
    BOOST_CHECK_EQUAL(f, 11.5);
    BOOST_CHECK_EQUAL(s, "string_with_no_spaces");
}

BOOST_AUTO_TEST_SUITE_END()
