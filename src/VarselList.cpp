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
#include "VarselWin.hpp"
#include "VarselApp.hpp"
#include "FileDataSource.hpp"
#include "ArchiveDataSource.hpp"
#include "GitDataSource.hpp"

VarselList::VarselList(
      BaseObjectType* cobject
    , const Glib::RefPtr<Gtk::Builder>& builder
    , const Glib::ustring& name
    , const std::shared_ptr<DataSource>& data
    , VarselWin* varselWin)
: Gtk::Window(cobject)
, m_data{data}
, m_varselWin{varselWin}
{
    set_title(name);
    auto pix = Gdk::Pixbuf::create_from_resource(m_varselWin->getApplication()->get_resource_base_path() + "/varsel.png");
    set_icon(pix);

    builder->get_widget("paned", m_paned);
    auto config = m_varselWin->getKeyFile();
    int pos = config->getInteger(m_data->getConfigGroup(), PANED_POS, 200);
    m_paned->set_position(pos);

    m_refTreeModel = data->createTree();
    //tree->clear();    consider these for update ?
    //list->clear();
    data->update(m_refTreeModel);
    auto treeObj = builder->get_object("tree_view");
    m_treeView = Glib::RefPtr<Gtk::TreeView>::cast_dynamic(treeObj);
    m_treeView->append_column(_("Name"), data->m_treeColumns->m_name);

    m_data->addActions(m_actions);
    auto refActionGroup = Gio::SimpleActionGroup::create();
    for (auto& action : m_actions) {
        auto gAction = action->getAction();
        refActionGroup->add_action(gAction);
    }
    insert_action_group(ACTION_GROUP, refActionGroup);

    m_treeView->set_model(m_refTreeModel);
    m_treeView->expand_all();
    m_treeView->get_selection()->signal_changed().connect(
            sigc::mem_fun(*this, &VarselList::updateList));

    auto listObj = builder->get_object("list_view");
    m_listView = Glib::RefPtr<Gtk::TreeView>::cast_dynamic(listObj);
    m_listView->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_MULTIPLE);

    m_kfTableManager = std::make_shared<psc::ui::KeyfileTableManager>(data->getListColumns(), m_varselWin->getKeyFile()->getConfig(), data->getConfigGroup());
    m_kfTableManager->setup(this);
    m_kfTableManager->setup(m_listView);

    auto chlds = m_refTreeModel->children();
    if (!chlds.empty()) {
        m_treeView->get_selection()->select(chlds.begin());
        updateList();
    }

    m_listView->signal_button_press_event().connect(
        sigc::mem_fun(*this, &VarselList::on_view_button_press_event), false);
    m_listView->signal_button_release_event().connect(
        sigc::mem_fun(*this, &VarselList::on_view_button_release_event), false);
    m_listView->add_events(Gdk::EventMask::BUTTON_PRESS_MASK
                         | Gdk::EventMask::BUTTON_RELEASE_MASK);
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
        std::cout << "VarselList::updateList no selection" << std::endl;
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
                auto name = row.get_value(m_data->m_treeColumns->m_name);
                auto fileName = m_data->getFileName(name);
                if (fileName) {
                    files.push_back(fileName);
                }
            }
        }
        else {
            Gtk::TreeModel::Path path;
            if (m_listView->get_path_at_pos(event->x, event->y, path)) {
                auto model = m_listView->get_model();
                auto iter = model->get_iter(path);
                auto row = *iter;
                auto name = row.get_value(m_data->m_treeColumns->m_name);
                auto fileName = m_data->getFileName(name);
                if (fileName) {
                    files.push_back(fileName);
                }
            }
            else {
                // No click into list?
            }

        }
        // -> check if context is not empty
        for (auto& action : m_actions) {
            action->setContext(files);
            if (action->isAvail()) {
                auto menuItem = Gio::MenuItem::create(action->getLabel(), std::string(ACTION_GROUP) + "." + action->getName());
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
    auto config = m_varselWin->getKeyFile();
    config->setInteger(m_data->getConfigGroup(), PANED_POS, pos);
    m_kfTableManager->saveConfig(this);
    Gtk::Window::on_hide();
}

VarselList*
VarselList::show(
      const Glib::ustring& name
    , const std::shared_ptr<DataSource>& data
    , VarselWin* varselWin)
{
    VarselList* varselList = nullptr;
    auto builder = Gtk::Builder::create();
    try {
        auto varselApp = varselWin->getApplication();
        builder->add_from_resource(varselApp->get_resource_base_path() + "/varsel-win.ui");
        builder->get_widget_derived("VarselList", varselList, name, data, varselWin);
        varselApp->add_window(*varselList);
        varselList->show_all();
    }
    catch (const Glib::Error &ex) {
        varselWin->show_error(
            psc::fmt::vformat(
                  _("Error {} loading {}")
                , psc::fmt::make_format_args(ex, "varselWin")));
    }
    return varselList;
}

ListFactory::ListFactory(VarselWin* varselWin)
: m_varselWin{varselWin}
{
}


void
ListFactory::notify(const std::shared_ptr<BusEvent>& busEvent)
{
    auto openEvent = std::dynamic_pointer_cast<OpenEvent>(busEvent);
    if (openEvent) {
        for (auto& item : openEvent->getFiles()) {
            auto file = item->getFile();
            auto type = file->query_file_type();
            //auto basename = file->get_basename();
            //std::cout << "basename " << basename << std::endl;
            std::shared_ptr<DataSource> ds;
            auto gitDir = file->get_child(".git");  // wild guess identify or use git_repository_discover
            if (type == Gio::FileType::FILE_TYPE_REGULAR
             && ArchiveDataSource::can_handle(file)) {
                ds = std::make_shared<ArchiveDataSource>(file, m_varselWin->getApplication());
            }
            else if (type == Gio::FileType::FILE_TYPE_DIRECTORY
                  && gitDir->query_exists()) {
                ds = std::make_shared<GitDataSource>(file, m_varselWin->getApplication());
            }
            else if (type == Gio::FileType::FILE_TYPE_DIRECTORY) {
                ds = std::make_shared<FileDataSource>(file, m_varselWin->getApplication());
            }
            if (ds) {
                /*auto varselList = */
                VarselList::show(file->get_uri(), ds, m_varselWin);
                openEvent->remove(item);
            }
        }
    }
}