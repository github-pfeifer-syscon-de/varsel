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

#include <gtksourceview/gtksource.h>
#include <gtkmm.h>
#include <memory>

#include "EventBus.hpp"


class SourceView;

class SourceFile
{
public:
    SourceFile(SourceView* sourceWin);
    virtual ~SourceFile() = default;

    Gtk::Widget* buildSourceView(const Glib::RefPtr<Gio::File>& eventItem);
    void applyStyle(const std::string& style, const std::string& fontDesc);
    Glib::ustring getLabel();
    static GtkSourceLanguage* getSourceLanguage(const std::shared_ptr<EventItem>& item);
    void saveAs();
    void save();
    void load();
    bool checkSave();
    TextPos getPosition();
    void showDocumentRef(const TextPos& pos, const Glib::ustring& method);
    Glib::ustring getLanguage();
    Glib::RefPtr<Gio::File> getFile();
    Glib::ustring getText();
    void show(const TextPos& pos);
    Gtk::Widget* getWidget();

    static constexpr size_t BUF_SIZE{8u*1024u};

protected:
    void save(const Glib::RefPtr<Gio::File>& file);
    void load(const Glib::RefPtr<Gio::File>& file);
    Glib::RefPtr<Gio::File> selectFile(bool save);

    Glib::ustring getExtension();
private:
    SourceView* m_sourceWin;
    GtkSourceBuffer* m_buffer{nullptr};
    GtkSourceView* m_sourceView{nullptr};
    std::shared_ptr<EventItem> m_eventItem;
    Glib::RefPtr<Gtk::CssProvider> m_provider;
    Glib::ustring m_language;
    Gtk::ScrolledWindow* m_scrollView{nullptr};
};

