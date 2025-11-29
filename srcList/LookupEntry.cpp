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

#include <iostream>

#include "LookupEntry.hpp"

LookupEntry::LookupEntry(BaseObjectType* cobject
        , const Glib::RefPtr<Gtk::Builder>& builder)
: Gtk::SearchEntry(cobject)
{
    signal_search_changed().connect(
        sigc::mem_fun(*this, &LookupEntry::on_search_changed_event));
//        [this] {
//            if (!m_searchTimer.connected()) {
//                m_searchTimer.disconnect();     // cancel previous timer
//            }
//            m_searchTimer = Glib::signal_timeout().connect(
//                sigc::mem_fun(*this, &VarselList::on_search_changed_event)
//                , 200);
//        });

}

void
LookupEntry::set_entry_text(const Glib::ustring& text)
{
    m_blockLookupEvent = true;
    set_text(text);
    m_blockLookupEvent = false;
}

Glib::ustring
LookupEntry::getParsePath(Glib::RefPtr<Gio::File>& file)
{
    Glib::ustring ret;
    ret.reserve(64);
    auto next = file;
    while (next->get_path().size() > 2) {   // don't contine for e.g. c:
        ret.insert(0, next->get_parse_name());
        ret.insert(0, "/");
        next = next->get_parent();
    }
    return ret;
}

void
LookupEntry::on_file_find(Glib::RefPtr<Gio::AsyncResult>& result)
{
    Glib::RefPtr<Gio::FileEnumerator> en = m_fileLookupPath->enumerate_children_finish(result);
    while (true) {
        auto fileInfo = en->next_file();
        if (!fileInfo) {
            m_fileLooupCancelabel.reset();
            if (m_fileLookupMatched.size() == 1) {  // care only if matched
                auto matchedFile = m_fileLookupMatched[0];
                set_entry_text(getParsePath(matchedFile));
            }
            break;
        }
        auto file = m_fileLookupPath->get_child(fileInfo->get_name());
        if (file->get_parse_name().rfind(m_fileLookupMatch, 0) == 0) {  // ~startsWith
            m_fileLookupMatched.push_back(file);    // collect candidates
        }

    }
    en->close();
}

void
LookupEntry::on_search_changed_event()
{
    if (m_blockLookupEvent) {
        return;
    }
    const auto path = get_text();
    std::cout << "LookupEntry::on_search_changed_event " << path << std::endl;

    int pos = path.rfind('/');
    if (pos > 0 && path.length() - pos > 3) {
        if (m_fileLooupCancelabel) {
            m_fileLooupCancelabel->cancel();
        }
        m_fileLooupCancelabel = Gio::Cancellable::create();
        m_fileLookupPath = Gio::File::create_for_parse_name(path.substr(0, pos));
        m_fileLookupMatch = path.substr(pos + 1);
        m_fileLookupMatched.clear();
        m_fileLookupMatched.reserve(8);
        m_fileLookupPath->enumerate_children_async(
            sigc::mem_fun(*this, &LookupEntry::on_file_find)
            , m_fileLooupCancelabel);
    }
}
