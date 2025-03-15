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

#include <fcntl.h>
#include <iostream>
#include <psc_format.hpp>

#include "Archiv.hpp"

ArchivEntry::ArchivEntry()
{
}

ArchivEntry::ArchivEntry(struct archive_entry *entry)
{
    m_path = archive_entry_pathname_utf8(entry);
    if (archive_entry_filetype_is_set(entry)) {
        m_mode = archive_entry_filetype(entry);
    }
    if (archive_entry_symlink_utf8(entry)) {
        m_link = archive_entry_symlink_utf8(entry);
        m_linkType = LinkType::Symbolic;
        if (m_mode == 0) {
            m_mode = AE_IFLNK;  // this may not be provided by archive (symlink?)
        }
    }
    else if (archive_entry_hardlink_is_set(entry)) {
        m_link = archive_entry_hardlink(entry);
        m_linkType = LinkType::Hard;
        if (m_mode == 0) {
            m_mode = AE_IFLNK;  // this may not be provided by archive (symlink?)
        }
    }
    if (archive_entry_perm_is_set(entry)) {
        m_permission = archive_entry_perm(entry);
    }
    if (archive_entry_size_is_set(entry)) {
        m_size = archive_entry_size(entry);
    }
    if (archive_entry_uname_utf8(entry)) {
        m_user = archive_entry_uname_utf8(entry);
    }
    if (archive_entry_gname_utf8(entry)) {
        m_group = archive_entry_gname_utf8(entry);
    }
    if (archive_entry_ctime_is_set(entry)) {
        m_created = archive_entry_ctime(entry);
    }
    if (archive_entry_mtime_is_set(entry)) {
        m_modified = archive_entry_mtime(entry);
    }
}

ArchivException::ArchivException(const std::string& msg)
: m_msg{msg}
{

}

const char*
ArchivException::what() const noexcept
{
    return m_msg.c_str();
}

static int
c_read_open(struct archive *a, void *client_data)
{
    auto archivpp = reinterpret_cast<Archiv*>(client_data);
    return archivpp->cc_readopen(a);
}

static la_ssize_t
c_read(struct archive *a, void *client_data, const void **buff)
{
    auto archivpp = reinterpret_cast<Archiv*>(client_data);
    return archivpp->cc_read(a, buff);
}

static la_ssize_t
c_read_skip(struct archive *a, void *client_data, off_t request)
{
    auto archivpp = reinterpret_cast<Archiv*>(client_data);
    return archivpp->cc_readskip(a, request);
}

static int
c_read_close(struct archive *a, void *client_data)
{
    auto archivpp = reinterpret_cast<Archiv*>(client_data);
    return archivpp->cc_readclose();
}

 static int
 c_write_open(struct archive *a, void *client_data)
 {
    auto archivpp = reinterpret_cast<Archiv*>(client_data);
    return archivpp->cc_writeopen(a);
 }

static la_ssize_t
c_write(struct archive *a, void *client_data, const void *buffer, size_t length)
{
    auto archivpp = reinterpret_cast<Archiv*>(client_data);
    return archivpp->cc_write(a, buffer, length);
}

static int
c_write_close(struct archive *a, void *client_data)
{
    auto archivpp = reinterpret_cast<Archiv*>(client_data);
    return archivpp->cc_writeclose(a);
}

static int
c_write_free(struct archive *a, void *client_data)
{
    auto archivpp = reinterpret_cast<Archiv*>(client_data);
    return archivpp->cc_writefree(a);
}

Archiv::Archiv(Glib::RefPtr<Gio::File> file)
: m_file{file}
{
}

Archiv::~Archiv()
{
    cc_readclose(); // just in case this was left out
    cc_writeclose(nullptr);
}

void
Archiv::read(ArchivListener* listener)
{
    struct archive* archiv = archive_read_new();
    archive_read_support_filter_all(archiv);
    archive_read_support_format_all(archiv);
    Glib::ustring msg;
    ArchivSummary summary;
    int ret = archive_read_open2(archiv, reinterpret_cast<void*>(this), c_read_open, c_read, c_read_skip, c_read_close);
    if (ret == ARCHIVE_OK) {
        struct archive_entry *entry;
        while ((ret = archive_read_next_header(archiv, &entry)) == ARCHIVE_OK) {
            // the entry seems a internal structure as it doesn't change so no need to free as it seems
            auto archivEntry = std::make_shared<ArchivEntry>(entry);
            listener->archivUpdate(archivEntry);
            int ret = archivEntry->handleContent(archiv);
            if (ret != ARCHIVE_OK) {
                auto archErr = archive_error_string(archiv);
                msg = archErr ? std::string(archErr) : psc::fmt::vformat(_("Archiv error {}"), psc::fmt::make_format_args(ret));
                break;
            }
        }
        if (ret != ARCHIVE_EOF) {
            auto archErr = archive_error_string(archiv);
            msg = archErr ? std::string(archErr) : psc::fmt::vformat(_("Archiv error {}"), psc::fmt::make_format_args(ret));
        }
        else {
            // returns separate compressions e.g. tar.gz file will be gzip, none, zip file will be none
            for (int i = 0; i <  archive_filter_count(archiv); ++i) {
                int cmp = archive_filter_code(archiv, i);
                if (cmp != ARCHIVE_FILTER_NONE) {
                    const char* compress = archive_filter_name(archiv, i);
                    if (compress) {
                        m_readFormats.push_back(std::string(compress));
                    }
                }
                else {
                    const char* fmt = archive_format_name(archiv);
                    if (fmt) {
                        m_readFormats.push_back(std::string(fmt));
                    }
                }
            }
        }
        summary.setEntries(archive_file_count(archiv));
        archive_read_close(archiv);
    }
    else {
        auto archErr = archive_error_string(archiv);
        msg = archErr ? std::string(archErr) : psc::fmt::vformat(_("Archiv error {}"), psc::fmt::make_format_args(ret));
    }
    if (archiv) {
        archive_read_free(archiv);
    }
    if (!msg.empty()) { // keep processing do this as last step so we free c-side
        throw ArchivException(msg);
    }
    //std::cout << "msg " << msg << std::endl;
    listener->archivDone(summary, msg);
}


void
Archiv::setFormat(struct archive* archiv)
{
    for (auto fmt : m_writeFormats) {
        if (fmt & ARCHIVE_FORMAT_BASE_MASK) {
            int ret = archive_write_set_format(archiv, fmt);
            if (ret != ARCHIVE_OK) {
                throw ArchivException(psc::fmt::format("Format {} unknown ", fmt));  // this is a internal message
            }
        }
        else {
            int ret = archive_write_add_filter(archiv, fmt);
            if (ret != ARCHIVE_OK) {
                throw ArchivException(psc::fmt::format("Compression {} unknown ", fmt));  // this is a internal message
            }
        }
    }
}

void
Archiv::write(ArchivProvider* provider)
{
    struct archive* archiv = archive_write_new();
    setFormat(archiv);
    int ret = archive_write_open2(archiv, reinterpret_cast<void*>(this), c_write_open, c_write, c_write_close, c_write_free);
    Glib::ustring msg;
    if (ret == ARCHIVE_OK) {
        while (true) {
            auto srcEntry = provider->getNextEntry();
            if (!srcEntry) {
                break;
            }
            //std::cout << "Archiv::write filename " << srcEntry->getPath() << std::endl;
            struct archive_entry *entry{nullptr};
            if (srcEntry->getMode() == AE_IFDIR) {
                entry = archive_entry_new();
                archive_entry_set_pathname_utf8(entry, srcEntry->getPath().c_str());
                archive_entry_set_filetype(entry, srcEntry->getMode());
                archive_entry_set_perm(entry, srcEntry->getPermission());
            }
            else if (srcEntry->getMode() == AE_IFREG) {
                entry = archive_entry_new();
                archive_entry_set_pathname_utf8(entry, srcEntry->getPath().c_str());
                archive_entry_set_filetype(entry, srcEntry->getMode());
                archive_entry_set_size(entry, srcEntry->getSize());
                archive_entry_set_perm(entry, srcEntry->getPermission());
                archive_write_header(archiv, entry);
                ret = provider->writeContent(this, archiv, entry);
                if (ret != ARCHIVE_OK) {
                    auto archErr = archive_error_string(archiv);
                    msg = archErr ? std::string(archErr) : psc::fmt::vformat(_("Archiv error {}"), psc::fmt::make_format_args(ret));
                    break;
                }
            }
            if (entry) {
                archive_entry_free(entry);
            }
        }
        archive_write_close(archiv);
    }
    else {
        auto archErr = archive_error_string(archiv);
        msg = archErr ? std::string(archErr) : psc::fmt::vformat(_("Archiv error {}"), psc::fmt::make_format_args(ret));
    }
    if (archiv) {
        archive_write_free(archiv);
    }
    if (!msg.empty()) { // keep process so we free c-side
        throw ArchivException(msg);
    }
    return;
}

bool
Archiv::canRead()
{
// see no alternative to opening it ...
    bool has_entries{false};
    struct archive* archiv = archive_read_new();
    archive_read_support_filter_all(archiv);
    archive_read_support_format_all(archiv);
    int ret = archive_read_open2(archiv, reinterpret_cast<void*>(this), c_read_open, c_read, c_read_skip, c_read_close);
    if (ret == ARCHIVE_OK) {
        struct archive_entry *entry;
        while (archive_read_next_header(archiv, &entry) == ARCHIVE_OK) {
            has_entries = true;
            break;  // finding one entry is sufficent for testing
        }
        archive_read_close(archiv);
    }
    archive_read_free(archiv);
    //std::cout << "Archiv::canRead " << std::boolalpha << has_entries << std::endl;
    return has_entries;
}


std::vector<std::string>
Archiv::getReadFormats()
{
    return m_readFormats;
}

void
Archiv::addWriteFormat(int fmt)
{
    m_writeFormats.push_back(fmt);
}

void
Archiv::setError(struct archive *archiv, const Glib::Error& err, const char* where)
{
    std::string errWhat = err.what();
    //auto path = m_file->get_path(); path already included
    archive_set_error(archiv, ARCHIVE_FATAL, "%s error %s", errWhat.c_str(), where);
}


int
Archiv::cc_readopen(struct archive *archiv)
{
    try {
        m_fileInputstream = m_file->read();
        return ARCHIVE_OK;
    }
    catch (const Glib::Error& err) {    // the expected insight what went wrong is not happening, but at least we get the localisation
        setError(archiv, err, _("open"));
    }
    return ARCHIVE_FATAL;
}

la_ssize_t
Archiv::cc_read(struct archive *archiv, const void **ebuff)
{
    try {
        if (!buff) {
            buff = std::make_unique<BUFFER_ARRAY>();    // use ptr for not overloading stack
        }
        *ebuff = buff.get();
        return m_fileInputstream->read(buff.get(), BUF_SIZE);
    }
    catch (const Glib::Error& err) {
        setError(archiv, err, _("read"));
    }
    return ARCHIVE_FATAL;
}

la_ssize_t
Archiv::cc_readskip(struct archive *archiv, off_t request)
{
    try {
        return m_fileInputstream->skip(request);
    }
    catch (const Glib::Error& err) {
        setError(archiv, err, _("skip"));
    }
    return ARCHIVE_FATAL;
}

int
Archiv::cc_readclose()
{
    if (m_fileInputstream) {
        try {
            m_fileInputstream->close();
        }
        catch (...) {          // just don't complain
        }
        m_fileInputstream.reset();
    }
    return ARCHIVE_OK;
}

int
Archiv::cc_writeopen(struct archive *archiv)
{
    try {
        m_fileOutputstream = m_file->replace("", false, Gio::FileCreateFlags::FILE_CREATE_REPLACE_DESTINATION);
        return ARCHIVE_OK;
    }
    catch (const Glib::Error& err) {    // the expected insight what went wrong is not happening, but at least we get the localisation
        setError(archiv, err, _("open"));
    }
    return ARCHIVE_FATAL;
}

la_ssize_t
Archiv::cc_write(struct archive *archiv, const void *buffer, size_t length)
{
    try {
        return m_fileOutputstream->write(buffer, length);
    }
    catch (const Glib::Error& err) {
        setError(archiv, err, _("write"));
    }
    return ARCHIVE_FATAL;
}

int
Archiv::cc_writeclose(struct archive *archiv)
{
    if (m_fileOutputstream) {
        try {
            m_fileOutputstream->close();
            return ARCHIVE_OK;
        }
        catch (const Glib::Error& err) {
            if (archiv) {
                setError(archiv, err, _("close"));
            }
        }
        m_fileOutputstream.reset();
    }
    return ARCHIVE_FATAL;
}

int
Archiv::cc_writefree(struct archive *archiv)
{
    return ARCHIVE_OK;
}

ArchivFileProvider::ArchivFileProvider(const Glib::RefPtr<Gio::File>& dir, bool useSubDirs)
: m_dir{dir}
, m_useSubDirs{useSubDirs}
, m_entry{std::make_shared<ArchivEntry>()}
{
    m_workers.emplace_back(std::make_shared<ArchivDirWalker>(m_dir, this));
}

std::shared_ptr<ArchivEntry>
ArchivFileProvider::getNextEntry()
{
    while (true) {
        //std::cout << "ArchivFileProvider::getNextEntry " << m_workers.size()  << std::endl;
        if (m_workers.begin() != m_workers.end()) {
            auto worker = *(m_workers.rbegin());
            auto activFile = worker->scanEntries();
            if (!activFile) {
                m_workers.pop_back();   // if worker has no more entries remove it
                continue;
            }
            Gio::FileType fileType = activFile->query_file_type(Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
            if (fileType == Gio::FileType::FILE_TYPE_DIRECTORY) {
                // unsure how to pass directories to archive, here we just found on, it may or may no contain usable files
                //   (but it seems we can live without if we don't want explicit permissions)
                m_workers.emplace_back(std::make_shared<ArchivDirWalker>(activFile, this));
                continue;               // and use the created entry
            }
            else {
                m_activFile = activFile;
                m_entry->setMode(AE_IFREG);
                m_entry->setPath(m_dir->get_relative_path(m_activFile));
                m_entry->setPermission(m_permission);
                auto info = m_activFile->query_info("*", Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
                m_entry->setSize(info->get_size());
                return m_entry;
            }
        }
        else {
            break;
        }
    }
    std::cout << "ArchivFileProvider::getNextEntry null " << std::endl;
    return nullptr;
}

int
ArchivFileProvider::writeContent(Archiv* archiv, struct archive* structarchiv, struct archive_entry *entry)
{
    std::unique_ptr<Archiv::BUFFER_ARRAY> buff = std::make_unique<Archiv::BUFFER_ARRAY>();    // use ptr for not overloading stack
    Glib::RefPtr<Gio::FileInputStream> fileInputstream;
    int ret = ARCHIVE_OK;
    try {
        fileInputstream = m_activFile->read();
        while (true)  {
            int len = fileInputstream->read(buff.get(), Archiv::BUF_SIZE);
            if (len <= 0) {
                break;
            }
            int ret = archive_write_data(structarchiv, buff.get(), len);
            if (ret != ARCHIVE_OK) {
                break;
            }
        }
    }
    catch (const Glib::Error& err) {
        archiv->setError(structarchiv, err, _("read source"));
        ret = ARCHIVE_FATAL;
    }
    if (fileInputstream) {
        try {
            fileInputstream->close();
        }
        catch (const Glib::Error& err) {    // fails most likely due to previous error, so ignore
        }
    }
    return ret;
}

ArchivDirWalker::ArchivDirWalker(Glib::RefPtr<Gio::File> scanDir, ArchivFileProvider* provider)
: m_scanDir{scanDir}
, m_provider{provider}
{
}

Glib::RefPtr<Gio::File>
ArchivDirWalker::scanEntries()
{
    try {
        if (!m_entries) {   // do this lazily to catch error
            m_entries = m_scanDir->enumerate_children("*", Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
        }
        while (true) {
            auto fileInfo = m_entries->next_file();
            //std::cout << "ArchivFileProvider::scanEntries " << m_scanDir->get_path()
            //          << " found " << (fileInfo ? fileInfo->get_name() : std::string("non")) << std::endl;
            if (!fileInfo) {
                break;
            }
            auto activFile = m_scanDir->get_child(fileInfo->get_name());
            Gio::FileType fileType = activFile->query_file_type(Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
            if (m_provider->useSubDirs() && fileType == Gio::FileType::FILE_TYPE_DIRECTORY) {
                return activFile;
            }
            if (fileType == Gio::FileType::FILE_TYPE_REGULAR
             && m_provider->isFilterEntry(activFile)) {
                return activFile;
            }
        }
    }
    catch (const Gio::Error& err) {
        if (m_provider->isFailForScanError()) {
            auto errWhat = std::string(err.what());
            auto path = m_scanDir->get_path();
            auto msg = psc::fmt::vformat(_("Error {} scanning {}"),
                            psc::fmt::make_format_args(errWhat, path));
            throw(ArchivException(msg));
        }
        else {
            std::cout << "ArchivDirWalker::scanEntries error " << err.what() << " but continue!" << std::endl;
        }
    }
    return Glib::RefPtr<Gio::File>();
}

