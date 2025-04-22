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

#include <glibmm.h>
#include <giomm.h>

#include "DataSource.hpp"


class FileDataSource
: public DataSource
{
public:
    FileDataSource(ListApp* application);
    explicit FileDataSource(const FileDataSource& orig) = delete;
    virtual ~FileDataSource() = default;

    virtual void update(
          const Glib::RefPtr<Gio::File>& file
        , std::shared_ptr<psc::ui::TreeNode> treeItem
        , const Glib::RefPtr<psc::ui::TreeNodeModel>& treeModel
        , ListListener* listListener) override;
    virtual const char* getConfigGroup() override;

    Glib::ustring readableFileType(Gio::FileType fileType);
    void setFileValues(Gtk::TreeRow& row
        , const Glib::RefPtr<Gio::File>& file
        , const Glib::RefPtr<Gio::FileInfo>& fileInfo
        , const std::shared_ptr<ListColumns>& listColumns);
    void paste(const std::vector<Glib::ustring>& uris
             , const Glib::RefPtr<Gio::File>& dir
             , bool isMove
             , VarselList* win) override;
    void distribute(const std::vector<PtrEventItem>& items, Gtk::Menu* menu, Gtk::Window* win) override;

protected:

private:
};

