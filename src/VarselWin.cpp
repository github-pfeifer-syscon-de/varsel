/* -*- Mode: C; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2024 rpf
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <StringUtils.hpp>
#include <psc_i18n.hpp>
#include <psc_format.hpp>
#include <array>
#include <gdk/gdk.h>
#include <unistd.h> // chdir

#include "VarselWin.hpp"
#include "VarselApp.hpp"
#include "PrefDialog.hpp"

static std::string
getWhere(GtkWidget* widget)
{
    auto vte_terminal = VTE_TERMINAL(widget);
    auto uri = vte_terminal_ref_termprop_uri(vte_terminal, VTE_TERMPROP_CURRENT_DIRECTORY_URI);
    std::string strUri;
    if (uri) {
        auto chrUri = g_uri_to_string(uri);
        //std::cout << "terminalKeyEvent dir " << strUri << std::endl;
        strUri = chrUri;
        g_free(chrUri);
        g_uri_unref(uri);
        auto selected = vte_terminal_get_text_selected(vte_terminal, VTE_FORMAT_TEXT);
        auto file = Gio::File::create_for_uri(strUri);
        if (selected) {
            auto testFile = file->get_child(selected);
            if (testFile->query_exists()) { // if get a file from selection use it.
                strUri = testFile->get_uri();
            }
            g_free(selected);
        }
    }
    else {
        std::cout << "no path from terinal use in .bashrc \". /etc/profile.d/vte.sh\"" << std::endl;
    }
    return strUri;
}

static void
terminalExited(VteTerminal* self, gint status, gpointer user_data)
{
    auto varselView = static_cast<VarselView*>(user_data);
    varselView->close();
}

static bool
terminalKeyEvent(GtkWidget* widget, GdkEventKey* key, gpointer user_data)
{
    auto varselView = static_cast<VarselView*>(user_data);
    if (key->keyval == GDK_KEY_F9
     && key->type == GDK_KEY_RELEASE
     && ((key->state & GDK_SHIFT_MASK) == 0)
     && ((key->state & GDK_CONTROL_MASK) == 0)
     && ((key->state & GDK_MOD1_MASK) == 0) ) {   // represents alt
        //const char* dir = vte_terminal_get_current_directory_uri(vte_terminal);   deprecated and empty
        //std::cout << "Got uri" << std::endl;
        auto uri = getWhere(widget);
        if (varselView && !uri.empty()) {
            varselView->showFile(uri);
        }
        else {
            std::cout << "terminalKeyEvent missing user_data/no uri " << uri << std::endl;
        }
        return true;    // steal
    }
    else if ((key->keyval == GDK_KEY_T)
     && key->type == GDK_KEY_RELEASE
     && ((key->state & GDK_SHIFT_MASK) != 0)
     && ((key->state & GDK_CONTROL_MASK) != 0)
     && ((key->state & GDK_MOD1_MASK) == 0) ) {
        auto uri = getWhere(widget);
        if (varselView && !uri.empty()) {
            //varselView->m_openUri = uri;
            //varselView->m_dispatchOpen.emit();
            varselView->openTerm(uri);    // has issues with event context?
        }
        else {
            std::cout << "terminalKeyEvent missing user_data/no uri " << uri << std::endl;
        }
        return true;    // steal
    }
//    std::cout << "key " << key->keyval
//              << " type " << (key->type == GDK_KEY_PRESS
//                            ? "press"
//                            : (key->type == GDK_KEY_RELEASE
//                                ? "release"
//                                : "?"))
//              << (key->state & GDK_SHIFT_MASK ? " shift " : "")
//              << (key->state & GDK_CONTROL_MASK ? " ctrl " : "")
//              << (key->state & GDK_MOD1_MASK ? " alt" : "")
//              << " user " << std::hex << user_data << std::dec << std::endl;
    return false;   // continue processing
}

static void
childSetupFunc(gpointer user_data)
{
    //std::cout << "childSetupFunc "
    //          << " user " << std::hex << user_data << std::dec << std::endl;
    auto varselView = reinterpret_cast<VarselView*>(user_data);
    if (varselView) {
        varselView->apply_dir();
    }
}

static void
terminalSpawnAsyncCallback(
      VteTerminal* terminal
    , GPid pid
    , GError* error
    , gpointer user_data)
{
    //std::cout << "started pid " << pid
    //          << " user " << std::hex << user_data << std::dec << std::endl;

//    auto varselWin = reinterpret_cast<VarselWin*>(user_data);
//    if (varselWin) {
//        varselWin->apply_dir();
//    }

    g_signal_connect(GTK_WIDGET(terminal)
                    , "key_press_event"
                    , GCallback(&(terminalKeyEvent))
                    , user_data);
    g_signal_connect(GTK_WIDGET(terminal)
                    , "key_release_event"
                    , GCallback(&(terminalKeyEvent))
                    , user_data);
    g_signal_connect(GTK_WIDGET(terminal)
                    , "child-exited"
                    , GCallback(&(terminalExited))
                    , user_data);
}

TabLabel::TabLabel(const Glib::ustring& label, VarselView* varselView)
: EventBox::EventBox()
{
    auto button = Gtk::make_managed<Gtk::Button>();
    auto image = Gtk::make_managed<Gtk::Image>(Gtk::Stock::CLOSE, Gtk::ICON_SIZE_MENU);
    button->set_image(*image);
    button->set_relief(Gtk::ReliefStyle::RELIEF_NONE);

    button->set_focus_on_click(false);
    button->signal_clicked().connect(
            sigc::mem_fun(*varselView, &VarselView::close));

    auto glabel = Gtk::make_managed<Gtk::Label>(label);
    glabel->set_use_markup(false);
    glabel->set_use_underline(false);

    auto hbox = Gtk::make_managed<Gtk::HBox>(false, 0);
    hbox->set_spacing(0);
    hbox->add(*glabel);
    hbox->add(*button);

    add(*hbox);
    show_all();
}


VarselView::VarselView(const std::string& path, VarselWin* varselWin)
: m_uri{path}
, m_varselWin{varselWin}
{
    m_scrollView = Gtk::make_managed<Gtk::ScrolledWindow>();
    m_scrollView->set_propagate_natural_height(true);
    m_scrollView->set_propagate_natural_width(true);
    m_vte_terminal = VTE_TERMINAL(vte_terminal_new());
    vte_terminal_set_size(m_vte_terminal, 120, 40);

    /* see @sakura Figure out if we have rgba capabilities. Without this transparency won't work as expected */
//	GtkCssProvider* provider = gtk_css_provider_new();
//	GdkScreen* screen = gtk_widget_get_screen(GTK_WIDGET(sakura.fade_window));
//	gtk_css_provider_load_from_data(provider, FADE_WINDOW_CSS, -1, NULL);
//	gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
//	g_object_unref(provider);

//	GdkScreen* screen = gtk_widget_get_screen (GTK_WIDGET(m_vte_terminal));
//	GdkVisual* visual = gdk_screen_get_rgba_visual(screen);
//    std::cout << "Is composite " << std::boolalpha << gdk_screen_is_composited(screen) << std::endl;
//    std::cout << "visual " << std::hex << visual << std::endl;
//	if (visual != NULL && gdk_screen_is_composited(screen)) {
//		gtk_widget_set_visual(GTK_WIDGET(m_vte_terminal), visual);
//	}

    m_defaultFont = pango_font_description_copy(vte_terminal_get_font(m_vte_terminal));
    gtk_container_add(GTK_CONTAINER(m_scrollView->gobj()), GTK_WIDGET(m_vte_terminal));

    const char* shell = getenv("SHELL");
    if (!shell) {
        shell = "/bin/sh";
    }
    std::array<const char*,2> argv;
    argv[0] = shell;
    argv[1] = nullptr;
    vte_terminal_spawn_async(
          m_vte_terminal
        , VtePtyFlags::VTE_PTY_DEFAULT
        , nullptr                           // current dir
        , const_cast<char**>(&argv[0])      // argv cast need to match decl
        , nullptr                           // env (inherrit) or ? __environ
        , GSpawnFlags::G_SPAWN_DEFAULT      // spawn_flags ? do_not_reap_child
        , childSetupFunc                    // child_setup
        , this                              // child_setup_data
        , nullptr                           // GDestroyNotify
        , -1                                // timeout (default)
        , nullptr                           // cancelable
        , terminalSpawnAsyncCallback        // VteTerminalSpawnAsyncCallback
        , this);                            // userdata

}

void
VarselView::apply_dir()
{
    auto path = getPath();
    if (!path.empty()) {
        chdir(path.c_str());
        // example howto paste command into terminal
        //std::string cd = std::string("cd ") + file->get_path() + "\n";
        //vte_terminal_feed_child(m_vte_terminal, cd.c_str(), cd.length());

    }
}

void
VarselView::showFile(const std::string& uri)
{
    m_varselWin->showFile(uri);
}

void
VarselView::openTerm(const std::string& uri)
{
    //std::cout << "VarselView::openTerm " << uri << std::endl;
    m_varselWin->openTerm(uri);
}

void
VarselView::close()
{
    // as it seems the closing logic is done by default
    if (!m_closing) {   // avoid double activation on gui click (as event is delivered )
        m_closing = true;
        m_varselWin->close(this);
    }
}

Gtk::Widget*
VarselView::getLabel()
{
    return Gtk::make_managed<TabLabel>(getName(), this);
}

void
VarselView::apply_font(const Glib::ustring& font, const Gdk::RGBA& backgrd)
{
    //std::cout << "VarselWin::apply_font " << font << std::endl;
    if (!font.empty()) {
        Pango::FontDescription desc = Pango::FontDescription(font);
        //std::cout << "VarselWin::apply_font " << desc.to_string() << std::endl;
        //auto font_desc = pango_font_description_from_string(font_name.c_str());
        vte_terminal_set_font(m_vte_terminal, desc.gobj());
        //pango_font_description_free(font_desc);
    }
    else if (m_defaultFont) {
        vte_terminal_set_font(m_vte_terminal, m_defaultFont);
    }

    vte_terminal_set_color_background(m_vte_terminal, backgrd.gobj());
}

Gtk::ScrolledWindow*
VarselView::getScroll()
{
    return m_scrollView;
}

std::string
VarselView::getUri()
{
    std::string ret;
    auto uri = vte_terminal_ref_termprop_uri(m_vte_terminal, VTE_TERMPROP_CURRENT_DIRECTORY_URI);
    if (uri) {
        auto chrUri = g_uri_to_string(uri);
        ret = std::string(chrUri);
        // g_free or g_object_unref(uri);  //will crash
        g_free(chrUri);
    }
    return ret;
}

Glib::ustring
VarselView::getPath()
{
    Glib::ustring path;
    auto file = Gio::File::create_for_uri(m_uri);  // better way to use use only path?
    if (file->get_uri_scheme() == "file"
     && file->query_exists()) { // only apply if exists
        auto fileType = file->query_file_type(); // check  is dir
        if (fileType == Gio::FileType::FILE_TYPE_DIRECTORY) {
            path = file->get_path();
        }
    }
    return path;
}

Glib::ustring
VarselView::getName()
{
    if (m_name.empty()) {
        auto path = getPath();
        if (path.empty()) {
            m_name = Glib::get_current_dir();
        }
        else {
            m_name = path;
        }
        auto home = Glib::get_home_dir();       // use ~ for home
        if (m_name.length() >= home.length()
         && m_name.substr(0, home.length()) == home) {
            m_name = "~" + m_name.substr(home.length());
        }
    }
    return m_name;
}

VarselWin::VarselWin(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder, VarselApp* varselApp)
: Gtk::ApplicationWindow(cobject)       //Calls the base class constructor
, m_application{varselApp}
{
    set_title(_("Varsel"));
    auto pix = Gdk::Pixbuf::create_from_resource(varselApp->get_resource_base_path() + "/varsel.png");
    set_icon(pix);
	/* see @sakura Main window opacity must be set. Otherwise vte widget will remain opaque */
    auto screen = get_screen();
    auto visual = screen->get_rgba_visual();
	if (visual && screen->is_composited()) {
        gtk_widget_set_visual(GTK_WIDGET(gobj()), visual->gobj());
	}
    auto backgrd = m_application->getKeyFile()->getColor(CONFIG_GRP, PrefDialog::CONFIG_BACKGROUND);
    set_opacity(backgrd.get_alpha());   // make this transparent so the term opacity is shown

    refBuilder->get_widget("notebook", m_notebook);
    auto config = getKeyFile();
    for (size_t i = 1; i <= MAX_PATHS; ++i) {
        auto key = Glib::ustring::sprintf(FMT_PATH, CONFIG_PATH, i);
        Glib::ustring uri = config->getString(CONFIG_GRP, key);
        if (i == 1 || !uri.empty()) {
            openTerm(uri);
        }
    }


    activate_actions();
    show_all_children();
}


std::shared_ptr<VarselConfig>
VarselWin::getKeyFile()
{
    return m_application->getKeyFile();
}

void
VarselWin::showFile(const std::string& uri)
{
    auto file = Gio::File::create_for_uri(uri);
    std::vector<Glib::RefPtr<Gio::File>> files;
    files.push_back(file);
    showFiles(files);
}

//void
//VarselWin::checkAfterSend(const std::shared_ptr<BusEvent>& event)
//{
//    //std::cout << "after send compl " << std::boolalpha << event->isCompleted() << std::endl;
//    if (!event->isCompleted()) {
//        auto msg = event->getCompletionInfo();
//        if (!msg.empty()){
//            auto showMsg = psc::fmt::vformat(_("Unhandled files {}"), psc::fmt::make_format_args(msg));
//            showMessage(showMsg, Gtk::MessageType::MESSAGE_WARNING);
//        }
//    }
//}

void
VarselWin::showFiles(const std::vector<Glib::RefPtr<Gio::File>>& files)
{
    auto openEvent = std::make_shared<OpenEvent>();
    openEvent->setContext(files);
    if (openEvent->isAvail()) {
        m_application->getEventBus()->send(openEvent);
        //checkAfterSend(openEvent);
    }
    else {
        std::cout << "VarselWin::showFiles open action not availabele" << std::endl;
    }
}

void
VarselWin::openTerm(const std::string& uri)
{
    //std::cout << "openTerm "
    //          << " uri \"" << uri << "\"" << std::endl;
    auto view = std::make_shared<VarselView>(uri, this);
    view->apply_font(getFont(), getBackground());
    auto widget = view->getScroll();
    m_notebook->append_page(*widget, *view->getLabel());
    widget->show_all();
    m_views.push_back(view);
}

VarselApp*
VarselWin::getApplication()
{
    return m_application;
}

void
VarselWin::activate_actions()
{
    auto pref_action = Gio::SimpleAction::create("config");
    pref_action->signal_activate().connect(
        [this]  (const Glib::VariantBase& value)
		{
			try {
				auto builder = Gtk::Builder::create();
				PrefDialog* prefDialog;
				builder->add_from_resource(m_application->get_resource_base_path() + "/pref-dlg.ui");
				builder->get_widget_derived("PrefDialog", prefDialog, this);
				prefDialog->run();
				delete prefDialog;  // as this is a toplevel component shoud destroy -> works
			}
			catch (const Glib::Error &ex) {
                showMessage(psc::fmt::vformat(
                        _("Unable to load {} error {}"),
                          psc::fmt::make_format_args("pref-dlg", ex)), Gtk::MessageType::MESSAGE_WARNING);
			}
		});
    add_action(pref_action);

}

std::string
VarselWin::getFont()
{
    std::string font;
    auto config = getKeyFile();
    if (config->hasKey(CONFIG_GRP, PrefDialog::DEFAULT_FONT)) {
        if (!config->getBoolean(CONFIG_GRP, PrefDialog::DEFAULT_FONT, true)
         && config->hasKey(CONFIG_GRP, PrefDialog::CONFIG_FONT)) {
            font = config->getString(CONFIG_GRP, PrefDialog::CONFIG_FONT);
        }
    }
    return font;
}

Gdk::RGBA
VarselWin::getBackground()
{
    auto config = getKeyFile();
    Gdk::RGBA backgrd{config->getColor(CONFIG_GRP, PrefDialog::CONFIG_BACKGROUND)};
    return backgrd;
}

void
VarselWin::apply_font(const std::string& font, const Gdk::RGBA& backgrd)
{
    set_opacity(backgrd.get_alpha());   // make this transparent so the term opacity is shown
    for (auto& view : m_views) {
        view->apply_font(font, backgrd);
    }
}

void
VarselWin::close(VarselView* varselView)
{
    for (auto& view : m_views) {
        if (view.get() == varselView) {
            m_notebook->remove_page(*view->getScroll());
            m_views.remove(view);
            break;
        }
    }
}


void
VarselWin::showMessage(const Glib::ustring& msg, Gtk::MessageType msgType)
{
    Gtk::MessageDialog messagedialog(*this, msg, false, msgType);
    messagedialog.set_transient_for(*this);
    messagedialog.run();
    messagedialog.hide();
}



void
VarselWin::on_hide()
{
    auto iter = m_views.begin();
    for (size_t i = 1; i <= MAX_PATHS; ++i) {   // rewrite all key to a value
        std::string uri;
        if (iter != m_views.end()) {
            auto& view = *iter;
            uri = view->getUri();
            ++iter;
        }
        auto key = Glib::ustring::sprintf(FMT_PATH, CONFIG_PATH, i);
        //std::cout << "hide " << i << " key " << key << " uri " << uri << std::endl;
        getKeyFile()->setString(CONFIG_GRP, key, uri);
    }
    m_application->save_config();
    Gtk::ApplicationWindow::on_hide();
//    for (auto i = m_lists.begin(); i != m_lists.end(); ) {
//        auto ref = *i;
//        //std::cout << std::hex << "Hiding " << ref << std::dec << std::endl;
//        ref->setVarselWin(nullptr); // avoid concurrent destruction
//        ref->hide();
//        delete ref;
//        i = m_lists.erase(i);
//    }
}
