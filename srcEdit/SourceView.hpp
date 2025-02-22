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

#pragma once

#include <gtksourceview/gtksource.h>
#include <gtkmm.h>
#include <vector>
#include <memory>

#include "EventBus.hpp"


class EditApp;
class SourceView;

class SourceFile
{
public:
    SourceFile(EditApp* application);
    virtual ~SourceFile() = default;

    Gtk::Widget* buildSourceView(SourceView* win, const Glib::RefPtr<Gio::File>& eventItem);
    void applyStyle(const std::string& style, const std::string& fontDesc);
    Glib::ustring getLabel();
    static GtkSourceLanguage* getSourceLanguage(const std::shared_ptr<EventItem>& item);
    void saveAs(SourceView* win);
    void save(SourceView *win);
    void load(SourceView *win);
    bool checkSave(SourceView* win);

    static constexpr size_t BUF_SIZE{8u*1024u};

protected:
    void save(SourceView* win, const Glib::RefPtr<Gio::File>& file);
    void load(SourceView* win, const Glib::RefPtr<Gio::File>& file);
    Glib::RefPtr<Gio::File> selectFile(SourceView* win, bool save);

    Glib::ustring getExtension();
private:
    EditApp* m_application;
    GtkSourceBuffer* m_buffer{nullptr};
    GtkSourceView* m_sourceView{nullptr};
    std::shared_ptr<EventItem> m_eventItem;
    Glib::RefPtr<Gtk::CssProvider> m_provider;
};

class SourceView
: public Gtk::ApplicationWindow
{
public:
    SourceView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder, EditApp* varselApp);
    explicit SourceView(const SourceView& orig) = delete;
    virtual ~SourceView() = default;

    void applyStyle(const std::string& style, const std::string& fontDesc);
    void newfile(const Glib::VariantBase& val);
    void save(const Glib::VariantBase& val);
    void saveAs(const Glib::VariantBase& val);
    void load(const Glib::VariantBase& val);
    void close(const Glib::VariantBase& val);
    void quit(const Glib::VariantBase& val);
    int showMessage(const Glib::ustring& msg, Gtk::MessageType msgType = Gtk::MessageType::MESSAGE_WARNING);
    void changeLabel(SourceFile* sourceFile);
    EditApp* getApplication()
    {
        return m_application;
    }
    void showFiles(const std::vector<Glib::RefPtr<Gio::File>>& matchingFiles);


protected:
    void createActions();
    void addFile(const Glib::RefPtr<Gio::File>& item);


private:
    EditApp* m_application;
    Gtk::Notebook* m_notebook{nullptr};
    std::vector<std::shared_ptr<SourceFile>> m_files;
};


