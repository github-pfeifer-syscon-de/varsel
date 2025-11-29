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
#include <psc_format.hpp>
#include <StringUtils.hpp>

#include "VarselList.hpp"

#include "CopyDialog.hpp"

bool
CopyModeAsk::isOverwrite(
    const Glib::RefPtr<Gio::File>& src
    , const Glib::RefPtr<Gio::File>& target)
{
    auto msg = Glib::ustring::sprintf(_("Overwrite %s"), target->get_parse_name());
    Gtk::MessageDialog msgDlg(*m_copyDialog->getWindow(), msg, false, Gtk::MessageType::MESSAGE_QUESTION, Gtk::ButtonsType::BUTTONS_YES_NO, true);
    auto ret = msgDlg.run();
    if (ret == Gtk::RESPONSE_NO) {
        return false;
    }
    return true;
}

bool
CopyModeCheck::isOverwrite(
    const Glib::RefPtr<Gio::File>& src
    , const Glib::RefPtr<Gio::File>& target)
{
    auto srcInfo = src->query_info("standard::size,time::modified", Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
    auto targetInfo = target->query_info("standard::size,time::modified", Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
    if (srcInfo->get_modification_date_time()
     .compare(targetInfo->get_modification_date_time()) > 0) {
        return true;
    }
    if (srcInfo->get_size()
     != targetInfo->get_size()) {
        return true;
    }
    return false;
}

CopyItem::CopyItem(const Glib::RefPtr<Gio::File>& src
            , const Glib::RefPtr<Gio::File>& target
            , CopyDialog* copyDialog)
: m_src{src}
, m_target{target}
, m_copyDialog{copyDialog}
{
}

void
CopyItem::copy()
{
    auto fileType = m_src->query_file_type(Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
    if (fileType == Gio::FileType::FILE_TYPE_REGULAR) {
        copyFile();
    }
    else if (fileType == Gio::FileType::FILE_TYPE_DIRECTORY) {
        copyDir();
    }
    else if (fileType == Gio::FileType::FILE_TYPE_SYMBOLIC_LINK) {
        copySymlink();
    }
}

void
CopyItem::copyFile()
{
    auto newFile = m_target->get_child(m_src->get_basename());
    if (newFile->query_exists()) {
        auto copyMode = m_copyDialog->getCopyMode();
        if (!copyMode->isOverwrite(m_src, newFile)) {
            m_copyDialog->showText(Glib::ustring::sprintf(_("Skipped %s\n"), newFile->get_parse_name()));
            m_copyDialog->next();
            return;
        }
    }
    std::cout << "Copy " << m_src->get_parse_name() << " to " << m_target->get_parse_name() << std::endl;
    m_src->copy_async(newFile
              , sigc::mem_fun(*m_copyDialog, &CopyDialog::progress)
              , sigc::mem_fun(*this, &CopyItem::ready)
              , m_copyDialog->getCancelable()
              , Gio::FILE_COPY_OVERWRITE);
}


void
CopyItem::copySymlink()
{
    std::cout << "Howto copy symlink " << m_src->get_path() << "?" << std::endl;
    //dir->make_symbolic_link();
}


void
CopyItem::copyDir()
{
    auto targetDir = m_target->get_child(m_src->get_basename());
    if (!targetDir->query_exists()) {
        targetDir->make_directory();        // care about permission, user...
    }
    Glib::RefPtr<Gio::FileEnumerator> enumerat = m_src->enumerate_children(
              m_copyDialog->getCancelable()
            , "*"
            , Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
    while (true) {
        auto fileInfo = enumerat->next_file();
        if (!fileInfo) {
            break;
        }
        auto childFile = m_src->get_child(fileInfo->get_name());
        m_copyDialog->addItem(childFile, targetDir);
    }
}

void
CopyItem::ready(Glib::RefPtr<Gio::AsyncResult>& result)
{
    std::cout << "CopyItem::ready " << m_src->get_parse_name() << std::endl;
    bool success = m_src->copy_finish(result);
    auto newFile = m_target->get_child(m_src->get_basename());
    m_copyDialog->notify(newFile, success);
    // ensure this is the last action here as the reference to this was cleared already
}

CopyDialog::CopyDialog(BaseObjectType* cobject
        , const Glib::RefPtr<Gtk::Builder>& builder
        , const std::vector<Glib::ustring>& uris
        , const Glib::RefPtr<Gio::File>& dirs
        , bool isMove
        , VarselList* varselList)
: Gtk::Dialog(cobject)
, m_dir{dirs}
, m_isMove{isMove}
, m_varselList{varselList}
, m_cancelable{Gio::Cancellable::create()}
{
    builder->get_widget("target", m_target);
    builder->get_widget("progress", m_progress);
    builder->get_widget("text", m_text);
    builder->get_widget("overwrite", m_overwrite);
    builder->get_widget("apply", m_apply);
    m_target->set_text(m_dir->get_parse_name());
    for (auto uri : uris) {
        auto file = Gio::File::create_for_uri(uri);
        addItem(file, m_dir);
    }
    m_apply->signal_clicked().connect(sigc::mem_fun(*this, &CopyDialog::apply));
}


void
CopyDialog::apply()
{
    m_apply->set_sensitive(false);
    auto id = m_overwrite->get_active_id();
    if (id == "ask") {
        m_copyMode = std::make_shared<CopyModeAsk>(this);
    }
    else if (id == "check") {
        m_copyMode = std::make_shared<CopyModeCheck>(this);
    }
    else if (id == "all") {
        m_copyMode = std::make_shared<CopyModeAll>(this);
    }
    else {
        m_copyMode = std::make_shared<CopyModeNo>(this);
    }
    next(); // start working
}

void
CopyDialog::addItem(const Glib::RefPtr<Gio::File>& src
        , const Glib::RefPtr<Gio::File>& target)
{
    m_copyQueue.emplace(
            std::make_shared<CopyItem>(src, target, this));
}

void
CopyDialog::progress(goffset current_num_bytes, goffset total_num_bytes)
{
    std::cout << "FileDataSource::progress "
              << current_num_bytes <<  "/" << total_num_bytes << std::endl;
    auto part = static_cast<double>(current_num_bytes) / static_cast<double>(total_num_bytes);
    m_progress->set_fraction(part);
}

VarselList*
CopyDialog::getWindow()
{
    return m_varselList;
}

bool
CopyDialog::isMove()
{
    return m_isMove;
}

PtrCopyMode
CopyDialog::getCopyMode()
{
    return m_copyMode;
}

void
CopyDialog::notify(const Glib::RefPtr<Gio::File>& target, bool success)
{
    if (!success) {
        auto msg = Glib::ustring::sprintf(_("Error writing %s!\nContinue?"), target->get_parse_name());
        Gtk::MessageDialog msgDlg(*m_varselList, msg, false, Gtk::MessageType::MESSAGE_QUESTION, Gtk::ButtonsType::BUTTONS_YES_NO, true);
        if (!msgDlg.run()) {
            return;
        }
        showText(Glib::ustring::sprintf(_("Error %s writing\n"), target->get_parse_name()));
    }
    else {
        showText(Glib::ustring::sprintf(_("Copied %s\n"), target->get_parse_name()));
    }
    next();
}

Glib::RefPtr<Gio::Cancellable>
CopyDialog::getCancelable()
{
    return m_cancelable;
}

void
CopyDialog::showText(const Glib::ustring& text)
{
    auto buf = m_text->get_buffer();
    buf->insert_at_cursor(text);
    buf->move_mark(buf->get_insert(), buf->end());
}


void
CopyDialog::next()
{
    if (!m_copyQueue.empty()) {
        m_item = m_copyQueue.front();   // keep the reference around until the next is called
        m_copyQueue.pop();  // remove the item!
        m_item->copy();
    }
    else {
        m_item.reset();
    }
}


void
CopyDialog::show(
      const std::vector<Glib::ustring>& uris
    , const Glib::RefPtr<Gio::File>& dir
    , bool isMove
    , VarselList* varselList)
{
    CopyDialog* copyDialog = nullptr;
    auto builder = Gtk::Builder::create();
    try {
        builder->add_from_resource(varselList->get_application()->get_resource_base_path() + "/dlgCopy.ui");
        builder->get_widget_derived("dlgCopy", copyDialog, uris, dir, isMove, varselList);
        copyDialog->set_transient_for(*varselList);
        if (copyDialog->run() == Gtk::ResponseType::RESPONSE_OK) {

        }
        delete copyDialog;
    }
    catch (const Glib::Error &ex) {
        //listApp->showMessage(
        Glib::ustring msg = ex.what();
        std::cout << psc::fmt::vformat(
                  _("Error {} loading {}")
                , psc::fmt::make_format_args(msg, "dlgCopy")) << std::endl;
        // , Gtk::MessageType::MESSAGE_WARNING)
    }
}
