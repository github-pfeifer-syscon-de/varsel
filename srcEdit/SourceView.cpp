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

#include <iostream>
#include <psc_i18n.hpp>
#include <Log.hpp>

#include "SourceView.hpp"
#include "EditApp.hpp"
#include "PrefSourceView.hpp"



SourceDocumentRef::SourceDocumentRef(const Glib::RefPtr<Gio::File>& file, const TextPos& pos, SourceView* sourceView, const Glib::ustring& method)
: CclsDocumentRef::CclsDocumentRef(file, pos, method)
, m_sourceView{sourceView}
{
}

void
SourceDocumentRef::result(const std::shared_ptr<psc::json::JsonValue>& json)
{
    if (json->isArray()) {
        auto arr = json->getArray();
        if (arr->getSize() > 0) {
            auto val = arr->get(0);
            auto obj = val->getObject();
            auto uri = obj->getValue("uri");
            auto rangeVal = obj->getValue("range");
            auto range = rangeVal->getObject();
            auto startVal = range->getValue("start");
            auto start = startVal->getObject();
            auto line = start->getValue("line");
            auto character = start->getValue("character");
            Glib::RefPtr<Gio::File> file = Gio::File::create_for_uri(uri->getString());
            TextPos pos{
                static_cast<int>(line->getInt()),
                static_cast<int>(character->getInt())};
            m_sourceView->gotoFilePos(file, pos);
            return;
        }
    }
    std::cout << "Empty/unknown structure for definition!" << std::endl;
    // popup ?
}

SourceView::SourceView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder, EditApp* varselApp)
: Gtk::ApplicationWindow(cobject)
, m_application{varselApp}
{
    set_title(_("Source"));
    auto pix = Gdk::Pixbuf::create_from_resource(varselApp->get_resource_base_path() + "/varsel.png");
    set_icon(pix);
    createActions();
    refBuilder->get_widget("notebook", m_notebook);
    set_default_size(640, 480);
}

std::shared_ptr<SourceFile>
SourceView::addFile(const Glib::RefPtr<Gio::File>& item)
{
    auto sourceFile = std::make_shared<SourceFile>(this);
    auto widget = sourceFile->buildSourceView(item);
    m_notebook->append_page(*widget, sourceFile->getLabel());

    auto settings = m_application->getKeyFile();
    Glib::ustring fontDesc;
    if (!settings->getBoolean(PrefSourceView::CONFIG_SRCVIEW_GRP, PrefSourceView::DEFAULT_FONT, true)
     && settings->hasKey(PrefSourceView::CONFIG_SRCVIEW_GRP, PrefSourceView::CONFIG_FONT)) {
        fontDesc = settings->getString(PrefSourceView::CONFIG_SRCVIEW_GRP, PrefSourceView::CONFIG_FONT);
    }
    Glib::ustring style;
    if (settings->hasKey(PrefSourceView::CONFIG_SRCVIEW_GRP, PrefSourceView::SOURCE_STYLE)) {
        style = settings->getString(PrefSourceView::CONFIG_SRCVIEW_GRP, PrefSourceView::SOURCE_STYLE);
    }
    sourceFile->applyStyle(style, fontDesc);

    widget->show_all();
    // create these early as init takes some time
    auto language = mapLanguage(sourceFile->getLanguage());
    if (!language.empty()) {
        auto dir = item->get_parent();
        auto dotccls = dir->get_child(".ccls");
        if (dotccls->query_exists()) {
            // use only only one language server per source dir
            auto langServIt = m_ccLangServers.find(dir->get_path());
            if (langServIt == m_ccLangServers.end()) {
                auto ccLangServer = std::make_shared<CcLangServer>();
                auto init = std::make_shared<CclsInit>(dir);
                ccLangServer->communicate(init);
                m_ccLangServers.insert(std::make_pair(dir->get_path(), ccLangServer));
            }
        }
    }
    m_files.push_back(sourceFile);
    return sourceFile;
}

std::shared_ptr<SourceFile>
SourceView::findView(const Glib::RefPtr<Gio::File>& file)
{
    std::shared_ptr<SourceFile> src;
    for (auto& view : m_files) {
        if (view->getFile()->get_path() == file->get_path()) {
            src = view;
            break;
        }
    }
    return src;
}

int
SourceView::getIndex(const std::shared_ptr<SourceFile>& view)
{
    for (size_t i = 0; i < m_files.size(); ++i) {
        auto v = m_files[i];
        if (view == v) {
            return i;
        }
    }
    return 0;
}


void
SourceView::showFiles(const std::vector<Glib::RefPtr<Gio::File>>& matchingFiles)
{
    for (size_t n = 0; n < matchingFiles.size(); ++n) {
        auto item = matchingFiles[n];
        addFile(item);
    }
    //applyStyle(style, fontDesc);
}

void
SourceView::createActions()
{
    auto newAction = Gio::SimpleAction::create("new");
    add_action(newAction);
    newAction->signal_activate().connect(sigc::mem_fun(*this, &SourceView::newfile));
    auto saveAction = Gio::SimpleAction::create("save");
    add_action(saveAction);
    saveAction->signal_activate().connect(sigc::mem_fun(*this, &SourceView::save));
    auto saveAsAction = Gio::SimpleAction::create("saveAs");
    add_action(saveAsAction);
    saveAsAction->signal_activate().connect(sigc::mem_fun(*this, &SourceView::saveAs));
    auto loadAction = Gio::SimpleAction::create("load");
    add_action(loadAction);
    loadAction->signal_activate().connect(sigc::mem_fun(*this, &SourceView::load));
    auto closeAction = Gio::SimpleAction::create("close");
    add_action(closeAction);
    closeAction->signal_activate().connect(sigc::mem_fun(*this, &SourceView::close));
    //auto quitAction = Gio::SimpleAction::create(QUIT_ACTION);
    //refActionGroup->add_action(quitAction);
    //quitAction->signal_activate().connect(sigc::mem_fun(*this, &SourceView::quit));
    auto pref_action = Gio::SimpleAction::create("config");
    pref_action->signal_activate().connect(
        [this]  (const Glib::VariantBase& value)
		{
			try {
				auto builder = Gtk::Builder::create();
				PrefSourceView* prefDialog;
				builder->add_from_resource(m_application->get_resource_base_path() + "/pref-srcView.ui");
				builder->get_widget_derived("PrefDialog", prefDialog, this);
				prefDialog->run();
				delete prefDialog;  // as this is a toplevel component shoud destroy -> works
			}
			catch (const Glib::Error &ex) {
                showMessage(psc::fmt::vformat(
                        _("Unable to load {} error {}"),
                          psc::fmt::make_format_args("pref-srcView", ex)), Gtk::MessageType::MESSAGE_WARNING);
			}
		});
    add_action(pref_action);
}

void
SourceView::changeLabel(SourceFile* sourceFile)
{
    for (int n = 0; n < m_notebook->get_n_pages(); ++n) {
        auto& srcFile = m_files[n];
        if (srcFile.get() == sourceFile) {
            m_notebook->set_tab_label_text(*m_notebook->get_nth_page(n), srcFile->getLabel());
            break;
        }
    }
}


void
SourceView::applyStyle(const std::string& style, const std::string& fontDesc)
{
    for (auto iter = m_files.begin(); iter != m_files.end(); ++iter) {
        auto& file = *iter;
        file->applyStyle(style, fontDesc);
    }
}


void
SourceView::newfile(const Glib::VariantBase& val)
{
    Glib::RefPtr<Gio::File> file;
    addFile(file);
}


void
SourceView::save(const Glib::VariantBase& val)
{
    int page = m_notebook->get_current_page();
    auto srcFile = m_files[page];
    std::cout << "SourceView::save"
              << " page " << page
              << " src " << srcFile->getLabel()  << std::endl;
    srcFile->save();
}

void
SourceView::saveAs(const Glib::VariantBase& val)
{
    int page = m_notebook->get_current_page();
    auto srcFile = m_files[page];
    std::cout << "SourceView::saveAs"
              << " page " << page
              << " src " << srcFile->getLabel()  << std::endl;
    srcFile->saveAs();
}

void
SourceView::load(const Glib::VariantBase& val)
{
    std::cout << "SourceView::load" << std::endl;
    int page = m_notebook->get_current_page();
    auto srcFile = m_files[page];
    srcFile->load();
}

void
SourceView::close(const Glib::VariantBase& val)
{
    std::cout << "SourceView::close" << std::endl;
    int page = m_notebook->get_current_page();
    auto srcFile = m_files[page];
    srcFile->checkSave();
    m_notebook->remove_page(page);
    m_files.erase(m_files.begin() + page);
    if (m_files.empty()) {
        quit(val);
    }
}


void
SourceView::quit(const Glib::VariantBase& val)
{
    std::cout << "SourceView::quit" << std::endl;
    hide();
}

// Convert the language used
//   by source view to language server names
//   at the moment only c++ is supported but
//   if you are bold you may add more ...
Glib::ustring
SourceView::mapLanguage(const Glib::ustring& viewLanguage)
{
    if (StringUtils::startsWith(viewLanguage, "C++")) {  // "C++" "C++ Header"
        return "cpp";
    }
    return "";
}

void
SourceView::showDocumentRef(SourceFile* sourceFile, const TextPos& pos, const Glib::ustring& method)
{
    auto file = sourceFile->getFile();
    auto language = mapLanguage(sourceFile->getLanguage());
    if (!language.empty()) {
        auto dir = file->get_parent();
        auto dotccls = dir->get_child(".ccls");
        if (!dotccls->query_exists()) {
            auto path = dir->get_path();
            showMessage(psc::fmt::vformat(_("To use language functions the file .ccls is required in {} and a ccls install")
                                            , psc::fmt::make_format_args(path)));
            return; // no lookup possible
        }
        std::cout << "SourceView::showDocumentRef"
                  << " method " << method
                  << " file " << file->get_path()
                  << " pos " << pos.line << ", " << pos.character << std::endl;
        auto ccLangServIt = m_ccLangServers.find(dir->get_path());
        if (ccLangServIt != m_ccLangServers.end()) {
            auto ccLangServer = (*ccLangServIt).second;
           // it might be more appropriate to do this when showing/changing tab
            auto open = std::make_shared<CclsOpen>(file, language, 0);
            ccLangServer->communicate(open);

            auto lookup = std::make_shared<SourceDocumentRef>(file, pos, this, method);
            ccLangServer->communicate(lookup);

            auto close = std::make_shared<CclsClose>(file);
            ccLangServer->communicate(close);
        }
    }
    else {
        psc::log::Log::logAdd(psc::log::Level::Debug, psc::fmt::format("No definition for src {} lang {}", file->get_path(), language));
    }
}

void
SourceView::gotoFilePos(const Glib::RefPtr<Gio::File>& file, const TextPos& pos)
{
    std::cout << "SourceView::gotDefinition"
              << " file " << file->get_path()
              << " pos " << pos.line << ", " << pos.character << std::endl;
    auto view = findView(file);
    if (!view) {
        view = addFile(file);
    }
    m_notebook->set_current_page(getIndex(view));
    view->show(pos);
}



int
SourceView::showMessage(const Glib::ustring& msg, Gtk::MessageType msgType)
{
    Gtk::ButtonsType buttons = Gtk::ButtonsType::BUTTONS_OK;
    if (msgType == Gtk::MessageType::MESSAGE_QUESTION) {
        buttons = Gtk::ButtonsType::BUTTONS_YES_NO;
    }
    Gtk::MessageDialog messagedialog(*this, msg, false, msgType, buttons);
    messagedialog.set_transient_for(*this);
    int ret = messagedialog.run();
    messagedialog.hide();
    return ret;
}

