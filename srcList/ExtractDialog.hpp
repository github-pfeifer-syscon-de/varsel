/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4;  coding: utf-8; -*-  */
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

#include <memory>
#include <set>
#include <gtkmm.h>

#include "Archiv.hpp"
#include "ThreadWorker.hpp"
#include "ArchiveDataSource.hpp"


class ArchivExtractEntry
: public ArchivEntry
{
public:
    ArchivExtractEntry(struct archive_entry *entry, const Glib::RefPtr<Gio::File>& dir);
    virtual ~ArchivExtractEntry() = default;

    int handleContent(struct archive* archiv) override;
private:
    Glib::RefPtr<Gio::File> m_dir;
};


class ArchivExtractWorker
: public ThreadWorker <std::shared_ptr<ArchivEntry>, ArchivSummary>
, public ArchivListener
{
public:
    ArchivExtractWorker(
              const Glib::RefPtr<Gio::File>& archiveFile
            , const Glib::RefPtr<Gio::File>& extractDir
            , const std::vector<PtrEventItem>& items
            , ArchivListener* archivListener);
    explicit ArchivExtractWorker(const ArchivExtractWorker& orig) = delete;
    virtual ~ArchivExtractWorker() = default;

    PtrArchivEntry createEntry(struct archive_entry *entry) override;
    void archivUpdate(const std::shared_ptr<ArchivEntry>& entry) override;
    void archivDone(ArchivSummary archivSummary, const Glib::ustring& msg) override;

protected:

    ArchivSummary doInBackground() override;
    void process(const std::vector<std::shared_ptr<ArchivEntry>>& entries) override;
    void done() override;


private:
    Glib::RefPtr<Gio::File> m_archivFile;
    Glib::RefPtr<Gio::File> m_extractDir;
    std::set<Glib::ustring> m_items;
    ArchivSummary m_archivSummary;
    ArchivListener* m_archivListener;
};



class ExtractDialog
: public Gtk::Dialog
, public ArchivListener
{
public:
    ExtractDialog(BaseObjectType* cobject
        , const Glib::RefPtr<Gtk::Builder>& builder
        , const Glib::RefPtr<Gio::File>& file
        , const std::vector<PtrEventItem>& items
        , Gtk::Window* win);
    virtual ~ExtractDialog() = default;
    void archivUpdate(const PtrArchivEntry& entry) override;
    void archivDone(ArchivSummary archivSummary, const Glib::ustring& msg) override;
    Glib::RefPtr<Gio::File> getDirectory();
    static Glib::RefPtr<Gio::File> show(
                 const Glib::RefPtr<Gio::File>& file
                , const std::vector<PtrEventItem>& items
                , Gtk::Window* win);
protected:
    void selected();
    void extract();


private:
    Glib::RefPtr<Gio::File> m_file;
    Glib::RefPtr<Gio::File> m_dir;
    std::vector<PtrEventItem> m_items;
    Gtk::Window* m_win;
    std::shared_ptr<ArchivExtractWorker> m_archivExtractWorker;

    Gtk::Label* m_archive;
    Gtk::FileChooserButton* m_target;
    Gtk::ProgressBar* m_progress;
    Gtk::Label* m_info;
    Gtk::Button* m_cancel;
    Gtk::Button* m_open;
    Gtk::Button* m_apply;
    uint32_t m_extracted{};
};
