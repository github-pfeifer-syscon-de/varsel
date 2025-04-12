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

#include <iostream>
#include <psc_i18n.hpp>
#include <psc_format.hpp>
#include <Log.hpp>


#include "VarselList.hpp"
#include "ListApp.hpp"
#include "FileDataSource.hpp"
#include "ArchiveDataSource.hpp"
#include "GitDataSource.hpp"

VarselList::VarselList(
      BaseObjectType* cobject
    , const Glib::RefPtr<Gtk::Builder>& builder
    , ListApp* listApp)
: Gtk::ApplicationWindow(cobject)
, ListListener()
, m_listApp{listApp}
{
    auto pix = Gdk::Pixbuf::create_from_resource(m_listApp->get_resource_base_path() + "/varsel.png");
    set_icon(pix);

    builder->get_widget("paned", m_paned);

    auto treeObj = builder->get_object("tree_view");
    m_treeView = Glib::RefPtr<Gtk::TreeView>::cast_dynamic(treeObj);

    auto listObj = builder->get_object("list_view");
    m_listView = Glib::RefPtr<Gtk::TreeView>::cast_dynamic(listObj);
    m_listView->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_MULTIPLE);

    m_listView->signal_button_press_event().connect(
        sigc::mem_fun(*this, &VarselList::on_view_button_press_event), false);
    m_listView->signal_button_release_event().connect(
        sigc::mem_fun(*this, &VarselList::on_view_button_release_event), false);
    m_listView->add_events(Gdk::EventMask::BUTTON_PRESS_MASK
                         | Gdk::EventMask::BUTTON_RELEASE_MASK);
    set_default_size(640, 480);
}

void
VarselList::showFile(const Glib::RefPtr<Gio::File>& file)
{
    auto info = file->query_info("*");
    set_title(info->get_display_name());
    m_data = setupDataSource(file);
    m_treeView->append_column(_("Name"), m_data->m_treeColumns->m_name);
    m_refTreeModel = m_data->createTree();
    // create individual config instances so wo keep the interference to a minimum (but some might be inevitable, if multiple instances exist, the last saved wins)
    std::string confName = std::string("va_") + m_data->getConfigGroup() + std::string(".conf");
    m_config = std::make_shared<VarselConfig>(confName.c_str());

    int pos = m_config->getInteger(m_data->getConfigGroup(), PANED_POS, 200);
    m_paned->set_position(pos);

    //tree->clear();    consider these for update ?
    //list->clear();
    m_data->update(m_refTreeModel, this);

    m_data->addActions(m_actions);
    for (auto& action : m_actions) {
        auto gAction = action->getAction();
        add_action(gAction);
    }

    m_treeView->set_model(m_refTreeModel);
    m_treeView->expand_all();
    m_treeView->get_selection()->signal_changed().connect(
            sigc::mem_fun(*this, &VarselList::updateList));

    m_kfTableManager = std::make_shared<psc::ui::KeyfileTableManager>(m_data->getListColumns(), getKeyFile()->getConfig(), m_data->getConfigGroup());
    m_kfTableManager->setup(this);
    m_kfTableManager->setup(m_listView);

    auto chlds = m_refTreeModel->children();
    if (!chlds.empty()) {
        m_treeView->get_selection()->select(chlds.begin());
        updateList();
    }

}

void
VarselList::save_config()
{
    try {
        m_config->saveConfig();
    }
    catch (const Glib::Error& exc) {
        psc::log::Log::logAdd(psc::log::Level::Error, psc::fmt::format("File error {} saving config", exc));
    }
}

std::shared_ptr<VarselConfig>
VarselList::getKeyFile()
{
    return m_config;
}

std::shared_ptr<DataSource>
VarselList::setupDataSource(const Glib::RefPtr<Gio::File>& file)
{
    std::shared_ptr<DataSource> ds;
    auto gitDir = file->get_child(".git");  // wild guess identify or use git_repository_discover
    auto type = file->query_file_type();
    if (type == Gio::FileType::FILE_TYPE_REGULAR
     && ArchiveDataSource::can_handle(file)) {
        ds = std::make_shared<ArchiveDataSource>(file, m_listApp);
    }
    else if (type == Gio::FileType::FILE_TYPE_DIRECTORY
          && gitDir->query_exists()) {
        ds = std::make_shared<GitDataSource>(file, m_listApp);
    }
    else if (type == Gio::FileType::FILE_TYPE_DIRECTORY) {
        ds = std::make_shared<FileDataSource>(file, m_listApp);
    }
    //if (ds) {
    //    VarselList::show(file->get_uri(), ds, m_listApp);
    //    std::cout << "ListFactory::notify remove " << item->getFile()->get_path() << std::endl;
    //}
    return ds;
}

void
VarselList::showMessage(const Glib::ustring& msg, Gtk::MessageType msgType)
{
    Gtk::MessageDialog messagedialog(*this, msg, false, msgType);
    messagedialog.set_transient_for(*this);
    messagedialog.run();
    //messagedialog.hide(); the example leaves this out?
}

void
VarselList::nodeAdded(const std::shared_ptr<BaseTreeNode>& baseTreeNode)
{
    if (baseTreeNode->getDepth() <= 2) {
        m_treeView->expand_to_path(baseTreeNode->getPath());
    }
}

void
VarselList::listDone(Severity severity, const Glib::ustring& msg)
{
    std::cout << "VarselList::listDone " << msg << std::endl;
    //if (severity > Severity::Info) {
    showMessage(msg, severity == Severity::Warning ? Gtk::MessageType::MESSAGE_WARNING : Gtk::MessageType::MESSAGE_ERROR);
    //}
}

void
VarselList::updateList()
{
    auto sel = m_treeView->get_selection();
    auto iter = sel->get_selected();
    if (iter) {
        auto row = m_refTreeModel->get_node(iter);
        auto ftn = dynamic_cast<BaseTreeNode*>(row);
        if (ftn) {
            m_listView->set_model(ftn->getEntries());
        }
        else {
            std::cout << "VarselList::updateList no BaseTreeNode" << std::endl;
        }
    }
    else {
        auto model = Gtk::ListStore::create(*m_data->getListColumns());
        m_listView->set_model(model);
    }
}

bool
VarselList::on_view_button_press_event(GdkEventButton* event)
{
    if (event->button == GDK_BUTTON_SECONDARY) {
        return true;    // avoid changing selection before popup
        //std::cout << "VarselList::on_button_release_event "
        //          << std::hex << event->type
        //          << " press " << GdkEventType::GDK_BUTTON_PRESS << std::dec
        //          << " btn " << event->button << std::endl;
    }
    return false;
}

bool
VarselList::on_view_button_release_event(GdkEventButton* event)
{
    //auto gdkEvent = Glib::wrap(reinterpret_cast<GdkEvent*>(event), true);
    if (event->button == GDK_BUTTON_SECONDARY) {    // completly capture second secondary button
        // if avail -> gdkEvent.triggers_context_menu()
        auto gioMenu = Gio::Menu::create();
        std::vector<Glib::RefPtr<Gio::File>> files;
        auto select = m_listView->get_selection();
        if (select->count_selected_rows() > 0) {
            auto paths = select->get_selected_rows();
            for (auto path : paths) {
                auto model = m_listView->get_model();
                auto iter = model->get_iter(path);
                auto row = *iter;
                auto listCols = m_data->getListColumns();
                auto file = row.get_value(listCols->m_file);
                if (file->query_exists()) {
                    files.push_back(file);
                }
            }
        }
        else {
            Gtk::TreeModel::Path path;
            if (m_listView->get_path_at_pos(event->x, event->y, path)) {
                auto model = m_listView->get_model();
                auto iter = model->get_iter(path);
                auto row = *iter;
                auto listCols = m_data->getListColumns();
                auto file = row.get_value(listCols->m_file);
                if (file->query_exists()) {
                    files.push_back(file);
                }
            }
            else {
                // Not clicked into list?
            }
        }
        // -> check if context is not empty
        for (auto& action : m_actions) {
            action->setContext(files);
            //action->setEventNotifyContext(m_listApp);
            if (action->isAvail()) {
                auto menuItem = Gio::MenuItem::create(action->getLabel(), "win." + action->getName());
                gioMenu->append_item(menuItem);
            }
        }
        auto menu = Gtk::make_managed<Gtk::Menu>(gioMenu);
        menu->show_all();
        menu->attach_to_widget(*this); // this does the trick and calls the destructor
        menu->popup(event->button, event->time);
        return true;    // we are "done" (prevent secondary click to change selected)
    }
    return false;
}



void
VarselList::on_hide()
{
    //std::cout << "VarselList::on_hide" << std::endl;
    int pos = m_paned->get_position();
    auto config = getKeyFile();
    config->setInteger(m_data->getConfigGroup(), PANED_POS, pos);
    m_kfTableManager->saveConfig(this);
    save_config();
    Gtk::Window::on_hide();
}

VarselList*
VarselList::show(
      const Glib::ustring& name
    , const std::shared_ptr<DataSource>& data
    , ListApp* listApp)
{
    VarselList* varselList = nullptr;
    auto builder = Gtk::Builder::create();
    try {
        builder->add_from_resource(listApp->get_resource_base_path() + "/varsel-win.ui");
        builder->get_widget_derived("VarselList", varselList, listApp);
        listApp->add_window(*varselList);
        varselList->show_all();
    }
    catch (const Glib::Error &ex) {
        //listApp->showMessage(
        //    psc::fmt::vformat(
        //          _("Error {} loading {}")
        //        , psc::fmt::make_format_args(ex, "varselWin")), Gtk::MessageType::MESSAGE_WARNING);
    }
    return varselList;
}
