/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4 -*-  */
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

#pragma once

#include <archive.h>
#include <archive_entry.h>
#include <glibmm.h>
#include <giomm.h>

class ArchivEntry
{
public:
    ArchivEntry(struct archive_entry *entry);
    explicit ArchivEntry(const ArchivEntry& orig) = delete;
    virtual ~ArchivEntry() = default;

    Glib::ustring getPath()
    {
        return m_path;
    }
    mode_t getMode()
    {
        return m_mode;
    }
    mode_t getPermission()
    {
        return m_permission;
    }
    Glib::ustring getUser()
    {
        return m_user;
    }
    Glib::ustring getGroup()
    {
        return m_group;
    }
    time_t getCreated()
    {
        return m_created;
    }
    time_t getModified()
    {
        return m_modified;
    }
    la_int64_t getSize()
    {
        return m_size;
    }
private:
    Glib::ustring m_path;
    mode_t m_mode{0};
    mode_t m_permission{0};
    Glib::ustring m_user;
    Glib::ustring m_group;
    time_t m_created{0};
    time_t m_modified{0};
    la_int64_t m_size{0};
};

class ArchivSummary
{
public:
    ArchivSummary()
    {
    }
    ArchivSummary(const ArchivSummary& orig) = default;
    virtual ~ArchivSummary() = default;
    size_t getEntries()
    {
        return m_entries;
    }
    void setEntries(size_t entries)
    {
        m_entries = entries;
    }
private:
    size_t m_entries{0};
};

class ArchivListener
{
public:
    ArchivListener();
    explicit ArchivListener(const ArchivListener& orig) = delete;
    virtual ~ArchivListener() = default;

    virtual void archivUpdate(const std::shared_ptr<ArchivEntry>& entry) = 0;
    virtual void archivDone(ArchivSummary archivSummary) = 0;
};

class Archiv
{
public:
    Archiv(Glib::RefPtr<Gio::File> file, ArchivListener* listener);
    explicit Archiv(const Archiv& orig) = delete;
    virtual ~Archiv() = default;

    void read();
    bool canRead();
private:
    Glib::RefPtr<Gio::File> m_file;
    ArchivListener* m_listener;
};

