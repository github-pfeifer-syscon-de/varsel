/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2025 RPf <gpl3@pfeifer-syscon.de>
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

#include <psc_i18n.hpp>
#include <psc_format.hpp>
#include <StringUtils.hpp>

#include "config.h"
#include "ExtractDialog.hpp"
#include "ListApp.hpp"
#include "VarselList.hpp"

ArchivExtractEntry::ArchivExtractEntry(struct archive_entry *entry, const Glib::RefPtr<Gio::File>& dir)
: ArchivEntry::ArchivEntry(entry)
, m_dir{dir}
{
}


int
ArchivExtractEntry::handleContent(struct archive* archiv)
{
    int ret{ARCHIVE_OK};
    if (!m_dir) {           // convention no dir -> no restore
        ret = archive_read_data_skip(archiv);
        return ret;
    }
    auto file = m_dir->get_child(getPath());    // should we work with temps???
    //std::cout << "ArchivExtractEntry::handleContent " << getPath()
    //          << " mode " << getMode()
    //          << " dir " << (m_dir ? m_dir->get_path() : std::string("noDir")) << std::endl;
    if (getMode() == AE_IFLNK) {
        auto dir = file->get_parent();
        try {
            if (getLinkType() == LinkType::Symbolic) {
                if (!dir->query_exists()) {
                    // if any part exists as file will throw "Error creating directory ... Not a directory"
                    dir->make_directory_with_parents();
                }
                if (!file->query_exists()) {
                    file->make_symbolic_link(getLinkPath());
                }
            }
            else {
                std::cout << "Unable to handle hard links!" << std::endl;
            }
        }
        catch (const Glib::Error& err) {
            setError(archiv, err, _("Creating symlink"));
            ret = ARCHIVE_FAILED;
        }
    }
    else if (getMode() == AE_IFDIR) {
        if (!file->query_exists()) {
            try {
                file->make_directory_with_parents();
            }
            catch (const Glib::Error& err) {
                setError(archiv, err, _("Creating directory"));
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
                    stream->seek(offset, Glib::SeekType::SEEK_TYPE_SET);
                    auto wsize = stream->write(buff, len);
                    if (static_cast<size_t>(wsize) != len) {
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
#           ifndef __WIN32__
            if (getPermission() > 0) {
                file->set_attribute_uint32("unix::mode"
                                         , getPermission()
                                         , Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
            }
#           endif
        }
        catch (const Glib::Error& err) {
            setError(archiv, err, _("Writing content"));
            ret = ARCHIVE_FAILED;
            remove = true;
        }
        //std::cout << "   ret " << ret
        //          << " remove " << std::boolalpha << remove << std::endl;
        if (remove) {       // don't keep incomplete result
            file->remove();
        }
    }
    else {
        std::cout << "ArchivExtractEntry::handleContent unhandeld mode " << getMode() << std::endl;
    }
    return ret;
}

bool
ArchivExtractEntry::isUsed()
{
    return m_dir.operator bool();
}

// use additional listener for processing in main thread
ArchivExtractWorker::ArchivExtractWorker(
              const Glib::RefPtr<Gio::File>& archiveFile
            , const Glib::RefPtr<Gio::File>& extractDir
            , const std::vector<PtrEventItem>& items
            , ArchivListener* archivListener)
: ThreadWorker()
, ArchivListener()
, m_archivFile{archiveFile}
, m_extractDir{extractDir}
, m_archivListener{archivListener}
{
    for (auto& item : items) {
        // unify this with file creation on extraction
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
    Glib::RefPtr<Gio::File> dir;
    Glib::ustring path = archive_entry_pathname_utf8(entry);
    if (m_items.empty() || m_items.contains(path)) {
        dir = m_extractDir;
        //std::cout << "ArchivExtractWorker::createEntry extract " << path
        //          << " items " << m_items.size()
        //          << " dir " << (m_extractDir ? m_extractDir->get_path() : std::string("noDir")) << std::endl;
    }
    return std::make_shared<ArchivExtractEntry>(entry, dir);
}

void
ArchivExtractWorker::archivUpdate(const std::shared_ptr<ArchivEntry>& entry)
{
    // this is called from thread context, and push to main thread
     auto extract = std::dynamic_pointer_cast<ArchivExtractEntry>(entry);
     if (extract) {
        notify(extract);
     }
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
ArchivExtractWorker::process(const std::vector<PtrArchivExtractEntry>& entries)
{
    // here we are back to main thread ...
    for (auto entry : entries) {
        //std::cout << "main archiv path " << entry->getPath() << std::endl;
        if (entry->isUsed()) {
            m_archivListener->archivUpdate(entry);
        }
    }
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
    m_archivListener->archivDone(m_archivSummary, msg);
}


ExtractDialog::ExtractDialog(
      BaseObjectType* cobject
    , const Glib::RefPtr<Gtk::Builder>& builder
    , const Glib::RefPtr<Gio::File>& file
    , const std::vector<PtrEventItem>& items
    , Gtk::Window* win)
: Gtk::Dialog(cobject)
, ArchivListener()
, m_file{file}
, m_items{items}
, m_win{win}
{
    builder->get_widget("archive", m_archive);
    builder->get_widget("target", m_target);
    builder->get_widget("progress", m_progress);
    builder->get_widget("info", m_info);
    builder->get_widget("cancel", m_cancel);
    builder->get_widget("open", m_open);
    builder->get_widget("apply", m_apply);
    m_apply->set_sensitive(false);
    m_open->set_sensitive(false);

    m_archive->set_text(file->get_path());
    m_target->signal_selection_changed().connect(
            sigc::mem_fun(*this, &ExtractDialog::selected));
    m_apply->signal_clicked().connect(
            sigc::mem_fun(*this, &ExtractDialog::extract));
}

void
ExtractDialog::selected()
{
    m_apply->set_sensitive(true);
}


//    Gtk::Window* m_win{};
//    auto fileChooser = Gtk::FileChooserDialog(*m_win
//                            , _("Extract to")
//                            , Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SELECT_FOLDER
//                            , Gtk::DIALOG_MODAL | Gtk::DIALOG_DESTROY_WITH_PARENT);
//    fileChooser.add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
//    fileChooser.add_button(_("_Extract"), Gtk::RESPONSE_ACCEPT);
//    if (fileChooser.run() == Gtk::RESPONSE_ACCEPT) {
//        auto dir = fileChooser.get_file();
//    }

void
ExtractDialog::extract()
{
    m_dir = m_target->get_file();
    m_apply->set_sensitive(false);
    m_target->set_sensitive(false);
    m_cancel->set_sensitive(false);     // while working don't allow close
    m_archivExtractWorker = std::make_shared<ArchivExtractWorker>(m_file, m_dir, m_items, this);
    //std::cout << "ArchiveDataSource::update" << m_archivWorker.get() << std::endl;
    m_archivExtractWorker->execute();
}

void
ExtractDialog::archivUpdate(const PtrArchivEntry& entry)
{
    ++m_extracted;
    if (m_items.size() > 0) {
        auto fract = static_cast<double>(m_extracted) / static_cast<double>(m_items.size());
        m_progress->set_fraction(fract);
    }
    else {
        m_progress->pulse();    // show as unknown
    }
    m_progress->set_text(entry->getPath());
}

void
ExtractDialog::archivDone(ArchivSummary archivSummary, const Glib::ustring& msg)
{
    if (!msg.empty())  {
        m_info->set_text(msg);
    }
    else {
        m_info->set_text(Glib::ustring::sprintf(_("Completed %d entries"), archivSummary.getEntries()));
    }
    m_progress->set_fraction(1.0);      // indicate as completed
    m_cancel->set_sensitive(true);      // now we are ready to close
    auto varselList = dynamic_cast<VarselList*>(m_win);
    m_open->set_sensitive(varselList);       // open only works if invoked from list
}

Glib::RefPtr<Gio::File>
ExtractDialog::getDirectory()
{
    return m_dir;
}

Glib::RefPtr<Gio::File>
ExtractDialog::show(
                  const Glib::RefPtr<Gio::File>& file
                , const std::vector<PtrEventItem>& items
                , Gtk::Window* win)
{
    ExtractDialog* extractDialog = nullptr;
    auto builder = Gtk::Builder::create();
    Glib::RefPtr<Gio::File> ret;
    try {
        builder->add_from_resource(win->get_application()->get_resource_base_path() + "/dlgExtract.ui");
        builder->get_widget_derived("dlgProgress", extractDialog, file, items, win);
        extractDialog->set_transient_for(*win);
        if (extractDialog->run() == Gtk::ResponseType::RESPONSE_OK) {
            ret = extractDialog->getDirectory();
        }
        delete extractDialog;
        // here we run into some problem how to delete ?
    }
    catch (const Glib::Error &ex) {
        //listApp->showMessage(
        Glib::ustring msg = ex.what();
        std::cout << psc::fmt::vformat(
                  _("Error {} loading {}")
                , psc::fmt::make_format_args(msg, "dlgExtract")) << std::endl;
        // , Gtk::MessageType::MESSAGE_WARNING)
    }
    return ret;
}
