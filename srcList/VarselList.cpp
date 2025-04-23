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
#include "config.h"
#include <ListFactory.hpp>
#include <SourceFactory.hpp>

enum class CLIPBOARD_TARGET {
  TEXT_URI_LIST,
  GNOME_COPIED_FILES,
  UTF8_STRING,
};


VarselList::VarselList(
      BaseObjectType* cobject
    , const Glib::RefPtr<Gtk::Builder>& builder
    , ListApp* listApp)
: Gtk::ApplicationWindow(cobject)
, ListListener()
, m_listApp{listApp}
{
    auto pix = Gdk::Pixbuf::create_from_resource(m_listApp->get_resource_base_path() + "/va_list.png");
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
    m_listView->signal_key_release_event().connect(
        sigc::mem_fun(*this, &VarselList::on_view_key_release_event), false);
    m_listView->add_events(Gdk::EventMask::BUTTON_PRESS_MASK
                         | Gdk::EventMask::BUTTON_RELEASE_MASK
                         | Gdk::EventMask::KEY_RELEASE_MASK);
    set_default_size(640, 480);
}

void
VarselList::showFile(const Glib::RefPtr<Gio::File>& file)
{
    auto info = file->query_info("*");
    set_title(info->get_display_name());
    m_data = setupDataSource(file);
    if (m_treeView->get_columns().size() == 0) {
        m_treeView->append_column(_("Name"), m_data->m_treeColumns->m_name);
    }
    m_refTreeModel = m_data->createTree();
    // create individual config instances so wo keep the interference to a minimum (but some might be inevitable, if multiple instances exist, the last saved wins)
    std::string confName = std::string("va_") + m_data->getConfigGroup() + std::string(".conf");
    m_config = std::make_shared<VarselConfig>(confName.c_str());

    int pos = m_config->getInteger(m_data->getConfigGroup(), PANED_POS, 200);
    m_paned->set_position(pos);

    std::shared_ptr<BaseTreeNode> btn;
    m_data->update(file, btn, m_refTreeModel, this);
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
        ds = std::make_shared<ArchiveDataSource>(m_listApp);
    }
    else if (type == Gio::FileType::FILE_TYPE_DIRECTORY
          && gitDir->query_exists()) {
        ds = std::make_shared<GitDataSource>(m_listApp);
    }
    else if (type == Gio::FileType::FILE_TYPE_DIRECTORY) {
        ds = std::make_shared<FileDataSource>(m_listApp);
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
    showMessage(msg,
            severity == Severity::Warning
            ? Gtk::MessageType::MESSAGE_WARNING
            : Gtk::MessageType::MESSAGE_ERROR);
    //}
}

void
VarselList::updateList()
{
    auto sel = m_treeView->get_selection();
    auto iter = sel->get_selected();
    if (iter) {
        auto node = m_refTreeModel->get_node(iter);
        auto btn = dynamic_cast<BaseTreeNode*>(node);
        if (btn) {
            auto ftn = dynamic_cast<FileTreeNode*>(btn);
            if (ftn) {  // no dynamic updating if not filetreenode
                if (ftn && !ftn->isQueried()) {
                    auto file = ftn->getDirFile();
                    // convert to shared ...
                    auto parent  = ftn->getParent();
                    auto sftn = parent->getItem(ftn->getChildIdx());
                    m_data->update(file, sftn, m_refTreeModel, this);
                }
            }
            m_listView->set_model(btn->getEntries());
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
VarselList::getSelection(GdkEventButton* event, std::vector<PtrEventItem>& items)
{
    auto select = m_listView->get_selection();
    if (select->count_selected_rows() > 0) {
        auto paths = select->get_selected_rows();
        for (auto path : paths) {
            auto model = m_listView->get_model();
            auto iter = model->get_iter(path);
            auto row = *iter;
            auto listCols = m_data->getListColumns();
            auto file = row.get_value(listCols->m_file);
            // create in any case as we decide how to distribute by processing in DataModel
            auto item = std::make_shared<EventItem>(file);
            items.emplace_back(std::move(item));
        }
    }
    else if (event != nullptr) {
        Gtk::TreeModel::Path path;
        if (m_listView->get_path_at_pos(event->x, event->y, path)) {
            auto model = m_listView->get_model();
            auto iter = model->get_iter(path);
            auto row = *iter;
            auto listCols = m_data->getListColumns();
            auto file = row.get_value(listCols->m_file);
            // create in any case as we decide how to distribute by processing in DataModel
            auto item = std::make_shared<EventItem>(file);
            items.emplace_back(std::move(item));
        }
        else {
            // Not clicked into list?
        }
    }
    return !items.empty();
}

bool
VarselList::on_view_button_release_event(GdkEventButton* event)
{
    //auto gdkEvent = Glib::wrap(reinterpret_cast<GdkEvent*>(event), true);
    if (event->button == GDK_BUTTON_SECONDARY) {    // completly capture second secondary button
        std::vector<PtrEventItem> items;
        items.reserve(8);
        if (getSelection(event, items)) {
            auto menu = Gtk::make_managed<Gtk::Menu>();
            m_data->distribute(items, menu, this);
            menu->show_all();
            menu->attach_to_widget(*this); // this does the trick and calls the destructor
            menu->popup(event->button, event->time);
        }
        return true;    // we are "done" (prevent secondary click to change selected)
    }
    return false;
}

Glib::ustring
VarselList::getSelectionAsText(const char* prefix, bool format_for_text)
{
    Glib::ustring text;
    text.reserve(64);
    if (prefix) {
        text += prefix + '\n';
    }
    for (size_t i = 0; i < m_selecion.size(); ++i) {
        auto& select = m_selecion[i];
        text += format_for_text
                ? select->get_parse_name()
                : Glib::ustring(select->get_uri());
        if (i < m_selecion.size() - 1) {
            text += '\n';
        }
    }
    return text;
}

// see https://github.com/xfce-mirror/thunar/blob/3de231d2dec33ca48b73391386d442231baace3e/thunar/thunar-clipboard-manager.c
void
VarselList::getClipboard(Gtk::SelectionData& data, guint intType)
{
    auto type = static_cast<CLIPBOARD_TARGET>(intType);
#   ifdef DEBUG
    std::cout << "VarselList::getClipboard " << static_cast<int>(type)
              << " selected " << m_selecion.size()
              << " kind " << (m_clipboardMove ? "move" : "copy")  << std::endl;
#   endif
    switch (type) {
    case CLIPBOARD_TARGET::TEXT_URI_LIST: {
        std::vector<Glib::ustring> uris;
        uris.reserve(m_selecion.size());
        for (auto& select : m_selecion) {
            uris.push_back(select->get_uri());
        }
        data.set_uris(uris);
        }
        break;
    case CLIPBOARD_TARGET::GNOME_COPIED_FILES: {
        const char* prefix = m_clipboardMove ? CLIPBOARD_CUT : CLIPBOARD_COPY;
        Glib::ustring text = getSelectionAsText(prefix, false);
        data.set(data.get_target(), text);
        }
        break;
    case CLIPBOARD_TARGET::UTF8_STRING: {
        Glib::ustring text = getSelectionAsText(nullptr, true);
        data.set_text(text);
        }
        break;
    }
}

void
VarselList::clearClipboard()
{
    // release the infos linked to clipboard ...
    m_selecion.clear();
    m_clipboardMove = false;
}

void
VarselList::setClipboard(bool clipboardMove)
{
    m_clipboardMove = clipboardMove;
    std::vector<PtrEventItem> items;
    items.reserve(8);
    if (getSelection(nullptr, items)) {
        m_selecion.clear();
        m_selecion.reserve(items.size());
        for (auto& item : items) {
            m_selecion.push_back(item->getFile());
        }
        Gtk::TargetEntry target_entry0{CLIPBOARD_URIS_CONTENT_TYPE
                , static_cast<Gtk::TargetFlags>(0)
                , static_cast<int>(CLIPBOARD_TARGET::TEXT_URI_LIST)};
        Gtk::TargetEntry target_entry1{CLIPBOARD_GNOME_FILES_CONTENT_TYPE
                , static_cast<Gtk::TargetFlags>(0)
                , static_cast<int>(CLIPBOARD_TARGET::GNOME_COPIED_FILES)};
        Gtk::TargetEntry target_entry2{CLIPBOARD_UTF8_STRINGS_CONTENT_TYPE
                , static_cast<Gtk::TargetFlags>(0)
                , static_cast<int>(CLIPBOARD_TARGET::UTF8_STRING)};
        std::vector<Gtk::TargetEntry> list;
        list.emplace_back(std::move(target_entry0));
        list.emplace_back(std::move(target_entry1));    // thunar expects the gnome-type
        list.emplace_back(std::move(target_entry2));
        auto refClipboard = Gtk::Clipboard::get();
        refClipboard->set_can_store(list);
        if (!refClipboard->set(list
                , sigc::mem_fun(*this, &VarselList::getClipboard)
                , sigc::mem_fun(*this, &VarselList::clearClipboard))) {
            std::cout << "Unable to set clipboard" << std::endl;
        }
    }
}

bool
VarselList::on_view_key_release_event(GdkEventKey* key)
{
    if (key->type == GDK_KEY_RELEASE
     && ((key->state & GDK_SHIFT_MASK) == 0)
     && ((key->state & GDK_CONTROL_MASK) != 0)
     && ((key->state & GDK_MOD1_MASK) == 0) ) {   // represents alt
        auto upper = g_unichar_toupper(key->keyval);
#       ifdef DEBUG
        std::cout << "Got key" << key->keyval << " up " << upper << std::endl;
#       endif
        if (upper == GDK_KEY_C) {
            setClipboard(false);
            return true;
        }
        if (upper == GDK_KEY_X) {
            setClipboard(true);
            return true;
        }
        if (upper == GDK_KEY_V) {
            auto refClipboard = Gtk::Clipboard::get();
            refClipboard->request_targets(
                    sigc::mem_fun(*this, &VarselList::on_targets_received));
            return true;
        }
    }
    return false;
}

void
VarselList::on_text_received(const Glib::ustring& text)
{
    //auto refClipboard = Gtk::Clipboard::get();
    //auto text = refClipboard->read_text_finish(result);
#   ifdef DEBUG
    std::cout << "VarselList::on_text_received " << text << std::endl;
#   endif
  //Do something with the pasted data.
}

void
VarselList::on_target_received(const Gtk::SelectionData& selection)
{
    auto dataString = selection.get_data_as_string();
#   ifdef DEBUG
    std::cout << "VarselList::on_target_received data " << dataString << std::endl;
#   endif
    std::vector<Glib::ustring> uris;
    uris.reserve(8);
    StringUtils::split(dataString, '\n', uris);
    bool isMove{false};
    if (uris.size() > 0
     && (CLIPBOARD_CUT == uris[0]
     ||  CLIPBOARD_COPY == uris[0])) {
        isMove = CLIPBOARD_CUT == uris[0];
        uris.erase(uris.begin());   // remove the copy/move identifier
    }
#   ifdef DEBUG
    std::cout << "VarselList::on_target_received uris " << uris.size() << std::endl;
#   endif
    if (uris.size() > 0) {
        auto sel = m_treeView->get_selection();
        auto iter = sel->get_selected();
        if (iter) {
            auto node = m_refTreeModel->get_node(iter);
            auto ftn = dynamic_cast<FileTreeNode*>(node);
            auto dir = ftn->getDirFile();
            m_data->paste(uris, dir, isMove, this);
        }
        else {
            std::cout << "VarselList::on_uri_received no selection!" << std::endl;
        }
    }
}

void
VarselList::on_targets_received(const std::vector<Glib::ustring>& targets)
{
    auto refClipboard = Gtk::Clipboard::get();
#   ifdef DEBUG
    std::cout << "VarselList::on_targets_received " << targets.size() << std::endl;
#   endif
    bool requested{false};
    for (auto& target : targets) {
        std::cout << "  target " << target << std::endl;
        if (CLIPBOARD_GNOME_FILES_CONTENT_TYPE == target) {
            requested = true;
            refClipboard->request_contents(target
                    , sigc::mem_fun(*this, &VarselList::on_target_received));
        }
    }
    if (!requested) {
        for (auto& target : targets) {
            if (CLIPBOARD_URIS_CONTENT_TYPE == target) {
                requested = true;
                refClipboard->request_contents(target
                        , sigc::mem_fun(*this, &VarselList::on_target_received));
            }
        }
    }
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
    VarselList* varselList{};
    auto builder = Gtk::Builder::create();
    try {
        builder->add_from_resource(listApp->get_resource_base_path() + "/varsel-win.ui");
        builder->get_widget_derived("VarselList", varselList, listApp);
        listApp->add_window(*varselList);
        varselList->show_all();
    }
    catch (const Glib::Error &ex) {
        //showMessage(
        std::cout << psc::fmt::vformat(
                  _("Error {} loading {}")
                , psc::fmt::make_format_args(ex, "varselWin")) << std::endl;
        //, Gtk::MessageType::MESSAGE_WARNING);
    }
    return varselList;
}
