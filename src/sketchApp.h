/*
 * Reference:
 *    https://www.gtkmm.org
 *
 * Requeriment
 *    libgtkmm-4.0-dev
 */
#pragma once

#include <gtkmm-4.0/gtkmm/application.h>
#include <gtkmm-4.0/gtkmm/builder.h>

class SketchApp : public Gtk::Application {
public:
    static Glib::RefPtr<SketchApp> create();

protected:
    SketchApp();

    void on_startup() override;
    void on_activate() override;

private:
    Glib::RefPtr<Gtk::Builder> m_refBuilder;
    Glib::ustring getUI();

    void create_window();
    void destroy_window(Gtk::Window *window);
    void on_menu_file_quit();
};
