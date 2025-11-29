/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2024 RPf 
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

#include "DataSource.hpp"
#include "Archiv.hpp"
#include "ThreadWorker.hpp"
#include "ExtractDialog.hpp"


class ArchivListWorker
: public ThreadWorker <std::shared_ptr<ArchivEntry>, ArchivSummary>
, public ArchivListener
{
public:
    ArchivListWorker(
              const Glib::RefPtr<Gio::File>& file
            , ArchivListener* archivListener);
    explicit ArchivListWorker(const ArchivListWorker& orig) = delete;
    virtual ~ArchivListWorker() = default;

    void archivUpdate(const std::shared_ptr<ArchivEntry>& entry) override;
    void archivDone(ArchivSummary archivSummary, const Glib::ustring& msg) override;

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
    ArchiveDataSource(ListApp* application);
    explicit ArchiveDataSource(const ArchiveDataSource& orig) = delete;
    virtual ~ArchiveDataSource() = default;

    void update(
          const Glib::RefPtr<Gio::File>& file
        , std::shared_ptr<psc::ui::TreeNode> treeItem
        , const Glib::RefPtr<psc::ui::TreeNodeModel>& treeModel
        , ListListener* listListener) override;
    const char* getConfigGroup() override;
    static bool can_handle(const Glib::RefPtr<Gio::File>& file);
    std::shared_ptr<ListColumns> getListColumns() override;

    void archivUpdate(const std::shared_ptr<ArchivEntry>& entry)  override;
    void archivDone(ArchivSummary archivSummary, const Glib::ustring& errMsg) override;
    void paste(const std::vector<Glib::ustring>& uris
            , const Glib::RefPtr<Gio::File>& dir
            , bool isMove
            , VarselList* win) override;
    void distribute(const std::vector<PtrEventItem>& items, Gtk::Menu* menu, Gtk::Window* win) override;
    Gtk::MenuItem* createItem(const std::vector<PtrEventItem>& items, Gtk::Menu* gtkMenu, const Glib::ustring& name, Gtk::Window* win);
    void do_handle(const std::vector<PtrEventItem>& items, Gtk::Window* win);
private:
    Glib::RefPtr<Gio::File> m_file;
    Glib::RefPtr<psc::ui::TreeNodeModel> m_treeModel;
    std::shared_ptr<FileTreeNode> m_treeItem;
    std::shared_ptr<ArchivListWorker> m_archivWorker;
    size_t m_entries{0u};
    ListListener* m_listListener{nullptr};
};

