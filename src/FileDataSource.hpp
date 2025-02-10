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
    FileDataSource(const Glib::RefPtr<Gio::File>& file, VarselApp* application);
    explicit FileDataSource(const FileDataSource& orig) = delete;
    virtual ~FileDataSource() = default;

    void update(
          const Glib::RefPtr<psc::ui::TreeNodeModel>& treeModel) override;
    const char* getConfigGroup() override;
    Glib::RefPtr<Gio::File> getFileName(const std::string& name) override;

private:
    Glib::RefPtr<Gio::File> m_file;
};

