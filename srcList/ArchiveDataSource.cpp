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

#include "ArchiveDataSource.hpp"




// use additional listener for processing in main thread
ArchivWorker::ArchivWorker(const Glib::RefPtr<Gio::File>& file
            , ArchivListener* archivListener)
: ThreadWorker()
, ArchivListener()
, m_file{file}
, m_archivListener{archivListener}
{
}

void
ArchivWorker::archivUpdate(const std::shared_ptr<ArchivEntry>& entry)
{
    // this is called from thread context ...
    //std::cout << "thread archiv path " << entry->getPath() << std::endl;
    notify(entry);
}

void
ArchivWorker::archivDone(ArchivSummary archivSummary, const Glib::ustring& msg)
{
    m_archivSummary = archivSummary;
}

ArchivSummary
ArchivWorker::doInBackground()
{
    //std::cout << "ArchivWorker::doInBackground " << m_file->get_path() << std::endl;
    Archiv archiv(m_file);
    archiv.read(this);
    return m_archivSummary;
}

void
ArchivWorker::process(const std::vector<std::shared_ptr<ArchivEntry>>& entries)
{
    // here we are back to main thread ...
    for (auto entry : entries) {
        //std::cout << "main archiv path " << entry->getPath() << std::endl;
        m_archivListener->archivUpdate(entry);
    }
}

void
ArchivWorker::done()
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

ArchiveDataSource::ArchiveDataSource(const Glib::RefPtr<Gio::File>& file, ListApp* application)
: DataSource::DataSource(application)
, ArchivListener::ArchivListener()
, m_file{file}
{
}

bool
ArchiveDataSource::can_handle(const Glib::RefPtr<Gio::File>& file)
{
    Archiv archiv(file);
    bool ret = archiv.canRead();
    std::cout << "ArchiveDataSource::can_handle " << file->get_path() << std::boolalpha << " ret " << ret << std::endl;
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
    Glib::ustring stype;
    if (entry->getMode() > 0) {
        switch (entry->getMode()) {
        case AE_IFREG:  // Regular file
            stype = "File";
            break;
        case AE_IFLNK:  // Symbolic link
            stype = "Link";
            break;
        case AE_IFSOCK: // Socket
            stype = "Socket";
            break;
        case AE_IFCHR:  // Character device
            stype = "Character device";
            break;
        case AE_IFBLK:  // Block device
            stype = "Block device";
            break;
        case AE_IFDIR:  // Directory
            stype = "Directory";
            break;
        case AE_IFIFO:  // Named pipe (fifo)
            stype = "Fifo";
            break;
        }
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
        for (auto iter = parts.begin(); iter != parts.end(); ++iter) {
            auto spart = *iter;
            if (!spart.empty()) {
                //std::cout << "ArchiveDataSource::update" << "   part " << part << std::endl;
                auto foundNode = addNode->findNode(spart);
                if (!foundNode) {
                    //std::cout << "ArchiveDataSource::update creating " << spart  << std::endl;
                    auto partItem = std::make_shared<FileTreeNode>(spart, addNode->getDepth() + 1);
                    addNode->addChild(partItem);
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
          const Glib::RefPtr<psc::ui::TreeNodeModel>& treeModel
        , ListListener* listListener)
{
    m_listListener = listListener;
    m_treeModel = treeModel;
    m_treeItem = std::make_shared<FileTreeNode>("", 0);
    treeModel->append(m_treeItem);

    m_archivWorker = std::make_shared<ArchivWorker>(m_file, this);
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


Glib::RefPtr<Gio::File>
ArchiveDataSource::getFileName(const std::string& name)
{
    //auto file = m_file->get_child(name);
    // expand to temp?
    return Glib::RefPtr<Gio::File>();
}