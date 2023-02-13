/*
 * Reference:
 *    https://www.gtkmm.org
 *
 * Requeriment
 *    libgtkmm-4.0-dev
 */
#include "sketchApp.h"

int main(int argc, char *argv[])
{
    auto application = SketchApp::create();
    const int status = application->run(argc, argv);

    return status;
}
