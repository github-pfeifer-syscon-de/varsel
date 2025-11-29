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
#include <psc_i18n.hpp>
#include <Log.hpp>

#include "LspServer.hpp"
#include "SourceView.hpp"
#include "SourceFile.hpp"

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


static bool
editorKeyEvent(GtkWidget* widget, GdkEventKey* key, gpointer user_data)
{
    auto sourceFile = static_cast<SourceFile*>(user_data);
//    std::cout << "editorKeyEvent"
//      << " key " << key->keyval
//      << " type " << key->type
//      << " shift " << std::boolalpha << (key->state & GDK_SHIFT_MASK)
//      << " cntl " << std::boolalpha << (key->state & GDK_CONTROL_MASK)
//      << " alt " << std::boolalpha << (key->state & GDK_MOD1_MASK) << std::endl;
    auto uppper = g_unichar_toupper(key->keyval);
    if ((uppper == GDK_KEY_D)
     && key->type == GDK_KEY_RELEASE
     && ((key->state & GDK_CONTROL_MASK) != 0)
     && ((key->state & GDK_MOD1_MASK) == 0) ) {   // represents alt
        auto pos = sourceFile->getPosition();
        std::cout << "editorKeyEvent"
          << " line " << pos.getStartLine()
          << " char " << pos.getStartCharacter() << std::endl;
        Glib::ustring method;
        if ((key->state & GDK_SHIFT_MASK) == 0) {
            method = SourceDocumentRef::DEFININION_METHOD;
        }
        else {
            method = SourceDocumentRef::DECLATATION_METHOD;
        }
        sourceFile->showDocumentRef(pos, method);
        return true;    // steal
    }
    return false;   // continue processing
}

SourceFile::SourceFile(SourceView* sourceWin)
: m_sourceWin{sourceWin}
{
}

void
SourceFile::saveAs()
{
        //filter->set_name("Text");
    auto  file = selectFile(true);
    if (file) {
        bool doSave = true;
        if (file->query_exists()) {
            auto path = file->get_path();
            int ret = m_sourceWin->showMessage(psc::fmt::vformat(_("Replace file {}?"),
                                          psc::fmt::make_format_args(path)), Gtk::MessageType::MESSAGE_QUESTION);
            doSave = ret == Gtk::ResponseType::RESPONSE_YES;
        }
        if (doSave) {
            save(file);
        }

    }

}

Glib::RefPtr<Gio::File>
SourceFile::selectFile(bool save)
{
    VarselFileChooser fileChooser{m_sourceWin, save};
    if (save) {     // only use filter on save-as
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
    }
    else {
        if (m_eventItem && m_eventItem->getFile()) {       // if possible use directory
            fileChooser.set_file(m_eventItem->getFile()->get_parent());
        }
    }
    int ret = fileChooser.run();
    if (ret == Gtk::ResponseType::RESPONSE_ACCEPT) {
        return fileChooser.get_file();
    }
    return Glib::RefPtr<Gio::File>();
}


void
SourceFile::save()
{
    save(m_eventItem->getFile());
}

void
SourceFile::load()
{
    auto file = selectFile(false);
    if (file) {
        checkSave();
        load(file);
        m_eventItem = std::make_shared<EventItem>(file);
        m_sourceWin->changeLabel(this);
    }
}

bool
SourceFile::checkSave()
{
    bool changed = gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(m_buffer));
    if (changed) {
        auto file = m_eventItem->getFile();
        auto path = file->get_path();
        int ret = m_sourceWin->showMessage(psc::fmt::vformat(_("Save file {}?"),
                                      psc::fmt::make_format_args(path)), Gtk::MessageType::MESSAGE_QUESTION);
        if (ret == Gtk::ResponseType::RESPONSE_YES) {
            save(file);
            return true;
        }
    }
    return false;
}

void
SourceFile::save(const Glib::RefPtr<Gio::File>& file)
{
    auto text = getText();
    try {
        // use no etag
        auto output = file->replace("", true, Gio::FileCreateFlags::FILE_CREATE_REPLACE_DESTINATION);
        output->write(text);
        output->close();
    }
    catch (const Glib::Error& exc) {
        auto path = file->get_path();
        m_sourceWin->showMessage(psc::fmt::vformat(_("Error {} saving {}"),
                            psc::fmt::make_format_args(exc, path)));
    }
}

void
SourceFile::load(const Glib::RefPtr<Gio::File>& file)
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
        m_sourceWin->showMessage(psc::fmt::vformat(_("Error {} loading {}!"),
                              psc::fmt::make_format_args(err, path)));
    }
    gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(m_buffer), false); // as no modification has occurred yet
}

Glib::ustring
SourceFile::getExtension()
{
    Glib::ustring ext = StringUtils::getExtension(m_eventItem->getFile());
    return ext;
}

LspLocation
SourceFile::getPosition()
{
    GtkTextMark* mark = gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(m_buffer));
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(GTK_TEXT_BUFFER(m_buffer), &iter, mark);
    auto line = gtk_text_iter_get_line(&iter);
    auto characters = gtk_text_iter_get_line_offset(&iter);
    LspLocation loc{static_cast<uint32_t>(line), static_cast<uint32_t>(characters)};
    loc.setUri(getFile());
    return loc;
}

void
SourceFile::showDocumentRef(const LspLocation& pos, const Glib::ustring& method)
{
    m_sourceWin->showDocumentRef(this, pos, method);
}

Glib::RefPtr<Gio::File>
SourceFile::getFile()
{
    return m_eventItem->getFile();
}

Glib::ustring
SourceFile::getText()
{
    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(m_buffer), &start, &end);
    char* text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(m_buffer), &start, &end, false);
    //std::cout << "Got text " << strlen(text) << std::endl;
    Glib::ustring utext{text};
    g_free(reinterpret_cast<void*>(text));
    return utext;
}

Gtk::Widget*
SourceFile::getWidget()
{
    return m_scrollView;
}

Glib::ustring
SourceFile::getLanguage()
{
    return m_language;
}

void
SourceFile::show(const LspLocation& pos)
{
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line_offset(GTK_TEXT_BUFFER(m_buffer), &iter, pos.getStartLine(), pos.getStartCharacter());
    gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(m_buffer), &iter);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(m_sourceView), &iter, 0, true, 0.5, 0.5);
}

Gtk::Widget*
SourceFile::buildSourceView(const Glib::RefPtr<Gio::File>& file)
{
    m_scrollView = Gtk::make_managed<Gtk::ScrolledWindow>();
    GtkSourceLanguage* srcLang{nullptr};
    if (file) {
        m_eventItem = std::make_shared<EventItem>(file);
        // no free langManager,srcLang
        srcLang = getSourceLanguage(m_eventItem);
    }
    if (srcLang) {
        m_buffer = gtk_source_buffer_new_with_language(srcLang);
        m_language = gtk_source_language_get_name(srcLang);
    }
    else {
        m_buffer = gtk_source_buffer_new(nullptr);
    }
    if (file) {
        load(file);
    }
    gtk_source_buffer_set_highlight_syntax(m_buffer, true);
    m_sourceView = GTK_SOURCE_VIEW(gtk_source_view_new_with_buffer(m_buffer));
    gtk_source_view_set_show_line_numbers(m_sourceView, true);
    //gtk_source_view_set_show_line_marks(m_sourceView, true);
    //m_defaultFont = pango_font_description_copy(gtk_source_view_get_font(m_sourceView));

    gtk_widget_add_events(GTK_WIDGET(m_sourceView), GDK_KEY_RELEASE_MASK);
    //g_signal_connect(GTK_WIDGET(m_sourceView)
    //                , "key_press_event"
    //                , GCallback(&(editorKeyEvent))
    //                , this);
    g_signal_connect(GTK_WIDGET(m_sourceView)
                    , "key_release_event"
                    , GCallback(&(editorKeyEvent))
                    , this);

    gtk_container_add(GTK_CONTAINER(m_scrollView->gobj()), GTK_WIDGET(m_sourceView));
    return m_scrollView;
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
    GtkStyleContext* context = gtk_widget_get_style_context(GTK_WIDGET(m_sourceView));
    if (m_provider) {   // remove previous style
        gtk_style_context_remove_provider(context,
                                 GTK_STYLE_PROVIDER(m_provider->gobj()));
        m_provider.reset();
    }
    if (!fontDesc.empty()) {
        m_provider = Gtk::CssProvider::create();
        // does not work
        //auto context = gtk_widget_get_pango_context(GTK_WIDGET(m_sourceView));
        //pango_context_set_font_description(context, desc.gobj());
        Pango::FontDescription desc = Pango::FontDescription(fontDesc);
        Glib::ustring size;
        if (desc.get_set_fields() & Pango::FontMask::FONT_MASK_SIZE) {
            size.reserve(16);
            size += std::to_string(static_cast<int>((float)desc.get_size() / 1024.0f));
            size += desc.get_size_is_absolute() ? "px" : "pt";
        }
        Glib::ustring css = Glib::ustring::sprintf("#cssView {\n"
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
        m_provider->load_from_data(css);
        gtk_widget_set_name(GTK_WIDGET(m_sourceView), "cssView");
        gtk_style_context_add_provider(context,
                                 GTK_STYLE_PROVIDER(m_provider->gobj()),
                                 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
}

Glib::ustring
SourceFile::getLabel()
{
    if (m_eventItem) {
        auto fileInfo = m_eventItem->getFileInfo();
        return fileInfo->get_display_name();
    }
    return _("New");
}
