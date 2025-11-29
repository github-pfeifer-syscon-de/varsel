/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4;  coding: utf-8; -*-  */
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

#include <gtkmm.h>
#include <psc_i18n.hpp>
#include <KeyfileTableManager.hpp>
#include <TreeNodeModel.hpp>
#include <cstdint>

class SizeConverter
: public psc::ui::CustomConverter<goffset>
{
public:
    SizeConverter(Gtk::TreeModelColumn<goffset>& col);
    virtual ~SizeConverter() = default;

    void convert(Gtk::CellRenderer* rend, const Gtk::TreeModel::iterator& iter) override;
private:

};

class PermConverter
: public psc::ui::CustomConverter<uint32_t>
{
public:
    PermConverter(Gtk::TreeModelColumn<uint32_t>& col);
    virtual ~PermConverter() = default;

    void convert(Gtk::CellRenderer* rend, const Gtk::TreeModel::iterator& iter) override;
};

class DateConverter
: public psc::ui::CustomConverter<Glib::DateTime>
{
public:
    DateConverter(Gtk::TreeModelColumn<Glib::DateTime>& col);
    virtual ~DateConverter() = default;

    void convert(Gtk::CellRenderer* rend, const Gtk::TreeModel::iterator& iter) override;
};

class IconConverter
: public psc::ui::CustomConverter<Glib::RefPtr<Glib::Object>>
{
public:
    IconConverter(Gtk::TreeModelColumn<Glib::RefPtr<Glib::Object>>& col);
    virtual ~IconConverter() = default;
    static constexpr auto LOOKUP_ICON_SIZE{16};
    Gtk::CellRenderer* createCellRenderer() override;

    void convert(Gtk::CellRenderer* rend, const Gtk::TreeModel::iterator& iter) override;
};


class ListColumns
: public psc::ui::ColumnRecord
{
public:
    Gtk::TreeModelColumn<Glib::ustring> m_name;
    Gtk::TreeModelColumn<goffset> m_size;
    Gtk::TreeModelColumn<Glib::ustring> m_type;
    Gtk::TreeModelColumn<uint32_t> m_mode;
    Gtk::TreeModelColumn<Glib::ustring> m_user;
    Gtk::TreeModelColumn<Glib::ustring> m_group;
    Gtk::TreeModelColumn<Glib::DateTime> m_modified;
    Gtk::TreeModelColumn<Glib::ustring> m_contentType;
    Gtk::TreeModelColumn<Glib::RefPtr<Glib::Object>> m_icon;
    Gtk::TreeModelColumn<Glib::ustring> m_symLink;

    Gtk::TreeModelColumn<Glib::RefPtr<Gio::FileInfo>> m_fileInfo;
    Gtk::TreeModelColumn<Glib::RefPtr<Gio::File>> m_file;
    ListColumns();

};
