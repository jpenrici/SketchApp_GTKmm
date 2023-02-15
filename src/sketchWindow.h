/*
 * Reference:
 *    https://www.gtkmm.org
 *
 * Requeriment
 *    libgtkmm-4.0-dev
 */
#pragma once

#include <gtkmm-4.0/gtkmm.h>

class SketchWindow : public Gtk::ApplicationWindow {
public:
    SketchWindow();
    virtual ~SketchWindow() {};

private:

    struct Position {
        double X, Y;
        Position()
        {
            X = 0.0, Y = 0.0;
        }
        Position(double x, double y);
    };

    enum TypeShape { NONE, LINE, RECTANGLE, CIRCLE };

    struct Shape {
        unsigned int typeShape = NONE;
        Position position1, position2;
        Gdk::RGBA color = Gdk::RGBA(0.0, 0.0, 0.0, 1.0);

        Shape(unsigned int tShape, Position pos1, Position pos2, Gdk::RGBA c);
    };

    unsigned int m_Shape;
    double m_CursorSize;
    std::vector<Shape> m_vectorShapes;
    Position m_Position1, m_Position2;

    Gtk::Box m_VBox1, m_VBox2, m_HBox1;
    Gtk::Button m_Button1, m_Button2;
    Gtk::ColorButton m_ColorButton1;
    Gtk::PopoverMenu m_MenuPopup;
    Gtk::Statusbar m_StatusBar;
    Gtk::DrawingArea m_DrawingArea;
    Gtk::SpinButton m_SpinButton;
    Gdk::RGBA m_Color;

    Glib::RefPtr<Gtk::Builder> m_refBuilder;
    Glib::ustring getUI();

    std::unique_ptr<Gtk::ColorChooserDialog> m_pColorChooserDialog;
    std::unique_ptr<Gtk::AboutDialog> m_pAboutDialog;
    std::unique_ptr<Gtk::MessageDialog> m_pMessageDialog;

    void on_menu_file_new();
    void on_menu_others();
    void on_menu_help_about();
    void on_popup_button_pressed(int /* n_press */, double x, double y);
    void on_mouse_button_pressed(int /* n_press */, double x, double y);
    void on_button1_clicked();
    void on_button2_clicked();
    void on_choose_color();
    void change_color();
    void on_colorChooserDialog_response(int response_id);
    void on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height);
    void info(Glib::ustring message);
};
