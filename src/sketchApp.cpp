#include "sketchApp.h"
#include "sketchWindow.h"

#include <iostream>

using namespace std;

SketchApp::SketchApp() : Gtk::Application("org.gtkmm.SketchApp") {}

Glib::RefPtr<SketchApp> SketchApp::create()
{
    return Glib::make_refptr_for_instance<SketchApp>(new SketchApp());
}

void SketchApp::on_startup()
{
    Gtk::Application::on_startup();

    // Action
    add_action("quit", sigc::mem_fun(*this, &SketchApp::quit));

    // Menu
    m_refBuilder = Gtk::Builder::create();
    try {
        m_refBuilder->add_from_string(getUI());
    }
    catch (const Glib::Error &ex) {
        cerr << "Error: " << ex.what();
    }

    auto object = m_refBuilder->get_object("menu");
    auto gmenu = dynamic_pointer_cast<Gio::Menu>(object);
    if (gmenu) {
        set_menubar(gmenu);
    }
}

void SketchApp::on_activate()
{
    create_window();
}

void SketchApp::create_window()
{
    auto win = new SketchWin();
    add_window(*win);
    win->signal_hide().connect(
                sigc::bind(sigc::mem_fun(*this, &SketchApp::destroy_window), win));
    win->signal_destroy().connect(
                sigc::bind(sigc::mem_fun(*this, &SketchApp::destroy_window), win));
    win->set_show_menubar(true);
    win->show();
}

void SketchApp::destroy_window(Gtk::Window *window)
{
    delete window;
}

void SketchApp::quit()
{
    quit();

    vector<Gtk::Window *> windows = get_windows();
    if (windows.size() > 0) {
        windows[0]->hide();
    }
}

Glib::ustring SketchApp::getUI()
{
    return {
        "<interface>"
        "  <!-- menubar -->"
        "  <menu id='menu'>"
        "    <submenu>"
        "      <attribute name='label' translatable='yes'>_File</attribute>"
        "      <section>"
        "        <item>"
        "          <attribute name='label' translatable='yes'>_New</attribute>"
        "          <attribute name='action'>win.new</attribute>"
        "        </item>"
        "      <submenu>"
        "        <attribute name='label' translatable='yes'>Export</attribute>"
        "        <section>"
        "          <item>"
        "            <attribute name='label' translatable='yes'>.TXT</attribute>"
        "            <attribute name='action'>win.saveTxt</attribute>"
        "          </item>"
        "          <item>"
        "            <attribute name='label' translatable='yes'>.SVG</attribute>"
        "            <attribute name='action'>win.saveSvg</attribute>"
        "          </item>"
        "        </section>"
        "      </submenu>"
        "      </section>"
        "      <section>"
        "        <item>"
        "          <attribute name='label' translatable='yes'>_Quit</attribute>"
        "          <attribute name='action'>app.quit</attribute>"
        "        </item>"
        "      </section>"
        "    </submenu>"
        "    <submenu>"
        "      <attribute name='label' translatable='yes'>_Edit</attribute>"
        "      <section>"
        "        <item>"
        "          <attribute name='label' translatable='yes'>_Undo</attribute>"
        "          <attribute name='action'>win.undo</attribute>"
        "        </item>"
        "        <item>"
        "          <attribute name='label' translatable='yes'>_Redo</attribute>"
        "          <attribute name='action'>win.redo</attribute>"
        "        </item>"
        "      </section>"
        "    </submenu>"
        "    <submenu>"
        "      <attribute name='label' translatable='yes'>_Help</attribute>"
        "      <section>"
        "        <item>"
        "          <attribute name='label' translatable='yes'>_About</attribute>"
        "          <attribute name='action'>win.about</attribute>"
        "        </item>"
        "      </section>"
        "    </submenu>"
        "  </menu>"
        "</interface>"
    };
}
