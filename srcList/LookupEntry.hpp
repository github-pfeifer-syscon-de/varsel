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

class LookupEntry
: public Gtk::SearchEntry
{
public:
    LookupEntry(BaseObjectType* cobject
        , const Glib::RefPtr<Gtk::Builder>& builder);
    explicit LookupEntry(const LookupEntry& orig) = delete;
    virtual ~LookupEntry() = default;

    void set_entry_text(const Glib::ustring& text);

protected:
    void on_file_find(Glib::RefPtr<Gio::AsyncResult>& result);
    void on_search_changed_event();

    Glib::ustring getParsePath(Glib::RefPtr<Gio::File>& file);

private:
    bool m_blockLookupEvent{false};
    Glib::RefPtr<Gio::File> m_fileLookupPath;
    Glib::ustring m_fileLookupMatch;
    std::vector<Glib::RefPtr<Gio::File>> m_fileLookupMatched;
    Glib::RefPtr<Gio::Cancellable> m_fileLooupCancelabel;
    //sigc::connection m_searchTimer;
};

