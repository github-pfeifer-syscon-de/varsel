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

#include "Archiv.hpp"

ArchivEntry::ArchivEntry(struct archive_entry *entry)
{
    m_path = archive_entry_pathname_utf8(entry);
    if (archive_entry_perm_is_set(entry)) {
        m_permission = archive_entry_perm(entry);
    }
    if (archive_entry_filetype_is_set(entry)) {
        m_mode = archive_entry_filetype(entry);
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

ArchivListener::ArchivListener()
{
}

Archiv::Archiv(Glib::RefPtr<Gio::File> file, ArchivListener* listener)
: m_file{file}
, m_listener{listener}
{
}


static constexpr size_t BUF_SIZE{64u*1024u};

struct mydata
{
    int fd{0};
    char buff[BUF_SIZE];
    const char *name;
    virtual ~mydata()
    {
        myclose();
    }

    int myopen()
    {
        fd = open(name, O_RDONLY);
        return (fd >= 0 ? ARCHIVE_OK : ARCHIVE_FATAL);
    }
    la_ssize_t myread(const void **ebuff)
    {
        *ebuff = buff;
        return (read(fd, buff, BUF_SIZE));
    }
    int myclose()
    {
        if (fd > 0) {
            close(fd);
            fd = 0;
        }
        return ARCHIVE_OK;
    }
};


static la_ssize_t
myread(struct archive *a, void *client_data, const void **buff)
{
    auto mydata = reinterpret_cast<struct mydata*>(client_data);
    return mydata->myread(buff);
}

static int
myopen(struct archive *a, void *client_data)
{
    auto mydata = reinterpret_cast<struct mydata*>(client_data);
    return mydata->myopen();
}

static int
myclose(struct archive *a, void *client_data)
{
    auto mydata = reinterpret_cast<struct mydata*>(client_data);
    return mydata->myclose();
}

void
Archiv::read()
{
    struct mydata mydata;
    struct archive_entry *entry;
    struct archive* archiv = archive_read_new();

    auto name = m_file->get_path();
    //std::cout << "ArchiveDataSource::ArchiveDataSource name " << name << std::endl;

    mydata.name = name.c_str();
    archive_read_support_filter_all(archiv);
    archive_read_support_format_all(archiv);
    archive_read_open(archiv, reinterpret_cast<void*>(&mydata), myopen, myread, myclose);
    // returns separate compressions e.g. tar.gz file will be gzip, none, zip file will be none
    for (int i = 0; i <  archive_filter_count(archiv); ++i) {
        const char * compress = archive_filter_name(archiv, i);
        std::cout << i << " compress " << compress << std::endl;
    }

    while (archive_read_next_header(archiv, &entry) == ARCHIVE_OK) {
        auto archivEntry = std::make_shared<ArchivEntry>(entry);
        //archive_entry_free(entry); the wiki examples doesn't care about this
        m_listener->archivUpdate(archivEntry);
        archive_read_data_skip(archiv);
    }
    ArchivSummary summary;
    summary.setEntries(archive_file_count(archiv));
    archive_read_close(archiv);
    archive_read_free(archiv);
    m_listener->archivDone(summary);
}

bool
Archiv::canRead()
{
// see no alternative to open it ...
    bool has_entries{false};
    struct mydata mydata;
    struct archive_entry *entry;
    struct archive* archiv = archive_read_new();
    auto name = m_file->get_path();
    //std::cout << "ArchiveDataSource::ArchiveDataSource name " << name << std::endl;
    mydata.name = name.c_str();
    archive_read_support_filter_all(archiv);
    archive_read_support_format_all(archiv);
    archive_read_open(archiv, reinterpret_cast<void*>(&mydata), myopen, myread, myclose);
    //has_entries = archive_file_count(archiv) > 0; reports only read entries ...
    while (archive_read_next_header(archiv, &entry) == ARCHIVE_OK) {
        has_entries = true;
        break;  // finding one entry is sufficent for testing
        //archive_read_data_skip(archiv);
    }
    archive_read_close(archiv);
    archive_read_free(archiv);

    return has_entries;
}

