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

#include <gtkmm.h>
#include <vector>
#include <map>
#include <memory>

#include "CCLangServer.hpp"
#include "EventBus.hpp"
#include "SourceFile.hpp"

class EditApp;
class SourceView;
class SourceFile;

class SourceDocumentRef
: public CclsDocumentRef
{
public:
    SourceDocumentRef(const Glib::RefPtr<Gio::File>& file, const TextPos& pos, SourceView* sourceView, const Glib::ustring& method);
    explicit SourceDocumentRef(const SourceDocumentRef& orig) = delete;
    virtual ~SourceDocumentRef() = default;

    void result(const std::shared_ptr<psc::json::JsonValue>& json) override;

private:
    SourceView* m_sourceView;
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
    void showDocumentRef(SourceFile* sourceFile, const TextPos& pos, const Glib::ustring& method);
    void gotoFilePos(const Glib::RefPtr<Gio::File>& file, const TextPos& pos);
    Glib::ustring mapLanguage(const Glib::ustring& viewLanguage);

protected:
    void createActions();
    std::shared_ptr<SourceFile> addFile(const Glib::RefPtr<Gio::File>& item);
    std::shared_ptr<SourceFile> findView(const Glib::RefPtr<Gio::File>& item);
    int getIndex(const std::shared_ptr<SourceFile>& view);
private:
    EditApp* m_application;
    Gtk::Notebook* m_notebook{nullptr};
    std::vector<std::shared_ptr<SourceFile>> m_files;
    std::map<std::string, PtrCcLangServer> m_ccLangServers;
};


