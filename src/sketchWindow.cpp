#include "sketchWindow.h"
#include "gdkmm/general.h"

#include <giomm/simpleactiongroup.h>
#include <gtkmm-4.0/gdkmm/cairoutils.h>
#include <gtkmm-4.0/gdkmm/rgba.h>
#include <gtkmm-4.0/gtkmm/adjustment.h>
#include <gtkmm-4.0/gtkmm/eventcontrollerkey.h>
#include <gtkmm-4.0/gtkmm/gestureclick.h>
#include <gtkmm-4.0/gtkmm/gesturedrag.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <math.h>

using namespace std;

SketchWin::SketchWin() : Gtk::ApplicationWindow()
{
    // Menu - Action
    add_action("new", sigc::bind(sigc::mem_fun(*this, &SketchWin::on_menu_clean), true));
    add_action("undo", sigc::mem_fun(*this, &SketchWin::on_menu_undo));
    add_action("redo", sigc::mem_fun(*this, &SketchWin::on_menu_redo));
    add_action("saveTxt", sigc::mem_fun(*this, &SketchWin::on_menu_save));
    add_action("saveSvg", sigc::mem_fun(*this, &SketchWin::on_menu_save));
    add_action("about", sigc::mem_fun(*this, &SketchWin::on_menu_help));

    // MenuPopup
    m_refBuilder = Gtk::Builder::create();
    try {
        m_refBuilder->add_from_string(getUI());
    }
    catch (const Glib::Error &ex) {
        cerr << "Error: " << ex.what();
    }

    auto object = m_refBuilder->get_object("menupopup");
    auto gmenu = dynamic_pointer_cast<Gio::Menu>(object);
    if (gmenu) {
        m_MenuPopup.set_parent(m_DrawingArea);
        m_MenuPopup.set_menu_model(gmenu);
        m_MenuPopup.set_has_arrow(false);

        auto refActGrp = Gio::SimpleActionGroup::create();
        refActGrp->add_action("colorBkg", sigc::mem_fun(*this, &SketchWin::setBackground));
        refActGrp->add_action("clean", sigc::bind(sigc::mem_fun(*this, &SketchWin::on_menu_clean), true));
        refActGrp->add_action("undo", sigc::mem_fun(*this, &SketchWin::on_menu_undo));
        refActGrp->add_action("redo", sigc::mem_fun(*this, &SketchWin::on_menu_redo));
        insert_action_group("popup", refActGrp);

        // Mouse : Left Button
        auto refMouse1 = Gtk::GestureClick::create();
        refMouse1->set_button(GDK_BUTTON_SECONDARY);
        refMouse1->signal_pressed().connect(sigc::mem_fun(*this, &SketchWin::on_menuPopup));
        m_DrawingArea.add_controller(refMouse1);
    }

    // Mouse : Right Button
    auto refMouse2 = Gtk::GestureClick::create();
    refMouse2->set_button(GDK_BUTTON_PRIMARY);
    refMouse2->signal_pressed().connect(sigc::mem_fun(*this, &SketchWin::setCursorPosition));
    refMouse2->signal_released().connect(sigc::mem_fun(*this, &SketchWin::setCursorPosition));
    m_DrawingArea.add_controller(refMouse2);

    auto refMouse3 = Gtk::GestureDrag::create();
    refMouse3->set_button(GDK_BUTTON_PRIMARY);
    refMouse3->signal_drag_update().connect(sigc::mem_fun(*this, &SketchWin::setRulerPosition));
    m_DrawingArea.add_controller(refMouse3);

    // Keyboard
    auto controller = Gtk::EventControllerKey::create();
    controller->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    controller->signal_key_pressed().connect(sigc::mem_fun(*this, &SketchWin::on_key_pressed), false);
    add_controller(controller);

    // Message Dialog
    m_pMsgDialog.reset(new Gtk::MessageDialog(*this, "Info"));
    m_pMsgDialog->set_modal(true);
    m_pMsgDialog->set_hide_on_close(true);
    m_pMsgDialog->signal_response().connect(sigc::hide(sigc::mem_fun(*m_pMsgDialog, &Gtk::Widget::hide)));

    // About Dialog
    m_pAboutDialog.reset(new Gtk::AboutDialog);
    m_pAboutDialog->set_transient_for(*this);
    m_pAboutDialog->set_hide_on_close();
    //m_pAboutDialog->set_logo(Gdk::Texture::create_from_resource("Resources/logo.svg"));
    m_pAboutDialog->set_program_name("Simple Application");
    m_pAboutDialog->set_version("1.0.0");
    m_pAboutDialog->set_copyright("jpenrici");
    m_pAboutDialog->set_comments("Simple application using Gtkmm 4.");
    m_pAboutDialog->set_license("LGPL");
    m_pAboutDialog->set_website("http://www.gtkmm.org");
    m_pAboutDialog->set_website_label("gtkmm website");
    m_pAboutDialog->set_authors(std::vector<Glib::ustring> {"jpenrici"});

    // Button
    for (unsigned int i = 0; i < shapeLabels.size(); i++) {
        m_Btn.push_back(Gtk::Button(shapeLabels[i]));
        m_Btn.back().set_visible(true);
        m_Btn.back().set_can_focus(false);
        m_Btn.back().signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &SketchWin::setShape), ShapeID(i)));
    }

    m_ColorBtn_Fill.set_rgba(Gdk::RGBA(0.0, 0.0, 0.0, 0.0));
    m_ColorBtn_Fill.set_visible(true);
    m_ColorBtn_Fill.set_can_focus(false);
    m_ColorBtn_Fill.set_tooltip_text("Fill color");

    m_ColorBtn_Stroke.set_rgba(Gdk::RGBA(0.0, 0.0, 0.0, 1.0));
    m_ColorBtn_Stroke.set_visible(true);
    m_ColorBtn_Stroke.set_can_focus(false);
    m_ColorBtn_Stroke.set_tooltip_text("Stroke color");

    auto adjustment1 = Gtk::Adjustment::create(5.0, 1.0, 30.0, 1.0, 0.0, 0.0);
    m_SpinBtn_Stroke.set_adjustment(adjustment1);
    m_SpinBtn_Stroke.set_can_focus(false);
    m_SpinBtn_Stroke.set_tooltip_text("Stroke width");

    auto adjustment2 = Gtk::Adjustment::create(3.0, 3.0, 25.0, 1.0, 0.0, 0.0);
    m_SpinBtn_Step.set_adjustment(adjustment2);
    m_SpinBtn_Step.set_can_focus(false);
    m_SpinBtn_Step.set_tooltip_text("Sides");

    // Box
    m_VBox.set_orientation(Gtk::Orientation::VERTICAL);
    m_VBox.set_homogeneous(false);
    m_HBox.set_orientation(Gtk::Orientation::HORIZONTAL);
    m_HBox.set_homogeneous(false);
    m_VBox_SidePanel.set_orientation(Gtk::Orientation::VERTICAL);
    m_VBox_SidePanel.set_homogeneous(false);
    m_VBox_Attributes.set_orientation(Gtk::Orientation::VERTICAL);
    m_VBox_Attributes.set_homogeneous(false);
    m_VBox_Attributes.set_margin_top(15);
    m_HBox_ColorBtn.set_orientation(Gtk::Orientation::HORIZONTAL);
    m_HBox_ColorBtn.set_homogeneous(true);

    // DrawingArea
    m_ColorBkg = Gdk::RGBA(1.0, 1.0, 1.0, 1.0);
    m_DrawingArea.set_draw_func(sigc::mem_fun(*this, &SketchWin::draw));
    m_DrawingArea.set_margin(10);
    m_DrawingArea.set_expand(true);

    // StatusBar
    m_StatusBar.set_visible(true);
    m_StatusBar.set_can_focus(false);
    m_StatusBar.set_margin(5);

    // Window
    set_title("Sketch App");
    set_default_size(800, 600);
    set_resizable(false);

    m_HBox_ColorBtn.append(m_ColorBtn_Fill);
    m_HBox_ColorBtn.append(m_ColorBtn_Stroke);

    m_VBox_Attributes.append(m_SpinBtn_Stroke);
    m_VBox_Attributes.append(m_HBox_ColorBtn);

    for (auto &btn : m_Btn) {
        m_VBox_SidePanel.append(btn);
    }
    m_VBox_SidePanel.append(m_SpinBtn_Step);
    m_VBox_SidePanel.append(m_VBox_Attributes);

    m_HBox.append(m_VBox_SidePanel);
    m_HBox.append(m_DrawingArea);

    m_VBox.append(m_HBox);
    m_VBox.append(m_StatusBar);

    set_child(m_VBox);

    // Initialize
    m_BkpLimit = 250;
    on_menu_clean(true);
}

void SketchWin::setShape(ShapeID id)
{
    m_SpinBtn_Step.set_visible(id == POLYGON);

    for (auto &btn : m_Btn) {
        btn.set_sensitive(!(string(btn.get_label()) == shapeLabels[id]));
    }

    m_Elements.push_back(DrawingElement(id, shapeLabels[id]));
}

void SketchWin::setBackground()
{
    if (!m_pColorDialog) {
        m_pColorDialog.reset(new Gtk::ColorChooserDialog("", *this));
        m_pColorDialog->set_modal(true);
        m_pColorDialog->set_hide_on_close(true);
        m_pColorDialog->signal_response().connect(sigc::bind(sigc::mem_fun(*this, &SketchWin::dialog_response), "colorChooserDialog"));
        m_pColorDialog->set_rgba(m_ColorBkg);
        m_pColorDialog->show();
    }
    else {
        m_pColorDialog->show();
    }
}

void SketchWin::setCursorPosition(int n, double x, double y)
{
    m_Cursor = Point(x, y);
    isDragging = false;
    updateShapes();
}

void SketchWin::setRulerPosition(double x, double y)
{
    m_Ruler = Point(m_Cursor.X + x, m_Cursor.Y + y);
    isDragging = true;
    updateShapes();
}

void SketchWin::updateShapes()
{
    m_StatusBar.remove_all_messages();

    if (!m_Elements.empty()) {
        m_Elements.back().fillColor = m_ColorBtn_Fill.get_rgba();
        m_Elements.back().strokeColor = m_ColorBtn_Stroke.get_rgba();
        m_Elements.back().strokeWidth = m_SpinBtn_Stroke.get_value();

        if (m_Elements.back().points.empty()) {
            m_Elements.back().points.push_back(m_Cursor);
        }
        if (!m_Elements.back().points.back().equal(m_Cursor)) {
            m_Elements.back().points.push_back(m_Cursor);
            m_Undo.push_back(m_Elements);
        }

        m_StatusBar.push("Point " + to_string(m_Elements.back().points.size()));

        if (m_Elements.back().id == POLYGON) {
            m_Elements.back().value = 360 / m_SpinBtn_Step.get_value();
        }
        else {
            m_Elements.back().value = 0.2;
        }

        if (m_Elements.back().id != POLYLINE) {
            if (m_Elements.back().points.size() == 2) {
                if (m_Elements.back().id == POLYGON) {

                }
                m_Elements.push_back(DrawingElement(m_Elements.back().id));
            }
        }
    }

    m_DrawingArea.queue_draw();
}

void SketchWin::draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height)
{
    auto line = [&cr](Point p1, Point p2) {
        cr->move_to(p1.X, p1.Y);
        cr->line_to(p2.X, p2.Y);
        cr->stroke();
    };

    auto rectangle = [&cr](Point p, double width, double height, bool fill = false) {
        cr->rectangle(p.X, p.Y, width, height);
        if (fill) { cr->fill(); } else { cr->stroke(); }
    };

    auto arc = [&cr](Point p, double r, bool fill = false) {
        cr->arc(p.X, p.Y, r, 0.0, 2 * numbers::pi);
        if (fill) { cr->fill(); } else { cr->stroke(); }
    };

    auto circle = [&cr](Point p, int angle, double rX, double rY, double step, bool fill = false) {
        angle += 360;
        double x0 = p.X + rX * cos(angle * numbers::pi / 180);
        double y0 = p.Y + rY * sin(angle * numbers::pi / 180);
        while (angle > 0) {
            double x1 = p.X + rX * cos(angle * numbers::pi / 180);
            double y1 = p.Y + rY * sin(angle * numbers::pi / 180);
            cr->move_to(p.X, p.Y);
            if (!fill) { cr->move_to(x0, y0); }
            cr->line_to(x1, y1);
            cr->stroke();
            x0 = x1;
            y0 = y1;
            angle -= step;
        }
    };

    // Background
    cr->save();
    Gdk::Cairo::set_source_rgba(cr, m_ColorBkg);
    cr->paint();

    // Shapes
    for (auto &e : m_Elements) {
        cr->scale(1.0, 1.0);
        if (e.points.size() >= 2) {
            cr->set_line_width(e.strokeWidth);
            if (e.id == LINE || e.id == POLYLINE) {
                Gdk::Cairo::set_source_rgba(cr, e.strokeColor.rgba());
                for (int i = 1; i < e.points.size(); i++) {
                    line(e.points[i - 1], e.points[i]);
                }
            }
            Point p1 = e.points.front();
            Point p2 = e.points.back();
            if (e.id == RECTANGLE) {
                Gdk::Cairo::set_source_rgba(cr, e.strokeColor.rgba());
                rectangle(p1, p1.lengthX(p2), p1.lengthY(p2));
                Gdk::Cairo::set_source_rgba(cr, e.fillColor.rgba());
                rectangle(p1, p1.lengthX(p2), p1.lengthY(p2), true);
            }
            if (e.id == CIRCLE || e.id == ELLIPSE) {
                double rX = (e.id == ELLIPSE) ? p1.lengthX(p2) : p1.length(p2);
                double rY = (e.id == ELLIPSE) ? p1.lengthY(p2) : p1.length(p2);
                Gdk::Cairo::set_source_rgba(cr, e.fillColor.rgba());
                circle(p1, 0, rX, rY, e.value, true);
                Gdk::Cairo::set_source_rgba(cr, e.strokeColor.rgba());
                circle(p1, 0, rX, rY, e.value);
            }
            if (e.id == POLYGON) {
                Gdk::Cairo::set_source_rgba(cr, e.strokeColor.rgba());
                circle(p1, p1.angle(p2), p1.length(p2), p1.length(p2), e.value);
            }
        }
    }

    // Ruler
    if (!m_Elements.empty()) {
        if (isDragging) {
            auto e = m_Elements.back();
            Point p = e.points.back();
            Gdk::Cairo::set_source_rgba(cr, Gdk::RGBA(1.0, 0.0, 0.0, 0.5));
            cr->set_line_width(1.0);
            if (e.id == RECTANGLE) {
                rectangle(p, m_Ruler.X - p.X, m_Ruler.Y - p.Y);
            }
            if (e.id == CIRCLE) {
                arc(p, p.length(m_Ruler));
            }
            if (e.id != ELLIPSE && e.id != NONE) {
                line(p, m_Ruler);
                arc(m_Ruler, 5.0);
            }

            double rX = (e.id == ELLIPSE) ? p.lengthX(m_Ruler) : p.length(m_Ruler);
            double rY = (e.id == ELLIPSE) ? p.lengthY(m_Ruler) : p.length(m_Ruler);
            double angle = (e.id == ELLIPSE) ? 0.0 : p.angle(m_Ruler);
            double step = (e.id == ELLIPSE) ? 0.2 : e.value;
            if (e.id == ELLIPSE) {
                line(Point(p.X, p.Y), Point(p.X + rX, p.Y));
                arc(Point(p.X + rX, p.Y), 5.0);
                line(Point(p.X, p.Y), Point(p.X, p.Y + rY));
                arc(Point(p.X, p.Y + rY), 5.0);
                arc(m_Ruler, 5.0);
            }
            if (e.id == ELLIPSE || e.id == POLYGON) {
                circle(p, angle, rX, rY, step);
            }
        }
    }

    // Cursor
    Gdk::Cairo::set_source_rgba(cr, m_ColorBtn_Stroke.get_rgba());
    cr->set_line_width(2.0);
    arc(m_Cursor, 5.0);
}

void SketchWin::on_menu_help()
{
    m_pAboutDialog->set_visible(true);
    m_pAboutDialog->present();
}

void SketchWin::on_menuPopup(int n, double x, double y)
{
    const Gdk::Rectangle rect(x, y, 1, 1);
    m_MenuPopup.set_pointing_to(rect);
    m_MenuPopup.popup();
}

void SketchWin::on_menu_clean(bool all)
{
    for (auto &btn : m_Btn) {
        btn.set_sensitive(true);
    }
    m_SpinBtn_Step.set_visible(false);

    isDragging = false;

    if (!m_Elements.empty()) {
        if (all) {
            m_Elements.clear();
        } else {
            m_Elements.back().id = NONE;
        }
    }

    m_StatusBar.remove_all_messages();
    m_StatusBar.push("Select a shape to get started.");

    m_DrawingArea.queue_draw();
}

void SketchWin::on_menu_undo()
{
    if (m_Undo.size() > m_BkpLimit) {
        m_Undo.erase(m_Undo.begin());
    }

    if (!m_Undo.empty()) {
        m_Redo.push_back(m_Undo.back());
        m_Undo.pop_back();
        if (!m_Undo.empty()) {
            m_Elements = m_Undo.back();
            auto e = m_Elements.back();
            if (!e.points.empty()) {
                m_ColorBtn_Fill.set_rgba(e.fillColor.rgba());
                m_ColorBtn_Stroke.set_rgba(e.strokeColor.rgba());
                m_SpinBtn_Stroke.set_value(e.strokeWidth);
                for (auto &btn : m_Btn) {
                    btn.set_sensitive(!(string(btn.get_label()) == shapeLabels[e.id]));
                }
                setCursorPosition(0, e.points.back().X, e.points.back().Y);
            }
        }
        else {
            on_menu_clean(true);
        }
    }
}

void SketchWin::on_menu_redo()
{
    if (m_Redo.size() > m_BkpLimit) {
        m_Redo.erase(m_Redo.begin());
    }

    if (!m_Redo.empty()) {
        m_Undo.push_back(m_Redo.back());
        m_Elements = m_Redo.back();
        auto e = m_Elements.back();
        if (!e.points.empty()) {
            m_ColorBtn_Fill.set_rgba(e.fillColor.rgba());
            m_ColorBtn_Stroke.set_rgba(e.strokeColor.rgba());
            m_SpinBtn_Stroke.set_value(e.strokeWidth);
            for (auto &btn : m_Btn) {
                btn.set_sensitive(!(string(btn.get_label()) == shapeLabels[e.id]));
            }
            setCursorPosition(0, e.points.back().X, e.points.back().Y);
        }
        m_Redo.pop_back();
    }
}

bool SketchWin::on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state)
{
    if ((keyval == GDK_KEY_z) && (state & (Gdk::ModifierType::SHIFT_MASK | Gdk::ModifierType::CONTROL_MASK | Gdk::ModifierType::ALT_MASK)) == Gdk::ModifierType::CONTROL_MASK) {
        on_menu_undo();
        m_StatusBar.push("Undo ...");
    }
    if ((keyval == GDK_KEY_y) && (state & (Gdk::ModifierType::SHIFT_MASK | Gdk::ModifierType::CONTROL_MASK | Gdk::ModifierType::ALT_MASK)) == Gdk::ModifierType::CONTROL_MASK) {
        on_menu_redo();
        m_StatusBar.push("Redo ...");
    }
    if (keyval == GDK_KEY_Escape) {
        on_menu_clean(false);
        m_StatusBar.push("Escape ...");
    }

    return false;
}

void SketchWin::on_menu_save()
{
    if (m_Elements.empty()) {
        m_StatusBar.push("Nothing to do!");
        return;
    }

    if (!m_pFileDialog) {
        m_pFileDialog.reset(new Gtk::FileChooserDialog("Save", Gtk::FileChooser::Action::SAVE));
        m_pFileDialog->set_transient_for(*this);
        m_pFileDialog->set_modal(true);
        m_pFileDialog->set_hide_on_close(true);
        m_pFileDialog->signal_response().connect(sigc::bind(sigc::mem_fun(*this, &SketchWin::dialog_response), "saveDialog"));
        m_pFileDialog->add_button("_Cancel", Gtk::ResponseType::CANCEL);
        m_pFileDialog->add_button("_Save", Gtk::ResponseType::OK);
        m_pFileDialog->show();
    }
    else {
        m_pFileDialog->show();
    }
}

void SketchWin::dialog_response(int response_id, const Glib::ustring &dialogName)
{
    if (dialogName == "colorChooserDialog") {
        m_pColorDialog->hide();
        if (response_id == Gtk::ResponseType::OK) {
            m_ColorBkg = m_pColorDialog->get_rgba();
            m_DrawingArea.queue_draw();
        }
    }
    if (dialogName == "saveDialog") {
        if (response_id == Gtk::ResponseType::OK) {
            m_pFileDialog->hide();
            save(m_pFileDialog->get_file()->get_path());
        }
        if (response_id == Gtk::ResponseType::CANCEL) {
            m_pFileDialog->hide();
        }
    }
}

SketchWin::Point::Point()
    : X(0.0), Y(0.0) {}

SketchWin::Point::Point(double x, double y)
    : X(x < 0 ? 0.0 : x), Y(y < 0 ? 0.0 : y) {}

bool SketchWin::Point::equal(Point p)
{
    return X == p.X && Y == p.Y;
}

int SketchWin::Point::angle(Point p)
{
    int angle = atan((Y - p.Y) / (X - p.X)) * 180.0 / numbers::pi;
    if (p.X >  X && p.Y == Y) { angle = 0; }
    if (p.X == X && p.Y <  Y) { angle = 90; }
    if (p.X <  X && p.Y == Y) { angle = 180; }
    if (p.X == X && p.Y <  Y) { angle = 270; }
    if (p.X <  X && p.Y >  Y) { angle += 180; }
    if (p.X <  X && p.Y <  Y) { angle += 180; }
    if (p.X >  X && p.Y <  Y) { angle += 360; }
    return angle;
}

double SketchWin::Point::length(Point p)
{
    return sqrt(pow((X - p.X), 2) + pow((Y - p.Y), 2));
}

double SketchWin::Point::lengthX(Point p)
{
    return max(X, p.X) - min(X, p.X);
}

double SketchWin::Point::lengthY(Point p)
{
    return max(Y, p.Y) - min(Y, p.Y);
}

string SketchWin::Point::txt()
{
    return "(" + svg() + ")";
}

string SketchWin::Point::svg()
{
    return to_string(X) + " " + to_string(Y);
}

SketchWin::Color::Color()
    : R(0.0), G(0.0), B(0.0), A(1.0) {}

SketchWin::Color::Color(float r, float g, float b, float a)
{
    R = r < 0.0 ? 0.0 : r > 255.0 ? 1.0 : r / 255;
    G = g < 0.0 ? 0.0 : g > 255.0 ? 1.0 : g / 255;
    B = b < 0.0 ? 0.0 : b > 255.0 ? 1.0 : b / 255;
    A = a < 0.0 ? 0.0 : a > 255.0 ? 1.0 : a / 255;
}

SketchWin::Color::Color(Gdk::RGBA color)
    : R(color.get_red()),
      G(color.get_green()),
      B(color.get_blue()),
      A(color.get_alpha()) {}

Gdk::RGBA SketchWin::Color::rgba()
{
    return Gdk::RGBA(R, G, B, A);
}

string SketchWin::Color::txt()
{
    return {to_string(int(R * 255)) + "," + to_string(int(G * 255)) + ","
                + to_string(int(B * 255)) + "," + to_string(int(A * 255))};
}

SketchWin::DrawingElement::DrawingElement()
    : id(NONE), points({}) {}

SketchWin::DrawingElement::DrawingElement(ShapeID id)
    : id(id), points({}) {}

SketchWin::DrawingElement::DrawingElement(ShapeID id, string name)
    : id(id), name(name), points({}) {}

SketchWin::DrawingElement::DrawingElement(ShapeID id, vector<Point> points)
    : id(id), points(points) {}

void SketchWin::info(Glib::ustring message)
{
    m_pMsgDialog->set_secondary_text(message);
    m_pMsgDialog->set_visible(true);
    m_pMsgDialog->present();
}

void SketchWin::save(string path)
{
    string extension = filesystem::path(path).extension();

    string text = "";
    if (extension == ".txt" || extension == ".TXT") {
        for (auto &e : m_Elements) {
            if (e.id != NONE) {
                if (!e.points.empty()) {
                    text += "\nShape: " + shapeLabels[e.id];
                    text += "\nFill Color: " + e.fillColor.txt();
                    text += "\nStroke Color: " + e.strokeColor.txt();
                    text += "\nStroke Width: " + to_string(e.strokeWidth);
                    text += "\nPoints:";
                    for (auto &p : e.points) {
                        text +=  p.txt() + " ";
                    }
                    text += "\n";
                }
            }
        }
    }

    if (extension == ".svg" || extension == ".SVG") {
        text = "<svg>\n";
        text += "</svg>";
    }

    if (!text.empty()) {
        try {
            fstream fileout;
            fileout.open(path, ios::out);
            fileout << text;
            fileout.close();
            m_StatusBar.push("Save in " + path);
        }
        catch (...) {
            info("There was something wrong!");
        }
    }
}

Glib::ustring SketchWin::getUI()
{
    return {
        "<interface>"
        "  <!-- menu-popup -->"
        "  <menu id='menupopup'>"
        "    <section>"
        "      <item>"
        "        <attribute name='label' translatable='yes'>Undo</attribute>"
        "        <attribute name='action'>popup.undo</attribute>"
        "        <attribute name='can-focus'>false</attribute>"
        "      </item>"
        "      <item>"
        "        <attribute name='label' translatable='yes'>Redo</attribute>"
        "        <attribute name='action'>popup.redo</attribute>"
        "        <attribute name='can-focus'>false</attribute>"
        "      </item>"
        "    </section>"
        "    <section>"
        "      <item>"
        "        <attribute name='label' translatable='yes'>Background</attribute>"
        "        <attribute name='action'>popup.colorBkg</attribute>"
        "        <attribute name='can-focus'>false</attribute>"
        "      </item>"
        "    </section>"
        "    <section>"
        "      <item>"
        "        <attribute name='label' translatable='yes'>Clean</attribute>"
        "        <attribute name='action'>popup.clean</attribute>"
        "        <attribute name='can-focus'>false</attribute>"
        "      </item>"
        "    </section>"
        "  </menu>"
        "</interface>"
    };
}
