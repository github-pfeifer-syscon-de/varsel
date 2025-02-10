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

#pragma once

#include <DataSource.hpp>
#include <memory>

#include "Archiv.hpp"
#include "ThreadWorker.hpp"


class ArchivWorker
: public ThreadWorker <std::shared_ptr<ArchivEntry>, ArchivSummary>
, public ArchivListener
{
public:
    ArchivWorker(
              const Glib::RefPtr<Gio::File>& file
            , ArchivListener* archivListener);
    explicit ArchivWorker(const ArchivWorker& orig) = delete;
    virtual ~ArchivWorker() = default;

    void archivUpdate(const std::shared_ptr<ArchivEntry>& entry) override;
    void archivDone(ArchivSummary archivSummary) override;

protected:

    ArchivSummary doInBackground() override;
    void process(const std::vector<std::shared_ptr<ArchivEntry>>& entries) override;
    void done() override;


private:
    Glib::RefPtr<Gio::File> m_file;
    ArchivListener* m_archivListener;
    ArchivSummary m_archivSummary;
};

class ArchiveDataSource
: public DataSource
, public ArchivListener
{
public:
    ArchiveDataSource(const Glib::RefPtr<Gio::File>& file, VarselApp* application);
    explicit ArchiveDataSource(const ArchiveDataSource& orig) = delete;
    virtual ~ArchiveDataSource() = default;

    void update(
          const Glib::RefPtr<psc::ui::TreeNodeModel>& treeModel) override;
    const char* getConfigGroup() override;
    Glib::RefPtr<Gio::File> getFileName(const std::string& name) override;
    static bool can_handle(const Glib::RefPtr<Gio::File>& file);
    std::shared_ptr<ListColumns> getListColumns() override;

    void archivUpdate(const std::shared_ptr<ArchivEntry>& entry)  override;
    void archivDone(ArchivSummary archivSummary) override;

private:
    Glib::RefPtr<Gio::File> m_file;
    Glib::RefPtr<psc::ui::TreeNodeModel> m_treeModel;
    std::shared_ptr<FileTreeNode> m_treeItem;
    std::shared_ptr<ArchivWorker> m_archivWorker;
    size_t m_entries{0u};
};

