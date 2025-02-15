/* -*- Mode: C; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2020 rpf
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

#pragma once

#include <gtkmm.h>
#include <vector>
#include <memory>
#include <Log.hpp>
#include <vte/vte.h>

#include "DataSource.hpp"

class VarselApp;
class VarselList;
class KeyConfig;
class VarselWin;
class TabLabel;

class VarselView
{
public:
    VarselView(const std::string& uri, VarselWin* varselWin);
    explicit VarselView(const VarselView& varselView) = delete;
    virtual ~VarselView() = default;

    void apply_dir();
    void showFile(const std::string& uri);
    void openTerm(const std::string& uri);
    void apply_font(const Glib::ustring& font);
    Gtk::ScrolledWindow* getScroll();
    Gtk::Widget* getLabel();
    std::string getUri();
    Glib::ustring getName();
    void close();
protected:
    Glib::ustring getPath();
private:
    std::string m_uri;
    Glib::ustring m_name;
    VarselWin* m_varselWin;
    Gtk::ScrolledWindow* m_scrollView;
    VteTerminal* m_vte_terminal;
    PangoFontDescription* m_defaultFont{nullptr};
};


class TabLabel
: public Gtk::EventBox
{
public:

    TabLabel(const Glib::ustring& label, VarselView* varselView);
    explicit TabLabel(const TabLabel& notebookTabLabel) = delete;
    virtual ~TabLabel() = default;

    //void close();
private :
    //VarselView* m_varselView;
};

class VarselWin
: public Gtk::ApplicationWindow
, public EventNotifyContext
{
public:
    VarselWin(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder, VarselApp* varselApp);
    virtual ~VarselWin() = default;

    void on_hide() override;
    void show_error(const Glib::ustring& msg, Gtk::MessageType type = Gtk::MessageType::MESSAGE_WARNING);
    void showFile(const std::string& uri);
    void showFiles(const std::vector<Glib::RefPtr<Gio::File>>& files);
    void checkAfterSend(const std::shared_ptr<BusEvent>& event) override;
    void openTerm(const std::string& uri);
    void remove(VarselList* varselList);
    VarselApp* getApplication();
    std::string getFont();
    void apply_font(const std::string& font);
    void close(VarselView* view);
    std::shared_ptr<KeyConfig> getKeyFile();

    static constexpr auto MAX_PATHS = 20u;
    static constexpr auto FMT_PATH = "%s%02d";
    static constexpr auto CONFIG_GRP = "varsel";
    static constexpr auto CONFIG_PATH = "path";
protected:
    void showMessage(const Glib::ustring& msg, Gtk::MessageType msgType = Gtk::MessageType::MESSAGE_INFO);

private:
    void activate_actions();

    Gtk::Notebook* m_notebook;
    VarselApp* m_application;
    std::shared_ptr<psc::log::Log> m_log;
    std::list<std::shared_ptr<VarselView>> m_views;
};

