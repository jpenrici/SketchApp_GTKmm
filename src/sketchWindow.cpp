#include "sketchWindow.h"

#include <iostream>

using namespace std;

SketchWindow::SketchWindow()
    : Gtk::ApplicationWindow()
{
    // Menu - Action
    add_action("new", sigc::mem_fun(*this, &SketchWindow::on_menu_file_new));
    add_action("undo", sigc::mem_fun(*this, &SketchWindow::on_menu_others));
    add_action("redo", sigc::mem_fun(*this, &SketchWindow::on_menu_others));
    add_action("about", sigc::mem_fun(*this, &SketchWindow::on_menu_help_about));

    // MenuPopup
    m_refBuilder = Gtk::Builder::create();
    try {
        m_refBuilder->add_from_string(getUI_MenuPopup());
    }
    catch (const Glib::Error &ex) {
        cerr << "Error: " << ex.what();
    }

    auto object = m_refBuilder->get_object("menupopup");
    auto gmenu = dynamic_pointer_cast<Gio::Menu>(object);
    if (!gmenu) {
        g_warning("GMenu not found");
    }
    else {
        m_MenuPopup.set_parent(m_DrawingArea);
        m_MenuPopup.set_menu_model(gmenu);
        m_MenuPopup.set_has_arrow(false);

        auto refActionGroup = Gio::SimpleActionGroup::create();
        refActionGroup->add_action("color",
                                   sigc::mem_fun(*this, &SketchWindow::on_choose_color));
        insert_action_group("popup", refActionGroup);

        auto refGesture_menuPopup = Gtk::GestureClick::create();
        refGesture_menuPopup->set_button(GDK_BUTTON_SECONDARY);
        refGesture_menuPopup->signal_pressed().connect(
                    sigc::mem_fun(*this, &SketchWindow::on_popup_button_pressed));
        m_DrawingArea.add_controller(refGesture_menuPopup);
    }

    // Mouse
    auto refGesture_mouse = Gtk::GestureClick::create();
    refGesture_mouse->set_button(GDK_BUTTON_PRIMARY);
    refGesture_mouse->signal_pressed().connect(
                sigc::mem_fun(*this, &SketchWindow::on_mouse_button_pressed));
    m_DrawingArea.add_controller(refGesture_mouse);

    // Message Dialog
    m_pMessageDialog.reset(new Gtk::MessageDialog(*this, "Info"));
    m_pMessageDialog->set_modal(true);
    m_pMessageDialog->set_hide_on_close(true);
    m_pMessageDialog->signal_response().connect(
                sigc::hide(sigc::mem_fun(*m_pMessageDialog, &Gtk::Widget::hide)));

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

    m_pAboutDialog->set_authors(std::vector<Glib::ustring>{"jpenrici"});

    // Button
    m_Button1.set_label("Button 1");
    m_Button1.set_visible(true);
    m_Button1.set_can_focus(false);
    m_Button1.signal_clicked().connect(
                sigc::mem_fun(*this, &SketchWindow::on_button1_clicked));

    m_Button2.set_label("Button 2");
    m_Button2.set_visible(true);
    m_Button2.set_can_focus(false);
    m_Button2.signal_clicked().connect(
                sigc::mem_fun(*this, &SketchWindow::on_button2_clicked));

    m_Color.set_rgba(0.0, 0.0, 0.0, 1.0);
    m_ColorButton1.set_rgba(m_Color);
    m_ColorButton1.set_visible(true);
    m_ColorButton1.set_can_focus(false);
    m_ColorButton1.signal_color_set().connect(
                sigc::mem_fun(*this, &SketchWindow::change_color));

    // Box
    m_VBox1.set_orientation(Gtk::Orientation::VERTICAL);
    m_VBox1.set_homogeneous(false);

    m_VBox2.set_orientation(Gtk::Orientation::VERTICAL);
    m_VBox2.set_homogeneous(false);

    m_HBox1.set_orientation(Gtk::Orientation::HORIZONTAL);
    m_HBox1.set_homogeneous(false);

    // DrawingArea
    m_DrawingArea.set_draw_func(
                sigc::mem_fun(*this, &SketchWindow::on_draw));
    m_DrawingArea.set_expand(true);

    // StatusBar
    m_StatusBar.set_visible(true);
    m_StatusBar.set_can_focus(false);
    m_StatusBar.set_margin_start(10);
    m_StatusBar.set_margin_end(10);
    m_StatusBar.set_margin_top(6);
    m_StatusBar.set_margin_bottom(6);
    m_StatusBar.push("StatusBar!");

    // Window
    set_title("Draw");
    set_default_size(800, 600);
    set_resizable(false);

    m_VBox2.append(m_ColorButton1);
    m_VBox2.append(m_Button1);
    m_VBox2.append(m_Button2);

    m_HBox1.append(m_VBox2);
    m_HBox1.append(m_DrawingArea);

    m_VBox1.append(m_HBox1);
    m_VBox1.append(m_StatusBar);

    set_child(m_VBox1);
}

void SketchWindow::on_menu_file_new()
{
    m_StatusBar.remove_all_messages();
    m_StatusBar.push("A menu item was selected.");
}

void SketchWindow::on_menu_others()
{
    m_StatusBar.remove_all_messages();
    m_StatusBar.push("A menu item was selected.");
}

void SketchWindow::on_menu_help_about()
{
    m_pAboutDialog->set_visible(true);
    m_pAboutDialog->present();
}

void SketchWindow::on_popup_button_pressed(int /* n_press */, double x, double y)
{
    const Gdk::Rectangle rect(x, y, 1, 1);
    m_MenuPopup.set_pointing_to(rect);
    m_MenuPopup.popup();
}

void SketchWindow::on_mouse_button_pressed(int /* n_press */, double x, double y)
{
    m_Position2.X = m_Position.X;
    m_Position2.Y = m_Position.Y;

    m_Position = Position(x, y);
    cout << m_Position2.X << "," << m_Position2.Y << " : "
         << m_Position.X << "," << m_Position.Y << endl;

    m_DrawingArea.queue_draw();
}

void SketchWindow::on_button1_clicked()
{
    m_pMessageDialog->set_secondary_text("Button 1");
    m_pMessageDialog->set_visible(true);
    m_pMessageDialog->present();
}

void SketchWindow::on_button2_clicked()
{;
    m_pMessageDialog->set_secondary_text("Button 2");
    m_pMessageDialog->set_visible(true);
    m_pMessageDialog->present();
}

void SketchWindow::on_draw(const Cairo::RefPtr<Cairo::Context> &cr,
                           int width, int height)
{
    cr->set_line_width(10.0);
    cr->set_source_rgba(m_Color.get_red(), m_Color.get_green(), m_Color.get_blue(),
                        m_Color.get_alpha());

    cr->move_to(m_Position2.X, m_Position2.Y);
    cr->line_to(m_Position.X, m_Position.Y);
    cr->stroke();
}

void SketchWindow::change_color()
{
    m_Color = m_ColorButton1.get_rgba();
    m_DrawingArea.queue_draw();
}

void SketchWindow::on_choose_color()
{
    if (!m_pColorChooserDialog) {
        m_pColorChooserDialog.reset(new Gtk::ColorChooserDialog("", *this));
        m_pColorChooserDialog->set_modal(true);
        m_pColorChooserDialog->set_hide_on_close(true);
        m_pColorChooserDialog->signal_response().connect(
                    sigc::mem_fun(*this, &SketchWindow::on_colorChooserDialog_response));
        m_pColorChooserDialog->set_rgba(m_Color);
        m_pColorChooserDialog->show();
    }
    else {
        m_pColorChooserDialog->show();
    }
}

void SketchWindow::on_colorChooserDialog_response(int response_id)
{
    m_pColorChooserDialog->hide();
    switch (response_id) {
    case Gtk::ResponseType::OK: {
        m_ColorButton1.set_rgba(m_pColorChooserDialog->get_rgba());
        change_color();
        break;
    }
    case Gtk::ResponseType::CANCEL:
    default: {
        break;
    }
    }
}

SketchWindow::Position::Position(double x, double y)
{
    if (x < 0) {
        x = 0.0;
    }
    if (y < 0) {
        y = 0.0;
    }
    X = x;
    Y = y;
}

Glib::ustring SketchWindow::getUI_MenuPopup()
{
    return {
        "<interface>"
        "  <!-- menu-popup -->"
        "  <menu id='menupopup'>"
        "    <section>"
        "      <item>"
        "        <attribute name='label' translatable='yes'>Color</attribute>"
        "        <attribute name='action'>popup.color</attribute>"
        "        <attribute name='can-focus'>false</attribute>"
        "      </item>"
        "    </section>"
        "  </menu>"
        "</interface>"
    };
}
