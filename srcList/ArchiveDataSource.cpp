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
#include <StringUtils.hpp>
#include <gtkmm.h>

#include "ArchiveDataSource.hpp"
#include "config.h"
#include "VarselList.hpp"

// use additional listener for processing in main thread
ArchivListWorker::ArchivListWorker(
              const Glib::RefPtr<Gio::File>& file
            , ArchivListener* archivListener)
: ThreadWorker()
, ArchivListener()
, m_file{file}
, m_archivListener{archivListener}
{
}

void
ArchivListWorker::archivUpdate(const std::shared_ptr<ArchivEntry>& entry)
{
    // this is called from thread context ...
    //std::cout << "thread archiv path " << entry->getPath() << std::endl;
    notify(entry);
}

void
ArchivListWorker::archivDone(ArchivSummary archivSummary, const Glib::ustring& msg)
{
    m_archivSummary = archivSummary;
}

ArchivSummary
ArchivListWorker::doInBackground()
{
    //std::cout << "ArchivWorker::doInBackground " << m_file->get_path() << std::endl;
    Archiv archiv(m_file);
    archiv.read(this);
    return m_archivSummary;
}

void
ArchivListWorker::process(const std::vector<std::shared_ptr<ArchivEntry>>& entries)
{
    // here we are back to main thread ...
    for (auto entry : entries) {
        //std::cout << "main archiv path " << entry->getPath() << std::endl;
        m_archivListener->archivUpdate(entry);
    }
}

void
ArchivListWorker::done()
{
    //std::cout << "ArchivWorker::done " << m_archivSummary.getEntries() << std::endl;
    Glib::ustring msg;
    try {
        getResult();
    }
    catch (const std::exception& exc) {
        std::cout << "archiv error " << exc.what() << std::endl;
        msg = exc.what();
    }
    m_archivListener->archivDone(m_archivSummary, msg);
}

ArchiveDataSource::ArchiveDataSource(ListApp* application)
: DataSource::DataSource(application)
, ArchivListener::ArchivListener()
{
}

bool
ArchiveDataSource::can_handle(const Glib::RefPtr<Gio::File>& file)
{
    Archiv archiv(file);
    bool ret = archiv.canRead();
#   ifdef DEBUG
    std::cout << "ArchiveDataSource::can_handle " << file->get_path() << std::boolalpha << " ret " << ret << std::endl;
#   endif
    return ret;
}

void
ArchiveDataSource::archivUpdate(const std::shared_ptr<ArchivEntry>& entry)
{
    // here we are back to main thread ...
    //std::cout << "outer archiv path " << entry->getPath() << std::endl;
    ++m_entries;
    auto file = Gio::File::create_for_path("/" + entry->getPath()); // make absolute otherwise, local path will be prefixed
    Glib::ustring path,name;
    Glib::ustring stype = entry->getModeName();
    if (entry->getMode() > 0) {
        if (entry->getMode() == AE_IFDIR) {
            path = file->get_parse_name();
            name = "";
        }
        else {
            path = file->get_parent()->get_parse_name();
            name = file->get_basename();
        }
    }
    else {     // for this case we are clueless
        path = "";
        name = file->get_parse_name();  // represent unstructured?
    }

    std::shared_ptr<BaseTreeNode> addNode = m_treeItem;
    if (!path.empty()) {
        //std::cout << "ArchiveDataSource::archivUpdate"
        //          << " path " << path << std::endl;
        auto fspath = Gio::File::create_for_path(path); // std::filesystem::path(path);
        std::list<Glib::ustring> parts;
        while (fspath->has_parent()) {
            parts.push_front(fspath->get_basename());
            fspath = fspath->get_parent();
        }
        Glib::ustring path;
        for (auto iter = parts.begin(); iter != parts.end(); ++iter) {
            auto spart = *iter;
            if (!spart.empty()) {
                path += "/" + spart;
                //std::cout << "ArchiveDataSource::update" << "   part " << part << std::endl;
                auto foundNode = addNode->findNode(spart);
                if (!foundNode) {
                    //std::cout << "ArchiveDataSource::update creating " << spart  << std::endl;
                    auto file = Gio::File::create_for_path(path);
                    auto partItem = std::make_shared<FileTreeNode>(file, spart, addNode->getDepth() + 1);
                    addNode->addChild(partItem);
                    partItem->setQueried(true); // don't expect to find more
                    m_treeModel->memory_row_inserted(partItem); // notify as the model was attached
                    foundNode = partItem;
                    if (m_listListener) {
                        m_listListener->nodeAdded(partItem);
                    }
                }
                addNode = foundNode;
            }
        }
    }
    if (!name.empty()) {
        auto iter = addNode->appendList();
        auto row = *iter;
        auto listColumns = getListColumns();
        row.set_value<Glib::ustring>(listColumns->m_name, name);
        row.set_value(listColumns->m_size, entry->getSize());
        row.set_value(listColumns->m_type, stype);
        row.set_value(listColumns->m_mode, entry->getPermission());
        row.set_value(listColumns->m_user, entry->getUser());
        row.set_value(listColumns->m_group, entry->getGroup());

        row.set_value(listColumns->m_file, file);   // pass as "virtual" file
        //row.set_value(listColumns->m_fileInfo, fileInfo);
    }


}

// Archiv Listener
void
ArchiveDataSource::archivDone(ArchivSummary archivSummary, const Glib::ustring& errorMsg)
{
    if (m_listListener) {
        Severity sev = Severity::Info;
        Glib::ustring msg;
        if (!errorMsg.empty()) {
            sev = Severity::Error;
            msg = Glib::ustring::sprintf(_("Error %s"), errorMsg);
        }
        if (archivSummary.getEntries() != m_entries) {
            sev = Severity::Warning;
            msg = Glib::ustring::sprintf(_("Missmatch transfered files %d got %d"), archivSummary.getEntries(), m_entries);
        }
        else {
            msg = Glib::ustring::sprintf(_("Transfered files %d"), archivSummary.getEntries());
        }
        std::cout << "ArchiveDataSource::archivDone " << msg << std::endl;
        m_listListener->listDone(sev, msg);
        m_listListener = nullptr;   // this should no long be used
    }
    // this is risky ... so leave it for now
    //m_archivWorker.reset();     // clear reference ? even if this will return there (better way?)
    //m_treeModel.reset();        // clear reference to avoid interlocking reference count
}

void
ArchiveDataSource::update(
          const Glib::RefPtr<Gio::File>& file
        , std::shared_ptr<psc::ui::TreeNode> treeItem
        , const Glib::RefPtr<psc::ui::TreeNodeModel>& treeModel
        , ListListener* listListener)
{
    m_file = file;
    m_listListener = listListener;
    m_treeModel = treeModel;
    if (!treeItem) {
        auto dir = Gio::File::create_for_path("/");
        treeItem = m_treeItem = std::make_shared<FileTreeNode>(dir, "", 0);
        treeModel->append(m_treeItem);
    }
    auto fileTreeNode = std::dynamic_pointer_cast<FileTreeNode>(treeItem);
    fileTreeNode->setQueried(true);

    m_archivWorker = std::make_shared<ArchivListWorker>(m_file, this);
    //std::cout << "ArchiveDataSource::update" << m_archivWorker.get() << std::endl;
    m_archivWorker->execute();
}

const char*
ArchiveDataSource::getConfigGroup()
{
    return "ArchivData";
}

std::shared_ptr<ListColumns>
ArchiveDataSource::getListColumns()
{
    return FileTreeNode::getListColumns();
}


void
ArchiveDataSource::paste(
          const std::vector<Glib::ustring>& uris
        , const Glib::RefPtr<Gio::File>& dir
        , bool isMove
        , VarselList* win)
{
    std::cout << "ArchiveDataSource::paste " << uris.size() << std::endl;
}

Gtk::MenuItem*
ArchiveDataSource::createItem(const std::vector<PtrEventItem>& items, Gtk::Menu* gtkMenu, const Glib::ustring& name, Gtk::Window* win)
{
    auto menuItem = Gtk::make_managed<Gtk::MenuItem>(name);
    gtkMenu->append(*menuItem);
    menuItem->signal_activate().connect(
        sigc::bind(
            sigc::mem_fun(*this, &ArchiveDataSource::do_handle)
        , items, win));
    return menuItem;
}

void
ArchiveDataSource::distribute(const std::vector<PtrEventItem>& items, Gtk::Menu* menu, Gtk::Window* win)
{
    std::vector<PtrEventItem> empty;
    createItem(empty, menu, Glib::ustring::sprintf(_("Extract %s"), "all"), win);
    createItem(items, menu, Glib::ustring::sprintf(_("Extract %s"), "these"), win);
    for (auto& item : items) {
        std::vector<PtrEventItem> eventItems;
        eventItems.push_back(item);
        createItem(eventItems, menu, item->getFile()->get_basename(), win);
    }
}

void
ArchiveDataSource::do_handle(const std::vector<PtrEventItem>& items, Gtk::Window* win)
{
    auto dir = ExtractDialog::show(m_file, items, win);
    auto varselList = dynamic_cast<VarselList*>(win);
    if (dir && varselList) {
        varselList->showFile(dir);
    }
}