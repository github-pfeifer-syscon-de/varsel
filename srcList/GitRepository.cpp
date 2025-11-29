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

#include <psc_format.hpp>
#include <cmath>
#include <cstring>
#include <limits>

#include "GitRepository.hpp"

namespace psc {
namespace git {


GitException::GitException(const std::string& msg)
: m_msg{msg}
{

}

const char*
GitException::what() const noexcept
{
    return m_msg.c_str();
}

// --------------------------------------------------------
//    Signature
// --------------------------------------------------------
Signature::Signature(const git_signature* signature)
: m_name{signature->name}
, m_email{signature->email}
{
    Glib::DateTime dt = Glib::DateTime::create_now_utc(signature->when.time);
    unsigned long m = std::abs(signature->when.offset) % 60;
    unsigned long h = signature->when.offset / 60;
    Glib::TimeZone tz = Glib::TimeZone::create(Glib::ustring::sprintf("%02d%02d", h, m));
    m_when = dt.to_timezone(tz);
}

std::string
Signature::getAuthor()
{
    return m_name;
}

std::string
Signature::getEmail()
{
    return m_email;
}

Glib::DateTime
Signature::getWhen()
{
    return m_when;
}

// --------------------------------------------------------
//    Commit
// --------------------------------------------------------

Commit::Commit(git_commit *commit)
: m_commit{commit}
{

}

Commit::~Commit()
{
    if (m_commit) {
        git_commit_free(m_commit);
    }

}

std::string
Commit::getMessage()
{
    // Print some of the commit's properties
    auto msg = git_commit_message(m_commit);
    std::string smsg{msg};
    return smsg;
}

std::shared_ptr<Signature>
Commit::getSignature()
{
    std::shared_ptr<Signature> signature;
    const git_signature* git_signature = git_commit_author(m_commit);
    if (git_signature) {
        signature = std::make_shared<Signature>(git_signature);
        //git_signature_free(git_signature);
    }
    return signature;
}

// --------------------------------------------------------
//    Repository
// --------------------------------------------------------

Repository::Repository(const std::string& path, bool open)
: m_path{path}
{
    if (open) {
        //int error = git_repository_open(&m_repo, path.c_str());
        /* Open repository in given directory (or fail if not a repository) */
        int error = git_repository_open_ext(
            &m_repo, path.c_str(), GIT_REPOSITORY_OPEN_NO_SEARCH, nullptr);
        if (error < 0) {
            throw GitException(
                    psc::fmt::format("Could not open repository {}", git_error_last()->message));
        }
    }
}

Repository::~Repository()
{
    if (m_repo) {
        git_repository_free(m_repo);
    }
}

std::string
Repository::errorMsg(int error, const std::string& message)
{
	const git_error *lg2err = git_error_last();
	const char *lg2msg = "";
    const char *lg2spacer = "";
	if (lg2err != nullptr && lg2err->message != nullptr) {
		lg2msg = lg2err->message;
		lg2spacer = " - ";
	}
    auto msg = psc::fmt::format("{} [{}]{}{}", message, error, lg2spacer, lg2msg);
    return msg;
}

std::shared_ptr<Commit>
Repository::getSingelCommit(const std::string& rev)
{
    git_object *head_commit;
    int error = git_revparse_single(&head_commit, m_repo, rev.c_str());
    if (error != 0) {
        throw GitException(
                errorMsg(error,
                    psc::fmt::format("No commit found for {}", rev)));
    }
    git_commit *commit = (git_commit*)head_commit;
    return std::make_shared<Commit>(commit);
}

std::string
Repository::getBranch()
{
    std::string sbranch;
	git_reference *head{nullptr};
	int error = git_repository_head(&head, m_repo);
	if (error == GIT_EUNBORNBRANCH || error == GIT_ENOTFOUND) {
        // leave empty
    }
	else if (!error) {
		const char *branch = git_reference_shorthand(head);
        if (branch) {
            sbranch = branch;
        }
	}
    else {
		throw GitException(
                errorMsg(error, "Failed to get current branch"));
    }
	git_reference_free(head);
    return sbranch;
}

std::vector<std::string>
Repository::getIter(const std::string& query)
{
    std::vector<std::string> ret;
    ret.reserve(16);
    git_reference_iterator *iter = nullptr;
    int error = git_reference_iterator_glob_new(&iter, m_repo, query.c_str());
    if (!error) {
        const char *name = nullptr;
        while (!(error = git_reference_next_name(&name, iter))) {
            ret.emplace_back(std::string(name));
        }
        if (error != GIT_ITEROVER) {
            throw GitException(
                    errorMsg(error,
                        psc::fmt::format("Failed to iter query {}", query)));
        }
    }
    else {
        throw GitException(
                errorMsg(error,
                        psc::fmt::format("Failed to query query {}", query)));
    }
    return ret;
}



Remote
Repository::getRemote(const std::string& query)
{
    Remote remoteCpp;
    git_remote *remote = NULL;
    int error = git_remote_lookup(&remote, m_repo, query.c_str());
    if (error) {
        throw GitException(
                errorMsg(error,
                    psc::fmt::format("Failed to lookup remote query {}", query)));
    }
    const char *name = git_remote_name(remote);
    std::string ret;
    if (name) {
        remoteCpp.setName(name);
    }
    const char *url  = git_remote_url(remote);
    if (url) {
        remoteCpp.setUrl(url);
    }
    const char *pushurl = git_remote_pushurl(remote);
    if (pushurl) {
        remoteCpp.setPushUrl(pushurl);
    }
    git_remote_free(remote);
    return remoteCpp;
}

static int
fetch_progress(
            const git_indexer_progress *stats,
            void *payload)
{
    Repository *repo = static_cast<Repository*>(payload);
    return repo->fetchProgress(stats);
}

static void
checkout_progress(
            const char *path,
            size_t curent,
            size_t total,
            void *payload)
{
    Repository *repo = static_cast<Repository*>(payload);
    repo->checkoutProgress(path, curent, total);
}

void
Repository::clone(const std::string& url)
{
    git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;

    clone_opts.checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
    clone_opts.checkout_opts.progress_cb = checkout_progress;
    clone_opts.checkout_opts.progress_payload = this;
    clone_opts.fetch_opts.callbacks.transfer_progress = fetch_progress;
    clone_opts.fetch_opts.callbacks.payload = this  ;

    int error = git_clone(&m_repo, url.c_str(), m_path.c_str(), &clone_opts);
    if (error) {
        throw GitException(
            errorMsg(error, "Failed to clone"));
    }
}

void
Repository::setCheckoutListener(CheckoutListener* checkoutListenr)
{
    m_checkoutListenr = checkoutListenr;
}


void
Repository::checkoutProgress(const char *path, size_t curent, size_t total)
{
    if (m_checkoutListenr) {
        m_checkoutListenr->checkoutProgress(path, curent, total);
    }
}

void
Repository::setFetchListener(FetchListener* fetchListener)
{
    m_fetchListener = fetchListener;
}


int
Repository::fetchProgress(const git_indexer_progress *stats)
{
    if (m_fetchListener) {
        return m_fetchListener->fetchProgress(stats);
    }
    return 0;
}


Status
Repository::getStatus()
{
    return Status(m_repo);
}

StatusIterator::StatusIterator(Status* status, size_t index)
: m_status{status}
, m_index{index}
{
}

StatusIterator
StatusIterator::operator++()
{
    if (m_status
     && m_index < m_status->getMax()
     && m_status->inc(this)) {
        ++m_index;
    }
    else {
        m_index = std::numeric_limits<size_t>::max();
    }
    return *this;
}

Status::Status(git_repository* repos)
{
    //std::cout << "Status::Status" << std::endl;
    git_status_options statusopt = GIT_STATUS_OPTIONS_INIT;
    statusopt.show  = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    statusopt.flags =
        GIT_STATUS_OPT_INCLUDE_UNMODIFIED |
        GIT_STATUS_OPT_INCLUDE_UNTRACKED |
        GIT_STATUS_OPT_INCLUDE_IGNORED |
        GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX |
        GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;

    int error = git_status_list_new(&m_status, repos, &statusopt);
    if (error) {
        throw GitException(
                Repository::errorMsg(error, "Failed to get status"));
    }
    m_maxi = git_status_list_entrycount(m_status);
}


Status::Status(Status&& status)
{
    m_status = status.m_status;
    m_file = status.m_file;
    status.m_status = nullptr;
}

Status::~Status()
{
    if (m_status) {
        git_status_list_free(m_status);
        m_status = nullptr;
    }
}

bool
Status::inc(const StatusIterator* iter)
{
    size_t index = iter->getIndex();
    if (index < m_maxi) {
        DirStatus dirStatIndex;
        DirStatus dirStatWorkdir;
        //std::cout << "Status::inc"
        //          << " status " << std::hex << m_status << std::dec
        //          << " index " << iter->getIndex() << "/" << m_maxi << std::endl;
        const git_status_entry* s = git_status_byindex(m_status, index);
        //std::cout << std::hex << "status " << s->status << " curr " << GIT_STATUS_CURRENT << std::dec << std::endl;
        if (s->status == GIT_STATUS_CURRENT) {  // this will not be reported ... (and dont see option for it)
            dirStatIndex.setStatus(FileStatus::Current);
        }
        else {
            if (s->status & GIT_STATUS_INDEX_NEW)
                dirStatIndex.setStatus(FileStatus::New);
            if (s->status & GIT_STATUS_INDEX_MODIFIED)
                dirStatIndex.setStatus(FileStatus::Modified);
            if (s->status & GIT_STATUS_INDEX_DELETED)
                dirStatIndex.setStatus(FileStatus::Deleted);
            if (s->status & GIT_STATUS_INDEX_RENAMED)
                dirStatIndex.setStatus(FileStatus::Deleted);
            if (s->status & GIT_STATUS_INDEX_TYPECHANGE)
                dirStatIndex.setStatus(FileStatus::TypeChange);
        }
        const char* old_path = s->head_to_index
                                ? s->head_to_index->old_file.path
                                : nullptr;
        const char* new_path = s->head_to_index
                                ? s->head_to_index->new_file.path
                                : nullptr;
        if (old_path && new_path && std::strcmp(old_path, new_path)) {
            dirStatIndex.setOldPath(old_path);
            dirStatIndex.setNewPath(new_path);
        }
        else if (old_path || new_path) {
            dirStatIndex.setOldPath(old_path ? old_path : new_path);
        }

        /**
         * With `GIT_STATUS_OPT_INCLUDE_UNMODIFIED` (not used in this example)
         * `index_to_workdir` may not be `NULL` even if there are
         * no differences, in which case it will be a `GIT_DELTA_UNMODIFIED`.
         */
        if (s->status == GIT_STATUS_CURRENT) {  /*  || s->index_to_workdir == nullptr */
            dirStatWorkdir.setStatus(FileStatus::Current);
        }
        else {
            /** Print out the output since we know the file has some changes */
            if (s->status & GIT_STATUS_WT_MODIFIED)
                dirStatWorkdir.setStatus(FileStatus::Modified);
            if (s->status & GIT_STATUS_WT_DELETED)
                dirStatWorkdir.setStatus(FileStatus::Deleted);
            if (s->status & GIT_STATUS_WT_RENAMED)
                dirStatWorkdir.setStatus(FileStatus::Renamed);
            if (s->status & GIT_STATUS_WT_TYPECHANGE)
                dirStatWorkdir.setStatus(FileStatus::TypeChange);
            if (s->status & GIT_STATUS_WT_NEW)
                dirStatWorkdir.setStatus(FileStatus::New);  // or named untracked
        }
        old_path = s->index_to_workdir
                                ? s->index_to_workdir->old_file.path
                                : nullptr;
        new_path = s->index_to_workdir
                                ? s->index_to_workdir->new_file.path
                                : nullptr;
        if (old_path && new_path && std::strcmp(old_path, new_path)) {
            dirStatWorkdir.setOldPath(old_path);
            dirStatWorkdir.setNewPath(new_path);
        }
        else if (old_path || new_path) {
            dirStatWorkdir.setOldPath(old_path ? old_path : new_path);
        }

        if (s->status == GIT_STATUS_IGNORED) {
            if (dirStatWorkdir.getStatus() == FileStatus::None)
                dirStatWorkdir.setStatus(FileStatus::Ignore);
            if (dirStatIndex.getStatus() == FileStatus::None)
                dirStatIndex.setStatus(FileStatus::Ignore);
        }

        m_file.setIndex(dirStatIndex);
        m_file.setWorkdir(dirStatWorkdir);
        return true;
    }
    return false;
}

File*
Status::getFile()
{
    return &m_file;
}

StatusIterator
Status::begin()
{
    return StatusIterator(this, 0);
}

StatusIterator
Status::end()
{
    return StatusIterator(this, std::numeric_limits<size_t>::max());
}

size_t
Status::getMax()
{
    return m_maxi;
}


} /* namespace git */
} /* namespace psc */
