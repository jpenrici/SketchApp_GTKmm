#include "sketchWindow.h"

#include <iostream>

using namespace std;

SketchWindow::SketchWindow() : Gtk::ApplicationWindow()
{
    // Menu - Action
    add_action("new", sigc::mem_fun(*this, &SketchWindow::cleanDrawArea));
    add_action("undo", sigc::mem_fun(*this, &SketchWindow::on_menu_undo));
    add_action("redo", sigc::mem_fun(*this, &SketchWindow::on_menu_redo));
    add_action("about", sigc::mem_fun(*this, &SketchWindow::help));

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
        refActionGroup->add_action("colorBackgrund", sigc::mem_fun(*this, &SketchWindow::setBackground));
        refActionGroup->add_action("clean", sigc::mem_fun(*this, &SketchWindow::cleanDrawArea));
        refActionGroup->add_action("undo", sigc::mem_fun(*this, &SketchWindow::on_menu_undo));
        refActionGroup->add_action("redo", sigc::mem_fun(*this, &SketchWindow::on_menu_redo));
        insert_action_group("popup", refActionGroup);

        // Mouse : Left Button
        auto refGesture_menuPopup = Gtk::GestureClick::create();
        refGesture_menuPopup->set_button(GDK_BUTTON_SECONDARY);
        refGesture_menuPopup->signal_pressed().connect(sigc::mem_fun(*this, &SketchWindow::menuPopup));
        m_DrawingArea.add_controller(refGesture_menuPopup);
    }

    // Mouse : Right Button
    auto refGesture_mouse = Gtk::GestureClick::create();
    refGesture_mouse->set_button(GDK_BUTTON_PRIMARY);
    refGesture_mouse->signal_pressed().connect(sigc::mem_fun(*this, &SketchWindow::setPosition));
    m_DrawingArea.add_controller(refGesture_mouse);

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
        m_Button.back().signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &SketchWindow::setShape), Shape(i)));
    }

    m_ColorButton_Fill.set_rgba(Gdk::RGBA(0.0, 0.0, 0.0, 0.0));
    m_ColorButton_Fill.set_visible(true);
    m_ColorButton_Fill.set_can_focus(false);
    m_ColorButton_Fill.signal_color_set().connect(sigc::mem_fun(*this, &SketchWindow::update));

    m_ColorButton_Stroke.set_rgba(Gdk::RGBA(0.0, 0.0, 0.0, 1.0));
    m_ColorButton_Stroke.set_visible(true);
    m_ColorButton_Stroke.set_can_focus(false);
    m_ColorButton_Stroke.signal_color_set().connect(sigc::mem_fun(*this, &SketchWindow::update));

    m_adjustment_SpinButton = Gtk::Adjustment::create(5.0, 0.0, 20.0, 1.0, 5.0, 0.0);
    m_SpinButton.set_adjustment(m_adjustment_SpinButton);
    m_SpinButton.signal_value_changed().connect(sigc::mem_fun(*this, &SketchWindow::update));

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
    m_DrawingArea.set_draw_func(sigc::mem_fun(*this, &SketchWindow::on_draw));
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
    m_BackUpLimit = 100;
    cleanDrawArea();
}

void SketchWindow::help()
{
    m_pAboutDialog->set_visible(true);
    m_pAboutDialog->present();
}

void SketchWindow::menuPopup(int n, double x, double y)
{
    const Gdk::Rectangle rect(x, y, 1, 1);
    m_MenuPopup.set_pointing_to(rect);
    m_MenuPopup.popup();
}

void SketchWindow::cleanDrawArea()
{
    for (auto &btn : m_Button) {
        btn.set_sensitive(true);
    }

    m_Elements.clear();
    m_DrawingArea.queue_draw();

    m_StatusBar.remove_all_messages();
    m_StatusBar.push("Select a shape to get started.");
}

void SketchWindow::update()
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
        if (m_Elements.back().shape == LINE || m_Elements.back().shape == RECTANGLE || m_Elements.back().shape == CIRCLE) {
            if (m_Elements.back().points.size() == 2) {
                m_Elements.push_back(DrawingElement(m_Elements.back().shape, {}));
            }
        }
    }

    m_DrawingArea.queue_draw();
}

void SketchWindow::setShape(Shape shape)
{
    for (auto &btn : m_Button) {
        string s = btn.get_label();
        btn.set_sensitive(!(s == labels[shape]));
    }

    m_Elements.push_back(DrawingElement(shape, {}));
}

void SketchWindow::setPosition(int n, double x, double y)
{
    m_Cursor = Point(x, y);
    update();
}

void SketchWindow::on_menu_undo()
{
    if (!m_Undo.empty()) {
        m_Redo.push_back(m_Undo.back());
        m_Undo.pop_back();
        if (!m_Undo.empty()) {
            m_Elements = m_Undo.back();
            if (!m_Elements.back().points.empty()) {
                m_ColorButton_Fill.set_rgba(m_Elements.back().fillColor);
                m_ColorButton_Stroke.set_rgba(m_Elements.back().strokeColor);
                m_SpinButton.set_value(m_Elements.back().strokeWidth);
                for (auto &btn : m_Button) {
                    string s = btn.get_label();
                    btn.set_sensitive(!(s == labels[m_Elements.back().shape]));
                }
                setPosition(0, m_Elements.back().points.back().X, m_Elements.back().points.back().Y);
            }
        }
        else {
            cleanDrawArea();
        }
    }
}

void SketchWindow::on_menu_redo()
{
    if (!m_Redo.empty()) {
        m_Undo.push_back(m_Redo.back());
        m_Elements = m_Redo.back();
        if (!m_Elements.back().points.empty()) {
            m_ColorButton_Fill.set_rgba(m_Elements.back().fillColor);
            m_ColorButton_Stroke.set_rgba(m_Elements.back().strokeColor);
            m_SpinButton.set_value(m_Elements.back().strokeWidth);
            for (auto &btn : m_Button) {
                string s = btn.get_label();
                btn.set_sensitive(!(s == labels[m_Elements.back().shape]));
            }
            setPosition(0, m_Elements.back().points.back().X, m_Elements.back().points.back().Y);
        }
        m_Redo.pop_back();
    }
}

void SketchWindow::on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height)
{
    // Background
    cr->save();
    Gdk::Cairo::set_source_rgba(cr, m_ColorBackground);
    cr->paint();

    // Shapes
    for (auto &element : m_Elements) {
        if (element.points.size() >= 2) {
            cr->set_line_width(element.strokeWidth);
            if (element.shape == LINE || element.shape == POLYLINE) {
                Gdk::Cairo::set_source_rgba(cr, element.strokeColor);
                for (int i = 1; i < element.points.size(); i++) {
                    cr->move_to(element.points[i - 1].X, element.points[i - 1].Y);
                    cr->line_to(element.points[i].X, element.points[i].Y);
                    cr->stroke();
                }
            }
            if (element.shape == RECTANGLE) {
                Gdk::Cairo::set_source_rgba(cr, element.strokeColor);
                cr->rectangle(element.points[0].X,
                        element.points[0].Y,
                        element.points[1].X - element.points[0].X,
                        element.points[1].Y - element.points[0].Y);
                cr->stroke();
                Gdk::Cairo::set_source_rgba(cr, element.fillColor);
                cr->rectangle(element.points[0].X,
                        element.points[0].Y,
                        element.points[1].X - element.points[0].X,
                        element.points[1].Y - element.points[0].Y);
                cr->fill();
            }
            if (element.shape == CIRCLE) {
                double radius = sqrt(pow((element.points[1].X - element.points[0].X), 2) +
                        pow((element.points[1].Y - element.points[0].Y), 2));
                Gdk::Cairo::set_source_rgba(cr, element.strokeColor);
                cr->arc(element.points[0].X, element.points[0].Y, radius, 0.0, 2 * numbers::pi);
                cr->stroke();
                Gdk::Cairo::set_source_rgba(cr, element.fillColor);
                cr->arc(element.points[0].X, element.points[0].Y, radius, 0.0, 2 * numbers::pi);
                cr->fill();
            }
        }
    }

    // Cursor
    Gdk::Cairo::set_source_rgba(cr, m_ColorButton_Stroke.get_rgba());
    cr->set_line_width(2.0);
    cr->arc(m_Cursor.X, m_Cursor.Y, 5.0, 0.0, 2 * numbers::pi);
    cr->stroke();
}

void SketchWindow::setBackground()
{
    if (!m_pColorChooserDialog) {
        m_pColorChooserDialog.reset(new Gtk::ColorChooserDialog("", *this));
        m_pColorChooserDialog->set_modal(true);
        m_pColorChooserDialog->set_hide_on_close(true);
        m_pColorChooserDialog->signal_response().connect(sigc::mem_fun(*this, &SketchWindow::colorChooserDialog_response));
        m_pColorChooserDialog->set_rgba(m_ColorBackground);
        m_pColorChooserDialog->show();
    }
    else {
        m_pColorChooserDialog->show();
    }
}

void SketchWindow::colorChooserDialog_response(int response_id)
{
    m_pColorChooserDialog->hide();
    if (response_id == Gtk::ResponseType::OK) {
        m_ColorBackground = m_pColorChooserDialog->get_rgba();
        m_DrawingArea.queue_draw();
    }
}

SketchWindow::Point::Point()
    : X(0.0), Y(0.0) {}

SketchWindow::Point::Point(double x, double y)
    : X(x < 0 ? 0.0 : x), Y(y < 0 ? 0.0 : y) {}

bool SketchWindow::Point::equal(Point p)
{
    return X == p.X && Y == p.Y;
}

SketchWindow::DrawingElement::DrawingElement()
    : shape(NONE) {}

SketchWindow::DrawingElement::DrawingElement(Shape shape)
    : shape(shape) {}

SketchWindow::DrawingElement::DrawingElement(Shape shape, vector<Point> points)
    : shape(shape), points(points) {}

void SketchWindow::info(Glib::ustring message)
{
    m_pMessageDialog->set_secondary_text(message);
    m_pMessageDialog->set_visible(true);
    m_pMessageDialog->present();
}

Glib::ustring SketchWindow::getUI()
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
