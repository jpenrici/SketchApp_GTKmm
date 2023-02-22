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
    add_action("new", sigc::mem_fun(*this, &SketchWin::cleanDrawArea));
    add_action("undo", sigc::mem_fun(*this, &SketchWin::on_menu_undo));
    add_action("redo", sigc::mem_fun(*this, &SketchWin::on_menu_redo));
    add_action("exportTxt", sigc::mem_fun(*this, &SketchWin::on_menu_save));
    add_action("exportSvg", sigc::mem_fun(*this, &SketchWin::on_menu_save));
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

        auto refActionGroup = Gio::SimpleActionGroup::create();
        refActionGroup->add_action("colorBackgrund", sigc::mem_fun(*this, &SketchWin::setBackground));
        refActionGroup->add_action("clean", sigc::mem_fun(*this, &SketchWin::cleanDrawArea));
        refActionGroup->add_action("undo", sigc::mem_fun(*this, &SketchWin::on_menu_undo));
        refActionGroup->add_action("redo", sigc::mem_fun(*this, &SketchWin::on_menu_redo));
        insert_action_group("popup", refActionGroup);

        // Mouse : Left Button
        auto refGesture1 = Gtk::GestureClick::create();
        refGesture1->set_button(GDK_BUTTON_SECONDARY);
        refGesture1->signal_pressed().connect(sigc::mem_fun(*this, &SketchWin::on_menuPopup));
        m_DrawingArea.add_controller(refGesture1);
    }

    // Mouse : Right Button
    auto refGesture2 = Gtk::GestureClick::create();
    refGesture2->set_button(GDK_BUTTON_PRIMARY);
    refGesture2->signal_pressed().connect(sigc::mem_fun(*this, &SketchWin::position));
    refGesture2->signal_released().connect(sigc::mem_fun(*this, &SketchWin::position));
    m_DrawingArea.add_controller(refGesture2);

    auto refGesture3 = Gtk::GestureDrag::create();
    refGesture3->set_button(GDK_BUTTON_PRIMARY);
    refGesture3->signal_drag_update().connect(sigc::mem_fun(*this, &SketchWin::move));
    m_DrawingArea.add_controller(refGesture3);

    // Keyboard
    auto controller = Gtk::EventControllerKey::create();
    controller->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    controller->signal_key_pressed().connect(sigc::mem_fun(*this, &SketchWin::on_key_pressed), false);
    add_controller(controller);

    // Message Dialog
    m_pMessageDialog.reset(new Gtk::MessageDialog(*this, "Info"));
    m_pMessageDialog->set_modal(true);
    m_pMessageDialog->set_hide_on_close(true);
    m_pMessageDialog->signal_response().connect(sigc::hide(sigc::mem_fun(*m_pMessageDialog, &Gtk::Widget::hide)));

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
    for (unsigned int i = 0; i < labels.size(); i++) {
        m_Button.push_back(Gtk::Button(labels[i]));
        m_Button.back().set_visible(true);
        m_Button.back().set_can_focus(false);
        m_Button.back().signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &SketchWin::setShape), Shape(i)));
    }

    m_ColorButton_Fill.set_rgba(Gdk::RGBA(0.0, 0.0, 0.0, 0.0));
    m_ColorButton_Fill.set_visible(true);
    m_ColorButton_Fill.set_can_focus(false);
    m_ColorButton_Fill.set_tooltip_text("Fill color");

    m_ColorButton_Stroke.set_rgba(Gdk::RGBA(0.0, 0.0, 0.0, 1.0));
    m_ColorButton_Stroke.set_visible(true);
    m_ColorButton_Stroke.set_can_focus(false);
    m_ColorButton_Stroke.set_tooltip_text("Stroke color");

    m_adjustment_SpinButton = Gtk::Adjustment::create(5.0, 1.0, 30.0, 1.0, 5.0, 0.0);
    m_SpinButton.set_adjustment(m_adjustment_SpinButton);
    m_SpinButton.set_can_focus(false);
    m_SpinButton.set_tooltip_text("Stroke width");

    // Box
    m_VBox.set_orientation(Gtk::Orientation::VERTICAL);
    m_VBox.set_homogeneous(false);
    m_HBox.set_orientation(Gtk::Orientation::HORIZONTAL);
    m_HBox.set_homogeneous(false);
    m_VBox_SidePanel.set_orientation(Gtk::Orientation::VERTICAL);
    m_VBox_SidePanel.set_homogeneous(false);
    m_HBox_ColorButtons.set_orientation(Gtk::Orientation::HORIZONTAL);
    m_HBox_ColorButtons.set_homogeneous(false);

    // DrawingArea
    m_ColorBackground = Gdk::RGBA(1.0, 1.0, 1.0, 1.0);
    m_DrawingArea.set_draw_func(sigc::mem_fun(*this, &SketchWin::on_draw));
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

    m_HBox_ColorButtons.append(m_ColorButton_Fill);
    m_HBox_ColorButtons.append(m_ColorButton_Stroke);

    for (auto &btn : m_Button) {
        m_VBox_SidePanel.append(btn);
    }
    m_VBox_SidePanel.append(m_SpinButton);
    m_VBox_SidePanel.append(m_HBox_ColorButtons);

    m_HBox.append(m_VBox_SidePanel);
    m_HBox.append(m_DrawingArea);

    m_VBox.append(m_HBox);
    m_VBox.append(m_StatusBar);

    set_child(m_VBox);

    // Initialize
    m_BackUpLimit = 250;
    cleanDrawArea();
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
        m_StatusBar.push("Escape ...");
    }

    return false;
}

void SketchWin::cleanDrawArea()
{
    for (auto &btn : m_Button) {
        btn.set_sensitive(true);
    }

    draggingMouse = false;
    m_Elements.clear();

    m_StatusBar.remove_all_messages();
    m_StatusBar.push("Select a shape to get started.");

    m_DrawingArea.queue_draw();
}

void SketchWin::setShape(Shape shape)
{
    for (auto &btn : m_Button) {
        string s = btn.get_label();
        btn.set_sensitive(!(s == labels[shape]));
    }

    m_Elements.push_back(DrawingElement(shape, {}));
}

void SketchWin::position(int n, double x, double y)
{
    m_Cursor = Point(x, y);
    draggingMouse = false;
    update();
}

void SketchWin::move(double x, double y)
{
    draggingMouse = true;
    m_Ruler = Point(m_Cursor.X + x, m_Cursor.Y + y);
    update();
}

void SketchWin::update()
{
    m_StatusBar.remove_all_messages();

    if (!m_Elements.empty()) {
        m_Elements.back().fillColor = m_ColorButton_Fill.get_rgba();
        m_Elements.back().strokeColor = m_ColorButton_Stroke.get_rgba();
        m_Elements.back().strokeWidth = m_SpinButton.get_value();
        if (m_Elements.back().points.empty()) {
            m_Elements.back().points.push_back(m_Cursor);
        }
        if (!m_Elements.back().points.back().equal(m_Cursor)) {
            m_Elements.back().points.push_back(m_Cursor);
            m_Undo.push_back(m_Elements);
        }

        if (m_Undo.size() > m_BackUpLimit) {
            m_Undo.erase(m_Undo.begin());
        }
        if (m_Redo.size() > m_BackUpLimit) {
            m_Redo.erase(m_Redo.begin());
        }

        m_StatusBar.push("Point " + to_string(m_Elements.back().points.size()));
        switch (m_Elements.back().shape) {
        case LINE:
        case RECTANGLE:
        case CIRCLE:
        case ELLIPSE:
            if (m_Elements.back().points.size() == 2) {
                m_Elements.push_back(DrawingElement(m_Elements.back().shape, {}));
            }
            break;
        default:
            break;
        }
    }

    m_DrawingArea.queue_draw();
}

void SketchWin::on_menu_undo()
{
    if (!m_Undo.empty()) {
        m_Redo.push_back(m_Undo.back());
        m_Undo.pop_back();
        if (!m_Undo.empty()) {
            m_Elements = m_Undo.back();
            if (!m_Elements.back().points.empty()) {
                m_ColorButton_Fill.set_rgba(m_Elements.back().fillColor.rgba());
                m_ColorButton_Stroke.set_rgba(m_Elements.back().strokeColor.rgba());
                m_SpinButton.set_value(m_Elements.back().strokeWidth);
                for (auto &btn : m_Button) {
                    string s = btn.get_label();
                    btn.set_sensitive(!(s == labels[m_Elements.back().shape]));
                }
                position(0, m_Elements.back().points.back().X, m_Elements.back().points.back().Y);
            }
        }
        else {
            cleanDrawArea();
        }
    }
}

void SketchWin::on_menu_redo()
{
    if (!m_Redo.empty()) {
        m_Undo.push_back(m_Redo.back());
        m_Elements = m_Redo.back();
        if (!m_Elements.back().points.empty()) {
            m_ColorButton_Fill.set_rgba(m_Elements.back().fillColor.rgba());
            m_ColorButton_Stroke.set_rgba(m_Elements.back().strokeColor.rgba());
            m_SpinButton.set_value(m_Elements.back().strokeWidth);
            for (auto &btn : m_Button) {
                string s = btn.get_label();
                btn.set_sensitive(!(s == labels[m_Elements.back().shape]));
            }
            position(0, m_Elements.back().points.back().X, m_Elements.back().points.back().Y);
        }
        m_Redo.pop_back();
    }
}

void SketchWin::on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height)
{
    // Background
    cr->save();
    Gdk::Cairo::set_source_rgba(cr, m_ColorBackground);
    cr->paint();

    // Shapes
    for (auto &element : m_Elements) {
        cr->scale(1.0, 1.0);
        if (element.points.size() >= 2) {
            cr->set_line_width(element.strokeWidth);
            if (element.shape == LINE || element.shape == POLYLINE) {
                Gdk::Cairo::set_source_rgba(cr, element.strokeColor.rgba());
                for (int i = 1; i < element.points.size(); i++) {
                    cr->move_to(element.points[i - 1].X, element.points[i - 1].Y);
                    cr->line_to(element.points[i].X, element.points[i].Y);
                    cr->stroke();
                }
            }
            if (element.shape == RECTANGLE) {
                Gdk::Cairo::set_source_rgba(cr, element.strokeColor.rgba());
                cr->rectangle(element.points[0].X,
                        element.points[0].Y,
                        element.points[1].X - element.points[0].X,
                        element.points[1].Y - element.points[0].Y);
                cr->stroke();
                Gdk::Cairo::set_source_rgba(cr, element.fillColor.rgba());
                cr->rectangle(element.points[0].X,
                        element.points[0].Y,
                        element.points[1].X - element.points[0].X,
                        element.points[1].Y - element.points[0].Y);
                cr->fill();
            }
            if (element.shape == CIRCLE || element.shape == ELLIPSE) {
                double radius = sqrt(pow((element.points[1].X - element.points[0].X), 2) +
                        pow((element.points[1].Y - element.points[0].Y), 2));
                double radiusX = radius;
                double radiusY = radius;
                if (element.shape == ELLIPSE) {
                    radiusX = (max(element.points[1].X, element.points[0].X) -
                            min(element.points[1].X, element.points[0].X));
                    radiusY = (max(element.points[1].Y, element.points[0].Y) -
                            min(element.points[1].Y, element.points[0].Y));
                }
                for (unsigned int i = 0; i <= 1; i++) {
                    double x0 = element.points[0].X + radiusX * cos(0);
                    double y0 = element.points[0].Y + radiusY * sin(0);
                    double angle = 0;
                    double step = 0.2;
                    while (angle <= 360) {
                        double x1 = element.points[0].X + radiusX * cos(angle * numbers::pi / 180);
                        double y1 = element.points[0].Y + radiusY * sin(angle * numbers::pi / 180);
                        Gdk::Cairo::set_source_rgba(cr, element.fillColor.rgba());
                        cr->move_to(element.points[0].X, element.points[0].Y);
                        if (i == 0) {
                            Gdk::Cairo::set_source_rgba(cr, element.strokeColor.rgba());
                            cr->move_to(x0, y0);
                        }
                        cr->line_to(x1, y1);
                        cr->stroke();
                        x0 = x1;
                        y0 = y1;
                        angle += step;
                    }
                }
            }
        }
    }

    // Ruler
    if (!m_Elements.empty()) {
        if (draggingMouse) {
            Gdk::Cairo::set_source_rgba(cr, Gdk::RGBA(1.0, 0.0, 0.0, 0.5));
            cr->set_line_width(1.0);
            cr->move_to(m_Elements.back().points.back().X, m_Elements.back().points.back().Y);
            cr->line_to(m_Ruler.X, m_Ruler.Y);
            cr->stroke();
            cr->set_line_width(2.0);
            cr->arc(m_Ruler.X, m_Ruler.Y, 5.0, 0.0, 2 * numbers::pi);
            cr->stroke();
            if (m_Elements.back().shape == RECTANGLE) {
                cr->rectangle(m_Elements.back().points.back().X,
                              m_Elements.back().points.back().Y,
                              m_Ruler.X - m_Elements.back().points.back().X,
                              m_Ruler.Y - m_Elements.back().points.back().Y);
                cr->stroke();
            }
            if (m_Elements.back().shape == CIRCLE) {
                double radius = sqrt(pow((m_Elements.back().points.back().X - m_Ruler.X), 2) +
                                     pow((m_Elements.back().points.back().Y - m_Ruler.Y), 2));
                cr->arc(m_Elements.back().points.back().X, m_Elements.back().points.back().Y,
                        radius, 0.0, 2 * numbers::pi);
                cr->stroke();
            }
        }
    }

    // Cursor
    Gdk::Cairo::set_source_rgba(cr, m_ColorButton_Stroke.get_rgba());
    cr->set_line_width(2.0);
    cr->arc(m_Cursor.X, m_Cursor.Y, 5.0, 0.0, 2 * numbers::pi);
    cr->stroke();
}

void SketchWin::setBackground()
{
    if (!m_pColorChooserDialog) {
        m_pColorChooserDialog.reset(new Gtk::ColorChooserDialog("", *this));
        m_pColorChooserDialog->set_modal(true);
        m_pColorChooserDialog->set_hide_on_close(true);
        m_pColorChooserDialog->signal_response().connect(sigc::bind(sigc::mem_fun(*this, &SketchWin::dialog_response), "colorChooserDialog"));
        m_pColorChooserDialog->set_rgba(m_ColorBackground);
        m_pColorChooserDialog->show();
    }
    else {
        m_pColorChooserDialog->show();
    }
}

void SketchWin::on_menu_save()
{
    if (m_Elements.empty()) {
        m_StatusBar.push("Nothing to do!");
        return;
    }

    if (!m_pFileChooserDialog) {
        m_pFileChooserDialog.reset(new Gtk::FileChooserDialog("Save", Gtk::FileChooser::Action::SAVE));
        m_pFileChooserDialog->set_transient_for(*this);
        m_pFileChooserDialog->set_modal(true);
        m_pFileChooserDialog->set_hide_on_close(true);
        m_pFileChooserDialog->signal_response().connect(sigc::bind(sigc::mem_fun(*this, &SketchWin::dialog_response), "saveDialog"));
        m_pFileChooserDialog->add_button("_Cancel", Gtk::ResponseType::CANCEL);
        m_pFileChooserDialog->add_button("_Save", Gtk::ResponseType::OK);
        m_pFileChooserDialog->show();
    }
    else {
        m_pFileChooserDialog->show();
    }
}

void SketchWin::dialog_response(int response_id, const Glib::ustring &dialogName)
{
    if (dialogName == "colorChooserDialog") {
        m_pColorChooserDialog->hide();
        if (response_id == Gtk::ResponseType::OK) {
            m_ColorBackground = m_pColorChooserDialog->get_rgba();
            m_DrawingArea.queue_draw();
        }
    }
    if (dialogName == "saveDialog") {
        if (response_id == Gtk::ResponseType::OK) {
            m_pFileChooserDialog->hide();
            save(m_pFileChooserDialog->get_file()->get_path());
        }
        if (response_id == Gtk::ResponseType::CANCEL) {
            m_pFileChooserDialog->hide();
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

string SketchWin::Point::str()
{
    return to_string(X) + "," + to_string(Y);
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

string SketchWin::Color::str()
{
    return to_string(R) + "," + to_string(G) + "," + to_string(B) + "," + to_string(A);
}

SketchWin::DrawingElement::DrawingElement()
    : shape(NONE) {}

SketchWin::DrawingElement::DrawingElement(Shape shape)
    : shape(shape) {}

SketchWin::DrawingElement::DrawingElement(Shape shape, vector<Point> points)
    : shape(shape), points(points) {}

void SketchWin::info(Glib::ustring message)
{
    m_pMessageDialog->set_secondary_text(message);
    m_pMessageDialog->set_visible(true);
    m_pMessageDialog->present();
}

void SketchWin::save(string path)
{
    string extension = filesystem::path(path).extension();

    string text = "";
    if (extension == ".txt" || extension == ".TXT") {
        for (auto &element : m_Elements) {
            text += "\n\nShape: " + labels[element.shape];
            text += "\nFill Color: " + element.fillColor.str();
            text += "\nStroke Color: " + element.fillColor.str();
            text += "\nStroke Width:" + to_string(element.strokeWidth);
            if (!element.points.empty()) {
                text += "\nPoints:";
                for (auto &p : element.points) {
                    text +=  p.str() + " ";
                }
            }
        }
    }

    if (extension == ".svg" || extension == ".SVG") {
        // TO DO
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
        "        <attribute name='action'>popup.colorBackgrund</attribute>"
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
