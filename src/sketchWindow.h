/*
 * Reference:
 *    https://www.gtkmm.org
 *
 * Requeriment
 *    libgtkmm-4.0-dev
 */
#pragma once

#include "gtkmm-4.0/gdkmm/rgba.h"
#include "gtkmm-4.0/gtkmm/aboutdialog.h"
#include "gtkmm-4.0/gtkmm/applicationwindow.h"
#include "gtkmm-4.0/gtkmm/box.h"
#include "gtkmm-4.0/gtkmm/builder.h"
#include "gtkmm-4.0/gtkmm/button.h"
#include "gtkmm-4.0/gtkmm/colorbutton.h"
#include "gtkmm-4.0/gtkmm/colorchooserdialog.h"
#include "gtkmm-4.0/gtkmm/drawingarea.h"
#include "gtkmm-4.0/gtkmm/filechooserdialog.h"
#include "gtkmm-4.0/gtkmm/messagedialog.h"
#include "gtkmm-4.0/gtkmm/popovermenu.h"
#include "gtkmm-4.0/gtkmm/spinbutton.h"
#include "gtkmm-4.0/gtkmm/statusbar.h"

#include <string>
#include <vector>

using namespace std;

class SketchWin : public Gtk::ApplicationWindow {
public:
    SketchWin();
    virtual ~SketchWin() {};

private:
    vector<string> shapeLabels {"Line", "Polyline", "Rectangle", "Circle", "Ellipse"};

    enum ShapeID {LINE, POLYLINE, RECTANGLE, CIRCLE, ELLIPSE, NONE};

    struct Point {
        double X, Y;

        Point();
        Point(double x, double y);

        bool equal(Point p);
        double length(Point p);
        double lengthX(Point p);
        double lengthY(Point p);
        string txt();
        string svg();
    };

    struct Color {
        float R, G, B, A;

        Color();
        Color(float r, float g, float b, float a);
        Color(Gdk::RGBA color);

        Gdk::RGBA rgba();
        string txt();
        string svg();
    };

    struct DrawingElement {
        ShapeID id;
        string name;
        vector<Point> points;

        float strokeWidth;
        Color strokeColor, fillColor;

        DrawingElement();
        DrawingElement(ShapeID id);
        DrawingElement(ShapeID id, string name);
        DrawingElement(ShapeID id, vector<Point> points);
    };

    bool isDragging;
    unsigned int m_BackUpLimit;

    Point m_Cursor, m_Ruler;
    vector<DrawingElement> m_Elements;
    vector<vector<DrawingElement> > m_Redo, m_Undo;

    Gdk::RGBA m_ColorBackground;
    Gtk::Box m_VBox, m_HBox, m_HBox_ColorButtons, m_VBox_SidePanel;
    Gtk::ColorButton m_ColorButton_Fill, m_ColorButton_Stroke;
    Gtk::DrawingArea m_DrawingArea;
    Gtk::PopoverMenu m_MenuPopup;
    Gtk::SpinButton m_SpinButton;
    Gtk::Statusbar m_StatusBar;
    vector<Gtk::Button> m_Button;

    Glib::RefPtr<Gtk::Adjustment> m_adjustment_SpinButton;
    Glib::RefPtr<Gtk::Builder> m_refBuilder;
    Glib::ustring getUI();

    unique_ptr<Gtk::AboutDialog> m_pAboutDialog;
    unique_ptr<Gtk::ColorChooserDialog> m_pColorDialog;
    unique_ptr<Gtk::FileChooserDialog> m_pFileDialog;
    unique_ptr<Gtk::MessageDialog> m_pMessageDialog;

    bool on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);
    void on_menuPopup(int n, double x, double y);
    void on_menu_help();
    void on_menu_undo();
    void on_menu_redo();
    void on_menu_save();
    void info(Glib::ustring message);

    void on_menu_clean();
    void setBackground();
    void dialog_response(int response_id, const Glib::ustring &dialogName);

    void setShape(ShapeID shape);
    void position(int n, double x, double y);
    void move(double x, double y);
    void update();
    void on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height);

    void save(string path);
};
