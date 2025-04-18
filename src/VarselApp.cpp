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
#include <exception>
#include <psc_i18n.hpp>
#include <psc_format.hpp>
#include <locale>
#include <clocale>
#include <git2.h>
#include <string_view>

#include "config.h"
#include "VarselApp.hpp"

VarselApp::VarselApp(int argc, char **argv)
: Gtk::Application(argc, argv, "de.pfeifer_syscon.varsel", Gio::ApplicationFlags::APPLICATION_HANDLES_OPEN)
, m_exec{argv[0]}
, m_log{psc::log::Log::create("varsel")}
{
}


void
VarselApp::on_activate()
{
    // will be used when called w/o parameter
    getOrCreateVarselWindow();
    Gtk::Application::on_activate();

}

VarselWin*
VarselApp::createVarselWindow()
{
    VarselWin* varselWindow{nullptr};
    auto builder = Gtk::Builder::create();
    try {
        builder->add_from_resource(get_resource_base_path() + "/varsel-appwin.ui");
        builder->get_widget_derived("Varsel", varselWindow, this);
        add_window(*varselWindow);      // do this in this order to ensures the menu-bar comes up...
        varselWindow->show();
    }
    catch (const Glib::Error &ex) {
        std::cerr << "Unable to load varselwindow " << ex.what() << std::endl;
    }
    return varselWindow;
}


VarselWin*
VarselApp::getOrCreateVarselWindow()
{
    // The application has been asked to open some files,
    // so let's open a new view for each one.
    VarselWin* appwindow = nullptr;
    auto windows = get_windows();
    for (size_t i = 0; i < windows.size(); ++i) {   // as there might be multiple windows
        auto testwindow = dynamic_cast<VarselWin*>(windows[i]);
        if (testwindow != nullptr) {
            appwindow = testwindow;
            break;
        }
    }
    if (!appwindow) {
        //std::cout << "No window matched creating" << std::endl;
        m_varselWindow = createVarselWindow();
    }
    else {
        //std::cout << "using existing window" << std::endl;
        // need to add to mode
        m_varselWindow = appwindow;
    }
    return m_varselWindow;
}


void
VarselApp::on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint)
{
    //std::cout << "VarselApp::on_open " << files.size() << std::endl;
    std::vector<Glib::RefPtr<Gio::File>> gioFiles;
    gioFiles.reserve(files.size());
    for (size_t i = 0; i < files.size(); ++i) {
        auto uri = files[i]->get_uri();
        std::cout << "Uri " << uri << std::endl;
        gioFiles.push_back(files[i]);
    }
    auto varselWindow = getOrCreateVarselWindow();
    auto mainCtx = Glib::MainContext::get_default();
    mainCtx->signal_idle().connect_once([=] () {  // delay opening
        varselWindow->openFiles(files);
    });
}


void
VarselApp::on_action_quit()
{
    m_varselWindow->hide();

    // and we shoud delete appWindow if we were not going exit anyway
    // Not really necessary, when Gtk::Widget::hide() is called, unless
    // Gio::Application::hold() has been called without a corresponding call
    // to Gio::Application::release().
    quit();
}

void
VarselApp::on_action_about() {
    auto builder = Gtk::Builder::create();
    try {
        builder->add_from_resource(get_resource_base_path() + "/abt-dlg.ui");
        auto dlgObj = builder->get_object("abt-dlg");
        auto dialog = Glib::RefPtr<Gtk::AboutDialog>::cast_dynamic(dlgObj);
        auto icon = Gdk::Pixbuf::create_from_resource(get_resource_base_path() + "/calcpp.png");
        dialog->set_logo(icon);
        dialog->set_transient_for(*m_varselWindow);
        dialog->show_all();
        dialog->run();
        dialog->hide();
    }
    catch (const Glib::Error &ex) {
        std::cerr << "Unable to load about-dialog: " << ex.what() << std::endl;
    }
}

std::string
VarselApp::get_file(const std::string& name)
{
    auto fullPath = Glib::canonicalize_filename(Glib::StdStringView(m_exec), Glib::get_current_dir());
    Glib::RefPtr<Gio::File> f = Gio::File::create_for_path(fullPath);
    auto dist_dir = f->get_parent()->get_parent();
    // this file identifies the development dir, beside executable
    auto readme = Glib::build_filename(dist_dir->get_path(), name);
    if (!Glib::file_test(readme, Glib::FileTest::FILE_TEST_IS_REGULAR)) {
        // alternative search in distribution
        Glib::RefPtr<Gio::File> packageData = Gio::File::create_for_path(PACKAGE_DATA_DIR); // see Makefile.am
        std::string base_name = packageData->get_basename();
        readme = Glib::build_filename(packageData->get_parent()->get_path(), "doc", base_name, name);
    }
    return readme;
}


void
VarselApp::on_action_help() {
    auto builder = Gtk::Builder::create();
    try {
        builder->add_from_resource(get_resource_base_path() + "/help-dlg.ui");
        auto dlgObj = builder->get_object("help-dlg");
        auto dialog = Glib::RefPtr<Gtk::Dialog>::cast_dynamic(dlgObj);
        auto textObj = builder->get_object("text");
        auto text = Glib::RefPtr<Gtk::TextView>::cast_dynamic(textObj);

        text->get_buffer()->set_text("lalala");
        dialog->set_transient_for(*m_varselWindow);
        dialog->show_all();
        dialog->run();
        dialog->hide();
    }
    catch (const Glib::Error &ex) {
        std::cerr << "Unable to load help-dlg.ui " << ex.what() << std::endl;
    }
}


void
VarselApp::on_startup()
{
    Gtk::Application::on_startup();

    add_action("quit", sigc::mem_fun(*this, &VarselApp::on_action_quit));
    add_action("about", sigc::mem_fun(*this, &VarselApp::on_action_about));
    add_action("help", sigc::mem_fun(*this, &VarselApp::on_action_help));

    auto builder = Gtk::Builder::create();
    try {
        builder->add_from_resource(get_resource_base_path() + "/app-menu.ui");
        auto menuObj = builder->get_object("menubar");
        auto menuBar = Glib::RefPtr<Gio::Menu>::cast_dynamic(menuObj);
        if (menuBar) {
            set_menubar(menuBar);
        }
        else {
            psc::log::Log::logAdd(psc::log::Level::Error, "Cound not find/cast menubar!" );
        }
    }
    catch (const Glib::FileError& ex) {
        psc::log::Log::logAdd(psc::log::Level::Error, std::format("Unable to load app-menu {}", ex.what()));
    }
}

void
VarselApp::save_config()
{
    try {
        m_config->saveConfig();
    }
    catch (const Glib::Error& exc) {
        psc::log::Log::logAdd(psc::log::Level::Error, psc::fmt::format("File error {} saving config", exc));
    }
}

std::shared_ptr<VarselConfig>
VarselApp::getKeyFile()
{
    if (!m_config) {
        m_config = std::make_shared<VarselConfig>("varsel.conf");
    }
    return m_config;
}


std::shared_ptr<EventBus>
VarselApp::getEventBus()
{
    if (!m_eventBus) {
        m_eventBus = std::make_shared<EventBus>();
    }
    return m_eventBus;
}

int
main(int argc, char** argv)
{
    char* loc = std::setlocale(LC_ALL, "");
    if (loc == nullptr) {
        std::cout << "error setlocale " << std::endl;
    }
    else {
        //std::cout << "setlocale " << loc << std::endl;
        // sync c++
        std::locale::global(std::locale(loc));
    }
    bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
    textdomain(PACKAGE);
    Glib::init();

    VarselApp app(argc, argv);
    return app.run();
}

