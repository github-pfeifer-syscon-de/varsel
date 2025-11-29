/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2025 RPf 
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

#include <string>
#include <exception>
#include <memory>
#include <glibmm.h>

#include <git2.h>


namespace psc {
namespace git {


class GitException
: public std::exception
{
public:
    GitException(const std::string& msg);
    virtual ~GitException() = default;

    const char* what() const noexcept;
private:
    std::string m_msg;
};

enum class FileStatus
{
      None
    , Current
    , Deleted
    , New
    , Modified
    , Renamed
    , TypeChange
    , Ignore
};

class Remote
{
public:
    Remote() = default;
    virtual ~Remote() = default;

    void setName(std::string_view name)
    {
        m_name = name;
    }
    std::string getName()
    {
        return m_name;
    }

    void setUrl(std::string_view url)
    {
        m_url = url;
    }
    std::string getUrl()
    {
        return m_url;
    }

    void setPushUrl(std::string_view pushurl)
    {
        m_pushUrl = pushurl;
    }
    std::string getPushUrl()
    {
        return m_pushUrl;
    }

private:
    std::string m_name;
    std::string m_url;
    std::string m_pushUrl;
};

class DirStatus
{
public:
    DirStatus()
    {
    }
    virtual ~DirStatus() = default;

    void setStatus(FileStatus fileStatus)
    {
        m_status = fileStatus;
    }
    FileStatus getStatus()
    {
        return m_status;
    }
    void setOldPath(std::string_view oldPath)
    {
        m_oldPath = oldPath;
    }
    std::string getOldPath()
    {
        return m_oldPath;
    }
    void setNewPath(std::string_view newPath)
    {
        m_newPath = newPath;
    }
    std::string getNewPath()
    {
        return m_newPath;
    }
    static std::string getStatus(FileStatus status)
    {
        using enum FileStatus;
        switch(status) {
        case Current:
            return "Current";
            break;
        case Deleted:
            return "Deleted";
            break;
        case New:
            return "New";
            break;
        case Modified:
            return "Modified";
            break;
        case Renamed:
            return "Renamed";
            break;
        case TypeChange:
            return "TypeChange";
            break;
        case Ignore:
            return "Ignore";
            break;
        case None:  // included for completeness
        default:
            return "";
        }

    }
    std::string to_string()
    {
        std::string s = getStatus(m_status);
        if (!s.empty()) {
            s += " " + (!m_newPath.empty()
                        ? m_oldPath + " -> " + m_newPath
                        : m_oldPath);
        }
        return s;
    }
private:
    FileStatus m_status{FileStatus::None};
    std::string m_oldPath;
    std::string m_newPath;
};


class File
{
public:
    File()
    {
    }
    File(const DirStatus& index, const DirStatus& workdir)
    : m_index{index}
    , m_workdir{workdir}
    {
    }
    virtual ~File() = default;

    void setIndex(const DirStatus& indexStatus)
    {
        m_index = indexStatus;
    }
    DirStatus getIndex()
    {
        return m_index;
    }
    void setWorkdir(const DirStatus& workdir)
    {
        m_workdir = workdir;
    }
    DirStatus getWorkdir()
    {
        return m_workdir;
    }
    std::string to_string()
    {
        return m_index.to_string() + "\t\t\t" + m_workdir.to_string();
    }
private:
    DirStatus m_index;
    DirStatus m_workdir;
};


class StatusIterator;


class Status
{
public:
    Status(git_repository* repos);
    // don't allow to copy this
    explicit Status(const Status& other) = delete;
    // but allow to move, (keep only one reference) so we won't run into freeing issues
    Status(Status&& other);
    virtual ~Status();
    bool inc(const StatusIterator* iter);
    File* getFile();
    StatusIterator begin();
    StatusIterator end();
    size_t getMax();

private:
    git_status_list *m_status{nullptr};
    File m_file;
    size_t m_maxi;
};

class StatusIterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = int64_t;
    using value_type        = File;
    using pointer_type      = File*;
    using reference_type    = File&;

public:
    StatusIterator(Status* status, size_t index);
    StatusIterator(const StatusIterator& other) = default;
    virtual ~StatusIterator() = default;
    StatusIterator operator++();
    StatusIterator operator++(int)
    {
        StatusIterator temp = *this;
        operator++();
        return temp;
    }
    value_type operator*()
    {
        return *m_status->getFile();
    }
    pointer_type operator->()
    {
        return m_status->getFile();
    }
    friend bool operator== (const StatusIterator& a, const StatusIterator& b)
    {
        return a.m_status == b.m_status && a.m_index == b.m_index;
    }
    friend bool operator!= (const StatusIterator& a, const StatusIterator& b)
    {
        return a.m_status != b.m_status || a.m_index != b.m_index;
    }
    size_t getIndex() const
    {
        return m_index;
    }
private:
    Status* m_status;
    size_t m_index;
};


class Signature
{
public:
    Signature(const git_signature* signature);
    virtual ~Signature() = default;

    std::string getAuthor();
    std::string getEmail();
    Glib::DateTime getWhen();

private:
    std::string m_name;
    std::string m_email;
    Glib::DateTime m_when;
};


class Commit
{
public:
    Commit(git_commit *commit);
    virtual ~Commit();

    std::string getMessage();
    std::shared_ptr<Signature> getSignature();
private:
    git_commit *m_commit{nullptr};
};

class CheckoutListener
{
public:
    virtual void checkoutProgress(const char *path, size_t curent, size_t total) = 0;
};

class FetchListener
{
public:
    virtual int fetchProgress(const git_indexer_progress *stats) = 0;
};

class Repository
{
public:
    Repository(const std::string& path, bool open = true);
    explicit Repository(const Repository& orig) = delete;
    virtual ~Repository();

    static std::string errorMsg(int error, const std::string& message);
    std::shared_ptr<Commit> getSingelCommit(const std::string& rev);
    std::string getBranch();
    Status getStatus();
    // e.g. "refs/heads/*"
    std::vector<std::string> getIter(const std::string& query);
    // e.g. "origin"
    Remote getRemote(const std::string& query);
    void clone(const std::string& url);
    void setCheckoutListener(CheckoutListener* checkoutListenr);
    void checkoutProgress(const char *path, size_t curent, size_t total);
    void setFetchListener(FetchListener* fetchListener);
    int fetchProgress(const git_indexer_progress *stats);
private:
    git_repository* m_repo{nullptr};
    std::string m_path;
    CheckoutListener* m_checkoutListenr{nullptr};
    FetchListener* m_fetchListener{nullptr};
};

} /* namespace git */
} /* namespace psc */
