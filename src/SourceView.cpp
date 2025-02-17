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
#include "VarselApp.hpp"
#include "PrefDialog.hpp"

/*
 * slightly customized file chooser
 */
class VarselFileChooser
: public Gtk::FileChooserDialog {
public:
    VarselFileChooser(Gtk::Window *win, bool save)
    : Gtk::FileChooserDialog(*win
                            , save
                            ? _("Save File")
                            : _("Open File")
                            , save
                            ? Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SAVE
                            : Gtk::FileChooserAction::FILE_CHOOSER_ACTION_OPEN
                            , Gtk::DIALOG_MODAL | Gtk::DIALOG_DESTROY_WITH_PARENT)
    {
        add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
        add_button(save
                    ? _("_Save")
                    : _("_Open"), Gtk::RESPONSE_ACCEPT);


    }

    virtual ~VarselFileChooser() = default;
protected:
private:
};


SourceFile::SourceFile(VarselApp* application)
: m_application{application}
{
}

void
SourceFile::saveAs(SourceView* win)
{
        //filter->set_name("Text");
    auto  file = selectFile(win, true);
    if (file) {
        bool doSave = true;
        if (file->query_exists()) {
            auto path = file->get_path();
            int ret = win->showMessage(psc::fmt::vformat(_("Replace file {}?"),
                                          psc::fmt::make_format_args(path)), Gtk::MessageType::MESSAGE_QUESTION);
            doSave = ret == Gtk::ResponseType::RESPONSE_YES;
        }
        if (doSave) {
            save(win, file);
        }

    }

}

Glib::RefPtr<Gio::File>
SourceFile::selectFile(SourceView* win, bool save)
{
    VarselFileChooser fileChooser{win, save};
    Glib::RefPtr<Gtk::FileFilter> filter = Gtk::FileFilter::create();
    auto fileInfo = m_eventItem->getFileInfo();
    if (!fileInfo->get_content_type().empty()) {
        filter->add_mime_type(fileInfo->get_content_type());
    }
    Glib::ustring ext = getExtension();
    if (!ext.empty()) {
        filter->add_pattern("*." + ext);
    }
    fileChooser.set_filter(filter);
    fileChooser.set_file(m_eventItem->getFile());
    int ret = fileChooser.run();
    if (ret == Gtk::ResponseType::RESPONSE_ACCEPT) {
        return fileChooser.get_file();
    }
    return Glib::RefPtr<Gio::File>();
}


void
SourceFile::save(SourceView* win)
{
    save(win, m_eventItem->getFile());
}

void
SourceFile::load(SourceView* win)
{
    auto file = selectFile(win, false);
    if (file) {
        checkSave(win);
        load(win, file);
        m_eventItem = std::make_shared<EventItem>(file);
        win->changeLabel(this);
    }
}

bool
SourceFile::checkSave(SourceView* win)
{
    bool changed = gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(m_buffer));
    if (changed) {
        auto file = m_eventItem->getFile();
        auto path = file->get_path();
        int ret = win->showMessage(psc::fmt::vformat(_("Save file {}?"),
                                      psc::fmt::make_format_args(path)), Gtk::MessageType::MESSAGE_QUESTION);
        if (ret == Gtk::ResponseType::RESPONSE_YES) {
            save(win, file);
            return true;
        }
    }
    return false;
}

void
SourceFile::save(SourceView* win, const Glib::RefPtr<Gio::File>& file)
{
    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(m_buffer), &start, &end);
    char* text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(m_buffer), &start, &end, false);
    std::cout << "Got text " << strlen(text) << std::endl;
    try {
        // use no etag
        auto output = file->replace("", true, Gio::FileCreateFlags::FILE_CREATE_REPLACE_DESTINATION);
        output->write(text, strlen(text));
        output->close();
    }
    catch (const Glib::Error& exc) {
        auto path = file->get_path();
        win->showMessage(psc::fmt::vformat(_("Error {} saving {}"),
                            psc::fmt::make_format_args(exc, path)));
    }
    g_free(reinterpret_cast<void*>(text));
}

void
SourceFile::load(SourceView* win, const Glib::RefPtr<Gio::File>& file)
{
    using BUFFER_ARRAY = std::array<gchar, BUF_SIZE>;
    //Glib::ustring cont;
    try {
        auto buff = std::make_unique<BUFFER_ARRAY>();    // better read in steps?
        auto inputStream = file->read();
        GtkTextIter iter;
        gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(m_buffer), &iter);
        gssize len;
        while ((len = inputStream->read(buff.get(), buff->size())) > 0) {
            gtk_text_buffer_insert(GTK_TEXT_BUFFER(m_buffer), &iter, reinterpret_cast<const gchar*>(buff.get()), len);
        }
        inputStream->close();
        //cont = Glib::file_get_contents(file->get_path());
    }
    catch (const Glib::Error& err) {
        auto path = file->get_path();
        win->showMessage(psc::fmt::vformat(_("Error {} loading {}!"),
                              psc::fmt::make_format_args(err, path)));
    }
    gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(m_buffer), false); // as no modification has occurred yet
}

Glib::ustring
SourceFile::getExtension()
{
    Glib::ustring ext;
    auto dispName = m_eventItem->getFileInfo()->get_display_name();
    auto pos = dispName.find_last_of('.');
    if (pos != dispName.npos) {
        ++pos;
        ext = dispName.substr(pos);
    }
    return ext;
}

Gtk::Widget*
SourceFile::buildSourceView(SourceView* win, std::shared_ptr<EventItem>& eventItem)
{
    m_eventItem = eventItem;
    auto scrollView = Gtk::make_managed<Gtk::ScrolledWindow>();
    auto file = eventItem->getFile();
    // no free langManager,srcLang
    auto srcLang = getSourceLanguage(eventItem);
    if (srcLang) {
        m_buffer = gtk_source_buffer_new_with_language(srcLang);
    }
    else {
        m_buffer = gtk_source_buffer_new(nullptr);
    }
    gtk_source_buffer_set_highlight_syntax(m_buffer, true);
    m_sourceView = GTK_SOURCE_VIEW(gtk_source_view_new_with_buffer(m_buffer));
    gtk_source_view_set_show_line_numbers(m_sourceView, true);
    //gtk_source_view_set_show_line_marks(m_sourceView, true);

    load(win, file);
    gtk_container_add(GTK_CONTAINER(scrollView->gobj()), GTK_WIDGET(m_sourceView));
    return scrollView;
}

GtkSourceLanguage*
SourceFile::getSourceLanguage(const std::shared_ptr<EventItem>& item)
{
    GtkSourceLanguage* srcLang{nullptr};
    auto file = item->getFile();
    auto info = item->getFileInfo();
    if (info->get_file_type() == Gio::FileType::FILE_TYPE_REGULAR) {
        auto content = info->get_content_type();
        auto base = file->get_basename();
        auto langManager = gtk_source_language_manager_get_default();
        srcLang = gtk_source_language_manager_guess_language(langManager, base.c_str(), content.c_str());
    }
    return srcLang;
}

void
SourceFile::applyStyle(const std::string& style, const std::string& fontDesc)
{
    // no free styleManager,scheme
    auto styleManager = gtk_source_style_scheme_manager_get_default();
    auto scheme = gtk_source_style_scheme_manager_get_scheme(styleManager, style.c_str());   //
    if (scheme && !style.empty()) {
        gtk_source_buffer_set_style_scheme(m_buffer, scheme);
    }
    if (!fontDesc.empty()) {
        // does not work
        //auto context = gtk_widget_get_pango_context(GTK_WIDGET(m_sourceView));
        //pango_context_set_font_description(context, desc.gobj());
        Pango::FontDescription desc = Pango::FontDescription(fontDesc);
        auto provider = Gtk::CssProvider::create();
        Glib::ustring size;
        if (desc.get_set_fields() & Pango::FontMask::FONT_MASK_SIZE) {
            size.reserve(16);
            size += std::to_string(static_cast<int>((float)desc.get_size() / 1024.0f));
            size += desc.get_size_is_absolute() ? "px" : "pt";
        }
        auto css = Glib::ustring::sprintf("#cssView {\n"
          "  font: %s \"%s\", Monospace;\n"
          "}", size, desc.get_family());
        //std::cout << "SourceFile::applyStyle"
        //          << fontDesc
        //          << std::hex << " fld " << desc.get_set_fields() << std::dec
        //          << " fam " << desc.get_family()
        //          << " siz " << (float)desc.get_size() / 1024.0f
        //          << " var " << desc.get_variant()
        //          << " wei " << desc.get_weight()
        //          << " sty " << desc.get_style()
        //          << " css " << css << std::endl;
        provider->load_from_data(css);
        gtk_widget_set_name(GTK_WIDGET(m_sourceView), "cssView");
        GtkStyleContext* context = gtk_widget_get_style_context(GTK_WIDGET(m_sourceView));
        gtk_style_context_add_provider(context,
                                 GTK_STYLE_PROVIDER(provider->gobj()),
                                 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);


    }

}

Glib::ustring
SourceFile::getLabel()
{
    auto fileInfo = m_eventItem->getFileInfo();
    return fileInfo->get_display_name();
}


SourceView::SourceView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder, VarselApp* varselApp, const std::vector<std::shared_ptr<EventItem>>& matchingFiles)
: Gtk::Window(cobject)
, m_application{varselApp}
{
    set_title(_("Source"));
    auto pix = Gdk::Pixbuf::create_from_resource(varselApp->get_resource_base_path() + "/varsel.png");
    set_icon(pix);

    Gtk::MenuBar* menuBar;
    refBuilder->get_widget("menuBar", menuBar);
    createMenu(menuBar);
    refBuilder->get_widget("notebook", m_notebook);
    for (size_t n = 0; n < matchingFiles.size(); ++n) {
        auto item = matchingFiles[n];
        auto sourceFile = std::make_shared<SourceFile>(m_application);
        auto widget = sourceFile->buildSourceView(this, item);
        m_notebook->append_page(*widget, sourceFile->getLabel());
        m_files.emplace_back(std::move(sourceFile));
    }
    auto settings = m_application->getKeyFile();
    Glib::ustring fontDesc;
    if (settings->hasKey(VarselWin::CONFIG_GRP, PrefDialog::DEFAULT_FONT)) {
        if (!settings->getBoolean(VarselWin::CONFIG_GRP, PrefDialog::DEFAULT_FONT, true)
         && settings->hasKey(VarselWin::CONFIG_GRP, PrefDialog::CONFIG_FONT)) {
            fontDesc = settings->getString(VarselWin::CONFIG_GRP, PrefDialog::CONFIG_FONT);
        }
    }
    Glib::ustring style;
    if (settings->hasKey(VarselWin::CONFIG_GRP, PrefDialog::SOURCE_STYLE)) {
        style = settings->getString(VarselWin::CONFIG_GRP, PrefDialog::SOURCE_STYLE);
    }
    applyStyle(style, fontDesc);
    set_default_size(640, 480);
}

void
SourceView::createMenu(Gtk::MenuBar* gtkMenuBar)
{
    auto refActionGroup = Gio::SimpleActionGroup::create();
    auto saveAction = Gio::SimpleAction::create(SAVE_ACTION);
    refActionGroup->add_action(saveAction);
    saveAction->signal_activate().connect(sigc::mem_fun(*this, &SourceView::save));
    auto saveAsAction = Gio::SimpleAction::create(SAVEAS_ACTION);
    refActionGroup->add_action(saveAsAction);
    saveAsAction->signal_activate().connect(sigc::mem_fun(*this, &SourceView::saveAs));
    auto loadAction = Gio::SimpleAction::create(LOAD_ACTION);
    refActionGroup->add_action(loadAction);
    loadAction->signal_activate().connect(sigc::mem_fun(*this, &SourceView::load));
    auto closeAction = Gio::SimpleAction::create(CLOSE_ACTION);
    refActionGroup->add_action(closeAction);
    closeAction->signal_activate().connect(sigc::mem_fun(*this, &SourceView::close));
    auto quitAction = Gio::SimpleAction::create(QUIT_ACTION);
    refActionGroup->add_action(quitAction);
    quitAction->signal_activate().connect(sigc::mem_fun(*this, &SourceView::quit));
    insert_action_group(ACTION_GROUP, refActionGroup);
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
SourceView::save(const Glib::VariantBase& val)
{
    int page = m_notebook->get_current_page();
    auto srcFile = m_files[page];
    std::cout << "SourceView::save"
              << " page " << page
              << " src " << srcFile->getLabel()  << std::endl;
    srcFile->save(this);
}

void
SourceView::saveAs(const Glib::VariantBase& val)
{
    int page = m_notebook->get_current_page();
    auto srcFile = m_files[page];
    std::cout << "SourceView::saveAs"
              << " page " << page
              << " src " << srcFile->getLabel()  << std::endl;
    srcFile->saveAs(this);
}

void
SourceView::load(const Glib::VariantBase& val)
{
    std::cout << "SourceView::load" << std::endl;
    int page = m_notebook->get_current_page();
    auto srcFile = m_files[page];
    srcFile->load(this);
}

void
SourceView::close(const Glib::VariantBase& val)
{
    std::cout << "SourceView::close" << std::endl;
    int page = m_notebook->get_current_page();
    auto srcFile = m_files[page];
    srcFile->checkSave(this);
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


SourceFactory::SourceFactory(VarselApp* varselApp)
: EventBusListener::EventBusListener()
, m_varselApp{varselApp}
{
}

void
SourceFactory::notify(const std::shared_ptr<BusEvent>& busEvent)
{
    auto openEvent = std::dynamic_pointer_cast<OpenEvent>(busEvent);
    if (openEvent) {
        std::vector<std::shared_ptr<EventItem>> matchingFiles;
        matchingFiles.reserve(16);
        for (auto item : openEvent->getFiles()) {
            GtkSourceLanguage* srcLang = SourceFile::getSourceLanguage(item);
            if (srcLang) {
                matchingFiles.push_back(item);
                openEvent->remove(item);
            }
        }
        if (matchingFiles.size() > 0) {
            createSourceWindow(matchingFiles);
        }
    }
}


SourceView*
SourceFactory::createSourceWindow(const std::vector<std::shared_ptr<EventItem>>& matchingFiles)
{
    SourceView* sourceView{nullptr};
    auto builder = Gtk::Builder::create();
    try {
        builder->add_from_resource(m_varselApp->get_resource_base_path() + "/varsel-srcwin.ui");
        builder->get_widget_derived("SourceView", sourceView, m_varselApp, matchingFiles);
        m_varselApp->add_window(*sourceView);      // do this in this order to ensures the menu-bar comes up...
        sourceView->show_all();
    }
    catch (const Glib::Error &ex) {
        std::cerr << "Unable to load sourceView " << ex.what() << std::endl;
    }
    return sourceView;
}
