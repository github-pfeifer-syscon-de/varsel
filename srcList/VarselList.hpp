/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2024 RPf <gpl3@pfeifer-syscon.de>
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

#include <memory>
#include <gtkmm.h>
#include <KeyfileTableManager.hpp>
#include <TreeNodeModel.hpp>

#include "DataSource.hpp"
#include <VarselConfig.hpp>


class ListApp;

class VarselList
: public Gtk::ApplicationWindow
, public ListListener
{
public:
    VarselList(
          BaseObjectType* cobject
        , const Glib::RefPtr<Gtk::Builder>& builder
        , ListApp* listApp);
    explicit VarselList(const VarselList& orig) = delete;
    virtual ~VarselList() = default;
    void on_hide() override;
    static VarselList* show(
          const Glib::ustring& name
        , const std::shared_ptr<DataSource>& data
        , ListApp* varselWin);
    bool on_view_button_press_event(GdkEventButton* event);
    bool on_view_button_release_event(GdkEventButton* event);
    void nodeAdded(const std::shared_ptr<BaseTreeNode>& baseTreeNode) override;
    void listDone(Severity severity, const Glib::ustring& msg) override;
    void showMessage(const Glib::ustring& msg, Gtk::MessageType msgType = Gtk::MessageType::MESSAGE_INFO);

    void showFile(const Glib::RefPtr<Gio::File>& file);
    //static constexpr auto ACTION_GROUP = "list";
    static constexpr auto PANED_POS = "panedPos";
    std::shared_ptr<VarselConfig> getKeyFile();
    void save_config();
protected:
    void updateList();
    std::shared_ptr<DataSource> setupDataSource(const Glib::RefPtr<Gio::File>& file);

private:
    Glib::RefPtr<Gtk::TreeView> m_treeView;
    Glib::RefPtr<Gtk::TreeView> m_listView;
    std::shared_ptr<DataSource> m_data;
    ListApp* m_listApp;
    std::shared_ptr<VarselConfig> m_config;

    std::shared_ptr<psc::ui::KeyfileTableManager> m_kfTableManager;

    std::vector<pDataAction> m_actions;
    Glib::RefPtr<psc::ui::TreeNodeModel> m_refTreeModel;
    Gtk::Paned* m_paned;
};


