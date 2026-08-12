#include <QApplication>

#include <gtest/gtest.h>

// The widget tests need a QApplication, which gtest_main does not create, so
// this binary provides its own entry point.
//
// The offscreen platform is set before QApplication is constructed so the
// tests run on a build agent with no display. Setting it here rather than in
// the CI workflow means the tests behave the same way locally.
int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication application(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
