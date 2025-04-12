/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
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


#include "ListColumns.hpp"

SizeConverter::SizeConverter(Gtk::TreeModelColumn<goffset>& col)
: psc::ui::CustomConverter<goffset>(col)
{
}

void
SizeConverter::convert(Gtk::CellRenderer* rend, const Gtk::TreeModel::iterator& iter)
{
    goffset value = 0l;
    iter->get_value(m_col.index(), value);
    auto textRend = static_cast<Gtk::CellRendererText*>(rend);
    textRend->property_text() = Glib::format_size(value, Glib::FORMAT_SIZE_IEC_UNITS);    // base x^2
}

PermConverter::PermConverter(Gtk::TreeModelColumn<uint32_t>& col)
: psc::ui::CustomConverter<uint32_t>(col)
{
}

void
PermConverter::convert(Gtk::CellRenderer* rend, const Gtk::TreeModel::iterator& iter)
{
    uint32_t fmode{0u};
    iter->get_value(m_col.index(), fmode);
    Glib::ustring smode;
    smode.reserve(16);
    smode += fmode & S_IRUSR ? 'r' : '-';
    smode += fmode & S_IWUSR ? 'w' : '-';
    smode += fmode & S_IXUSR ? 'x' : '-';
    smode += fmode & S_IRGRP ? 'r' : '-';
    smode += fmode & S_IWGRP ? 'w' : '-';
    smode += fmode & S_IXGRP ? 'x' : '-';
    smode += fmode & S_IROTH ? 'r' : '-';
    smode += fmode & S_IWOTH ? 'w' : '-';
    smode += fmode & S_IXOTH ? 'x' : '-';
    smode += Glib::ustring::sprintf(" %04o", fmode & 0x01ff);
    auto textRend = static_cast<Gtk::CellRendererText*>(rend);
    textRend->property_text() = smode;
}

DateConverter::DateConverter(Gtk::TreeModelColumn<Glib::DateTime>& col)
: psc::ui::CustomConverter<Glib::DateTime>(col)
{
}

void
DateConverter::convert(Gtk::CellRenderer* rend, const Gtk::TreeModel::iterator& iter)
{
    Glib::DateTime date;
    iter->get_value(m_col.index(), date);
    Glib::ustring dateText;
    if (date) {
        dateText = date.format("%x %X");
    }
    auto textRend = static_cast<Gtk::CellRendererText*>(rend);
    textRend->property_text() = dateText;
}

ListColumns::ListColumns()
{
    add(_("Name"), m_name);
    auto sizeConverter = std::make_shared<SizeConverter>(m_size);
    add<goffset>(_("Size"), sizeConverter, 1.0f);
    add(_("Type"), m_type, 1.0f);
    auto permConverter = std::make_shared<PermConverter>(m_mode);
    add<uint32_t>(_("Mode"), permConverter, 1.0f);
    add(_("User"), m_user);
    add(_("Group"), m_group);
    auto modifiedDate = std::make_shared<DateConverter>(m_modified);
    add<Glib::DateTime>(_("Modified"), modifiedDate);
    add(_("ContentType"), m_contentType);
    add<Glib::RefPtr<Gdk::Pixbuf>>(_("Icon"), m_icon);

    Gtk::TreeModel::ColumnRecord::add(m_fileInfo);
    Gtk::TreeModel::ColumnRecord::add(m_file);
}