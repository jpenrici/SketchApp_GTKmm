/*
 * Reference:
 *    https://www.gtkmm.org
 *
 * Requeriment
 *    libgtkmm-4.0-dev
 */
#pragma once

#include <gtkmm-4.0/gtkmm.h>

using namespace std;

class SketchWindow : public Gtk::ApplicationWindow {
public:
    SketchWindow();
    virtual ~SketchWindow() {};

private:
    vector<string> labels {"Line", "Polyline", "Rectangle", "Circle"};

    enum Shape {LINE, POLYLINE, RECTANGLE, CIRCLE, NONE};

    struct Point {
        double X, Y;
        Point();
        Point(double x, double y);
        bool equal(Point p);
    };

    struct DrawingElement {
        Shape shape;
        vector<Point> points;

        float strokeWidth;
        Gdk::RGBA strokeColor, fillColor;

        DrawingElement();
        DrawingElement(Shape shape);
        DrawingElement(Shape shape, vector<Point> points);
    };

    unsigned int m_BackUpLimit;

    Point m_Cursor;
    vector<DrawingElement> m_Elements;
    vector<vector<DrawingElement> > m_Undo;
    vector<vector<DrawingElement> > m_Redo;

    Gtk::Box m_VBox;
    Gtk::Box m_VBox_SidePanel;
    Gtk::Box m_HBox;
    Gtk::Box m_HBox_ColorButtons;
    Gtk::ColorButton m_ColorButton_Fill;
    Gtk::ColorButton m_ColorButton_Stroke;
    Gtk::SpinButton m_SpinButton;
    Gtk::PopoverMenu m_MenuPopup;
    Gtk::Statusbar m_StatusBar;
    Gtk::DrawingArea m_DrawingArea;
    Gdk::RGBA m_ColorBackground;
    vector<Gtk::Button> m_Button;

    Glib::RefPtr<Gtk::Adjustment> m_adjustment_SpinButton;
    Glib::RefPtr<Gtk::Builder> m_refBuilder;
    Glib::ustring getUI();

    unique_ptr<Gtk::ColorChooserDialog> m_pColorChooserDialog;
    unique_ptr<Gtk::MessageDialog> m_pMessageDialog;
    unique_ptr<Gtk::AboutDialog> m_pAboutDialog;

    void help();
    void update();
    void menuPopup(int n, double x, double y);
    void setPosition(int n, double x, double y);
    void setShape(Shape shape);
    void setBackground();
    void cleanDrawArea();
    void on_menu_undo();
    void on_menu_redo();
    void colorChooserDialog_response(int response_id);
    void on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height);
    void info(Glib::ustring message);
};
