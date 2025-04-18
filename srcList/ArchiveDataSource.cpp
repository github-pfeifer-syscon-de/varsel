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
#include <gtkmm.h>

#include "ArchiveDataSource.hpp"
#include "config.h"


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

ArchivExtractEntry::ArchivExtractEntry(struct archive_entry *entry, const Glib::RefPtr<Gio::File>& dir)
: ArchivEntry::ArchivEntry(entry)
, m_dir{dir}
{
}


int
ArchivExtractEntry::handleContent(struct archive* archiv)
{
    if (!m_dir) {           // convention no dir -> no restore
        if (getSize() == 0) {
            setSize(9999);  // as we are not interested to sum size just want to skip
        }
        return ArchivEntry::handleContent(archiv);
    }
    int ret{ARCHIVE_OK};
    auto file = m_dir->get_child(getPath());    // should we work with temps???
#   ifdef DEBUG
    std::cout << "ArchivExtractEntry::handleContent " << getPath()
              << " mode " << getMode()
              << " dir " << (m_dir ? m_dir->get_path() : std::string("noDir")) << std::endl;
#   endif
    if (getMode() == AE_IFLNK && getLinkType() == LinkType::Symbolic) {
        auto dir = file->get_parent();
        try {
            if (!dir->query_exists()) {
                // if any part exists as file will throw "Error creating directory ... Not a directory"
                dir->make_directory_with_parents();
            }
            if (!file->query_exists()) {
                file->make_symbolic_link(getLinkPath());
            }
        }
        catch (const Glib::Error& err) {
            setError(archiv, err, "Creating symlink");
            ret = ARCHIVE_FAILED;
        }
    }
    else if (getMode() == AE_IFDIR) {
        if (!file->query_exists()) {
            try {
                file->make_directory_with_parents();
            }
            catch (const Glib::Error& err) {
                setError(archiv, err, "Creating directory");
                ret = ARCHIVE_FAILED;
            }
        }
    }
    else if (getMode() == AE_IFREG) {
        auto dir = file->get_parent();
        bool remove{false};
        try {
            if (!dir->query_exists()) {
                // if any part exists as file will throw "Error creating directory ... Not a directory"
                dir->make_directory_with_parents();
            }
            Glib::RefPtr<Gio::FileOutputStream> stream;
            if (file->query_exists()) {
                stream = file->replace();
            }
            else {
                stream = file->create_file(Gio::FileCreateFlags::FILE_CREATE_NONE);
            }
            const void *buff;
            size_t len{0l};
            off_t offset{0l};
            do {
                ret = archive_read_data_block(archiv, &buff, &len, &offset);
                if (ret == ARCHIVE_OK) {
    #               ifdef DEBUG
                    std::cout << "   got"
                              << " offs " << offset
                              << " len " << len << std::endl;
    #               endif
                    stream->seek(offset, Glib::SeekType::SEEK_TYPE_SET);
                    auto wsize = stream->write(buff, len);
                    if (static_cast<size_t>(wsize) != len) {
                        //setError(archive, ARCHIVE_FAILED, "Writing content");
                        ret = ARCHIVE_FAILED;
                        remove = true;
                    }
                }
            } while (ret == ARCHIVE_OK);
            if (ret == ARCHIVE_EOF) {  // end of entry as it seems
                ret = ARCHIVE_OK;
            }
            stream->flush();
            stream->close();
            // restore user/group/date?
            auto info = file->query_info("*");
            info->set_attribute_int32("unix::mode", getMode());
            // see https://docs.gtk.org/glib/file-utils.html
            //int gret = g_chmod(file->get_path().c_str(), getMode());
        }
        catch (const Glib::Error& err) {
            setError(archiv, err, "Writing content");
            ret = ARCHIVE_FAILED;
            remove = true;
        }
#       ifdef DEBUG
        std::cout << "   ret " << ret
                  << " remove " << std::boolalpha << remove << std::endl;
#       endif
        if (remove) {       // don't keep incomplete result
            file->remove();
        }
    }
    else {
        std::cout << "ArchivExtractEntry::handleContent unhandeld mode " << getMode() << std::endl;
    }
    return ret;
}

// use additional listener for processing in main thread
ArchivExtractWorker::ArchivExtractWorker(
              const Glib::RefPtr<Gio::File>& archiveFile
            , const Glib::RefPtr<Gio::File>& extractDir
            , const std::vector<PtrEventItem>& items)
: ThreadWorker()
, ArchivListener()
, m_archivFile{archiveFile}
, m_extractDir{extractDir}
{
    for (auto& item : items) {
        Glib::ustring filePath = item->getFile()->get_path();
        if (filePath.length() > 0 && filePath[0] == '/') {
            filePath = filePath.substr(1);  // just use without preceeding "/"
        }
        m_items.insert(filePath);
    }
}

PtrArchivEntry
ArchivExtractWorker::createEntry(struct archive_entry *entry)
{
    Glib::ustring path = archive_entry_pathname_utf8(entry);
    if (m_items.empty() || m_items.contains(path)) {
#       ifdef DEBUG
        std::cout << "ArchivExtractWorker::createEntry extract " << path
                  << " items " << m_items.size()
                  << " dir " << (m_extractDir ? m_extractDir->get_path() : std::string("noDir")) << std::endl;
#       endif
        return std::make_shared<ArchivExtractEntry>(entry, m_extractDir);
    }
    return std::make_shared<ArchivEntry>(entry);
}

void
ArchivExtractWorker::archivUpdate(const std::shared_ptr<ArchivEntry>& entry)
{
    // this is called from thread context ...

    //notify(entry);
}

void
ArchivExtractWorker::archivDone(ArchivSummary archivSummary, const Glib::ustring& msg)
{
    m_archivSummary = archivSummary;
}

ArchivSummary
ArchivExtractWorker::doInBackground()
{
    //std::cout << "ArchivWorker::doInBackground " << m_file->get_path() << std::endl;
    Archiv archiv(m_archivFile);
    archiv.read(this);
    return m_archivSummary;
}

void
ArchivExtractWorker::process(const std::vector<std::shared_ptr<ArchivEntry>>& entries)
{
    // here we are back to main thread ...
    //for (auto entry : entries) {
        //std::cout << "main archiv path " << entry->getPath() << std::endl;
    //    m_archivListener->archivUpdate(entry);
    //}
}

void
ArchivExtractWorker::done()
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
    //m_archivListener->archivDone(m_archivSummary, msg);
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
          const Glib::RefPtr<psc::ui::TreeNodeModel>& treeModel
        , ListListener* listListener)
{
    m_listListener = listListener;
    m_treeModel = treeModel;
    m_treeItem = std::make_shared<FileTreeNode>("", 0);
    treeModel->append(m_treeItem);

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


Glib::RefPtr<Gio::File>
ArchiveDataSource::getFileName(const std::string& name)
{
    //auto file = m_file->get_child(name);
    // expand to temp?
    return Glib::RefPtr<Gio::File>();
}

void
ArchiveDataSource::paste(const std::vector<Glib::ustring>& uris, Gtk::Window* win)
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
#   ifdef DEBUG
    std::cout << "ArchiveDataSource::do_handle" << std::endl;
#   endif
    auto fileChooser = Gtk::FileChooserDialog(*win
                            , _("Extract to")
                            , Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SELECT_FOLDER
                            , Gtk::DIALOG_MODAL | Gtk::DIALOG_DESTROY_WITH_PARENT);
    fileChooser.add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
    fileChooser.add_button(_("_Extract"), Gtk::RESPONSE_ACCEPT);
    if (fileChooser.run() == Gtk::RESPONSE_ACCEPT) {
        auto dir = fileChooser.get_file();
        m_archivExtractWorker = std::make_shared<ArchivExtractWorker>(m_file, dir, items);
        //std::cout << "ArchiveDataSource::update" << m_archivWorker.get() << std::endl;
        m_archivExtractWorker->execute();
    }
}