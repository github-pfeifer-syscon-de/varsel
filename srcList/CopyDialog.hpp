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
#include <memory>
#include <vector>
#include <queue>

class VarselList;

class CopyDialog;

class CopyMode
{
public:
    CopyMode(CopyDialog* copyDialog)
    : m_copyDialog{copyDialog}
    {
    }
    explicit CopyMode(const CopyMode& other) = delete;
    ~CopyMode() = default;

    virtual bool isOverwrite(
        const Glib::RefPtr<Gio::File>& src
        , const Glib::RefPtr<Gio::File>& target) = 0;

protected:
    CopyDialog* m_copyDialog;
};

using PtrCopyMode = std::shared_ptr<CopyMode>;

class CopyModeAsk
: public CopyMode
{
public:
    CopyModeAsk(CopyDialog* copyDialog)
    : CopyMode(copyDialog)
    {
    }
    explicit CopyModeAsk(const CopyModeAsk& other) = delete;
    ~CopyModeAsk() = default;

    bool isOverwrite(
        const Glib::RefPtr<Gio::File>& src
        , const Glib::RefPtr<Gio::File>& target);
};

class CopyModeNo
: public CopyMode
{
public:
    CopyModeNo(CopyDialog* copyDialog)
    : CopyMode(copyDialog)
    {
    }
    explicit CopyModeNo(const CopyModeNo& other) = delete;
    ~CopyModeNo() = default;

    bool isOverwrite(
        const Glib::RefPtr<Gio::File>& src
        , const Glib::RefPtr<Gio::File>& target) {
        return false;
    }
};

class CopyModeCheck
: public CopyMode
{
public:
    CopyModeCheck(CopyDialog* copyDialog)
    : CopyMode(copyDialog)
    {
    }
    explicit CopyModeCheck(const CopyModeCheck& other) = delete;
    ~CopyModeCheck() = default;

    bool isOverwrite(
        const Glib::RefPtr<Gio::File>& src
        , const Glib::RefPtr<Gio::File>& target);
};

class CopyModeAll
: public CopyMode
{
public:
    CopyModeAll(CopyDialog* copyDialog)
    : CopyMode(copyDialog)
    {
    }
    explicit CopyModeAll(const CopyModeAll& other) = delete;
    ~CopyModeAll() = default;

    bool isOverwrite(
        const Glib::RefPtr<Gio::File>& src
        , const Glib::RefPtr<Gio::File>& target) {
        return true;
    }
};

class CopyItem
{
public:
    CopyItem(const Glib::RefPtr<Gio::File>& src
            , const Glib::RefPtr<Gio::File>& target
            , CopyDialog* copyDialog);
    explicit CopyItem(const CopyItem& other) = delete;
    ~CopyItem() = default;
    void copy();
protected:
    void ready(Glib::RefPtr<Gio::AsyncResult>& result);
    void copySymlink();
    void copyDir();
    void copyFile();

private:
    Glib::RefPtr<Gio::File> m_src;
    Glib::RefPtr<Gio::File> m_target;
    CopyDialog* m_copyDialog;
};

using PtrCopyItem = std::shared_ptr<CopyItem>;

enum class Overwrite
{
      None
    , Check
    , All
};

class CopyDialog
: public Gtk::Dialog
{
public:
    CopyDialog(BaseObjectType* cobject
        , const Glib::RefPtr<Gtk::Builder>& builder
        , const std::vector<Glib::ustring>& uris
        , const Glib::RefPtr<Gio::File>& dir
        , bool isMove
        , VarselList* varselList);
    explicit CopyDialog(const CopyDialog& orig) = delete;
    virtual ~CopyDialog() = default;

    void next();
    void notify(const Glib::RefPtr<Gio::File>& target, bool success);
    Glib::RefPtr<Gio::Cancellable> getCancel();
    void addItem(const Glib::RefPtr<Gio::File>& src
        , const Glib::RefPtr<Gio::File>& target);
    static void show(
          const std::vector<Glib::ustring>& uris
        , const Glib::RefPtr<Gio::File>& dir
        , bool isMove
        , VarselList* varselList);
    void progress(goffset current_num_bytes, goffset total_num_bytes);
    VarselList* getWindow();
    Glib::RefPtr<Gio::Cancellable> getCancelable();
    void showText(const Glib::ustring& text);
    bool isMove();
    PtrCopyMode getCopyMode();
protected:
    void apply();

    Glib::RefPtr<Gio::File> m_dir;
    const bool m_isMove;
    VarselList* m_varselList;
    Glib::RefPtr<Gio::Cancellable> m_cancelable;
    Gtk::Label* m_target;
    Gtk::ProgressBar* m_progress;
    Gtk::TextView* m_text;
    Gtk::ComboBoxText* m_overwrite;
    Gtk::Button* m_apply;
    std::queue<PtrCopyItem> m_copyQueue;
    PtrCopyItem m_item;
    PtrCopyMode m_copyMode;
private:

};

