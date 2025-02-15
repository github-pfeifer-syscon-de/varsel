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
#include <psc_i18n.hpp>

class ArchivEntry
{
public:
    ArchivEntry();
    ArchivEntry(struct archive_entry *entry);
    ArchivEntry(const ArchivEntry& orig) = default;
    virtual ~ArchivEntry() = default;

    Glib::ustring getPath()
    {
        return m_path;
    }
    void setPath(const Glib::ustring& path)
    {
        m_path = path;
    }
    mode_t getMode()
    {
        return m_mode;
    }
    void setMode(int mode)
    {
        m_mode = mode;
    }
    mode_t getPermission()
    {
        return m_permission;
    }
    void setPermission(mode_t permission)
    {
        m_permission = permission;
    }
    Glib::ustring getUser()
    {
        return m_user;
    }
    void setUser(const Glib::ustring& user)
    {
        m_user = user;
    }
    Glib::ustring getGroup()
    {
        return m_group;
    }
    void setGroup(const Glib::ustring& group)
    {
        m_group = group;
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
    void setSize(la_int64_t size)
    {
        m_size = size;
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

    virtual void archivUpdate(const std::shared_ptr<ArchivEntry>& entry) = 0;
    virtual void archivDone(ArchivSummary archivSummary, const Glib::ustring& errMsg) = 0;
protected:
    virtual int handleContent(struct archive* archiv)
    {
        return archive_read_data_skip(archiv);
    }

private:

    friend class Archiv;
};

class ArchivException
: public std::exception
{
public:
    ArchivException(const std::string& msg);
    virtual ~ArchivException() = default;

    const char* what() const noexcept;
private:
    std::string m_msg;
};

class ArchivProvider;

class Archiv
{
public:
    Archiv(Glib::RefPtr<Gio::File> file);
    explicit Archiv(const Archiv& orig) = delete;
    virtual ~Archiv();

    /**
     * reports content of archiv to listener
     *   at the end either a ArchivException
     *   is generated or a done is reported to the listener.
     * @param listener
     */
    void read(ArchivListener* listener);
    /**
     * builds archiv
     * (the format/filter should have been setup by addFormat)
     * @param provider to enumerate the content of archive
     */
    void write(ArchivProvider* provider);

    bool canRead();
    /**
     * archive must have been read, to contain infos
     * @return the combination of compressions & format used
     */
    std::vector<std::string> getReadFormats();
    /**
     * this commes with no checks at all, so it's up to you
     *   to select useful combinations, i found the following rules:
     *   - add compression first (if needed)
     *   - use only on format (this might be obvious, but just in case)
     * @param fmt for use with ARCHIVE_FORMAT_...ARCHIVE_COMPRESSION_... constants
     */
    void addWriteFormat(int fmt);
    static constexpr size_t BUF_SIZE{8u*1024u};

    int cc_readopen(struct archive *a);
    la_ssize_t cc_read(struct archive *a, const void **ebuff);
    la_ssize_t cc_readskip(struct archive *a, off_t request);
    int cc_readclose();

    int cc_writeopen(struct archive *archiv);
    la_ssize_t cc_write(struct archive *archiv, const void *buffer, size_t length);
    int cc_writeclose(struct archive *archiv);
    int cc_writefree(struct archive *archiv);

protected:
    void setError(struct archive *archiv, const Glib::Error& err, const char* where);
    void setFormat(struct archive* archiv);
    int writeContent(archive* archiv, struct archive_entry *entry, const Glib::RefPtr<Gio::File>& file);

private:
    Glib::RefPtr<Gio::File> m_file;
    std::vector<std::string> m_readFormats;
    std::vector<int> m_writeFormats;

    // these are the internal "low level" parts
    using BUFFER_ARRAY = std::array<uint8_t, BUF_SIZE>;
    Glib::RefPtr<Gio::FileInputStream> m_fileInputstream;
    Glib::RefPtr<Gio::FileOutputStream> m_fileOutputstream;
    std::unique_ptr<BUFFER_ARRAY> buff;
    //std::exception_ptr m_eptr;  // does not really allows deeper error inspection
    friend class ArchivFileProvider;
};


class ArchivProvider
{
public:
    virtual std::shared_ptr<ArchivEntry> getNextEntry() = 0;
    virtual int writeContent(Archiv* archiv, struct archive* struarchiv, struct archive_entry *entry) = 0;

};

class ArchivDirWalker;

/**
 * this is a basic implementation to pack files from a directory
 *   you probably want to customize this to your needs.
 * @param dir to look for files
 * @param useSubDirs look into sub directories
 */
class ArchivFileProvider
: public ArchivProvider
{
public:
    ArchivFileProvider(const Glib::RefPtr<Gio::File>& dir, bool useSubDirs);
    virtual ~ArchivFileProvider() = default;

    virtual bool isFilterEntry(const Glib::RefPtr<Gio::File>& file) = 0;

    std::shared_ptr<ArchivEntry> getNextEntry();
    int writeContent(Archiv* archiv, struct archive* structarchiv, struct archive_entry *entry);
    bool useSubDirs()
    {
        return m_useSubDirs;
    }
    void setPermission(mode_t permission)
    {
        m_permission = permission;
    }
    Glib::RefPtr<Gio::File> m_dir;
    bool m_useSubDirs;
    std::shared_ptr<ArchivEntry> m_entry;
    Glib::RefPtr<Gio::File> m_activFile;
    std::list<std::shared_ptr<ArchivDirWalker>> m_workers;
    mode_t m_permission{0644};
};

class ArchivDirWalker
{
public:
    ArchivDirWalker(Glib::RefPtr<Gio::File> scanDir, ArchivFileProvider* provider);
    virtual ~ArchivDirWalker() = default;
    Glib::RefPtr<Gio::File> scanEntries();

private:
    Glib::RefPtr<Gio::File> m_scanDir;
    ArchivFileProvider* m_provider;
    Glib::RefPtr<Gio::FileEnumerator> m_entries;

};
